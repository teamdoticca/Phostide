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

  // Fixed ROYGBIV ribbons across thickness — hue never scrolls along the bar.
  // Life comes from seam undulation, traveling sheen, and per-band luminance pulse.
  const float tCalm = t * 0.35f;
  const float i = std::clamp(intensity, 0.f, 1.f);
  const float satBase = 0.82f + 0.16f * i;
  const float valueBase = 0.42f + 0.40f * i;
  const float alpha = 0.78f + 0.18f * i;

  // Clear sat/value breath loops (obvious on a ~48px bar).
  const float breathV = 0.84f + 0.16f * std::sin(tCalm * 0.78f);
  const float breathS = 0.90f + 0.10f * std::sin(tCalm * 0.55f + 1.1f);
  const float breathSlow = 0.92f + 0.08f * std::sin(tCalm * 0.32f + 2.4f);

  // Traveling specular crawl along the bar (luminance only — no hue drag).
  const float sheenTravel = tCalm * 1.15f;
  const float sheenPulse = 0.50f + 0.50f * std::sin(tCalm * 1.25f);
  const float iriPhase = tCalm * 0.70f;

  // Soft-quantized ROYGBIV centers (desktop→outer): R O Y G C B V.
  constexpr float kBandHue[7] = {0.00f, 0.08f, 0.14f, 0.33f, 0.50f, 0.62f, 0.78f};
  // Staggered pulse phases so ribbons don't flash in lockstep.
  constexpr float kBandPulsePhase[7] = {0.0f, 0.9f, 1.7f, 2.4f, 3.3f, 4.1f, 5.0f};

  for (int alongI = 0; alongI < along; ++alongI) {
    const float alongF = static_cast<float>(alongI);
    const float alongN =
        along <= 1 ? 0.f : static_cast<float>(alongI) / static_cast<float>(along - 1);

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

      // Stronger band-boundary undulation — edges breathe; spectrum order stays put.
      const float wobA =
          Noise2(alongF * 0.012f, tCalm * 0.34f + 1.3f) * 2.f - 1.f;
      const float wobB =
          Noise2(alongF * 0.028f + 3.7f, tCalm * 0.52f + 4.1f) * 2.f - 1.f;
      const float wobC = std::sin(alongF * 0.022f + tCalm * 0.95f) *
                         std::sin(tCalm * 0.42f + alongN * 6.2831853f * 0.5f);
      const float wobble =
          (0.048f + 0.028f * i) * (0.50f * wobA + 0.32f * wobB + 0.18f * wobC);
      const float band = std::clamp(column + wobble, 0.f, 1.f);

      // Soft ribbon quantization: clear ROYGBIV strips with tiny blend at seams.
      const float bandF = band * 6.999f;
      const int bandLo = static_cast<int>(std::floor(bandF));
      const int bandHi = (std::min)(6, bandLo + 1);
      const float bandFrac = bandF - static_cast<float>(bandLo);
      // Sharper centers, soft 18% seam blend so strips read on ~48px.
      const float seam = std::clamp((bandFrac - 0.41f) / 0.18f, 0.f, 1.f);
      const float seamS = seam * seam * (3.f - 2.f * seam);
      const float hue =
          kBandHue[bandLo] + (kBandHue[bandHi] - kBandHue[bandLo]) * seamS;

      // Per-ribbon bevel: highlight near top of each strip, soft shadow at bottom.
      const float inBand = bandFrac;
      const float bevelHi = std::exp(-std::abs(inBand - 0.22f) * 7.5f);
      const float bevelLo = std::exp(-std::abs(inBand - 0.82f) * 6.0f);
      const float bevel = 1.f + 0.20f * bevelHi - 0.14f * bevelLo;

      // Async per-band luminance pulse (keeps hues fixed).
      const float pulseLo =
          0.5f + 0.5f * std::sin(tCalm * 1.05f + kBandPulsePhase[bandLo]);
      const float pulseHi =
          0.5f + 0.5f * std::sin(tCalm * 1.05f + kBandPulsePhase[bandHi]);
      const float bandPulse = 0.86f + 0.14f * (pulseLo + (pulseHi - pulseLo) * seamS);

      // Stronger luminance caustics / paper grain (anti burn-in).
      const float n1 =
          Noise2(alongF * 0.018f + tCalm * 0.28f, band * 3.1f + tCalm * 0.20f);
      const float n2 =
          Noise2(alongF * 0.042f - tCalm * 0.18f, band * 5.2f + tCalm * 0.30f + 2.1f);
      const float n3 =
          Noise2(alongF * 0.008f + tCalm * 0.10f, band * 1.6f - tCalm * 0.12f + 5.5f);
      const float grain =
          Noise2(alongF * 0.55f + band * 8.f, band * 11.f + tCalm * 0.08f);
      const float caustic =
          0.58f + 0.24f * n1 + 0.12f * n2 + 0.04f * n3 + 0.04f * (grain - 0.5f);

      // Traveling sheen: bright ridge crawls along the bar over fixed ribbons.
      const float travel =
          0.5f + 0.5f * std::sin(alongN * 6.2831853f * 1.15f - sheenTravel + band * 0.8f);
      const float travel2 =
          0.5f +
          0.5f * std::sin(alongN * 6.2831853f * 2.4f + sheenTravel * 0.65f + band * 1.6f + 1.3f);
      const float bandBloom =
          0.5f + 0.5f * std::sin(band * 9.0f + tCalm * 1.35f + alongN * 1.2f);
      const float sheen =
          std::pow(travel, 2.2f) * (0.55f + 0.45f * travel2) *
          (0.60f + 0.40f * bandBloom) * sheenPulse;

      const float live = caustic * breathV * breathSlow * bevel * bandPulse *
                         (0.82f + 0.18f * sheen);

      const float sat =
          satBase * breathS * (0.94f + 0.06f * n1) * (0.96f + 0.04f * bandBloom);
      const float val = valueBase * live;
      float r, g, b;
      HsvToRgb(hue, std::clamp(sat, 0.f, 1.f), std::clamp(val, 0.f, 1.f), r, g, b);

      // Specular flash riding the traveling sheen (still no hue shift).
      const float sparkCore = std::pow(sheen, 4.5f);
      const float sparkSoft = std::pow(sheen, 2.6f);
      const float sparkAmp = (0.12f + 0.18f * i) * breathV;
      const float spark = (0.55f * sparkCore + 0.45f * sparkSoft) * sparkAmp;
      r += spark;
      g += spark * 0.94f;
      b += spark * 0.88f;

      // Secondary iridescent tint — phases in place on fixed hue bases.
      const float iri =
          (0.5f + 0.5f * std::sin(band * 12.5f + iriPhase + alongN * 2.0f)) *
          (0.5f + 0.5f * std::sin(tCalm * 0.85f + band * 3.0f));
      const float iriAmt = (0.04f + 0.07f * i) * iri * live;
      r += iriAmt * (0.55f + 0.45f * std::sin(hue * 6.2831853f + 0.4f));
      g += iriAmt * (0.55f + 0.45f * std::sin(hue * 6.2831853f + 2.5f));
      b += iriAmt * (0.55f + 0.45f * std::sin(hue * 6.2831853f + 4.6f));

      Put(bgra, stride, x, y, w, h,
          {std::clamp(r, 0.f, 1.f), std::clamp(g, 0.f, 1.f), std::clamp(b, 0.f, 1.f), alpha});
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

  // Side-on ocean strip: rolling waterline silhouette + water body + crest foam.
  // Shape reads as waves; no volumetric teal fog / caustic mist.
  const float thick = static_cast<float>((std::max)(16, thicknessPx > 0 ? thicknessPx : across));
  const float thinFactor = std::clamp(thick / 72.f, 0.45f, 1.35f);
  // ~150–200px primary wavelength on a 2560 bar.
  const float worldScale = 0.028f + 0.010f * thinFactor;
  const float amp = (0.55f + 0.35f * intensity) * (0.85f + 0.15f * thinFactor);
  const float tCalm = t * 0.42f;
  // Few clear swell components — readable peaks, not dense chop soup.
  GerstnerWave waves[] = {
      {1.00f, 0.08f, 0.55f * amp, 0.52f, 0.72f, 0.95f},
      {0.88f, 0.40f, 0.28f * amp, 0.40f, 1.15f, 1.20f},
      {0.55f, -0.72f, 0.14f * amp, 0.34f, 1.85f, 1.45f},
      {-0.20f, 0.98f, 0.08f * amp, 0.28f, 2.55f, 1.70f},
  };
  constexpr int kWaveCount = 4;

  const float zDrift = tCalm * 0.55f;

  std::vector<float> heightField(static_cast<size_t>(along));
  std::vector<float> nxField(static_cast<size_t>(along));
  float hMin = 1e9f, hMax = -1e9f;
  for (int i = 0; i < along; ++i) {
    float height, nx, nz;
    const float xW = static_cast<float>(i) * worldScale;
    GerstnerSample(xW, zDrift, tCalm, waves, kWaveCount, height, nx, nz);
    // Tiny surface ripple only — must not blur the silhouette.
    height += (Noise2(xW * 1.6f + tCalm * 0.5f, zDrift * 0.8f) * 2.f - 1.f) * 0.018f * amp;
    heightField[static_cast<size_t>(i)] = height;
    nxField[static_cast<size_t>(i)] = nx;
    hMin = (std::min)(hMin, height);
    hMax = (std::max)(hMax, height);
  }
  {
    std::vector<float> smooth = heightField;
    for (int i = 1; i < along - 1; ++i) {
      smooth[static_cast<size_t>(i)] =
          0.15f * heightField[static_cast<size_t>(i - 1)] +
          0.70f * heightField[static_cast<size_t>(i)] +
          0.15f * heightField[static_cast<size_t>(i + 1)];
    }
    heightField.swap(smooth);
  }
  const float hRange = (std::max)(0.08f, hMax - hMin);
  const float alpha = 0.90f + 0.08f * intensity;

  for (int alongI = 0; alongI < along; ++alongI) {
    const float hn =
        (heightField[static_cast<size_t>(alongI)] - hMin) / hRange;  // 0 trough → 1 crest
    const float nx = nxField[static_cast<size_t>(alongI)];
    const float steep = std::clamp(std::abs(nx) * 1.70f, 0.f, 1.f);
    const float alongF = static_cast<float>(alongI);
    const float crest = std::pow(std::clamp(hn, 0.f, 1.f), 1.65f);
    // Waterline across the bar: troughs sit low, crests climb toward desktop edge.
    const float surface = 0.18f + 0.62f * hn;

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

      const float d = column - surface;  // >0 above water, <0 in water
      float r = 0.f, g = 0.f, b = 0.f;

      // Breaking strength: steep faces throw spray; rounded crest peaks stay quieter.
      const float breakFace =
          std::pow(steep, 1.65f) * (0.25f + 0.75f * hn) * (0.35f + 0.65f * (1.f - crest * 0.55f));
      const float breakPulse =
          0.55f + 0.45f * std::sin(alongF * 0.031f - tCalm * 2.1f + hn * 2.8f);

      if (d > 0.f) {
        // Air above the waterline — near-black base, then airborne break foam.
        r = 0.008f;
        g = 0.012f;
        b = 0.022f;

        // Splash / spray ejecting above the surface on breaking faces.
        const float airH = std::clamp(d / 0.28f, 0.f, 1.f);  // 0 at lip → 1 higher
        const float airFade = std::exp(-airH * (2.4f + 1.6f * (1.f - breakFace)));
        const float spitU = alongF * 0.038f - tCalm * 4.2f - nx * 2.0f;
        const float spitV = airH * 3.2f + tCalm * 0.85f;
        const float s1 = Noise2(spitU, spitV);
        const float s2 = Noise2(spitU * 1.9f + 3.4f, spitV * 0.70f - tCalm * 0.9f);
        const float s3 = Noise2(spitU * 0.55f - 2.1f, spitV * 1.8f + 5.2f);
        // Chunked droplets + thin spit sheets (not a soft glow veil).
        const float drops = std::clamp((s1 * 0.50f + s2 * 0.50f - 0.46f) / 0.26f, 0.f, 1.f);
        const float sheets = std::clamp((s2 * 0.55f + s3 * 0.45f - 0.50f) / 0.28f, 0.f, 1.f);
        const float flecks = std::clamp((s3 - 0.62f) / 0.22f, 0.f, 1.f);
        const float airborne =
            std::clamp((0.70f * drops + 0.45f * sheets + 0.35f * flecks) * airFade *
                           breakFace * breakPulse * (0.85f + 0.25f * intensity),
                       0.f, 0.92f);
        r = r * (1.f - airborne) + (0.90f + 0.08f * intensity) * airborne;
        g = g * (1.f - airborne) + (0.95f + 0.04f * intensity) * airborne;
        b = b * (1.f - airborne) + (0.98f + 0.02f * intensity) * airborne;
      } else {
        // Water body: deep navy at depth → clearer ocean blue near the surface.
        const float depth = std::clamp(-d / (std::max)(0.12f, surface), 0.f, 1.f);
        const float nearSurf = 1.f - depth;
        const float faceLit =
            0.78f + 0.22f * std::clamp(0.35f + 0.65f * (-nx), 0.f, 1.f);
        // Soft internal banding that follows the swell (water texture, not fog).
        const float band =
            0.92f + 0.08f * std::sin(alongF * 0.018f + depth * 4.5f - tCalm * 1.1f);
        const float water = faceLit * band * (0.85f + 0.15f * intensity);

        r = (0.02f + 0.03f * nearSurf + 0.04f * crest * nearSurf) * water;
        g = (0.10f + 0.18f * nearSurf + 0.10f * crest * nearSurf + 0.06f * intensity) * water;
        b = (0.22f + 0.28f * nearSurf + 0.08f * crest * nearSurf + 0.10f * intensity) * water;
        g += 0.06f * nearSurf * crest * water;
        b += 0.04f * nearSurf * water;
        const float deepMul = 0.55f + 0.45f * nearSurf;
        r *= deepMul;
        g *= deepMul;
        b *= deepMul * (0.92f + 0.08f * nearSurf);

        // Foamy water under the ridgeline: chunky white streaks with blue gaps.
        // Must not read as a soft cyan fade — high-contrast, advected ribbons.
        const float underCrest = std::clamp((-d) / 0.48f, 0.f, 1.f);  // 0 at lip → 1 deeper
        const float depthFade = std::exp(-underCrest * 2.6f);         // near-lip band
        const float crestFoam = crest * depthFade * (0.75f + 0.35f * intensity);
        // Horizontal rush (~45–70px) so foam looks like flowing water.
        const float flowU = alongF * 0.020f - tCalm * 3.6f - nx * 1.3f;
        const float flowV = underCrest * 2.4f + tCalm * 0.50f;
        const float n1 = Noise2(flowU, flowV);
        const float n2 = Noise2(flowU * 1.80f + 2.8f, flowV * 0.55f - tCalm * 0.75f);
        const float n3 = Noise2(flowU * 0.42f - 1.7f, flowV * 1.70f + 4.5f);
        // Hard gates → bright foam OR clear blue (little mid-gray mush).
        const float streak = std::clamp((n1 * 0.52f + n2 * 0.48f - 0.40f) / 0.28f, 0.f, 1.f);
        const float lace = std::clamp((n2 * 0.55f + n3 * 0.45f - 0.45f) / 0.30f, 0.f, 1.f);
        const float dots = std::clamp((n3 - 0.58f) / 0.25f, 0.f, 1.f);
        const float pattern = std::clamp(0.08f + 0.78f * streak + 0.40f * lace + 0.30f * dots,
                                         0.f, 1.f);
        const float faceBoost = 0.75f + 0.25f * steep;
        const float churn =
            std::clamp(crestFoam * pattern * faceBoost, 0.f, 0.95f);
        r = r * (1.f - churn) + (0.88f + 0.10f * intensity) * churn;
        g = g * (1.f - churn) + (0.94f + 0.04f * intensity) * churn;
        b = b * (1.f - churn) + (0.98f + 0.02f * intensity) * churn;
      }

      // Foam lip: stronger on breaking faces, quieter on rounded peaks.
      const float lipDist = std::abs(d);
      const float lip =
          std::exp(-lipDist * (24.f - 8.f * steep)) *
          (0.15f + 0.55f * crest + 0.55f * breakFace) * (0.25f + 0.75f * steep);
      float foam = lip;
      // Burst just above the lip where the face is breaking (ties spit to surface).
      if (d > 0.f && d < 0.16f) {
        const float burst =
            std::pow(Noise2(alongF * 0.055f - tCalm * 3.4f, d * 11.f + tCalm * 0.7f), 2.8f) *
            breakFace * breakPulse * (1.f - d / 0.16f);
        foam = std::max(foam, burst * 0.85f);
      }
      foam = std::clamp(foam, 0.f, 0.94f);
      r = r * (1.f - foam) + (0.90f + 0.08f * intensity) * foam;
      g = g * (1.f - foam) + (0.95f + 0.04f * intensity) * foam;
      b = b * (1.f - foam) + (0.98f + 0.02f * intensity) * foam;

      Put(bgra, stride, x, y, w, h,
          {std::clamp(r, 0.f, 1.f), std::clamp(g, 0.f, 1.f), std::clamp(b, 0.f, 1.f), alpha});
    }
  }
}

