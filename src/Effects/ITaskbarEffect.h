#pragma once

#include "../Common/PhostideCommon.h"

#include <d2d1_1.h>
#include <cstdint>

namespace phostide {

struct EffectContext {
  float width = 0.f;
  float height = 0.f;
  float timeSeconds = 0.f;
  float intensity = 0.45f;  // 0..1
};

class ITaskbarEffect {
 public:
  virtual ~ITaskbarEffect() = default;
  virtual ThemeId Id() const = 0;
  virtual const wchar_t* Name() const = 0;
  virtual void Update(ID2D1DeviceContext* dc, const EffectContext& ctx) = 0;
};

}  // namespace phostide
