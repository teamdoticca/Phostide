#include "Themes.h"

#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace phostide {
namespace {

D2D1_COLOR_F Premul(float r, float g, float b, float a) {
  return D2D1::ColorF(r * a, g * a, b * a, a);
}

void HsvToRgb(float h, float s, float v, float& r, float& g, float& b) {
  h = std::fmod(h, 1.f);
  if (h < 0.f) h += 1.f;
  const float i = std::floor(h * 6.f);
  const float f = h * 6.f - i;
  const float p = v * (1.f - s);
  const float q = v * (1.f - f * s);
  const float t = v * (1.f - (1.f - f) * s);
  switch (static_cast<int>(i) % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
}

float Hash21(float x, float y) {
  const float n = std::sin(x * 127.1f + y * 311.7f) * 43758.5453f;
  return n - std::floor(n);
}

float Noise2(float x, float y) {
  const float ix = std::floor(x);
  const float iy = std::floor(y);
  const float fx = x - ix;
  const float fy = y - iy;
  const float ux = fx * fx * (3.f - 2.f * fx);
  const float uy = fy * fy * (3.f - 2.f * fy);
  const float a = Hash21(ix, iy);
  const float b = Hash21(ix + 1.f, iy);
  const float c = Hash21(ix, iy + 1.f);
  const float d = Hash21(ix + 1.f, iy + 1.f);
  return a + (b - a) * ux + (c - a) * uy + (a - b - c + d) * ux * uy;
}

class RainbowEffect final : public ITaskbarEffect {
 public:
  ThemeId Id() const override { return ThemeId::Rainbow; }
  const wchar_t* Name() const override { return ThemeName(ThemeId::Rainbow); }

  void Update(ID2D1DeviceContext* dc, const EffectContext& ctx) override {
    const int bands = (std::max)(8, static_cast<int>(ctx.width / 24.f));
    const float bandW = ctx.width / static_cast<float>(bands);
    const float value = 0.18f + 0.22f * ctx.intensity;
    const float alpha = 0.35f + 0.45f * ctx.intensity;

    for (int i = 0; i < bands; ++i) {
      float r, g, b;
      const float h = (static_cast<float>(i) / bands) + ctx.timeSeconds * 0.035f;
      HsvToRgb(h, 0.75f, value, r, g, b);
      ComPtr<ID2D1SolidColorBrush> brush;
      dc->CreateSolidColorBrush(Premul(r, g, b, alpha), &brush);
      if (!brush) continue;
      const D2D1_RECT_F rect =
          D2D1::RectF(i * bandW, 0.f, (i + 1) * bandW + 1.f, ctx.height);
      dc->FillRectangle(rect, brush.Get());
    }
  }
};

class SeaWavesEffect final : public ITaskbarEffect {
 public:
  ThemeId Id() const override { return ThemeId::SeaWaves; }
  const wchar_t* Name() const override { return ThemeName(ThemeId::SeaWaves); }

  void Update(ID2D1DeviceContext* dc, const EffectContext& ctx) override {
    dc->Clear(Premul(0.02f, 0.05f, 0.08f, 0.55f));
    const float amp = ctx.height * (0.18f + 0.25f * ctx.intensity);
    const float alpha = 0.25f + 0.4f * ctx.intensity;

    ComPtr<ID2D1Factory> factory;
    dc->GetFactory(&factory);

    for (int layer = 0; layer < 3; ++layer) {
      const float speed = 0.4f + layer * 0.25f;
      const float freq = 0.008f + layer * 0.004f;
      const float phase = ctx.timeSeconds * speed + layer * 1.7f;
      const float r = 0.1f + layer * 0.05f;
      const float g = 0.35f + layer * 0.1f;
      const float b = 0.55f + layer * 0.1f;
      ComPtr<ID2D1SolidColorBrush> brush;
      dc->CreateSolidColorBrush(Premul(r, g, b, alpha / (1.f + layer)), &brush);
      if (!brush) continue;

      ComPtr<ID2D1PathGeometry> geo;
      factory->CreatePathGeometry(&geo);
      ComPtr<ID2D1GeometrySink> sink;
      geo->Open(&sink);
      sink->BeginFigure(D2D1::Point2F(0.f, ctx.height), D2D1_FIGURE_BEGIN_FILLED);
      sink->AddLine(D2D1::Point2F(0.f, ctx.height * 0.55f));
      for (float x = 0.f; x <= ctx.width; x += 6.f) {
        const float y =
            ctx.height * 0.55f + std::sin(x * freq + phase) * amp * (1.f - layer * 0.2f);
        sink->AddLine(D2D1::Point2F(x, y));
      }
      sink->AddLine(D2D1::Point2F(ctx.width, ctx.height));
      sink->EndFigure(D2D1_FIGURE_END_CLOSED);
      sink->Close();
      dc->FillGeometry(geo.Get(), brush.Get());
    }
  }
};

class WeatherEffect final : public ITaskbarEffect {
 public:
  ThemeId Id() const override { return ThemeId::Weather; }
  const wchar_t* Name() const override { return ThemeName(ThemeId::Weather); }

  void Update(ID2D1DeviceContext* dc, const EffectContext& ctx) override {
    const float base = 0.08f + 0.1f * ctx.intensity;
    const float flicker =
        0.02f * ctx.intensity * std::sin(ctx.timeSeconds * 1.3f + Noise2(ctx.timeSeconds, 2.f));
    dc->Clear(Premul(base + flicker, base + flicker * 0.9f, base + 0.03f, 0.65f));

    for (int i = 0; i < 5; ++i) {
      const float t = ctx.timeSeconds * (0.05f + i * 0.01f) + i * 12.f;
      const float cx = std::fmod(t * 40.f + i * 90.f, ctx.width + 120.f) - 60.f;
      const float cy = ctx.height * (0.3f + 0.4f * Hash21(static_cast<float>(i), 3.f));
      const float radius = 28.f + 40.f * Hash21(static_cast<float>(i), 7.f);
      const float a = (0.08f + 0.12f * ctx.intensity) * (0.6f + 0.4f * Noise2(cx * 0.01f, t));
      ComPtr<ID2D1SolidColorBrush> brush;
      dc->CreateSolidColorBrush(Premul(0.75f, 0.8f, 0.9f, a), &brush);
      if (!brush) continue;
      dc->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius * 0.55f), brush.Get());
    }
  }
};

