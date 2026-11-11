#include "LabGpu.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace phostide {
namespace {

struct alignas(16) ThemeCBuffer {
  float time;
  float intensity;
  float width;
  float height;
  uint32_t themeId;
  uint32_t edge;
  float quality;  // 1.0 keeps Lab GPU strip comparable to CPU
  float _pad1;
};

std::wstring Widen(const std::string& s) {
  if (s.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
  std::wstring out(n > 0 ? n - 1 : 0, L'\0');
  if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), n);
  return out;
}

std::string ReadFileUtf8(const std::wstring& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  std::string s = ss.str();
  if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
      static_cast<unsigned char>(s[1]) == 0xBB && static_cast<unsigned char>(s[2]) == 0xBF) {
    s.erase(0, 3);
  }
  return s;
}

std::wstring ModuleDir() {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  std::wstring p(path);
  const auto slash = p.find_last_of(L"\\/");
  if (slash != std::wstring::npos) p.resize(slash);
  return p;
}

}  // namespace

struct LabGpuPreview::Impl {
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;
  ComPtr<ID3D11VertexShader> vs;
  ComPtr<ID3D11PixelShader> ps;
  ComPtr<ID3D11Buffer> cbuffer;
  ComPtr<ID3D11RasterizerState> rast;
  ComPtr<ID3D11Texture2D> rt;
  ComPtr<ID3D11RenderTargetView> rtv;
  ComPtr<ID3D11Texture2D> staging;
  int rtW = 0;
  int rtH = 0;
};

LabGpuPreview::LabGpuPreview() = default;

LabGpuPreview::~LabGpuPreview() {
  delete impl_;
  impl_ = nullptr;
}

ULONGLONG FileWriteTime(const std::wstring& path) {
  WIN32_FILE_ATTRIBUTE_DATA fad{};
  if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad)) return 0;
  ULARGE_INTEGER u{};
  u.LowPart = fad.ftLastWriteTime.dwLowDateTime;
  u.HighPart = fad.ftLastWriteTime.dwHighDateTime;
  return u.QuadPart;
}

void LabGpuPreview::RememberShaderPath(const std::wstring& path) {
  shaderPath_ = path;
  shaderWriteTime_ = FileWriteTime(path);
}

std::string LabGpuPreview::LoadShaderSource() {
  // Prefer the live source tree so ThemeLab F5 / auto-reload picks up edits
  // without copying into out\Release or bouncing Explorer.
  const std::wstring dir = ModuleDir();
  const std::wstring candidates[] = {
      dir + L"\\..\\..\\src\\ExplorerTAP\\Themes.hlsl",
      dir + L"\\..\\ExplorerTAP\\Themes.hlsl",
      dir + L"\\Themes.hlsl",
  };
  for (const auto& c : candidates) {
    std::string s = ReadFileUtf8(c);
    if (!s.empty()) {
      RememberShaderPath(c);
      return s;
    }
  }
  wchar_t temp[MAX_PATH]{};
  GetTempPathW(MAX_PATH, temp);
  const std::wstring tempPath = std::wstring(temp) + L"Phostide\\Themes.hlsl";
  std::string s = ReadFileUtf8(tempPath);
  if (!s.empty()) RememberShaderPath(tempPath);
  return s;
}

bool LabGpuPreview::CompileShaders(const std::string& src) {
  ComPtr<ID3DBlob> vsBlob;
  ComPtr<ID3DBlob> psBlob;
  ComPtr<ID3DBlob> err;
  UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
  HRESULT hr = D3DCompile(src.data(), src.size(), "Themes.hlsl", nullptr, nullptr, "VSMain",
                          "vs_4_0", flags, 0, &vsBlob, &err);
  if (FAILED(hr)) {
    lastError_ = L"VS compile failed";
    if (err) lastError_ = Widen(static_cast<const char*>(err->GetBufferPointer()));
    return false;
  }
  err.Reset();
  hr = D3DCompile(src.data(), src.size(), "Themes.hlsl", nullptr, nullptr, "PSMain", "ps_4_0",
                  flags, 0, &psBlob, &err);
  if (FAILED(hr)) {
    lastError_ = L"PS compile failed";
    if (err) lastError_ = Widen(static_cast<const char*>(err->GetBufferPointer()));
    return false;
  }

  ComPtr<ID3D11VertexShader> vs;
  ComPtr<ID3D11PixelShader> ps;
  hr = impl_->device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                         nullptr, &vs);
  if (FAILED(hr)) {
    lastError_ = L"CreateVertexShader failed";
    return false;
  }
  hr = impl_->device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                        nullptr, &ps);
  if (FAILED(hr)) {
    lastError_ = L"CreatePixelShader failed";
    return false;
  }
  impl_->vs = vs;
  impl_->ps = ps;
  return true;
}

bool LabGpuPreview::CreatePipelineState() {
  if (!impl_->cbuffer) {
    D3D11_BUFFER_DESC cbd{};
    cbd.ByteWidth = sizeof(ThemeCBuffer);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = impl_->device->CreateBuffer(&cbd, nullptr, &impl_->cbuffer);
    if (FAILED(hr)) {
      lastError_ = L"Create cbuffer failed";
      return false;
    }
  }
  if (!impl_->rast) {
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    HRESULT hr = impl_->device->CreateRasterizerState(&rd, &impl_->rast);
    if (FAILED(hr)) {
      lastError_ = L"Create rast failed";
      return false;
    }
  }
  return true;
}

