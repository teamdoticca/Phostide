#include "GpuThemeRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <windows.ui.composition.interop.h>
#include <wrl/client.h>

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include <winrt/Windows.Foundation.Numerics.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#pragma pop_macro("GetCurrentTime")

using Microsoft::WRL::ComPtr;
namespace wuc = winrt::Windows::UI::Composition;
namespace wux = winrt::Windows::UI::Xaml;
namespace wuxh = winrt::Windows::UI::Xaml::Hosting;
namespace wuxm = winrt::Windows::UI::Xaml::Media;
namespace wuxs = winrt::Windows::UI::Xaml::Shapes;
namespace numerics = winrt::Windows::Foundation::Numerics;

namespace phostide {
namespace {

struct alignas(16) ThemeCBuffer {
  float time;
  float intensity;
  float width;
  float height;
  uint32_t themeId;
  uint32_t edge;
  float quality;  // 1.0 baseline; >1 denser Space stars (GPU headroom)
  float _pad1;
};

// Live taskbar GPU uses elevated quality; ThemeLab stays at 1.0 for CPU parity.
constexpr float kLiveGpuQuality = 1.5f;

// Themes.hlsl lives next to ExplorerTAP.dll (and in %TEMP%\Phostide after inject).
std::string LoadThemesHlsl() {
  auto tryRead = [](const std::wstring& path) -> std::string {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return {};
    LARGE_INTEGER sz{};
    if (!GetFileSizeEx(f, &sz) || sz.QuadPart <= 0 || sz.QuadPart > 2 * 1024 * 1024) {
      CloseHandle(f);
      return {};
    }
    std::string buf(static_cast<size_t>(sz.QuadPart), '\0');
    DWORD read = 0;
    const BOOL ok = ReadFile(f, buf.data(), static_cast<DWORD>(buf.size()), &read, nullptr);
    CloseHandle(f);
    if (!ok || read == 0) return {};
    buf.resize(read);
    return buf;
  };

  wchar_t modPath[MAX_PATH]{};
  HMODULE mod = nullptr;
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     reinterpret_cast<LPCWSTR>(reinterpret_cast<void*>(&LoadThemesHlsl)), &mod);
  if (mod && GetModuleFileNameW(mod, modPath, MAX_PATH)) {
    std::wstring dir(modPath);
    const auto slash = dir.find_last_of(L"\\/");
    if (slash != std::wstring::npos) dir.resize(slash);
    std::string src = tryRead(dir + L"\\Themes.hlsl");
    if (!src.empty()) return src;
  }

  wchar_t temp[MAX_PATH]{};
  GetTempPathW(MAX_PATH, temp);
  return tryRead(std::wstring(temp) + L"Phostide\\Themes.hlsl");
}

void LogGpu(const wchar_t* msg) {
  wchar_t dir[MAX_PATH]{};
  GetTempPathW(MAX_PATH, dir);
  wcscat_s(dir, L"Phostide");
  CreateDirectoryW(dir, nullptr);
  wchar_t path[MAX_PATH]{};
  swprintf_s(path, L"%s\\tap.log", dir);
  HANDLE f = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) return;
  SYSTEMTIME st{};
  GetLocalTime(&st);
  wchar_t line[512];
  swprintf_s(line, L"%02u:%02u:%02u GPU %s\r\n", st.wHour, st.wMinute, st.wSecond, msg);
  DWORD written = 0;
  WriteFile(f, line, static_cast<DWORD>(wcslen(line) * sizeof(wchar_t)), &written, nullptr);
  CloseHandle(f);
}

uint64_t ShapeKey(wuxs::Shape const& shape) {
  return reinterpret_cast<uint64_t>(winrt::get_abi(shape));
}

}  // namespace

struct GpuThemeRenderer::Target {
  wuxs::Shape shape{nullptr};
  wuc::SpriteVisual sprite{nullptr};
  wuc::CompositionDrawingSurface surface{nullptr};
  wuc::CompositionSurfaceBrush brush{nullptr};
  ComPtr<ID3D11Texture2D> offscreen;
  ComPtr<ID3D11RenderTargetView> offscreenRtv;
  int pixelW = 0;
  int pixelH = 0;
};

struct GpuThemeRenderer::Impl {
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;
  ComPtr<ID3D11VertexShader> vs;
  ComPtr<ID3D11PixelShader> ps;
  ComPtr<ID3D11Buffer> cbuffer;
  ComPtr<ID3D11RasterizerState> rast;
  ComPtr<ID3D11BlendState> blend;
  wuc::Compositor compositor{nullptr};
  wuc::CompositionGraphicsDevice graphics{nullptr};
  std::unordered_map<uint64_t, Target> targets;
  std::mutex mutex;
};