// Calm OLED storm film: dark sky + wind-driven rain sheets + brief bolts.
// Seamless forward loop (~30s real with tCalm = t * 0.20); no cam rewind / no fade-in.
void PaintWeather(uint8_t* bgra, int stride, int w, int h, float intensity, float t,
                  TaskbarEdge edge) {
  const bool vertical = (edge == TaskbarEdge::Left || edge == TaskbarEdge::Right);
  const int along = vertical ? h : w;
  const int across = vertical ? w : h;
  if (along < 2 || across < 2) return;

  const float tCalm = t * 0.20f;
  constexpr float kPeriod = 6.0f;  // ~30s real time at tCalm = t * 0.20
  float phase = std::fmod(tCalm, kPeriod) / kPeriod;
  if (phase < 0.f) phase += 1.f;

  auto smoother = [](float x) {
    x = std::clamp(x, 0.f, 1.f);
    return x * x * (3.f - 2.f * x);
  };
  auto ramp = [&](float p, float a, float b) {
    if (b <= a) return p >= b ? 1.f : 0.f;
    return smoother((p - a) / (b - a));
  };
  auto pulse = [&](float p, float a, float b, float c, float d) {
    return ramp(p, a, b) * (1.f - ramp(p, c, d));
  };

  // Timeline (phase 0→1, seamless wrap into steady rain — no fade-in at 0):
  //   0.00–0.22  steady wind rain (full at wrap)
  //   0.18–0.52  build: denser sheets + harder slant
  //   0.44–0.78  storm peak + lightning windows
  //   0.72–1.00  ease back to steady (matches phase 0)
  const float build = pulse(phase, 0.18f, 0.30f, 0.50f, 0.62f);
  const float peak = pulse(phase, 0.44f, 0.54f, 0.70f, 0.82f);
  const float ease = ramp(phase, 0.72f, 0.96f);  // 0→1 toward calm again
  const float storm =
      std::clamp(0.22f + 0.55f * build + 0.85f * peak * (1.f - 0.75f * ease), 0.18f, 1.15f);

  // Wind slant grows mid-loop (diagonal rain — never straight-down snow).
  const float windBase = 0.55f + 0.12f * std::sin(tCalm * 0.37f + 0.6f);
  const float wind = windBase + 0.55f * build + 0.70f * peak * (1.f - ease);
  // Rain density / brightness envelope — push for OLED-readable slant.
  const float rainAmt =
      std::clamp(0.55f + 0.55f * intensity + 0.60f * storm, 0.45f, 1.55f);

  // Brief full-bar flashes (1–3 frames of film time ≈ short tCalm blips).
  auto flashEnv = [&](float center, float width, float seed) {
    const float d = std::fabs(phase - center);
    const float half = width * 0.5f;
    if (d > half) return 0.f;
    const float u = 1.f - d / half;
    // Hard-ish core so bolts read as strikes, soft wings for afterglow.
    return std::pow(smoother(u), 0.55f) *
           (0.75f + 0.25f * Hash21(seed, center * 17.f));
  };
  const float flashA = flashEnv(0.50f, 0.036f, 1.1f);  // main strike
  const float flashB = flashEnv(0.58f, 0.024f, 2.7f);  // quick follow
  const float flashC = flashEnv(0.66f, 0.028f, 4.3f);  // late fork
  const float flash =
      std::clamp(flashA * 1.15f + flashB * 0.82f + flashC * 0.95f, 0.f, 1.55f);

  const float breath = 0.92f + 0.08f * std::sin(tCalm * 0.41f);
  const float alpha = 0.82f + 0.14f * intensity;
  const float alongScale = static_cast<float>((std::max)(1, along));
  const float acrossScale = static_cast<float>((std::max)(1, across));

  // Wind direction in along/across UV (bottom bar: rain falls "down" toward desktop
  // = across+, drifting along+ with wind). Vertical edges remap similarly.
  const float windSign = (edge == TaskbarEdge::Right || edge == TaskbarEdge::Top) ? -1.f : 1.f;

  // --- Pass 1: dark storm sky + rain sheets (every pixel, anti burn-in) ---
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
        // Bottom: depth 1 = desktop side (top of bar / "skyward" rain origin feel).
        depth = (edge == TaskbarEdge::Bottom) ? (1.f - sampleV) : sampleV;
      } else {
        y = alongI;
        x = acrossI;
        depth = (edge == TaskbarEdge::Left) ? sampleV : (1.f - sampleV);
      }
      depth = std::clamp(depth, 0.f, 1.f);

      // Deep navy / charcoal storm base — weather.jpg contrast for OLED rain.
      const float skyN =
          Noise2(alongF * 0.006f + tCalm * 0.04f, depth * 2.1f + tCalm * 0.03f + 1.4f);
      const float skyN2 =
          Noise2(alongF * 0.018f - tCalm * 0.025f, depth * 3.6f + tCalm * 0.05f + 4.2f);
      const float murk = 0.42f + 0.32f * skyN + 0.20f * skyN2;
      float r = (0.025f + 0.025f * intensity) * murk * breath;
      float g = (0.035f + 0.035f * intensity) * murk * breath;
      float b = (0.070f + 0.065f * intensity) * murk * breath;
      // Cooler lift toward sky (outer) edge — storm volume.
      const float skyLift = 0.82f + 0.22f * (1.f - depth);
      r *= skyLift * 0.88f;
      g *= skyLift * 0.94f;
      b *= skyLift * 1.18f;

      // Slanted rain UV: fall along depth, wind shear along bar — diagonal sheets.
      const float fallSpeed = 1.70f + 1.00f * storm;
      const float shear = wind * windSign * 1.55f;
      const float rainU =
          alongU * (16.f + 8.f * storm) + depth * shear * 3.6f - tCalm * fallSpeed * 0.55f;
      const float rainV =
          depth * (10.f + 5.f * storm) - alongU * shear * 2.8f + tCalm * fallSpeed;

      // Layered sheets (fine streaks + denser curtains) — never vertical-only.
      const float sheetA = Noise2(rainU, rainV * 0.55f + 2.1f);
      const float sheetB = Noise2(rainU * 1.7f + 3.3f, rainV * 0.9f - tCalm * 0.2f);
      const float sheetC = Noise2(rainU * 3.1f - 1.7f, rainV * 1.4f + 5.5f);
      // Streak ridges: thin bright diagonals via absolute saw — readable on OLED.
      const float streak = 1.f - std::fabs(sheetA * 2.f - 1.f);
      const float streak2 = 1.f - std::fabs(sheetB * 2.f - 1.f);
      const float drops = std::pow(streak, 2.6f + 0.9f * (1.f - storm)) * 0.95f +
                          std::pow(streak2, 3.2f) * 0.62f +
                          std::pow(sheetC, 4.5f) * 0.38f * rainAmt;

      // Soft curtain veil so the bar reads wet, not sparse snow.
      const float veil =
          std::pow(std::clamp(0.35f + 0.55f * sheetA + 0.25f * sheetB, 0.f, 1.f), 1.45f) *
          (0.22f + 0.32f * rainAmt);

      const float rainVis =
          std::clamp((drops * 1.15f + veil) * rainAmt * (0.85f + 0.40f * storm), 0.f, 1.45f);
      // Cool electric rain highlights (not dim grey).
      const float rr = 0.62f + 0.22f * intensity;
      const float rg = 0.72f + 0.18f * intensity;
      const float rb = 0.92f + 0.08f * intensity;
      const float rainMix = std::clamp(rainVis * (0.48f + 0.42f * intensity), 0.f, 0.92f);
      r = r * (1.f - rainMix) + rr * rainMix;
      g = g * (1.f - rainMix) + rg * rainMix;
      b = b * (1.f - rainMix) + rb * rainMix;

      // Full-bar lightning wash (electric blue/white) — readable flash on OLED.
      if (flash > 0.01f) {
        const float wash = flash * (0.55f + 0.55f * intensity) *
                           (0.70f + 0.30f * Noise2(alongF * 0.02f, depth * 2.f + tCalm));
        r = std::clamp(r + wash * 0.68f, 0.f, 1.f);
        g = std::clamp(g + wash * 0.88f, 0.f, 1.f);
        b = std::clamp(b + wash * 1.15f, 0.f, 1.f);
      }

      // Micro-shimmer (anti burn-in) across every pixel.
      const float sheen =
          0.5f + 0.5f * std::sin(alongF * 0.009f + depth * 3.1f + tCalm * 0.62f);
      const float live = 0.93f + 0.04f * skyN2 + 0.03f * sheen;
      r = std::clamp(r * live, 0.f, 1.f);
      g = std::clamp(g * live, 0.f, 1.f);
      b = std::clamp(b * live, 0.f, 1.f);

      Put(bgra, stride, x, y, w, h, {r, g, b, alpha});
    }
  }

  // --- Pass 2: branching bolt silhouettes (glow polylines) during flash windows ---
  auto drawBolt = [&](float originAlong, float seed, float strength) {
    if (strength < 0.04f) return;
    // Start near sky edge (depth≈0), strike toward desktop edge (depth≈1).
    float ax = originAlong;
    float ay = 0.02f + 0.06f * Hash21(seed, 1.3f);
    const int segs = 5 + static_cast<int>(Hash21(seed, 2.9f) * 3.f);
    for (int s = 0; s < segs; ++s) {
      const float sf = static_cast<float>(s);
      const float prog = (sf + 1.f) / static_cast<float>(segs);
      const float jitter =
          (Hash21(seed + sf * 0.37f, 4.1f) * 2.f - 1.f) * (0.035f + 0.04f * (1.f - prog));
      const float branchChance = Hash21(seed + sf, 7.7f);
      float bx = ax + (Hash21(seed + sf, 5.5f) * 2.f - 1.f) * 0.045f + jitter;
      // Keep bolt traveling across the short axis.
      float by = ay + (0.14f + 0.10f * Hash21(seed + sf, 8.2f));
      by = std::clamp(by, 0.f, 1.f);
      bx = std::clamp(bx, 0.f, 1.f);

      auto plotSeg = [&](float u0, float v0, float u1, float v1, float glow) {
        const int steps = 6 + static_cast<int>(across * 0.35f);
        for (int i = 0; i <= steps; ++i) {
          const float tt = static_cast<float>(i) / static_cast<float>((std::max)(1, steps));
          const float u = u0 + (u1 - u0) * tt;
          const float v = v0 + (v1 - v0) * tt;
          int px = 0, py = 0;
          if (!vertical) {
            px = static_cast<int>(u * (alongScale - 1.f) + 0.5f);
            const float sampleV =
                (edge == TaskbarEdge::Bottom) ? (1.f - v) : v;
            py = static_cast<int>(sampleV * (acrossScale - 1.f) + 0.5f);
          } else {
            py = static_cast<int>(u * (alongScale - 1.f) + 0.5f);
            const float sampleV =
                (edge == TaskbarEdge::Left) ? v : (1.f - v);
            px = static_cast<int>(sampleV * (acrossScale - 1.f) + 0.5f);
          }
          const float core = glow * strength * (0.72f + 0.45f * intensity);
          const float br = 0.82f + 0.16f * intensity;
          const float bg = 0.92f + 0.08f * intensity;
          const float bb = 1.00f;
          Put(bgra, stride, px, py, w, h, {br, bg, bb, std::clamp(core, 0.f, 1.f)});
          Put(bgra, stride, px + 1, py, w, h, {br, bg, bb, std::clamp(core * 0.55f, 0.f, 1.f)});
          Put(bgra, stride, px - 1, py, w, h, {br, bg, bb, std::clamp(core * 0.55f, 0.f, 1.f)});
          Put(bgra, stride, px + 2, py, w, h, {br * 0.7f, bg * 0.85f, bb, std::clamp(core * 0.28f, 0.f, 1.f)});
          Put(bgra, stride, px - 2, py, w, h, {br * 0.7f, bg * 0.85f, bb, std::clamp(core * 0.28f, 0.f, 1.f)});
          if (across > 2) {
            Put(bgra, stride, px, py + 1, w, h,
                {br, bg, bb, std::clamp(core * 0.45f, 0.f, 1.f)});
            Put(bgra, stride, px, py - 1, w, h,
                {br, bg, bb, std::clamp(core * 0.45f, 0.f, 1.f)});
          }
        }
      };

      plotSeg(ax, ay, bx, by, 0.95f * (1.f - prog * 0.25f));

      // Occasional fork branch.
      if (branchChance > 0.62f && s >= 1 && s < segs - 1) {
        const float fx =
            std::clamp(bx + (branchChance * 2.f - 1.f) * 0.08f, 0.f, 1.f);
        const float fy = std::clamp(by + 0.12f + Hash21(seed + sf, 9.9f) * 0.10f, 0.f, 1.f);
        plotSeg(bx, by, fx, fy, 0.55f);
      }

      ax = bx;
      ay = by;
    }
  };

  if (flashA > 0.05f) {
    drawBolt(0.22f + 0.08f * Hash21(phase * 10.f, 1.f), 11.f, flashA);
    drawBolt(0.61f + 0.05f * Hash21(phase * 10.f, 3.f), 13.f, flashA * 0.75f);
  }
  if (flashB > 0.05f) {
    drawBolt(0.78f + 0.06f * Hash21(phase * 12.f, 2.f), 17.f, flashB);
  }
  if (flashC > 0.05f) {
    drawBolt(0.38f + 0.07f * Hash21(phase * 14.f, 5.f), 19.f, flashC);
    drawBolt(0.12f + 0.04f * Hash21(phase * 14.f, 6.f), 23.f, flashC * 0.65f);
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

  // Calm clock + looping film phase (~35s real with tCalm = t * 0.20).
  const float tCalm = t * 0.20f;
  constexpr float kPeriod = 7.0f;
  float phase = std::fmod(tCalm, kPeriod) / kPeriod;
  if (phase < 0.f) phase += 1.f;

  auto smoother = [](float x) {
    x = std::clamp(x, 0.f, 1.f);
    return x * x * (3.f - 2.f * x);
  };
  auto ramp = [&](float p, float a, float b) {
    if (b <= a) return p >= b ? 1.f : 0.f;
    return smoother((p - a) / (b - a));
  };
  // Soft pulse: rise a→b, hold b→c, fall c→d.
  auto pulse = [&](float p, float a, float b, float c, float d) {
    return ramp(p, a, b) * (1.f - ramp(p, c, d));
  };

  // Act envelopes (soft overlaps — no hard cuts).
  // A wide ~0.00–0.28 | B pan ~0.18–0.55 | C flight ~0.32–0.88 | arrive ~0.84–1.00
  // Loop closes FORWARD: next nebula ahead becomes Act A (no camX/camYaw rewind).
  // actA starts FULL at phase=0 (no fade-in) so wrap from arrive is seamless.
  const float actA = pulse(phase, 0.00f, 0.00f, 0.18f, 0.30f);
  const float actB = pulse(phase, 0.16f, 0.26f, 0.46f, 0.58f);
  // C overlaps B so stars/tunnel rise while the nebula is still exiting left.
  const float actC = pulse(phase, 0.28f, 0.40f, 0.82f, 0.94f);
  const float arrive = ramp(phase, 0.84f, 0.98f);  // next nebula approaches ahead
  // Starfield reveal tracks the pan exit (not waiting for full Act C).
  const float starsIn = ramp(phase, 0.18f, 0.38f);

  // Camera: along-bar pan (world scroll) + vanishing-point yaw (side → tunnel).
  // Positive camX scrolls world LEFT on screen = classic rightward pan (nebula
  // exits left, fresh void/stars enter from the right) on a bottom taskbar.
  // camX only increases within a loop; phase wrap remaps via worldPeriod below.
  constexpr float kCamXPanAmt = 0.92f;
  constexpr float kCamXFlyAmt = 0.38f;
  constexpr float kCamXArriveAmt = 0.30f;
  constexpr float kWorldPeriod = kCamXPanAmt + kCamXFlyAmt + kCamXArriveAmt;  // 1.60
  const float camXHold = 0.02f + 0.015f * std::sin(tCalm * 0.31f);
  const float camXPan = ramp(phase, 0.18f, 0.52f);            // Act B pan right
  const float camXFly = ramp(phase, 0.52f, 0.88f) * kCamXFlyAmt;  // keep drifting in C
  const float camXArrive = arrive * kCamXArriveAmt;          // keep pressing forward
  const float camX = camXHold + camXPan * kCamXPanAmt + camXFly + camXArrive;

  // 0 = looking sideways along nebula, 1 = centered depth tunnel into stars.
  // Yaw begins mid-pan so streaks read as the cloud leaves, not after a void beat.
  float camYaw = ramp(phase, 0.28f, 0.52f) * (1.f - arrive);
  camYaw = std::clamp(camYaw, 0.f, 1.f);

  // Nebula dominates A/B; thins hard in C; returns via the cloud ahead (arrive).
  const float nebPresence =
      std::clamp(0.10f + 0.88f * (actA * 0.95f + actB * 0.80f) +
                     (1.f - camYaw) * 0.22f * (1.f - actC) + arrive * 0.90f -
                     actC * 0.55f * camYaw,
                 0.04f, 1.15f);
  // Stars rise with the exit pan, peak in C; yield as next nebula fills the frame.
  const float starHero =
      std::clamp(0.08f + 0.70f * starsIn + 0.40f * actB + 0.90f * actC + 0.06f * actA,
                 0.f, 1.20f) *
      (1.f - arrive * 0.85f);

  const float breath = 0.90f + 0.10f * std::sin(tCalm * 0.48f);
  const float breath2 = 0.92f + 0.08f * std::sin(tCalm * 0.27f + 1.9f);
  const float alpha = 0.84f + 0.12f * intensity;
  const float alongScale = static_cast<float>((std::max)(1, along));

  // Persistent world nebula: warm amber/cream band like space.jpg + dark dust rift.
  // World X is screen-along + camX (pan right = scroll left through cloud).
  const float nebCoreWorld = 0.42f;  // Act A frames the warm core
  const float nebCoreDepth = 0.52f + 0.05f * std::sin(tCalm * 0.17f + 0.8f);
  const float nebAlongSigma = 0.36f;  // wide enough to stay obvious while panning
  const float nebDepthSigma = 0.62f;
  const float nebGain = (0.88f + 0.28f * intensity) * breath * nebPresence;

  // Vanishing point drifts from slightly off-center (sideways vibe) → dead center.
  const float vpAlongBias = (0.18f - 0.18f * camYaw) * (vertical ? 0.f : 1.f);
  const float cx = (static_cast<float>(w) - 1.f) * (0.50f + vpAlongBias * 0.5f);
  const float cy = (static_cast<float>(h) - 1.f) * 0.50f;
  const float halfW = (std::max)(1.f, static_cast<float>(w) * 0.5f);
  const float halfH = (std::max)(1.f, static_cast<float>(h) * 0.5f);
  // Isotropic px scale so star rush stays circular on any bar width/DPI (wide
  // 2560×48 bars used to squash angles via radius*halfW / radius*halfH).
  const float iso = (std::min)(halfW, halfH);
  const float maxReach = ((std::max)(halfW, halfH) / iso) * 1.15f;

  // --- Pass 1: void + warm volumetric nebula (camera-mapped world) ---
  for (int alongI = 0; alongI < along; ++alongI) {
    const float alongF = static_cast<float>(alongI);
    const float alongU = alongF / alongScale;
    const float worldAlong = alongU + camX;

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

      // Deep OLED void.
      float r = 0.004f + 0.006f * intensity;
      float g = 0.004f + 0.005f * intensity;
      float b = 0.008f + 0.010f * intensity;

      // As camYaw rises, nebula drifts left/behind (parallax) and thins.
      // Two cores: current + next ahead (period = camX travel) so phase wrap
      // lands on the same opening composition without reversing the pan.
      // Gate the ahead cloud with arrive only — otherwise camX drift shows it
      // peeking from the right during the star stretch after the first exit.
      const float nebShift = camYaw * 0.38f;
      const float dAlong0 = (worldAlong - nebCoreWorld - nebShift) / nebAlongSigma;
      const float dAlong1 =
          (worldAlong - (nebCoreWorld + kWorldPeriod) - nebShift) / nebAlongSigma;
      const float dDepth = (depth - nebCoreDepth) / nebDepthSigma;
      const float env0 = std::exp(-(dAlong0 * dAlong0 + dDepth * dDepth * 0.55f));
      const float env1Raw = std::exp(-(dAlong1 * dAlong1 + dDepth * dDepth * 0.55f));
      const float env1 = env1Raw * arrive;
      const float envelope = (std::max)(env0, env1);
      const float dAlong = (env1 > env0) ? dAlong1 : dAlong0;

      // Fibrous noise in world space — diagonal BL→TR filaments (reference vibe).
      // Sample relative to nearest core so the ahead cloud matches Act A texture.
      const float localAlong = worldAlong - ((env1 > env0) ? kWorldPeriod : 0.f);
      const float pxN = localAlong * 6.5f + depth * 1.8f;
      const float pyN = depth * 3.2f - localAlong * 1.1f;
      const float drift = tCalm * 0.028f;

      const float bulky = Noise2(pxN * 0.55f + drift * 0.4f, pyN * 0.55f - 1.7f);
      const float mid = Noise2(pxN * 1.35f - drift * 0.6f, pyN * 1.55f + 2.3f);
      const float fine = Noise2(pxN * 3.4f + drift, pyN * 3.8f + 5.1f);
      const float filU = localAlong * 9.0f + depth * 4.5f + drift * 0.5f;
      const float filV = (-localAlong * 2.2f + depth * 1.4f) * 0.55f;
      const float filament = Noise2(filU, filV + 3.7f);
      const float filament2 = Noise2(filU * 1.7f - 2.1f, filV * 1.3f + 8.2f);

      const float ridge = 1.f - std::fabs(filament * 2.f - 1.f);
      const float ridge2 = 1.f - std::fabs(filament2 * 2.f - 1.f);
      const float wisps = std::pow(ridge, 2.4f) * 0.65f + std::pow(ridge2, 3.1f) * 0.45f;
      const float volume =
          std::pow(bulky, 1.35f) * 0.55f + std::pow(mid, 1.8f) * 0.35f +
          std::pow(fine, 2.4f) * 0.22f + wisps * 0.40f;

      const float coreDist =
          std::sqrt(dAlong * dAlong * 0.85f + dDepth * dDepth * 0.35f);
      const float core = std::exp(-coreDist * coreDist * 2.8f) * breath2;
      const float halo = std::exp(-coreDist * coreDist * 0.85f);

      float density = envelope * (0.35f + 0.85f * volume) * nebGain;
      // Soft falloff between clouds (emptier starfield after first cloud's tail).
      const float midGap = nebCoreWorld + 0.46f;
      const float pastCloud =
          smoother(std::clamp((worldAlong - midGap) / 0.50f, 0.f, 1.f)) *
          (1.f - smoother(std::clamp(
                     (worldAlong - (nebCoreWorld + kWorldPeriod - 0.55f)) / 0.45f, 0.f,
                     1.f)));
      density *= 1.f - pastCloud * (0.88f + 0.10f * actC) * (1.f - arrive * 0.85f);
      // Extra veil-down once yawed into the tunnel so stars punch through.
      density *= 1.f - actC * camYaw * 0.62f;
      density = std::clamp(density, 0.f, 1.4f);
      density = density * density * (3.f - 2.f * std::clamp(density, 0.f, 1.f));

      // Color ramp: void → amber/sienna → burnt orange → cream (space.jpg).
      const float shade =
          std::clamp(density * 0.90f + core * 0.60f + halo * 0.28f, 0.f, 1.f);
      const float bodyR = 0.62f + 0.32f * shade;
      const float bodyG = 0.22f + 0.28f * shade;
      const float bodyB = 0.05f + 0.08f * shade;
      const float hotR = 1.00f;
      const float hotG = 0.52f + 0.22f * mid;
      const float hotB = 0.10f + 0.08f * fine;
      const float creamR = 1.00f;
      const float creamG = 0.94f;
      const float creamB = 0.82f;

      const float hotMix = std::clamp(density * 0.82f + halo * 0.40f, 0.f, 1.f);
      const float creamMix = std::clamp(core * (1.00f + 0.40f * intensity), 0.f, 1.f);

      float nr = bodyR;
      float ng = bodyG;
      float nb = bodyB;
      nr = nr + (hotR - nr) * hotMix;
      ng = ng + (hotG - ng) * hotMix;
      nb = nb + (hotB - nb) * hotMix;
      nr = nr + (creamR - nr) * creamMix;
      ng = ng + (creamG - ng) * creamMix;
      nb = nb + (creamB - nb) * creamMix;

      const float grain = (fine - 0.5f) * density * 0.14f;
      nr += grain * 0.9f;
      ng += grain * 0.45f;
      nb += grain * 0.15f;

      // Dark dust rift through the bright core (silhouette filament like space.jpg).
      const float rift =
          std::pow(1.f - std::fabs(filament * 2.f - 1.f), 3.2f) * envelope * (0.55f + 0.35f * mid);
      const float riftAmt = rift * (0.42f + 0.18f * intensity) * (0.55f + 0.45f * creamMix);
      nr *= 1.f - riftAmt * 0.88f;
      ng *= 1.f - riftAmt * 0.92f;
      nb *= 1.f - riftAmt * 0.70f;

      const float recess = (1.f - wisps) * envelope * 0.20f * (1.f - creamMix);
      nr *= 1.f - recess * 0.55f;
      ng *= 1.f - recess * 0.65f;
      nb *= 1.f - recess * 0.40f;

      const float amt = std::clamp(
          density * (0.62f + 0.48f * intensity) + creamMix * 0.42f, 0.f, 1.f);
      r = r * (1.f - amt) + nr * amt;
      g = g * (1.f - amt) + ng * amt;
      b = b * (1.f - amt) + nb * amt;

      const float bloom = creamMix * creamMix * (0.32f + 0.28f * intensity);
      r = std::clamp(r + bloom * 0.65f, 0.f, 1.f);
      g = std::clamp(g + bloom * 0.50f, 0.f, 1.f);
      b = std::clamp(b + bloom * 0.26f, 0.f, 1.f);

      // Anti burn-in micro-shimmer (whole bar).
      const float sheen =
          0.5f + 0.5f * std::sin(alongF * 0.006f + depth * 2.4f + tCalm * 0.55f);
      const float live = 0.94f + 0.04f * fine + 0.02f * sheen;
      r *= live;
      g *= live;
      b *= live;

      const float edgeFade = 0.88f + 0.12f * depth;
      r *= edgeFade;
      g *= edgeFade;
      b *= edgeFade;

      Put(bgra, stride, x, y, w, h,
          {std::clamp(r, 0.f, 1.f), std::clamp(g, 0.f, 1.f), std::clamp(b, 0.f, 1.f), alpha});
    }
  }

  // --- Pass 2: field stars + radial rush (drawn AFTER nebula so they punch through) ---
  // Dense cool-white sparkle — pan parallax only (never snow-falling / vertical drift).
  constexpr int kFieldStars = 150;
  const float fieldBoost =
      (0.40f + 0.55f * (1.f - std::min(nebPresence, 1.f)) + 0.55f * starsIn +
       0.35f * actB + 0.55f * actC + 0.25f * camYaw) *
      (0.70f + 0.40f * intensity);
  for (int i = 0; i < kFieldStars; ++i) {
    const float fi = static_cast<float>(i);
    const float hx = Hash21(fi + 0.13f, 2.77f);
    const float hy = Hash21(fi + 3.91f, 5.41f);
    const float hBright = Hash21(fi + 7.07f, 1.19f);
    const float hTw = Hash21(fi + 4.33f, 9.01f);
    const float hSize = Hash21(fi + 1.67f, 6.53f);

    // Parallax with rightward pan (horizontal only — no vertical drift).
    float sx = hx * static_cast<float>(w) - camX * alongScale * 0.42f;
    sx = sx - std::floor(sx / static_cast<float>(w)) * static_cast<float>(w);
    const float sy = hy * static_cast<float>(h);
    const float twinkle =
        0.72f + 0.28f * std::sin(tCalm * (0.55f + hTw * 0.4f) + hTw * 6.2831853f);
    const float fieldAmt =
        fieldBoost * twinkle * (0.50f + 0.55f * hBright) * (0.55f + 0.50f * starHero);
    if (fieldAmt < 0.03f) continue;
    // Cool-white / pale-gold cores against the void (space.jpg starfield).
    const float sr = 0.90f + 0.10f * hBright;
    const float sg = 0.92f + 0.07f * hBright;
    const float sb = 0.98f + 0.02f * hBright;
    const float a = std::clamp(alpha * fieldAmt * (0.72f + 0.28f * starHero), 0.f, 1.f);
    const int ix = static_cast<int>(sx + 0.5f);
    const int iy = static_cast<int>(sy + 0.5f);
    Put(bgra, stride, ix, iy, w, h, {sr, sg, sb, a});
    if (hSize > 0.62f && fieldAmt > 0.35f) {
      Put(bgra, stride, ix + 1, iy, w, h, {sr, sg, sb, a * 0.40f});
      Put(bgra, stride, ix - 1, iy, w, h, {sr, sg, sb, a * 0.40f});
      if (h > 2) {
        Put(bgra, stride, ix, iy + 1, w, h, {sr, sg, sb, a * 0.32f});
        Put(bgra, stride, ix, iy - 1, w, h, {sr, sg, sb, a * 0.32f});
      }
    }
  }

  // Radial tunnel toward vanishing point. On a thin bar, bias angles along the
  // bar axis so streaks stay readable instead of exiting top/bottom immediately.
  constexpr int kStarCount = 210;
  const float rushSpeed = 0.040f + 0.20f * starHero;
  const float streakScale = 0.55f + 2.35f * starHero;
  for (int i = 0; i < kStarCount; ++i) {
    const float fi = static_cast<float>(i);
    const float hAngle = Hash21(fi + 0.37f, 1.71f);
    const float hPhase = Hash21(fi + 2.11f, 4.93f);
    const float hSpeed = Hash21(fi + 5.57f, 8.23f);
    const float hBright = Hash21(fi + 9.41f, 3.17f);
    const float hWarm = Hash21(fi + 6.13f, 7.77f);
    const float hSpread = Hash21(fi + 8.29f, 2.41f);

    // Prefer near-horizontal rays on bottom/top bars; full circle on vertical edges.
    float angle;
    if (!vertical) {
      const float side = (hAngle < 0.5f) ? 0.f : 3.14159265f;
      const float spread = (0.10f + 0.22f * hSpread) * (1.f - 0.35f * camYaw);
      angle = side + (hAngle * 2.f - (hAngle < 0.5f ? 0.f : 1.f)) * 2.f * spread;
    } else {
      angle = hAngle * 6.2831853f;
    }
    const float cosA = std::cos(angle);
    const float sinA = std::sin(angle);
    const float speed = rushSpeed * (0.55f + hSpeed * 0.95f);
    float u = hPhase + tCalm * speed;
    u -= std::floor(u);
    // Ease out of the vanishing point a bit earlier so cores read sooner.
    const float uEase = u * u * (0.65f + 0.35f * u);
    const float radius = uEase * maxReach;

    const float dx = cosA * radius * iso;
    const float dy = sinA * radius * iso;
    const float px = cx + dx;
    const float py = cy + dy;

    const float streakPx =
        (0.8f + uEase * (3.8f + hSpeed * 4.2f) + starHero * 2.2f) * streakScale;
    const float len = std::sqrt(dx * dx + dy * dy);
    const float rdx = len > 1e-3f ? dx / len : cosA;
    const float rdy = len > 1e-3f ? dy / len : sinA;

    const float twinkle =
        0.80f + 0.20f * std::sin(tCalm * (1.05f + hSpeed * 0.55f) + hPhase * 6.2831853f);
    const float bright =
        (0.22f + 0.90f * uEase) * (0.55f + 0.45f * intensity) * (0.50f + 0.50f * hBright) *
        twinkle * (0.40f + 0.70f * starHero) *
        (0.72f + 0.28f * camYaw + 0.22f * actC);
    if (bright < 0.030f) continue;

    // Cool-white with tiny warmth variation — readable on dark void.
    const float warm = hWarm * 0.05f;
    const float sr = std::clamp(0.88f + warm, 0.f, 1.f);
    const float sg = std::clamp(0.92f + warm * 0.30f, 0.f, 1.f);
    const float sb = 1.00f;
    const float starA = std::clamp(alpha * (0.70f + 0.30f * bright), 0.f, 1.f);

    const int steps = 1 + static_cast<int>(streakPx + 0.5f);
    for (int s = 0; s < steps; ++s) {
      const float tS = static_cast<float>(s) / static_cast<float>((std::max)(1, steps));
      const float ox = px - rdx * streakPx * tS;
      const float oy = py - rdy * streakPx * tS;
      const float fall = 1.f - tS * 0.62f;
      const float a = std::clamp(starA * bright * fall, 0.f, 1.f);
      const int ix = static_cast<int>(ox + 0.5f);
      const int iy = static_cast<int>(oy + 0.5f);
      Put(bgra, stride, ix, iy, w, h, {sr, sg, sb, a});
      if (uEase > 0.22f && starHero > 0.35f && s == 0) {
        Put(bgra, stride, ix + 1, iy, w, h, {sr, sg, sb, a * 0.55f});
        Put(bgra, stride, ix - 1, iy, w, h, {sr, sg, sb, a * 0.55f});
        Put(bgra, stride, ix, iy + 1, w, h, {sr, sg, sb, a * 0.42f});
        Put(bgra, stride, ix, iy - 1, w, h, {sr, sg, sb, a * 0.42f});
      }
    }
  }
}

