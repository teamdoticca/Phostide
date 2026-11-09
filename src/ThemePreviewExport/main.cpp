// Offline theme frame exporter — calls the same PaintThemePixels as ExplorerTAP.
// Usage: ThemePreviewExport.exe [outDir] [width] [height]
// Defaults: ../../site/public/themes  1920  48

#include "../ExplorerTAP/PixelThemes.h"

#include <wincodec.h>
#include <wrl/client.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

using Microsoft::WRL::ComPtr;

namespace fs = std::filesystem;

namespace {

struct ThemeExport {
  phostide::ThemeId id;
  const char* fileStem;
  float timeSeconds;
};

// Representative mid-loop frames (not empty / not pure flash).
constexpr ThemeExport kThemes[] = {
    {phostide::ThemeId::Rainbow, "rainbow", 3.2f},
    {phostide::ThemeId::SeaWaves, "sea", 5.5f},
    {phostide::ThemeId::Weather, "weather", 8.0f},
    {phostide::ThemeId::Space, "space", 6.0f},
    {phostide::ThemeId::Aurora, "aurora", 4.5f},
};

bool WritePngBgra(const fs::path& path, const uint8_t* bgra, int width, int height,
                  int stride) {
  ComPtr<IWICImagingFactory> factory;
  HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory));
  if (FAILED(hr)) return false;

  ComPtr<IWICBitmap> bitmap;
  hr = factory->CreateBitmapFromMemory(static_cast<UINT>(width), static_cast<UINT>(height),
                                       GUID_WICPixelFormat32bppBGRA, static_cast<UINT>(stride),
                                       static_cast<UINT>(stride * height),
                                       const_cast<BYTE*>(bgra), &bitmap);
  if (FAILED(hr)) return false;

  ComPtr<IWICStream> stream;
  hr = factory->CreateStream(&stream);
  if (FAILED(hr)) return false;
  hr = stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
  if (FAILED(hr)) return false;

  ComPtr<IWICBitmapEncoder> encoder;
  hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
  if (FAILED(hr)) return false;
  hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
  if (FAILED(hr)) return false;

  ComPtr<IWICBitmapFrameEncode> frame;
  ComPtr<IPropertyBag2> props;
  hr = encoder->CreateNewFrame(&frame, &props);
  if (FAILED(hr)) return false;
  hr = frame->Initialize(props.Get());
  if (FAILED(hr)) return false;
  hr = frame->SetSize(static_cast<UINT>(width), static_cast<UINT>(height));
  if (FAILED(hr)) return false;
  WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
  hr = frame->SetPixelFormat(&format);
  if (FAILED(hr)) return false;
  hr = frame->WriteSource(bitmap.Get(), nullptr);
  if (FAILED(hr)) return false;
  hr = frame->Commit();
  if (FAILED(hr)) return false;
  hr = encoder->Commit();
  return SUCCEEDED(hr);
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
  fs::path outDir = L"..\\..\\site\\public\\themes";
  int width = 1920;
  int height = 48;
  if (argc >= 2) outDir = argv[1];
  if (argc >= 3) width = _wtoi(argv[2]);
  if (argc >= 4) height = _wtoi(argv[3]);
  if (width < 64 || height < 8 || width > 7680 || height > 512) {
    std::fwprintf(stderr, L"Invalid size %dx%d\n", width, height);
    return 1;
  }

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    std::fwprintf(stderr, L"CoInitializeEx failed\n");
    return 1;
  }

  std::error_code ec;
  fs::create_directories(outDir, ec);

  const int stride = width * 4;
  std::vector<uint8_t> pixels(static_cast<size_t>(stride) * static_cast<size_t>(height));

  int ok = 0;
  for (const auto& theme : kThemes) {
    phostide::PaintContext ctx;
    ctx.theme = theme.id;
    ctx.intensity = 0.45f;
    ctx.timeSeconds = theme.timeSeconds;
    ctx.edge = phostide::TaskbarEdge::Bottom;
    ctx.vertical = false;
    ctx.thickness = height;
    ctx.length = width;

    phostide::PaintThemePixels(ctx, pixels.data(), width, height, stride);

    const fs::path out = outDir / (std::string(theme.fileStem) + ".png");
    if (!WritePngBgra(out, pixels.data(), width, height, stride)) {
      std::fwprintf(stderr, L"Failed writing %s\n", out.c_str());
      CoUninitialize();
      return 2;
    }
    std::wprintf(L"Wrote %s (%dx%d t=%.1f)\n", out.c_str(), width, height,
                 theme.timeSeconds);
    ++ok;
  }

  CoUninitialize();
  std::wprintf(L"Exported %d theme strips to %s\n", ok, outDir.c_str());
  return 0;
}