GpuThemeRenderer::~GpuThemeRenderer() {
  DetachAll();
  delete impl_;
  impl_ = nullptr;
}

void GpuThemeRenderer::ResetDevice() {
  DetachAll();
  delete impl_;
  impl_ = nullptr;
  ready_ = false;
  initAttempted_ = false;
}

bool GpuThemeRenderer::CreatePipeline() {
  if (!impl_ || !impl_->device) return false;
  ComPtr<ID3DBlob> vsBlob;
  ComPtr<ID3DBlob> psBlob;
  ComPtr<ID3DBlob> err;
  UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
  flags |= D3DCOMPILE_DEBUG;
#endif
  const std::string shaderSrc = LoadThemesHlsl();
  if (shaderSrc.empty()) {
    LogGpu(L"Themes.hlsl missing next to DLL / TEMP");
    return false;
  }
  HRESULT hr = D3DCompile(shaderSrc.data(), shaderSrc.size(), "Themes.hlsl", nullptr, nullptr,
                          "VSMain", "vs_4_0", flags, 0, &vsBlob, &err);
  if (FAILED(hr)) {
    LogGpu(L"VS compile failed");
    return false;
  }
  err.Reset();
  hr = D3DCompile(shaderSrc.data(), shaderSrc.size(), "Themes.hlsl", nullptr, nullptr, "PSMain",
                  "ps_4_0", flags, 0, &psBlob, &err);
  if (FAILED(hr)) {
    if (err) {
      const char* msg = static_cast<const char*>(err->GetBufferPointer());
      wchar_t wide[512];
      MultiByteToWideChar(CP_UTF8, 0, msg, -1, wide, 512);
      LogGpu(wide);
    } else {
      LogGpu(L"PS compile failed");
    }
    return false;
  }
  hr = impl_->device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                         nullptr, &impl_->vs);
  if (FAILED(hr)) return false;
  hr = impl_->device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                        nullptr, &impl_->ps);
  if (FAILED(hr)) return false;

  D3D11_BUFFER_DESC cbd{};
  cbd.ByteWidth = sizeof(ThemeCBuffer);
  cbd.Usage = D3D11_USAGE_DYNAMIC;
  cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  hr = impl_->device->CreateBuffer(&cbd, nullptr, &impl_->cbuffer);
  if (FAILED(hr)) return false;

  D3D11_RASTERIZER_DESC rd{};
  rd.FillMode = D3D11_FILL_SOLID;
  rd.CullMode = D3D11_CULL_NONE;
  rd.DepthClipEnable = TRUE;
  hr = impl_->device->CreateRasterizerState(&rd, &impl_->rast);
  if (FAILED(hr)) return false;

  D3D11_BLEND_DESC bd{};
  bd.RenderTarget[0].BlendEnable = TRUE;
  bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
  bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  hr = impl_->device->CreateBlendState(&bd, &impl_->blend);
  return SUCCEEDED(hr);
}

bool GpuThemeRenderer::EnsureDevice() {
  if (ready_) return true;
  if (initAttempted_) return false;
  initAttempted_ = true;

  impl_ = new Impl();
  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D_FEATURE_LEVEL level{};
  const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
                                      D3D_FEATURE_LEVEL_10_0};
  HRESULT hr =
      D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels,
                        ARRAYSIZE(levels), D3D11_SDK_VERSION, &impl_->device, &level, &impl_->context);
  if (FAILED(hr)) {
    LogGpu(L"D3D11 hardware device failed");
    ResetDevice();
    initAttempted_ = true;
    return false;
  }
  if (!CreatePipeline()) {
    LogGpu(L"pipeline create failed");
    ResetDevice();
    initAttempted_ = true;
    return false;
  }
  ready_ = true;
  LogGpu(L"D3D11 hardware ready");
  return true;
}

bool GpuThemeRenderer::EnsureOffscreen(Target& target) {
  if (!impl_ || !impl_->device) return false;
  if (target.offscreen && target.offscreenRtv && target.pixelW > 0 && target.pixelH > 0) {
    D3D11_TEXTURE2D_DESC desc{};
    target.offscreen->GetDesc(&desc);
    if (static_cast<int>(desc.Width) == target.pixelW &&
        static_cast<int>(desc.Height) == target.pixelH) {
      return true;
    }
  }
  target.offscreen.Reset();
  target.offscreenRtv.Reset();

  D3D11_TEXTURE2D_DESC td{};
  td.Width = static_cast<UINT>(target.pixelW);
  td.Height = static_cast<UINT>(target.pixelH);
  td.MipLevels = 1;
  td.ArraySize = 1;
  td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  td.SampleDesc.Count = 1;
  td.Usage = D3D11_USAGE_DEFAULT;
  td.BindFlags = D3D11_BIND_RENDER_TARGET;
  HRESULT hr = impl_->device->CreateTexture2D(&td, nullptr, &target.offscreen);
  if (FAILED(hr) || !target.offscreen) {
    LogGpu(L"offscreen CreateTexture2D failed");
    return false;
  }
  hr = impl_->device->CreateRenderTargetView(target.offscreen.Get(), nullptr, &target.offscreenRtv);
  if (FAILED(hr) || !target.offscreenRtv) {
    target.offscreen.Reset();
    LogGpu(L"offscreen CreateRTV failed");
    return false;
  }
  return true;
}

