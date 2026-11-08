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
                  TaskbarEdge edge) {
  const bool vertical = (edge == TaskbarEdge::Left || edge == TaskbarEdge::Right);
  const int along = vertical ? h : w;
  const int across = vertical ? w : h;
  if (along < 2 || across < 2) return;

  // Fixed ROYGBIV ribbons across thickness; only light/atmosphere animates (no hue scroll).
  const float tCalm = t * 0.20f;
  const float satBase = 0.58f + 0.26f * intensity;
  const float valueBase = 0.18f + 0.24f * intensity;
  const float alpha = 0.64f + 0.26f * intensity;
  const float breath = 0.92f + 0.08f * std::sin(tCalm * 0.55f);

  for (int alongI = 0; alongI < along; ++alongI) {
    const float alongF = static_cast<float>(alongI);

    for (int acrossI = 0; acrossI < across; ++acrossI) {
      const float sampleV =
          across <= 1 ? 0.f : static_cast<float>(acrossI) / static_cast<float>(across - 1);
      float column = sampleV;
      int x = 0, y = 0;
      if (!vertical) {
        x = alongI;
        y = acrossI;
        // Bottom: column 1 = toward desktop (top of bar) → red; 0 = outer → violet.
        column = (edge == TaskbarEdge::Bottom) ? (1.f - sampleV) : sampleV;
      } else {
        y = alongI;
        x = acrossI;
        column = (edge == TaskbarEdge::Left) ? sampleV : (1.f - sampleV);
      }
      column = std::clamp(column, 0.f, 1.f);

      // Tiny band-boundary wobble — spectrum stays put, edges breathe.
      const float wobble =
          0.02f * (Noise2(alongF * 0.008f, tCalm * 0.15f + 1.3f) * 2.f - 1.f);
      const float band = std::clamp(column + wobble, 0.f, 1.f);
      // Stable hue: red (0) at desktop side → violet (~0.78) at outer edge.
      const float hue = band * 0.78f;

      // Atmospheric live motion on every pixel (anti burn-in, not a marquee).
      const float n1 = Noise2(alongF * 0.014f + tCalm * 0.12f, band * 2.8f + tCalm * 0.09f);
      const float n2 =
          Noise2(alongF * 0.032f - tCalm * 0.08f, band * 4.6f + tCalm * 0.14f + 2.1f);
      const float sheen =
          0.5f + 0.5f * std::sin(alongF * 0.011f + band * 3.2f + tCalm * 0.70f);
      const float live = (0.68f + 0.20f * n1 + 0.08f * n2 + 0.04f * sheen) * breath;

      const float sat = satBase * (0.90f + 0.10f * breath) * (0.92f + 0.08f * n1);
      const float val = valueBase * live;
      float r, g, b;
      HsvToRgb(hue, sat, val, r, g, b);

      // Soft in-place specular pulse (does not travel along the bar).
      const float spark = std::pow(sheen, 12.f) * (0.06f + 0.10f * intensity) * live;
      r += spark;
      g += spark * 0.95f;
      b += spark * 0.90f;

      Put(bgra, stride, x, y, w, h, {r, g, b, alpha});
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

// Calm OLED space trip: fly toward a vanishing point (radial star rush) through
// an obvious warm nebula (cream core / burnt sienna / wispy tendrils on black).
void PaintSpace(uint8_t* bgra, int stride, int w, int h, float intensity, float t,
                TaskbarEdge edge) {
  const bool vertical = (edge == TaskbarEdge::Left || edge == TaskbarEdge::Right);
  const int along = vertical ? h : w;
  const int across = vertical ? w : h;
  if (along < 2 || across < 2) return;

  const float tCalm = t * 0.20f;
  const float breath = 0.90f + 0.10f * std::sin(tCalm * 0.48f);
  const float breath2 = 0.92f + 0.08f * std::sin(tCalm * 0.27f + 1.9f);
  const float alpha = 0.84f + 0.12f * intensity;
  const float alongScale = static_cast<float>((std::max)(1, along));

  // Warm nebula rides with the journey: large cloud region near bar center,
  // slow along-drift so you fly past/through it for a long stretch.
  const float nebCoreAlong = 0.48f + 0.04f * std::sin(tCalm * 0.11f);
  const float nebCoreDepth = 0.50f + 0.06f * std::sin(tCalm * 0.17f + 0.8f);
  // Cover ~45–55% of bar length — must read clearly on ~48px thickness.
  const float nebAlongSigma = 0.22f;
  const float nebDepthSigma = 0.55f;
  const float nebGain = (0.72f + 0.28f * intensity) * breath;

  // --- Pass 1: void + warm volumetric nebula (every pixel) ---
  for (int alongI = 0; alongI < along; ++alongI) {
    const float alongF = static_cast<float>(alongI);
    const float alongU = alongF / alongScale;

    for (int acrossI = 0; acrossI < across; ++acrossI) {
      const float sampleV =
          across <= 1 ? 0.5f : static_cast<float>(acrossI) / static_cast<float>(across - 1);
      float depth = sampleV;
      int x = 0, y = 0;
      if (!vertical) {
        x = alongI;
        y = acrossI;
        depth = (edge == TaskbarEdge::Bottom) ? (1.f - sampleV) : sampleV;
      } else {
        y = alongI;
        x = acrossI;
        depth = (edge == TaskbarEdge::Left) ? sampleV : (1.f - sampleV);
      }
      depth = std::clamp(depth, 0.f, 1.f);

      // Deep OLED void — black canvas for the nebula.
      float r = 0.004f + 0.006f * intensity;
      float g = 0.004f + 0.005f * intensity;
      float b = 0.008f + 0.010f * intensity;

      const float dAlong = (alongU - nebCoreAlong) / nebAlongSigma;
      const float dDepth = (depth - nebCoreDepth) / nebDepthSigma;
      const float envelope = std::exp(-(dAlong * dAlong + dDepth * dDepth * 0.55f));

      // Local coords for fibrous noise — diagonal filament direction (BL→TR vibe).
      const float pxN = alongU * 6.5f + depth * 1.8f;
      const float pyN = depth * 3.2f - alongU * 1.1f;
      const float drift = tCalm * 0.035f;

      // Multi-scale volume: bulky gas + wispy tendrils + granular dust.
      const float bulky = Noise2(pxN * 0.55f + drift * 0.4f, pyN * 0.55f - 1.7f);
      const float mid = Noise2(pxN * 1.35f - drift * 0.6f, pyN * 1.55f + 2.3f);
      const float fine = Noise2(pxN * 3.4f + drift, pyN * 3.8f + 5.1f);
      // Anisotropic filaments: stretch noise along diagonal.
      const float filU = alongU * 9.0f + depth * 4.5f + drift * 0.5f;
      const float filV = (-alongU * 2.2f + depth * 1.4f) * 0.55f;
      const float filament = Noise2(filU, filV + 3.7f);
      const float filament2 = Noise2(filU * 1.7f - 2.1f, filV * 1.3f + 8.2f);

      // Ridged wisps (hair-like edges bleeding into black).
      const float ridge = 1.f - std::fabs(filament * 2.f - 1.f);
      const float ridge2 = 1.f - std::fabs(filament2 * 2.f - 1.f);
      const float wisps = std::pow(ridge, 2.4f) * 0.65f + std::pow(ridge2, 3.1f) * 0.45f;
      const float volume =
          std::pow(bulky, 1.35f) * 0.55f + std::pow(mid, 1.8f) * 0.35f +
          std::pow(fine, 2.4f) * 0.22f + wisps * 0.40f;

      // Soft core glow (cream-white) — distance in bar space, thick enough for 48px.
      const float coreDist =
          std::sqrt(dAlong * dAlong * 0.85f + dDepth * dDepth * 0.35f);
      const float core = std::exp(-coreDist * coreDist * 2.8f) * breath2;
      const float halo = std::exp(-coreDist * coreDist * 0.85f);

      float density = envelope * (0.35f + 0.85f * volume) * nebGain;
      density = std::clamp(density, 0.f, 1.4f);
      // Push midtones so gas is obvious, not a faint wash.
      density = density * density * (3.f - 2.f * std::clamp(density, 0.f, 1.f));

      // Color ramp: void → deep copper → burnt sienna → rusty orange → cream.
      const float shade = std::clamp(density * 0.85f + core * 0.55f + halo * 0.25f, 0.f, 1.f);
      // Deep copper / sienna body
      const float bodyR = 0.55f + 0.35f * shade;
      const float bodyG = 0.18f + 0.22f * shade;
      const float bodyB = 0.04f + 0.06f * shade;
      // Hot orange mid
      const float hotR = 0.95f;
      const float hotG = 0.42f + 0.18f * mid;
      const float hotB = 0.08f + 0.06f * fine;
      // Cream-white / pale peach core
      const float creamR = 1.00f;
      const float creamG = 0.92f;
      const float creamB = 0.78f;

      const float hotMix = std::clamp(density * 0.75f + halo * 0.35f, 0.f, 1.f);
      const float creamMix = std::clamp(core * (0.85f + 0.35f * intensity), 0.f, 1.f);

      float nr = bodyR;
      float ng = bodyG;
      float nb = bodyB;
      nr = nr + (hotR - nr) * hotMix;
      ng = ng + (hotG - ng) * hotMix;
      nb = nb + (hotB - nb) * hotMix;
      nr = nr + (creamR - nr) * creamMix;
      ng = ng + (creamG - ng) * creamMix;
      nb = nb + (creamB - nb) * creamMix;

      // Dust mottling in dense regions (painterly grain).
      const float grain = (fine - 0.5f) * density * 0.12f;
      nr += grain * 0.9f;
      ng += grain * 0.45f;
      nb += grain * 0.15f;

      // Dark brownish recesses where wisps thin out.
      const float recess = (1.f - wisps) * envelope * 0.18f * (1.f - creamMix);
      nr *= 1.f - recess * 0.55f;
      ng *= 1.f - recess * 0.65f;
      nb *= 1.f - recess * 0.40f;

      const float amt = std::clamp(density * (0.55f + 0.45f * intensity) + creamMix * 0.35f,
                                   0.f, 1.f);
      r = r * (1.f - amt) + nr * amt;
      g = g * (1.f - amt) + ng * amt;
      b = b * (1.f - amt) + nb * amt;

      // Extra cream bloom so the core pops on OLED even at ~48px.
      const float bloom = creamMix * creamMix * (0.25f + 0.20f * intensity);
      r = std::clamp(r + bloom * 0.55f, 0.f, 1.f);
      g = std::clamp(g + bloom * 0.42f, 0.f, 1.f);
      b = std::clamp(b + bloom * 0.22f, 0.f, 1.f);

      // Tiny living sheen across the whole bar (anti burn-in).
      const float sheen =
          0.5f + 0.5f * std::sin(alongF * 0.006f + depth * 2.4f + tCalm * 0.55f);
      const float live = 0.94f + 0.04f * fine + 0.02f * sheen;
      r *= live;
      g *= live;
      b *= live;

      // Slightly darker toward outer edge of thickness.
      const float edgeFade = 0.88f + 0.12f * depth;
      r *= edgeFade;
      g *= edgeFade;
      b *= edgeFade;

      Put(bgra, stride, x, y, w, h,
          {std::clamp(r, 0.f, 1.f), std::clamp(g, 0.f, 1.f), std::clamp(b, 0.f, 1.f), alpha});
    }
  }

  // --- Pass 2: radial star tunnel (in front of / through nebula) ---
  const float cx = (static_cast<float>(w) - 1.f) * 0.50f +
                   (vertical ? 0.f : alongScale * 0.02f);
  const float cy = (static_cast<float>(h) - 1.f) * 0.50f;
  const float halfW = (std::max)(1.f, static_cast<float>(w) * 0.5f);
  const float halfH = (std::max)(1.f, static_cast<float>(h) * 0.5f);
  const float maxReach =
      std::sqrt(halfW * halfW + halfH * halfH) / (std::min)(halfW, halfH) * 1.15f;

  constexpr int kStarCount = 110;
  for (int i = 0; i < kStarCount; ++i) {
    const float fi = static_cast<float>(i);
    const float hAngle = Hash21(fi + 0.37f, 1.71f);
    const float hPhase = Hash21(fi + 2.11f, 4.93f);
    const float hSpeed = Hash21(fi + 5.57f, 8.23f);
    const float hBright = Hash21(fi + 9.41f, 3.17f);
    const float hWarm = Hash21(fi + 6.13f, 7.77f);

    const float angle = hAngle * 6.2831853f;
    const float cosA = std::cos(angle);
    const float sinA = std::sin(angle);
    const float speed = 0.055f + hSpeed * 0.14f;
    float u = hPhase + tCalm * speed;
    u -= std::floor(u);
    const float u2 = u * u;
    const float radius = u2 * maxReach;

    const float dx = cosA * radius * halfW;
    const float dy = sinA * radius * halfH;
    const float px = cx + dx;
    const float py = cy + dy;

    const float streakPx = 0.6f + u2 * (3.5f + hSpeed * 2.5f);
    const float len = std::sqrt(dx * dx + dy * dy);
    const float rdx = len > 1e-3f ? dx / len : cosA;
    const float rdy = len > 1e-3f ? dy / len : sinA;

    const float twinkle =
        0.78f + 0.22f * std::sin(tCalm * (1.05f + hSpeed * 0.55f) + hPhase * 6.2831853f);
    const float bright =
        (0.20f + 0.75f * u2) * (0.55f + 0.45f * intensity) * (0.65f + 0.35f * hBright) *
        twinkle;
    if (bright < 0.02f) continue;

    // Cool-white stars punch through warm nebula.
    const float warm = hWarm * 0.08f;
    const float sr = std::clamp(0.90f + warm, 0.f, 1.f);
    const float sg = std::clamp(0.93f + warm * 0.35f, 0.f, 1.f);
    const float sb = 1.00f;
    const float starA = std::clamp(alpha * (0.60f + 0.40f * bright), 0.f, 1.f);

    const int steps = 1 + static_cast<int>(streakPx + 0.5f);
    for (int s = 0; s < steps; ++s) {
      const float tS = static_cast<float>(s) / static_cast<float>(steps);
      const float ox = px - rdx * streakPx * tS;
      const float oy = py - rdy * streakPx * tS;
      const float fall = 1.f - tS * 0.72f;
      const float a = starA * bright * fall;
      const int ix = static_cast<int>(ox + 0.5f);
      const int iy = static_cast<int>(oy + 0.5f);
      Put(bgra, stride, ix, iy, w, h, {sr, sg, sb, a});
      if (u2 > 0.35f && s == 0) {
        Put(bgra, stride, ix + 1, iy, w, h, {sr, sg, sb, a * 0.35f});
        Put(bgra, stride, ix - 1, iy, w, h, {sr, sg, sb, a * 0.35f});
        Put(bgra, stride, ix, iy + 1, w, h, {sr, sg, sb, a * 0.28f});
        Put(bgra, stride, ix, iy - 1, w, h, {sr, sg, sb, a * 0.28f});
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
      PaintSpace(bgra, strideBytes, width, height, intensity, ctx.timeSeconds, ctx.edge);
      break;
    case ThemeId::Rainbow:
    default:
      PaintRainbow(bgra, strideBytes, width, height, intensity, ctx.timeSeconds, ctx.edge);
      break;
  }
}

}  // namespace phostide