// Aurora borealis: soft green→teal→violet curtains hang from the outer edge
// toward the desktop. Standing fabric undulation along the bar (no hue marquee).
void PaintAurora(uint8_t* bgra, int stride, int w, int h, float intensity, float t,
                 TaskbarEdge edge) {
  const bool vertical = (edge == TaskbarEdge::Left || edge == TaskbarEdge::Right);
  const int along = vertical ? h : w;
  const int across = vertical ? w : h;
  if (along < 2 || across < 2) return;

  // Vertical magenta/violet curtains falling from sky → ground (reference aurora).
  const float tCalm = t * 0.48f;
  const float i = std::clamp(intensity, 0.f, 1.f);
  const float alpha = 0.82f + 0.16f * i;
  const float alongScale = static_cast<float>((std::max)(1, along - 1));
  const float breath = 0.88f + 0.12f * std::sin(tCalm * 0.55f);
  const float breath2 = 0.90f + 0.10f * std::sin(tCalm * 0.31f + 1.9f);

  // Curtain pillars wander along the bar so every region gets light over time.
  constexpr int kCurtains = 8;
  float cHome[kCurtains];
  float cPhase[kCurtains];
  float cWidth[kCurtains];
  float cHue[kCurtains];  // 0 violet → 1 magenta/pink
  float cDrift[kCurtains];
  for (int s = 0; s < kCurtains; ++s) {
    const float fs = static_cast<float>(s);
    cHome[s] = Hash21(fs + 0.37f, 1.91f);
    cPhase[s] = Hash21(fs + 2.83f, 6.41f);
    cWidth[s] = 0.026f + 0.052f * Hash21(fs + 0.77f, 3.55f);
    cHue[s] = Hash21(fs + 4.12f, 8.09f);
    // Unique speed + direction; wraps the bar in ~25–70s of calm time.
    const float spd = 0.014f + 0.030f * Hash21(fs + 5.55f, 9.17f);
    const float dir = (Hash21(fs + 6.02f, 1.33f) > 0.5f) ? 1.f : -1.f;
    cDrift[s] = spd * dir;
  }

  for (int alongI = 0; alongI < along; ++alongI) {
    const float alongF = static_cast<float>(alongI);
    const float alongN = alongF / alongScale;

    // Slow lateral fabric sway of the whole field.
    const float sway =
        0.012f * std::sin(alongN * 6.2831853f * 0.9f + tCalm * 0.42f) +
        0.007f * std::sin(alongN * 6.2831853f * 2.1f - tCalm * 0.28f + 1.3f);

    float curtainField = 0.f;
    float hueField = 0.f;
    float hueW = 0.f;
    for (int s = 0; s < kCurtains; ++s) {
      // Continuous drift + slow noise wander (not fixed hotspots).
      const float wander =
          0.10f * (Noise2(tCalm * 0.065f + cPhase[s] * 3.1f, cHome[s] * 5.7f) * 2.f - 1.f) +
          0.045f * std::sin(tCalm * (0.28f + cPhase[s] * 0.22f) + cPhase[s] * 6.2831853f);
      float pos = cHome[s] + cDrift[s] * tCalm + wander;
      pos -= std::floor(pos);  // wrap 0..1 across the full bar
      float d = alongN - pos - sway;
      d -= std::round(d);
      // Width also breathes a little so pillars don't look stamped.
      const float width =
          cWidth[s] * (0.85f + 0.25f * (0.5f + 0.5f * std::sin(tCalm * 0.45f + cPhase[s] * 4.f)));
      const float env = std::exp(-(d * d) / (width * width));
      // Can dip near-dark so a curtain "leaves" and later returns elsewhere.
      const float pulse =
          0.15f + 0.85f * (0.5f + 0.5f * std::sin(tCalm * (0.55f + cPhase[s] * 0.40f) +
                                                   cPhase[s] * 6.2831853f));
      const float contrib = env * pulse;
      curtainField = (std::max)(curtainField, contrib);
      // Hue drifts slowly per pillar so color patches also migrate.
      const float hueNow =
          std::fmod(cHue[s] + tCalm * (0.02f + 0.03f * cPhase[s]) + 1.f, 1.f);
      hueField += hueNow * contrib;
      hueW += contrib;
    }
    curtainField = std::clamp(curtainField, 0.f, 1.f);
    const float pillarHue = (hueW > 1e-4f) ? (hueField / hueW) : 0.5f;

    // Fine vertical ray density between pillars (combed light).
    const float rays =
        0.55f + 0.45f * Noise2(alongN * 28.f + tCalm * 0.15f, tCalm * 0.08f + 2.1f);

    for (int acrossI = 0; acrossI < across; ++acrossI) {
      const float sampleV =
          across <= 1 ? 0.5f : static_cast<float>(acrossI) / static_cast<float>(across - 1);
      // sky = desktop side (top of bottom-taskbar); ground = outer edge.
      float sky = sampleV;
      int x = 0, y = 0;
      if (!vertical) {
        x = alongI;
        y = acrossI;
        sky = (edge == TaskbarEdge::Bottom) ? (1.f - sampleV) : sampleV;
      } else {
        y = alongI;
        x = acrossI;
        sky = (edge == TaskbarEdge::Left) ? sampleV : (1.f - sampleV);
      }
      sky = std::clamp(sky, 0.f, 1.f);
      const float fall = 1.f - sky;  // 0 at sky root → 1 toward ground

      // Night indigo base.
      float r = 0.012f + 0.010f * i;
      float g = 0.010f + 0.008f * i;
      float b = 0.040f + 0.025f * i;

      // Sparse stars in the dark sky gaps (not inside bright curtains).
      const float starN = Noise2(alongF * 0.55f, sky * 11.f + 3.7f);
      const float star = std::pow(std::clamp((starN - 0.88f) / 0.12f, 0.f, 1.f), 3.f) *
                         (1.f - curtainField) * sky;
      r += star * 0.55f;
      g += star * 0.55f;
      b += star * 0.70f;

      // Falling curtain: bright at sky, soft tip toward ground + streaming grain.
      const float lengthMul = 0.70f + 0.45f * curtainField;
      const float tip = std::exp(-fall * fall * (2.4f / lengthMul));
      const float body = std::exp(-std::abs(fall - 0.35f) * (2.2f - 0.6f * curtainField));
      float curtain = std::clamp(tip * 0.95f + body * 0.75f, 0.f, 1.4f);
      curtain *= curtainField * rays * breath;

      // Vertical fall motion — light streams sky → ground (the curtain drop).
      const float flowV = fall * 5.5f - tCalm * 1.85f;
      const float flowU = alongN * 14.f + sway * 40.f;
      const float stream1 = Noise2(flowU, flowV);
      const float stream2 = Noise2(flowU * 1.7f + 2.4f, flowV * 0.85f + tCalm * 0.2f);
      const float stream3 = Noise2(alongN * 55.f + tCalm * 0.3f, fall * 8.f - tCalm * 2.4f);
      const float silk =
          0.55f + 0.28f * stream1 + 0.17f * stream2 + 0.12f * std::pow(stream3, 2.2f);
      curtain *= silk;

      // Palette from reference: deep violet → magenta → pink-red toward tips.
      // Hue varies per pillar; fall shifts violet (sky) → pink (lower).
      const float hMix = std::clamp(pillarHue * 0.55f + fall * 0.45f, 0.f, 1.f);
      // Violet
      const float vR = 0.42f, vG = 0.12f, vB = 0.95f;
      // Magenta
      const float mR = 0.92f, mG = 0.18f, mB = 0.78f;
      // Pink-red lower glow
      const float pR = 0.95f, pG = 0.28f, pB = 0.48f;
      float cr, cg, cb;
      if (hMix < 0.55f) {
        const float m = hMix / 0.55f;
        cr = vR + (mR - vR) * m;
        cg = vG + (mG - vG) * m;
        cb = vB + (mB - vB) * m;
      } else {
        const float m = (hMix - 0.55f) / 0.45f;
        cr = mR + (pR - mR) * m;
        cg = mG + (pG - mG) * m;
        cb = mB + (pB - mB) * m;
      }

      const float amt =
          std::clamp(curtain * (0.55f + 0.55f * i) * breath2, 0.f, 1.f);
      r = r * (1.f - amt) + cr * amt;
      g = g * (1.f - amt) + cg * amt;
      b = b * (1.f - amt) + cb * amt;

      // Soft bloom so pillars feel luminous, not hard strips.
      const float bloom = curtain * curtain * (0.12f + 0.16f * i);
      r = std::clamp(r + bloom * cr * 0.55f, 0.f, 1.f);
      g = std::clamp(g + bloom * cg * 0.35f, 0.f, 1.f);
      b = std::clamp(b + bloom * cb * 0.65f, 0.f, 1.f);

      // Horizon wash: pink glow near ground edge under strong curtains.
      const float wash = std::pow(fall, 2.2f) * curtainField * (0.10f + 0.14f * i);
      r = std::clamp(r + wash * 0.55f, 0.f, 1.f);
      g = std::clamp(g + wash * 0.12f, 0.f, 1.f);
      b = std::clamp(b + wash * 0.22f, 0.f, 1.f);

      // Keep voids between curtains dark (night sky).
      const float voidKeep = 0.55f + 0.45f * curtainField;
      r *= 0.70f + 0.30f * voidKeep;
      g *= 0.70f + 0.30f * voidKeep;
      b *= 0.75f + 0.25f * voidKeep;

      Put(bgra, stride, x, y, w, h,
          {std::clamp(r, 0.f, 1.f), std::clamp(g, 0.f, 1.f), std::clamp(b, 0.f, 1.f), alpha});
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
      PaintWeather(bgra, strideBytes, width, height, intensity, ctx.timeSeconds, ctx.edge);
      break;
    case ThemeId::Space:
      PaintSpace(bgra, strideBytes, width, height, intensity, ctx.timeSeconds, ctx.edge);
      break;
    case ThemeId::Aurora:
      PaintAurora(bgra, strideBytes, width, height, intensity, ctx.timeSeconds, ctx.edge);
      break;
    case ThemeId::Rainbow:
    default:
      PaintRainbow(bgra, strideBytes, width, height, intensity, ctx.timeSeconds, ctx.edge);
      break;
  }
}

}  // namespace phostide