bool LabGpuPreview::CreateDevice() {
  impl_ = new Impl();
  D3D_FEATURE_LEVEL flOut{};
  const D3D_FEATURE_LEVEL fls[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, fls,
                                 ARRAYSIZE(fls), D3D11_SDK_VERSION, &impl_->device, &flOut,
                                 &impl_->context);
  if (FAILED(hr)) {
    lastError_ = L"D3D11 hardware device failed";
    delete impl_;
    impl_ = nullptr;
    return false;
  }
  const std::string src = LoadShaderSource();
  if (src.empty()) {
    lastError_ = L"Themes.hlsl not found (src\\ExplorerTAP or next to ThemeLab.exe)";
    delete impl_;
    impl_ = nullptr;
    return false;
  }
  if (!CompileShaders(src) || !CreatePipelineState()) {
    delete impl_;
    impl_ = nullptr;
    return false;
  }
  return true;
}

bool LabGpuPreview::Ensure() {
  if (ready_) return true;
  if (initAttempted_) return false;
  initAttempted_ = true;
  ready_ = CreateDevice();
  return ready_;
}

bool LabGpuPreview::ReloadShaders() {
  if (!impl_ || !impl_->device) {
    initAttempted_ = false;
    ready_ = false;
    return Ensure();
  }
  const std::string src = LoadShaderSource();
  if (src.empty()) {
    lastError_ = L"Themes.hlsl not found (src\\ExplorerTAP or next to ThemeLab.exe)";
    return false;
  }
  ComPtr<ID3D11VertexShader> oldVs = impl_->vs;
  ComPtr<ID3D11PixelShader> oldPs = impl_->ps;
  if (!CompileShaders(src)) {
    impl_->vs = oldVs;
    impl_->ps = oldPs;
    return false;
  }
  ready_ = true;
  lastError_.clear();
  return true;
}

bool LabGpuPreview::Render(ThemeId theme, float intensity, float timeSeconds, TaskbarEdge edge,
                           int width, int height, uint8_t* bgraOut, int strideBytes) {
  if (!Ensure() || !bgraOut || width < 2 || height < 2 || strideBytes < width * 4) return false;

  if (!impl_->rt || impl_->rtW != width || impl_->rtH != height) {
    impl_->rt.Reset();
    impl_->rtv.Reset();
    impl_->staging.Reset();
    impl_->rtW = impl_->rtH = 0;

    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(width);
    td.Height = static_cast<UINT>(height);
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET;
    HRESULT hr = impl_->device->CreateTexture2D(&td, nullptr, &impl_->rt);
    if (FAILED(hr)) {
      lastError_ = L"Create RT failed";
      return false;
    }
    hr = impl_->device->CreateRenderTargetView(impl_->rt.Get(), nullptr, &impl_->rtv);
    if (FAILED(hr)) {
      lastError_ = L"Create RTV failed";
      return false;
    }
    td.Usage = D3D11_USAGE_STAGING;
    td.BindFlags = 0;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    hr = impl_->device->CreateTexture2D(&td, nullptr, &impl_->staging);
    if (FAILED(hr)) {
      lastError_ = L"Create staging failed";
      return false;
    }
    impl_->rtW = width;
    impl_->rtH = height;
  }

  D3D11_MAPPED_SUBRESOURCE mapped{};
  HRESULT hr = impl_->context->Map(impl_->cbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return false;
  auto* cb = reinterpret_cast<ThemeCBuffer*>(mapped.pData);
  cb->time = timeSeconds;
  cb->intensity = intensity;
  cb->width = static_cast<float>(width);
  cb->height = static_cast<float>(height);
  cb->themeId = static_cast<uint32_t>(theme);
  cb->edge = static_cast<uint32_t>(edge);
  cb->quality = 1.0f;
  cb->_pad1 = 0.f;
  impl_->context->Unmap(impl_->cbuffer.Get(), 0);

  D3D11_VIEWPORT vp{};
  vp.Width = static_cast<float>(width);
  vp.Height = static_cast<float>(height);
  vp.MaxDepth = 1.f;
  ID3D11RenderTargetView* rtvs[] = {impl_->rtv.Get()};
  impl_->context->OMSetRenderTargets(1, rtvs, nullptr);
  impl_->context->RSSetViewports(1, &vp);
  impl_->context->RSSetState(impl_->rast.Get());
  impl_->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  impl_->context->VSSetShader(impl_->vs.Get(), nullptr, 0);
  impl_->context->PSSetShader(impl_->ps.Get(), nullptr, 0);
  ID3D11Buffer* cbs[] = {impl_->cbuffer.Get()};
  impl_->context->VSSetConstantBuffers(0, 1, cbs);
  impl_->context->PSSetConstantBuffers(0, 1, cbs);
  const float clear[4] = {0, 0, 0, 0};
  impl_->context->ClearRenderTargetView(impl_->rtv.Get(), clear);
  impl_->context->Draw(3, 0);
  impl_->context->OMSetRenderTargets(0, nullptr, nullptr);

  impl_->context->CopyResource(impl_->staging.Get(), impl_->rt.Get());
  hr = impl_->context->Map(impl_->staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
  if (FAILED(hr)) return false;
  const auto* src = static_cast<const uint8_t*>(mapped.pData);
  for (int y = 0; y < height; ++y) {
    std::memcpy(bgraOut + y * strideBytes, src + y * mapped.RowPitch,
                static_cast<size_t>(width) * 4u);
  }
  impl_->context->Unmap(impl_->staging.Get(), 0);
  return true;
}

}  // namespace phostide