bool GpuThemeRenderer::EnsureTarget(wuxs::Shape const& shape, int pixelW, int pixelH,
                                    Target** out) {
  if (!ready_ || !impl_ || !shape || pixelW < 1 || pixelH < 1) return false;
  const uint64_t key = ShapeKey(shape);
  auto it = impl_->targets.find(key);
  if (it != impl_->targets.end()) {
    if (it->second.pixelW == pixelW && it->second.pixelH == pixelH && it->second.surface &&
        it->second.offscreen && it->second.offscreenRtv) {
      *out = &it->second;
      return true;
    }
  }

  try {
    auto fe = shape.as<wux::FrameworkElement>();
    auto elementVisual = wuxh::ElementCompositionPreview::GetElementVisual(fe);
    if (!impl_->compositor) {
      impl_->compositor = elementVisual.Compositor();
      auto compositorNative =
          impl_->compositor.as<::ABI::Windows::UI::Composition::ICompositorInterop>();
      ComPtr<::ABI::Windows::UI::Composition::ICompositionGraphicsDevice> abiGraphics;
      const HRESULT hr = compositorNative->CreateGraphicsDevice(impl_->device.Get(), &abiGraphics);
      if (FAILED(hr) || !abiGraphics) {
        LogGpu(L"CreateGraphicsDevice failed");
        return false;
      }
      impl_->graphics =
          winrt::Windows::UI::Composition::CompositionGraphicsDevice{
              abiGraphics.Detach(), winrt::take_ownership_from_abi};
    }

    Target target;
    target.shape = shape;
    target.pixelW = pixelW;
    target.pixelH = pixelH;
    if (!EnsureOffscreen(target)) return false;
    target.surface = impl_->graphics.CreateDrawingSurface(
        winrt::Windows::Foundation::Size{static_cast<float>(pixelW), static_cast<float>(pixelH)},
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
        winrt::Windows::Graphics::DirectX::DirectXAlphaMode::Premultiplied);
    target.brush = impl_->compositor.CreateSurfaceBrush(target.surface);
    target.brush.Stretch(wuc::CompositionStretch::Fill);
    target.sprite = impl_->compositor.CreateSpriteVisual();
    target.sprite.RelativeSizeAdjustment({1.f, 1.f});
    target.sprite.Brush(target.brush);
    wuxh::ElementCompositionPreview::SetElementChildVisual(fe, target.sprite);

    // Keep XAML fill transparent so the composition child shows through.
    wuxm::SolidColorBrush clear;
    clear.Color(winrt::Windows::UI::Color{0, 0, 0, 0});
    shape.Fill(clear);

    impl_->targets[key] = std::move(target);
    *out = &impl_->targets[key];
    return true;
  } catch (...) {
    LogGpu(L"EnsureTarget exception");
    return false;
  }
}

