#pragma once

#include "ITaskbarEffect.h"

#include <memory>

namespace phostide {

std::unique_ptr<ITaskbarEffect> CreateRainbowEffect();
std::unique_ptr<ITaskbarEffect> CreateSeaWavesEffect();
std::unique_ptr<ITaskbarEffect> CreateWeatherEffect();
std::unique_ptr<ITaskbarEffect> CreateSpaceEffect();
std::unique_ptr<ITaskbarEffect> CreateEffect(ThemeId id);

}  // namespace phostide