class SpaceEffect final : public ITaskbarEffect {
 public:
  SpaceEffect() {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_real_distribution<float> uni(0.f, 1.f);
    stars_.resize(64);
    for (auto& s : stars_) {
      s.x = uni(rng);
      s.y = uni(rng);
      s.speed = 0.01f + uni(rng) * 0.04f;
      s.size = 0.6f + uni(rng) * 1.8f;
      s.phase = uni(rng) * 6.28f;
    }
  }

  ThemeId Id() const override { return ThemeId::Space; }
  const wchar_t* Name() const override { return ThemeName(ThemeId::Space); }

  void Update(ID2D1DeviceContext* dc, const EffectContext& ctx) override {
    const float nebula = 0.04f + 0.08f * ctx.intensity;
    float nr, ng, nb;
    HsvToRgb(std::fmod(ctx.timeSeconds * 0.02f, 1.f), 0.55f, 0.35f, nr, ng, nb);
    dc->Clear(Premul(nr * nebula, ng * nebula, nb * nebula, 0.7f));

    ComPtr<ID2D1SolidColorBrush> starBrush;
    dc->CreateSolidColorBrush(Premul(0.9f, 0.95f, 1.f, 0.35f + 0.45f * ctx.intensity),
                              &starBrush);
    if (!starBrush) return;

    for (auto& s : stars_) {
      float x = std::fmod(s.x + ctx.timeSeconds * s.speed, 1.f);
      if (x < 0.f) x += 1.f;
      const float twinkle = 0.5f + 0.5f * std::sin(ctx.timeSeconds * 3.f + s.phase);
      const float r = s.size * (0.6f + 0.4f * twinkle);
      dc->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x * ctx.width, s.y * ctx.height), r, r),
                      starBrush.Get());
    }
  }

 private:
  struct Star {
    float x, y, speed, size, phase;
  };
  std::vector<Star> stars_;
};