bool GpuThemeRenderer::DrawTarget(Target& target, ThemeId theme, float intensity,
                                  float timeSeconds, TaskbarEdge edge) {
  if (!impl_ || !target.surface) return false;
  if (!EnsureOffscreen(target)) return false;
  try {
    // 1) Finish the whole frame offscreen — compositor never sees a clear/partial.
    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = impl_->context->Map(impl_->cbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
      auto* cb = reinterpret_cast<ThemeCBuffer*>(mapped.pData);
      cb->time = timeSeconds;
      cb->intensity = intensity;
      cb->width = static_cast<float>(target.pixelW);
      cb->height = static_cast<float>(target.pixelH);
      cb->themeId = static_cast<uint32_t>(theme);
      cb->edge = static_cast<uint32_t>(edge);
      cb->quality = kLiveGpuQuality;
      cb->_pad1 = 0.f;
      impl_->context->Unmap(impl_->cbuffer.Get(), 0);
    }

    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.f;
    vp.TopLeftY = 0.f;
    vp.Width = static_cast<float>(target.pixelW);
    vp.Height = static_cast<float>(target.pixelH);
    vp.MinDepth = 0.f;
    vp.MaxDepth = 1.f;

    ID3D11RenderTargetView* offRtvs[] = {target.offscreenRtv.Get()};
    impl_->context->OMSetRenderTargets(1, offRtvs, nullptr);
    impl_->context->RSSetViewports(1, &vp);
    impl_->context->RSSetState(impl_->rast.Get());
    float blendFactor[4] = {};
    impl_->context->OMSetBlendState(impl_->blend.Get(), blendFactor, 0xffffffff);
    impl_->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    impl_->context->VSSetShader(impl_->vs.Get(), nullptr, 0);
    impl_->context->PSSetShader(impl_->ps.Get(), nullptr, 0);
    ID3D11Buffer* cbs[] = {impl_->cbuffer.Get()};
    impl_->context->VSSetConstantBuffers(0, 1, cbs);
    impl_->context->PSSetConstantBuffers(0, 1, cbs);
    const float clear[4] = {0, 0, 0, 0};
    impl_->context->ClearRenderTargetView(target.offscreenRtv.Get(), clear);
    impl_->context->Draw(3, 0);
    impl_->context->OMSetRenderTargets(0, nullptr, nullptr);

    // 2) Publish the finished frame to the live Composition surface.
    auto interop = target.surface.as<::ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();
    POINT offset{};
    ComPtr<IDXGISurface> dxgiSurface;
    hr = interop->BeginDraw(nullptr, IID_PPV_ARGS(&dxgiSurface), &offset);
    if (hr == static_cast<HRESULT>(0x80263004) /* E_SURFACE_CONTENTS_LOST */) {
      LogGpu(L"surface contents lost");
      return false;
    }
    if (FAILED(hr) || !dxgiSurface) return false;

    ComPtr<ID3D11Texture2D> tex;
    hr = dxgiSurface.As(&tex);
    if (FAILED(hr) || !tex) {
      interop->EndDraw();
      return false;
    }

    D3D11_BOX srcBox{};
    srcBox.left = 0;
    srcBox.top = 0;
    srcBox.front = 0;
    srcBox.right = static_cast<UINT>(target.pixelW);
    srcBox.bottom = static_cast<UINT>(target.pixelH);
    srcBox.back = 1;
    impl_->context->CopySubresourceRegion(tex.Get(), 0, static_cast<UINT>(offset.x),
                                          static_cast<UINT>(offset.y), 0, target.offscreen.Get(), 0,
                                          &srcBox);

    interop->EndDraw();
    return true;
  } catch (...) {
    LogGpu(L"DrawTarget exception");
    return false;
  }
}

bool GpuThemeRenderer::Attach(wuxs::Shape const& shape, int pixelW, int pixelH) {
  if (!EnsureDevice()) return false;
  std::lock_guard lock(impl_->mutex);
  Target* target = nullptr;
  return EnsureTarget(shape, pixelW, pixelH, &target);
}

void GpuThemeRenderer::Detach(wuxs::Shape const& shape) {
  if (!impl_ || !shape) return;
  std::lock_guard lock(impl_->mutex);
  const uint64_t key = ShapeKey(shape);
  auto it = impl_->targets.find(key);
  if (it == impl_->targets.end()) return;
  try {
    if (auto fe = shape.try_as<wux::FrameworkElement>()) {
      wuxh::ElementCompositionPreview::SetElementChildVisual(fe, nullptr);
    }
  } catch (...) {
  }
  impl_->targets.erase(it);
}

void GpuThemeRenderer::Resize(wuxs::Shape const& shape, int pixelW, int pixelH) {
  if (!ready_ || !impl_) return;
  std::lock_guard lock(impl_->mutex);
  Target* target = nullptr;
  EnsureTarget(shape, pixelW, pixelH, &target);
}

bool GpuThemeRenderer::Paint(wuxs::Shape const& shape, ThemeId theme, float intensity,
                             float timeSeconds, TaskbarEdge edge, int pixelW, int pixelH) {
  if (!EnsureDevice()) return false;
  std::lock_guard lock(impl_->mutex);
  Target* target = nullptr;
  if (!EnsureTarget(shape, pixelW, pixelH, &target) || !target) return false;
  if (!DrawTarget(*target, theme, intensity, timeSeconds, edge)) {
    // Device/surface lost — force reinit next time.
    ready_ = false;
    return false;
  }
  return true;
}

void GpuThemeRenderer::DetachAll() {
  if (!impl_) return;
  std::lock_guard lock(impl_->mutex);
  for (auto& [_, target] : impl_->targets) {
    try {
      if (target.shape) {
        if (auto fe = target.shape.try_as<wux::FrameworkElement>()) {
          wuxh::ElementCompositionPreview::SetElementChildVisual(fe, nullptr);
        }
      }
    } catch (...) {
    }
  }
  impl_->targets.clear();
}

}  // namespace phostide
