// Offline theme frame exporter — same PaintThemePixels as ExplorerTAP / ThemeLab.
//
// Still posters:
//   ThemePreviewExport.exe [outDir] [width] [height]
//   Defaults: ../../site/public/themes  1920  48
//
// Animated sequences (Native 48px Lab painter, intensity 100%):
//   ThemePreviewExport.exe --sequence [framesRoot] [width] [height] [fps] [durationSec] [stem?]
//   Defaults: ../../out/theme-frames  1920  48  20  12
//   Optional stem filters to one theme (e.g. space). Writes framesRoot/{stem}/frame_XXXX.png

#include "../ExplorerTAP/PixelThemes.h"

#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
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

// Same start times as ThemeLab ThemeBaseTime.
constexpr ThemeExport kThemes[] = {
    {phostide::ThemeId::Rainbow, "rainbow", 3.2f},
    {phostide::ThemeId::SeaWaves, "sea", 5.5f},
    {phostide::ThemeId::Weather, "weather", 1.5f},
    {phostide::ThemeId::Space, "space", 6.0f},
    {phostide::ThemeId::Aurora, "aurora", 4.5f},
};

bool WritePngBgra(IWICImagingFactory* factory, const fs::path& path, const uint8_t* bgra,
                  int width, int height, int stride) {
  ComPtr<IWICBitmap> bitmap;
  HRESULT hr = factory->CreateBitmapFromMemory(
      static_cast<UINT>(width), static_cast<UINT>(height), GUID_WICPixelFormat32bppBGRA,
      static_cast<UINT>(stride), static_cast<UINT>(stride * height),
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

void PaintInto(std::vector<uint8_t>& pixels, int width, int height, int stride,
               phostide::ThemeId id, float intensity, float timeSeconds) {
  phostide::PaintContext ctx;
  ctx.theme = id;
  ctx.intensity = intensity;
  ctx.timeSeconds = timeSeconds;
  ctx.edge = phostide::TaskbarEdge::Bottom;
  ctx.vertical = false;
  ctx.thickness = height;
  ctx.length = width;
  std::memset(pixels.data(), 0, pixels.size());
  phostide::PaintThemePixels(ctx, pixels.data(), width, height, stride);
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
  const bool sequence = (argc >= 2 && wcscmp(argv[1], L"--sequence") == 0);

  fs::path outDir = sequence ? L"..\\..\\out\\theme-frames" : L"..\\..\\site\\public\\themes";
  int width = 1920;
  int height = 48;
  int fps = 20;
  float durationSec = 12.f;
  std::string onlyStem;
  constexpr float kIntensity = 1.0f;  // ThemeLab default

  if (sequence) {
    if (argc >= 3) outDir = argv[2];
    if (argc >= 4) width = _wtoi(argv[3]);
    if (argc >= 5) height = _wtoi(argv[4]);
    if (argc >= 6) fps = _wtoi(argv[5]);
    if (argc >= 7) durationSec = static_cast<float>(_wtof(argv[6]));
    if (argc >= 8) {
      // Optional single-theme filter (e.g. "space").
      char narrow[64]{};
      WideCharToMultiByte(CP_UTF8, 0, argv[7], -1, narrow, static_cast<int>(sizeof(narrow)),
                          nullptr, nullptr);
      onlyStem = narrow;
    }
  } else {
    if (argc >= 2) outDir = argv[1];
    if (argc >= 3) width = _wtoi(argv[2]);
    if (argc >= 4) height = _wtoi(argv[3]);
  }

  if (width < 64 || height < 8 || width > 7680 || height > 512) {
    std::fwprintf(stderr, L"Invalid size %dx%d\n", width, height);
    return 1;
  }
  if (sequence && (fps < 1 || fps > 60 || durationSec < 0.5f || durationSec > 60.f)) {
    std::fwprintf(stderr, L"Invalid sequence fps=%d duration=%.2f\n", fps, durationSec);
    return 1;
  }

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    std::fwprintf(stderr, L"CoInitializeEx failed\n");
    return 1;
  }

  ComPtr<IWICImagingFactory> factory;
  hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&factory));
  if (FAILED(hr)) {
    std::fwprintf(stderr, L"WIC factory failed\n");
    CoUninitialize();
    return 1;
  }

  std::error_code ec;
  fs::create_directories(outDir, ec);

  const int stride = width * 4;
  std::vector<uint8_t> pixels(static_cast<size_t>(stride) * static_cast<size_t>(height));

  int exitCode = 0;
  if (!sequence) {
    int ok = 0;
    for (const auto& theme : kThemes) {
      PaintInto(pixels, width, height, stride, theme.id, kIntensity, theme.timeSeconds);
      const fs::path out = outDir / (std::string(theme.fileStem) + ".png");
      if (!WritePngBgra(factory.Get(), out, pixels.data(), width, height, stride)) {
        std::fwprintf(stderr, L"Failed writing %s\n", out.c_str());
        exitCode = 2;
        break;
      }
      std::wprintf(L"Wrote %s (%dx%d t=%.1f i=%.2f)\n", out.c_str(), width, height,
                   theme.timeSeconds, kIntensity);
      ++ok;
    }
    if (exitCode == 0) {
      std::wprintf(L"Exported %d theme strips to %s\n", ok, outDir.c_str());
    }
  } else {
    const int frameCount =
        (std::max)(1, static_cast<int>(durationSec * static_cast<float>(fps) + 0.5f));
    const float dt = 1.f / static_cast<float>(fps);
    int total = 0;

    for (const auto& theme : kThemes) {
      if (!onlyStem.empty() && onlyStem != theme.fileStem) continue;
      const fs::path themeDir = outDir / theme.fileStem;
      fs::remove_all(themeDir, ec);
      fs::create_directories(themeDir, ec);

      for (int f = 0; f < frameCount; ++f) {
        const float t = theme.timeSeconds + static_cast<float>(f) * dt;
        PaintInto(pixels, width, height, stride, theme.id, kIntensity, t);
        char name[64];
        std::snprintf(name, sizeof(name), "frame_%04d.png", f);
        const fs::path out = themeDir / name;
        if (!WritePngBgra(factory.Get(), out, pixels.data(), width, height, stride)) {
          std::fwprintf(stderr, L"Failed writing %s\n", out.c_str());
          exitCode = 2;
          break;
        }
      }
      if (exitCode != 0) break;
      total += frameCount;
      std::wprintf(L"Sequence %hs: %d frames @ %dfps from t=%.1f → %s\n", theme.fileStem,
                   frameCount, fps, theme.timeSeconds, themeDir.c_str());
    }
    if (exitCode == 0) {
      std::wprintf(L"Exported %d frames under %s\n", total, outDir.c_str());
    }
  }

  factory.Reset();
  CoUninitialize();
  return exitCode;
}
