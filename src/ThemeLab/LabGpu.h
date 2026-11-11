#pragma once

#include "../Common/PhostideCommon.h"
#include "../ExplorerTAP/TaskbarGeometry.h"

#include <cstdint>
#include <string>

namespace phostide {

// Offline D3D11 path that compiles the same Themes.hlsl used by ExplorerTAP.
class LabGpuPreview {
 public:
  LabGpuPreview();
  ~LabGpuPreview();

  LabGpuPreview(const LabGpuPreview&) = delete;
  LabGpuPreview& operator=(const LabGpuPreview&) = delete;

  // Ready after first successful Ensure(). False → use CPU-only lab.
  bool Ready() const { return ready_; }
  const std::wstring& LastError() const { return lastError_; }

  bool Ensure();
  // Recompile Themes.hlsl from disk without recreating the D3D device.
  // ThemeLab-only — does not touch Explorer / TAP.
  bool ReloadShaders();
  // Absolute path of the Themes.hlsl currently preferred for lab GPU.
  const std::wstring& ShaderPath() const { return shaderPath_; }
  // FILETIME of ShaderPath at last successful compile (0 if never).
  ULONGLONG ShaderWriteTime() const { return shaderWriteTime_; }
  // Paint theme into tightly packed BGRA8 (top-down). Returns false on failure.
  bool Render(ThemeId theme, float intensity, float timeSeconds, TaskbarEdge edge, int width,
              int height, uint8_t* bgraOut, int strideBytes);

 private:
  bool CreateDevice();
  bool CompileShaders(const std::string& src);
  bool CreatePipelineState();
  std::string LoadShaderSource();
  void RememberShaderPath(const std::wstring& path);

  struct Impl;
  Impl* impl_ = nullptr;
  bool ready_ = false;
  bool initAttempted_ = false;
  std::wstring lastError_;
  std::wstring shaderPath_;
  ULONGLONG shaderWriteTime_ = 0;
};

}  // namespace phostide