// Preview stub: soft green→teal→violet curtains with slow standing undulation.
class AuroraEffect final : public ITaskbarEffect {
 public:
  ThemeId Id() const override { return ThemeId::Aurora; }
  const wchar_t* Name() const override { return ThemeName(ThemeId::Aurora); }

  void Update(ID2D1DeviceContext* dc, const EffectContext& ctx) override {
    const float tCalm = ctx.timeSeconds * 0.20f;
    const float breath = 0.88f + 0.12f * std::sin(tCalm * 0.55f);
    dc->Clear(Premul(0.01f, 0.02f, 0.04f, 0.72f));

    const int bands = (std::max)(10, static_cast<int>(ctx.width / 28.f));
    const float bandW = ctx.width / static_cast<float>(bands);
    const float alpha = 0.18f + 0.35f * ctx.intensity;

    for (int i = 0; i < bands; ++i) {
      const float alongN = (static_cast<float>(i) + 0.5f) / static_cast<float>(bands);
      const float wave =
          0.5f + 0.5f * std::sin(alongN * 6.2831853f * 1.5f + tCalm * 0.85f);
      const float wave2 =
          0.5f + 0.5f * std::sin(alongN * 6.2831853f * 2.7f - tCalm * 0.55f + 1.3f);
      const float hang = std::clamp(0.35f + 0.45f * wave + 0.20f * wave2, 0.f, 1.f);
      const float shaft =
          std::pow(0.5f + 0.5f * std::sin(alongN * 6.2831853f * 3.f + tCalm * 0.40f), 4.f);

      // Green → teal → violet down the curtain height.
      float r = 0.10f + 0.20f * hang;
      float g = 0.55f + 0.20f * (1.f - hang);
      float b = 0.25f + 0.55f * hang;
      r = r * (1.f - hang * 0.35f) + 0.35f * hang * hang;
      g = g * (0.85f + 0.15f * (1.f - hang));
      b = b + hang * 0.25f;

      const float glow = (0.55f + 0.45f * hang) * breath * (0.75f + 0.45f * shaft);
      const float a = alpha * glow * (0.55f + 0.45f * ctx.intensity);
      ComPtr<ID2D1SolidColorBrush> brush;
      dc->CreateSolidColorBrush(Premul(r, g, b, a), &brush);
      if (!brush) continue;

      const float top = ctx.height * (0.08f + 0.20f * (1.f - hang));
      const float bot = ctx.height;
      dc->FillRectangle(D2D1::RectF(i * bandW, top, (i + 1) * bandW + 1.f, bot), brush.Get());
    }
  }
};

}  // namespace

std::unique_ptr<ITaskbarEffect> CreateRainbowEffect() {
  return std::make_unique<RainbowEffect>();
}
std::unique_ptr<ITaskbarEffect> CreateSeaWavesEffect() {
  return std::make_unique<SeaWavesEffect>();
}
std::unique_ptr<ITaskbarEffect> CreateWeatherEffect() {
  return std::make_unique<WeatherEffect>();
}
std::unique_ptr<ITaskbarEffect> CreateSpaceEffect() {
  return std::make_unique<SpaceEffect>();
}
std::unique_ptr<ITaskbarEffect> CreateAuroraEffect() {
  return std::make_unique<AuroraEffect>();
}

std::unique_ptr<ITaskbarEffect> CreateEffect(ThemeId id) {
  switch (id) {
    case ThemeId::SeaWaves: return CreateSeaWavesEffect();
    case ThemeId::Weather:  return CreateWeatherEffect();
    case ThemeId::Space:    return CreateSpaceEffect();
    case ThemeId::Aurora:   return CreateAuroraEffect();
    case ThemeId::Rainbow:
    default:                return CreateRainbowEffect();
  }
}

}  // namespace phostide
