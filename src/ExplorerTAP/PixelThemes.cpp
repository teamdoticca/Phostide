#include "PixelThemes.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace phostide {
namespace {

struct Rgba {
  float r, g, b, a;
};

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

void Put(uint8_t* bgra, int stride, int x, int y, int w, int h, Rgba c) {
  if (x < 0 || y < 0 || x >= w || y >= h) return;
  uint8_t* p = bgra + y * stride + x * 4;
  const float srcA = std::clamp(c.a, 0.f, 1.f);
  const float dstA = p[3] / 255.f;
  const float outA = srcA + dstA * (1.f - srcA);
  auto blend = [&](float src, uint8_t dstByte) {
    const float dst = dstByte / 255.f;
    if (outA <= 1e-5f) return static_cast<uint8_t>(0);
    return static_cast<uint8_t>(
        std::clamp((src * srcA + dst * dstA * (1.f - srcA)) / outA, 0.f, 1.f) * 255.f + 0.5f);
  };
  p[0] = blend(c.b, p[0]);
  p[1] = blend(c.g, p[1]);
  p[2] = blend(c.r, p[2]);
  p[3] = static_cast<uint8_t>(std::clamp(outA, 0.f, 1.f) * 255.f + 0.5f);
}

void Clear(uint8_t* bgra, int stride, int w, int h, Rgba c) {
  const uint8_t b = static_cast<uint8_t>(std::clamp(c.b, 0.f, 1.f) * 255.f + 0.5f);
  const uint8_t g = static_cast<uint8_t>(std::clamp(c.g, 0.f, 1.f) * 255.f + 0.5f);
  const uint8_t r = static_cast<uint8_t>(std::clamp(c.r, 0.f, 1.f) * 255.f + 0.5f);
  const uint8_t a = static_cast<uint8_t>(std::clamp(c.a, 0.f, 1.f) * 255.f + 0.5f);
  for (int y = 0; y < h; ++y) {
    uint8_t* row = bgra + y * stride;
    for (int x = 0; x < w; ++x) {
      row[x * 4 + 0] = b;
      row[x * 4 + 1] = g;
      row[x * 4 + 2] = r;
      row[x * 4 + 3] = a;
    }
  }
}

void PaintRainbow(uint8_t* bgra, int stride, int w, int h, float intensity, float t,
                  bool vertical) {
  const int along = vertical ? h : w;
  const int across = vertical ? w : h;
  const int bands = (std::max)(8, along / 24);
  const float bandW = static_cast<float>(along) / static_cast<float>(bands);
  const float value = 0.18f + 0.22f * intensity;
  const float alpha = 0.40f + 0.45f * intensity;
  for (int i = 0; i < bands; ++i) {
    float r, g, b;
    HsvToRgb((static_cast<float>(i) / bands) + t * 0.035f, 0.75f, value, r, g, b);
    const int a0 = static_cast<int>(i * bandW);
    const int a1 = static_cast<int>((i + 1) * bandW + 1.f);
    for (int acrossI = 0; acrossI < across; ++acrossI) {
      for (int alongI = a0; alongI < a1 && alongI < along; ++alongI) {
        const int x = vertical ? acrossI : alongI;
        const int y = vertical ? alongI : acrossI;
        Put(bgra, stride, x, y, w, h, {r, g, b, alpha});
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Gerstner / trochoidal waves — adapted from CaffeineViking/osgw (MIT)
// https://github.com/CaffeineViking/osgw  (Finch / GPU Gems Ch.1)
// ---------------------------------------------------------------------------
struct GerstnerWave {
  float dirX, dirZ;
  float amplitude;
  float steepness;
  float frequency;
  float speed;
};

void GerstnerSample(float x, float z, float time, const GerstnerWave* waves, int count,
                    float& heightOut, float& nxOut, float& nzOut) {
  float height = 0.f;
  float nx = 0.f;
  float ny = 1.f;
  float nz = 0.f;
  for (int i = 0; i < count; ++i) {
    const auto& w = waves[i];
    const float proj = x * w.dirX + z * w.dirZ;
    const float psi = proj * w.frequency + time * w.speed;
    const float s = std::sin(psi);
    const float c = std::cos(psi);
    height += w.amplitude * s;
    const float Af = w.amplitude * w.frequency;
    const float alpha = Af * s;
    ny -= w.steepness * alpha;
    const float omega = Af * c;
    nx -= w.dirX * omega;
    nz -= w.dirZ * omega;
  }
  heightOut = height;
  // Normalize xz of normal for lighting (ny kept relative).
  const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
  if (len > 1e-5f) {
    nxOut = nx / len;
    nzOut = nz / len;
  } else {
    nxOut = 0.f;
    nzOut = 0.f;
  }
}

void PaintSeaWavesGerstner(uint8_t* bgra, int stride, int w, int h, float intensity, float t,
                           TaskbarEdge edge, int thicknessPx) {
  const bool vertical = (edge == TaskbarEdge::Left || edge == TaskbarEdge::Right);
  const int along = vertical ? h : w;
  const int across = vertical ? w : h;
  if (along < 2 || across < 2) return;

  // Full-bar calm ocean: every pixel animated (anti burn-in), slow drift.
  const float thick = static_cast<float>((std::max)(16, thicknessPx > 0 ? thicknessPx : across));
  const float thinFactor = std::clamp(thick / 72.f, 0.45f, 1.35f);
  const float worldScale = 0.011f + 0.012f * thinFactor;
  const float amp = (0.34f + 0.36f * intensity) * (0.75f + 0.25f * thinFactor);
  const float tCalm = t * 0.20f;
  // Adapted from CaffeineViking/osgw (MIT) / GPU Gems Ch.1 — calm speeds.
  GerstnerWave waves[] = {
      {1.00f, 0.00f, 0.30f * amp, 0.26f, 0.68f, 0.30f},
      {0.94f, 0.28f, 0.20f * amp, 0.22f, 1.00f, 0.38f},
      {0.75f, -0.55f, 0.14f * amp, 0.20f, 1.30f, 0.46f},
      {-0.25f, 0.96f, 0.11f * amp, 0.24f, 1.65f, 0.52f},
      {0.58f, 0.78f, 0.08f * amp, 0.18f, 2.05f, 0.60f},
      {-0.82f, 0.42f, 0.07f * amp, 0.20f, 2.45f, 0.68f},
      {0.22f, -0.97f, 0.05f * amp, 0.16f, 2.95f, 0.76f},
      {0.96f, -0.22f, 0.04f * amp, 0.16f, 3.50f, 0.85f},
  };
  constexpr int kWaveCount = 8;

  std::vector<float> heightField(static_cast<size_t>(along));
  std::vector<float> nxField(static_cast<size_t>(along));
  float hMin = 1e9f, hMax = -1e9f;
  for (int i = 0; i < along; ++i) {
    float height, nx, nz;
    GerstnerSample(static_cast<float>(i) * worldScale, 0.08f * std::sin(tCalm * 0.30f), tCalm,
                   waves, kWaveCount, height, nx, nz);
    heightField[static_cast<size_t>(i)] = height;
    nxField[static_cast<size_t>(i)] = nx;
    hMin = (std::min)(hMin, height);
    hMax = (std::max)(hMax, height);
  }
  for (int pass = 0; pass < 2; ++pass) {
    std::vector<float> smooth = heightField;
    for (int i = 2; i < along - 2; ++i) {
      smooth[static_cast<size_t>(i)] =
          0.10f * heightField[static_cast<size_t>(i - 2)] +
          0.20f * heightField[static_cast<size_t>(i - 1)] +
          0.40f * heightField[static_cast<size_t>(i)] +
          0.20f * heightField[static_cast<size_t>(i + 1)] +
          0.10f * heightField[static_cast<size_t>(i + 2)];
    }
    heightField.swap(smooth);
  }
  const float hRange = (std::max)(0.08f, hMax - hMin);
  const float alpha = 0.82f + 0.14f * intensity;

  // Paint every pixel with visible slow motion (anti burn-in across full bar).
  for (int alongI = 0; alongI < along; ++alongI) {
    const float hn =
        (heightField[static_cast<size_t>(alongI)] - hMin) / hRange;  // 0 trough → 1 crest
    const float nx = nxField[static_cast<size_t>(alongI)];
    const float steep = std::clamp(std::abs(nx) * 1.35f, 0.f, 1.f);
    const float alongF = static_cast<float>(alongI);

    for (int acrossI = 0; acrossI < across; ++acrossI) {
      const float sampleU =
          across <= 1 ? 0.f : static_cast<float>(acrossI) / static_cast<float>(across - 1);
      float column = sampleU;
      int x = 0, y = 0;
      if (!vertical) {
        x = alongI;
        y = acrossI;
        column = (edge == TaskbarEdge::Bottom) ? (1.f - sampleU) : sampleU;
      } else {
        y = alongI;
        x = acrossI;
        column = (edge == TaskbarEdge::Left) ? sampleU : (1.f - sampleU);
      }
      column = std::clamp(column, 0.f, 1.f);

      // Multi-scale caustics: mid + fine, so every row/column keeps shifting.
      const float c1 = Noise2(alongF * 0.018f + tCalm * 0.22f, column * 3.2f + tCalm * 0.14f);
      const float c2 = Noise2(alongF * 0.045f - tCalm * 0.11f, column * 5.5f + tCalm * 0.19f + 2.4f);
      const float c3 = Noise2(alongF * 0.009f + tCalm * 0.07f, column * 1.6f - tCalm * 0.05f + 5.1f);
      const float caustic = 0.35f + 0.40f * c1 + 0.25f * c2 * c3;

      // Rolling luminance bands through the full thickness (not just a crest lip).
      const float rollA =
          0.5f + 0.5f * std::sin(alongF * 0.012f + column * 5.5f + tCalm * 0.85f + hn * 2.2f);
      const float rollB =
          0.5f + 0.5f * std::sin(alongF * 0.027f - column * 3.8f + tCalm * 1.15f + steep);
      const float live = 0.55f + 0.25f * caustic + 0.12f * rollA + 0.08f * rollB;

      // Whole-column lift from Gerstner height — troughs darker, crests brighter.
      const float swell = 0.72f + 0.38f * hn;

      // Ocean body: animated everywhere; still calm palette.
      float r = (0.03f + 0.05f * intensity + 0.07f * hn) * swell * live;
      float g = (0.12f + 0.18f * intensity + 0.16f * hn + 0.10f * column) * swell * live;
      float b = (0.26f + 0.28f * intensity + 0.20f * hn + 0.12f * column) * swell * live;
      // Mild depth gradient (outer a bit deeper) without killing motion.
      const float depthMul = 0.78f + 0.22f * column;
      r *= depthMul;
      g *= depthMul * 1.02f;
      b *= depthMul * 1.05f;

      // Soft specular from normals — broad, not a single pixel ridge.
      const float spec =
          std::pow(std::clamp(0.48f + 0.52f * (-nx), 0.f, 1.f), 9.f) *
          (0.10f + 0.16f * intensity) * (0.35f + 0.65f * live);
      r += spec;
      g += spec * 1.05f;
      b += spec * 1.12f;

      // Soft foam sheets that drift through most of the short axis.
      const float surfaceU = 0.28f + 0.44f * hn;
      const float foamCore = std::exp(-std::abs(column - surfaceU) * (2.4f - 0.6f * steep));
      const float foamSheet =
          0.55f * foamCore + 0.25f * std::exp(-std::abs(column - 0.55f * surfaceU) * 3.2f) +
          0.15f * caustic * (0.4f + 0.6f * column);
      const float foam = std::clamp(foamSheet * (0.30f + 0.35f * steep + 0.20f * hn), 0.f, 0.72f);
      r = r * (1.f - foam) + (0.70f + 0.14f * intensity) * foam;
      g = g * (1.f - foam) + (0.82f + 0.10f * intensity) * foam;
      b = b * (1.f - foam) + 0.94f * foam;

      Put(bgra, stride, x, y, w, h, {r, g, b, alpha});
    }
  }
}

void PaintWeather(uint8_t* bgra, int stride, int w, int h, float intensity, float t) {
  const float flicker = 0.02f * intensity * std::sin(t * 1.3f + Noise2(t, 2.f));
  const float base = 0.10f + 0.12f * intensity;
  Clear(bgra, stride, w, h, {base + flicker, base + flicker * 0.9f, base + 0.03f, 0.72f});
  for (int i = 0; i < 5; ++i) {
    const float ct = t * (0.05f + i * 0.01f) + i * 12.f;
    const float cx = std::fmod(ct * 40.f + i * 90.f, static_cast<float>(w + 120)) - 60.f;
    const float cy = h * (0.3f + 0.4f * Hash21(static_cast<float>(i), 3.f));
    const float radius = 28.f + 40.f * Hash21(static_cast<float>(i), 7.f);
    const float a = (0.10f + 0.14f * intensity) * (0.6f + 0.4f * Noise2(cx * 0.01f, ct));
    const int x0 = std::max(0, static_cast<int>(cx - radius));
    const int x1 = std::min(w, static_cast<int>(cx + radius));
    const int y0 = std::max(0, static_cast<int>(cy - radius * 0.55f));
    const int y1 = std::min(h, static_cast<int>(cy + radius * 0.55f));
    for (int y = y0; y < y1; ++y) {
      for (int x = x0; x < x1; ++x) {
        const float dx = (x - cx) / radius;
        const float dy = (y - cy) / (radius * 0.55f);
        const float d = dx * dx + dy * dy;
        if (d > 1.f) continue;
        Put(bgra, stride, x, y, w, h, {0.75f, 0.8f, 0.9f, a * (1.f - d)});
      }
    }
  }
}

void PaintSpace(uint8_t* bgra, int stride, int w, int h, float intensity, float t) {
  float nr, ng, nb;
  HsvToRgb(std::fmod(t * 0.02f, 1.f), 0.55f, 0.35f, nr, ng, nb);
  const float nebula = 0.05f + 0.10f * intensity;
  Clear(bgra, stride, w, h, {nr * nebula, ng * nebula, nb * nebula, 0.78f});
  for (int i = 0; i < 64; ++i) {
    const float seed = static_cast<float>(i);
    const float speed = 0.01f + Hash21(seed, 1.f) * 0.04f;
    const float size = 0.6f + Hash21(seed, 2.f) * 1.8f;
    const float phase = Hash21(seed, 3.f) * 6.28f;
    float x = std::fmod(Hash21(seed, 4.f) + t * speed, 1.f);
    if (x < 0.f) x += 1.f;
    const float y = Hash21(seed, 5.f);
    const float twinkle = 0.5f + 0.5f * std::sin(t * 3.f + phase);
    const float rad = size * (0.6f + 0.4f * twinkle);
    const float a = 0.35f + 0.45f * intensity;
    const int cx = static_cast<int>(x * w);
    const int cy = static_cast<int>(y * h);
    const int r = std::max(1, static_cast<int>(rad + 0.5f));
    for (int py = cy - r; py <= cy + r; ++py) {
      for (int px = cx - r; px <= cx + r; ++px) {
        const float dx = static_cast<float>(px - cx);
        const float dy = static_cast<float>(py - cy);
        if (dx * dx + dy * dy > rad * rad) continue;
        Put(bgra, stride, px, py, w, h, {0.9f, 0.95f, 1.f, a});
      }
    }
  }
}

}  // namespace

void PaintThemePixels(const PaintContext& ctx, uint8_t* bgra, int width, int height,
                      int strideBytes) {
  if (!bgra || width <= 0 || height <= 0 || strideBytes < width * 4) return;
  const float intensity = std::clamp(ctx.intensity, 0.f, 1.f);
  std::fill(bgra, bgra + strideBytes * height, static_cast<uint8_t>(0));

  switch (ctx.theme) {
    case ThemeId::SeaWaves:
      PaintSeaWavesGerstner(bgra, strideBytes, width, height, intensity, ctx.timeSeconds, ctx.edge,
                            ctx.thickness > 0 ? ctx.thickness : (ctx.vertical ? width : height));
      break;
    case ThemeId::Weather:
      PaintWeather(bgra, strideBytes, width, height, intensity, ctx.timeSeconds);
      break;
    case ThemeId::Space:
      PaintSpace(bgra, strideBytes, width, height, intensity, ctx.timeSeconds);
      break;
    case ThemeId::Rainbow:
    default:
      PaintRainbow(bgra, strideBytes, width, height, intensity, ctx.timeSeconds, ctx.vertical);
      break;
  }
}

}  // namespace phostide
