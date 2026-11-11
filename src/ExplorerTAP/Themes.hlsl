cbuffer ThemeCB : register(b0) {
  float time      : packoffset(c0.x);
  float intensity : packoffset(c0.y);
  float width     : packoffset(c0.z);
  float height    : packoffset(c0.w);
  uint themeId    : packoffset(c1.x);
  uint edge       : packoffset(c1.y);
  float quality   : packoffset(c1.z); // 1.0 baseline; >1 denser Space on live GPU
  float _pad1     : packoffset(c1.w);
};

struct VSOut {
  float4 pos : SV_Position;
  float2 uv  : TEXCOORD0;
};

VSOut VSMain(uint id : SV_VertexID) {
  VSOut o;
  float2 uv = float2((id << 1) & 2, id & 2);
  o.pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
  o.uv = uv;
  return o;
}

float Hash21(float2 p) {
  float n = sin(dot(p, float2(127.1, 311.7))) * 43758.5453;
  return frac(n);
}

float Noise2(float2 p) {
  float2 i = floor(p);
  float2 f = frac(p);
  float2 u = f * f * (3.0 - 2.0 * f);
  float a = Hash21(i);
  float b = Hash21(i + float2(1, 0));
  float c = Hash21(i + float2(0, 1));
  float d = Hash21(i + float2(1, 1));
  return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float Smoother(float x) {
  x = saturate(x);
  return x * x * (3.0 - 2.0 * x);
}

float3 HsvToRgb(float h, float s, float v) {
  h = frac(h);
  float3 rgb = saturate(abs(fmod(h * 6.0 + float3(0, 4, 2), 6.0) - 3.0) - 1.0);
  return v * lerp(float3(1, 1, 1), rgb, s);
}

float4 Premul(float3 rgb, float a) {
  a = saturate(a);
  return float4(saturate(rgb) * a, a);
}

void MapEdge(float2 uv, out float alongN, out float acrossN, out float column,
             out float alongF, out bool vertical) {
  vertical = (edge == 2 || edge == 3);
  float alongPx = vertical ? height : width;
  if (!vertical) {
    alongN = uv.x;
    acrossN = uv.y;
    column = (edge == 0) ? (1.0 - acrossN) : acrossN;
  } else {
    alongN = uv.y;
    acrossN = uv.x;
    column = (edge == 2) ? acrossN : (1.0 - acrossN);
  }
  column = saturate(column);
  alongF = alongN * max(alongPx, 1.0);
}

float4 PaintRainbow(float2 uv) {
  float alongN, acrossN, column, alongF;
  bool vertical;
  MapEdge(uv, alongN, acrossN, column, alongF, vertical);
  float tCalm = time * 0.35;
  float i = saturate(intensity);

  float breathV = 0.84 + 0.16 * sin(tCalm * 0.78);
  float breathS = 0.90 + 0.10 * sin(tCalm * 0.55 + 1.1);
  float breathSlow = 0.92 + 0.08 * sin(tCalm * 0.32 + 2.4);
  float sheenTravel = tCalm * 1.15;
  float sheenPulse = 0.50 + 0.50 * sin(tCalm * 1.25);
  float iriPhase = tCalm * 0.70;
  float satBase = 0.82 + 0.16 * i;
  float valueBase = 0.42 + 0.40 * i;
  float alpha = 0.78 + 0.18 * i;

  float wobA = Noise2(float2(alongF * 0.012, tCalm * 0.34 + 1.3)) * 2.0 - 1.0;
  float wobB = Noise2(float2(alongF * 0.028 + 3.7, tCalm * 0.52 + 4.1)) * 2.0 - 1.0;
  float wobC = sin(alongF * 0.022 + tCalm * 0.95) * sin(tCalm * 0.42 + alongN * 3.14159265);
  float wobble = (0.048 + 0.028 * i) * (0.50 * wobA + 0.32 * wobB + 0.18 * wobC);
  float band = saturate(column + wobble);

  float kBandHue[7] = {0.00, 0.08, 0.14, 0.33, 0.50, 0.62, 0.78};
  float kPulse[7] = {0.0, 0.9, 1.7, 2.4, 3.3, 4.1, 5.0};
  float bandF = band * 6.999;
  int bandLo = (int)floor(bandF);
  int bandHi = min(6, bandLo + 1);
  float bandFrac = bandF - (float)bandLo;
  float seam = saturate((bandFrac - 0.41) / 0.18);
  float seamS = seam * seam * (3.0 - 2.0 * seam);
  float hue = lerp(kBandHue[bandLo], kBandHue[bandHi], seamS);

  float bevelHi = exp(-abs(bandFrac - 0.22) * 7.5);
  float bevelLo = exp(-abs(bandFrac - 0.82) * 6.0);
  float bevel = 1.0 + 0.20 * bevelHi - 0.14 * bevelLo;

  float pulseLo = 0.5 + 0.5 * sin(tCalm * 1.05 + kPulse[bandLo]);
  float pulseHi = 0.5 + 0.5 * sin(tCalm * 1.05 + kPulse[bandHi]);
  float bandPulse = 0.86 + 0.14 * lerp(pulseLo, pulseHi, seamS);

  float n1 = Noise2(float2(alongF * 0.018 + tCalm * 0.28, band * 3.1 + tCalm * 0.20));
  float n2 = Noise2(float2(alongF * 0.042 - tCalm * 0.18, band * 5.2 + tCalm * 0.30 + 2.1));
  float n3 = Noise2(float2(alongF * 0.008 + tCalm * 0.10, band * 1.6 - tCalm * 0.12 + 5.5));
  float grain = Noise2(float2(alongF * 0.55 + band * 8.0, band * 11.0 + tCalm * 0.08));
  float caustic = 0.58 + 0.24 * n1 + 0.12 * n2 + 0.04 * n3 + 0.04 * (grain - 0.5);

  float travel = 0.5 + 0.5 * sin(alongN * 6.2831853 * 1.15 - sheenTravel + band * 0.8);
  float travel2 = 0.5 + 0.5 * sin(alongN * 6.2831853 * 2.4 + sheenTravel * 0.65 + band * 1.6 + 1.3);
  float bandBloom = 0.5 + 0.5 * sin(band * 9.0 + tCalm * 1.35 + alongN * 1.2);
  float sheen = pow(travel, 2.2) * (0.55 + 0.45 * travel2) * (0.60 + 0.40 * bandBloom) * sheenPulse;

  float live = caustic * breathV * breathSlow * bevel * bandPulse * (0.82 + 0.18 * sheen);
  float sat = satBase * breathS * (0.94 + 0.06 * n1) * (0.96 + 0.04 * bandBloom);
  float val = valueBase * live;
  float3 rgb = HsvToRgb(hue, saturate(sat), saturate(val));

  float sparkCore = pow(sheen, 4.5);
  float sparkSoft = pow(sheen, 2.6);
  float sparkAmp = (0.12 + 0.18 * i) * breathV;
  float spark = (0.55 * sparkCore + 0.45 * sparkSoft) * sparkAmp;
  rgb += float3(spark, spark * 0.94, spark * 0.88);

  float iri = (0.5 + 0.5 * sin(band * 12.5 + iriPhase + alongN * 2.0)) *
              (0.5 + 0.5 * sin(tCalm * 0.85 + band * 3.0));
  float iriAmt = (0.04 + 0.07 * i) * iri * live;
  rgb.r += iriAmt * (0.55 + 0.45 * sin(hue * 6.2831853 + 0.4));
  rgb.g += iriAmt * (0.55 + 0.45 * sin(hue * 6.2831853 + 2.5));
  rgb.b += iriAmt * (0.55 + 0.45 * sin(hue * 6.2831853 + 4.6));

  return Premul(rgb, alpha);
}

void Gerstner(float x, float z, float t, float amp, out float height, out float nx) {
  // dirX,dirZ, ampMul, steep, freq, speed — matches PixelThemes Sea swell set.
  float2 dirs[4] = { float2(1.00, 0.08), float2(0.88, 0.40), float2(0.55, -0.72), float2(-0.20, 0.98) };
  float amps[4] = { 0.55, 0.28, 0.14, 0.08 };
  float steeps[4] = { 0.52, 0.40, 0.34, 0.28 };
  float freqs[4] = { 0.72, 1.15, 1.85, 2.55 };
  float speeds[4] = { 0.95, 1.20, 1.45, 1.70 };
  height = 0.0;
  nx = 0.0;
  float ny = 1.0;
  float nz = 0.0;
  [unroll] for (int i = 0; i < 4; ++i) {
    float A = amps[i] * amp;
    float proj = x * dirs[i].x + z * dirs[i].y;
    float psi = proj * freqs[i] + t * speeds[i];
    float s = sin(psi);
    float c = cos(psi);
    height += A * s;
    float Af = A * freqs[i];
    float alpha = Af * s;
    ny -= steeps[i] * alpha;
    float omega = Af * c;
    nx -= dirs[i].x * omega;
    nz -= dirs[i].y * omega;
  }
  float len = sqrt(nx * nx + ny * ny + nz * nz);
  nx = (len > 1e-5) ? (nx / len) : 0.0;
}

float4 PaintSea(float2 uv) {
  float alongN, acrossN, column, alongF;
  bool vertical;
  MapEdge(uv, alongN, acrossN, column, alongF, vertical);
  float tCalm = time * 0.42;
  float i = saturate(intensity);
  float acrossPx = vertical ? width : height;
  float thick = max(16.0, acrossPx);
  float thinFactor = clamp(thick / 72.0, 0.45, 1.35);
  float worldScale = 0.028 + 0.010 * thinFactor;
  float amp = (0.55 + 0.35 * i) * (0.85 + 0.15 * thinFactor);
  float zDrift = tCalm * 0.55;
  float xW = alongF * worldScale;

  float height, nx;
  Gerstner(xW, zDrift, tCalm, amp, height, nx);
  height += (Noise2(float2(xW * 1.6 + tCalm * 0.5, zDrift * 0.8)) * 2.0 - 1.0) * 0.018 * amp;
  // Soft neighbor blend (CPU smooths the height field).
  float hL, hR, nxIgnore;
  Gerstner((alongF - 1.0) * worldScale, zDrift, tCalm, amp, hL, nxIgnore);
  Gerstner((alongF + 1.0) * worldScale, zDrift, tCalm, amp, hR, nxIgnore);
  height = 0.15 * hL + 0.70 * height + 0.15 * hR;

  float hRange = max(0.08, amp * 2.05);
  float hn = saturate((height + amp * 1.02) / hRange);
  float steep = saturate(abs(nx) * 1.70);
  float crest = pow(hn, 1.65);
  float surface = 0.18 + 0.62 * hn;
  float d = column - surface;
  float alpha = 0.90 + 0.08 * i;

  float breakFace = pow(steep, 1.65) * (0.25 + 0.75 * hn) * (0.35 + 0.65 * (1.0 - crest * 0.55));
  float breakPulse = 0.55 + 0.45 * sin(alongF * 0.031 - tCalm * 2.1 + hn * 2.8);

  float3 rgb;
  if (d > 0.0) {
    rgb = float3(0.008, 0.012, 0.022);
    float airH = saturate(d / 0.28);
    float airFade = exp(-airH * (2.4 + 1.6 * (1.0 - breakFace)));
    float spitU = alongF * 0.038 - tCalm * 4.2 - nx * 2.0;
    float spitV = airH * 3.2 + tCalm * 0.85;
    float s1 = Noise2(float2(spitU, spitV));
    float s2 = Noise2(float2(spitU * 1.9 + 3.4, spitV * 0.70 - tCalm * 0.9));
    float s3 = Noise2(float2(spitU * 0.55 - 2.1, spitV * 1.8 + 5.2));
    float drops = saturate((s1 * 0.50 + s2 * 0.50 - 0.46) / 0.26);
    float sheets = saturate((s2 * 0.55 + s3 * 0.45 - 0.50) / 0.28);
    float flecks = saturate((s3 - 0.62) / 0.22);
    float airborne = saturate((0.70 * drops + 0.45 * sheets + 0.35 * flecks) * airFade *
                              breakFace * breakPulse * (0.85 + 0.25 * i));
    airborne = min(airborne, 0.92);
    float3 spit = float3(0.90 + 0.08 * i, 0.95 + 0.04 * i, 0.98 + 0.02 * i);
    rgb = lerp(rgb, spit, airborne);
  } else {
    float depth = saturate(-d / max(0.12, surface));
    float nearSurf = 1.0 - depth;
    float faceLit = 0.78 + 0.22 * saturate(0.35 + 0.65 * (-nx));
    float band = 0.92 + 0.08 * sin(alongF * 0.018 + depth * 4.5 - tCalm * 1.1);
    float water = faceLit * band * (0.85 + 0.15 * i);
    rgb.r = (0.02 + 0.03 * nearSurf + 0.04 * crest * nearSurf) * water;
    rgb.g = (0.10 + 0.18 * nearSurf + 0.10 * crest * nearSurf + 0.06 * i) * water;
    rgb.b = (0.22 + 0.28 * nearSurf + 0.08 * crest * nearSurf + 0.10 * i) * water;
    rgb.g += 0.06 * nearSurf * crest * water;
    rgb.b += 0.04 * nearSurf * water;
    float deepMul = 0.55 + 0.45 * nearSurf;
    rgb.r *= deepMul;
    rgb.g *= deepMul;
    rgb.b *= deepMul * (0.92 + 0.08 * nearSurf);

    float underCrest = saturate((-d) / 0.48);
    float depthFade = exp(-underCrest * 2.6);
    float crestFoam = crest * depthFade * (0.75 + 0.35 * i);
    float flowU = alongF * 0.020 - tCalm * 3.6 - nx * 1.3;
    float flowV = underCrest * 2.4 + tCalm * 0.50;
    float n1 = Noise2(float2(flowU, flowV));
    float n2 = Noise2(float2(flowU * 1.80 + 2.8, flowV * 0.55 - tCalm * 0.75));
    float n3 = Noise2(float2(flowU * 0.42 - 1.7, flowV * 1.70 + 4.5));
    float streak = saturate((n1 * 0.52 + n2 * 0.48 - 0.40) / 0.28);
    float lace = saturate((n2 * 0.55 + n3 * 0.45 - 0.45) / 0.30);
    float dots = saturate((n3 - 0.58) / 0.25);
    float pattern = saturate(0.08 + 0.78 * streak + 0.40 * lace + 0.30 * dots);
    float faceBoost = 0.75 + 0.25 * steep;
    float churn = saturate(crestFoam * pattern * faceBoost);
    float3 foamCol = float3(0.88 + 0.10 * i, 0.94 + 0.04 * i, 0.98 + 0.02 * i);
    rgb = lerp(rgb, foamCol, churn);
  }

  float lipDist = abs(d);
  float lip = exp(-lipDist * (24.0 - 8.0 * steep)) *
              (0.15 + 0.55 * crest + 0.55 * breakFace) * (0.25 + 0.75 * steep);
  float foam = lip;
  if (d > 0.0 && d < 0.16) {
    float burst = pow(Noise2(float2(alongF * 0.055 - tCalm * 3.4, d * 11.0 + tCalm * 0.7)), 2.8) *
                  breakFace * breakPulse * (1.0 - d / 0.16);
    foam = max(foam, burst * 0.85);
  }
  foam = saturate(foam);
  float3 lipCol = float3(0.90 + 0.08 * i, 0.95 + 0.04 * i, 0.98 + 0.02 * i);
  rgb = lerp(rgb, lipCol, foam);
  return Premul(rgb, alpha);
}

float DistSeg(float2 p, float2 a, float2 b) {
  float2 pa = p - a;
  float2 ba = b - a;
  float h = saturate(dot(pa, ba) / max(dot(ba, ba), 1e-6));
  return length(pa - ba * h);
}

float FlashEnv(float phase, float center, float width, float seed) {
  float d = abs(phase - center);
  d = min(d, 1.0 - d);
  float halfW = width * 0.5;
  if (d > halfW) return 0.0;
  float u = 1.0 - d / halfW;
  return pow(Smoother(u), 0.45) * (0.85 + 0.15 * Hash21(float2(seed, center * 17.0)));
}

float BoltEnergy(float alongN, float sky, float origin, float seed, float strength, float i, bool spider) {
  if (strength < 0.035) return 0.0;
  float ax = origin;
  float ay = 0.02 + 0.06 * Hash21(float2(seed, 1.3));
  float energy = 0.0;
  float acrossPx = (edge == 2 || edge == 3) ? width : height;
  float mainR = ((acrossPx >= 40.0) ? 2.35 : 1.85) * (0.90 + 0.20 * i);
  float alongPx = (edge == 2 || edge == 3) ? height : width;
  [unroll] for (int s = 0; s < 6; ++s) {
    float sf = (float)s;
    float prog = (sf + 1.0) / 6.0;
    float sway = (Hash21(float2(seed + sf * 0.37, 4.1)) * 2.0 - 1.0) *
                 (spider ? 0.038 : 0.022) * (1.05 - prog * 0.25);
    float branchChance = Hash21(float2(seed + sf, 7.7));
    float bx = saturate(ax + (Hash21(float2(seed + sf, 5.5)) * 2.0 - 1.0) * (spider ? 0.045 : 0.024) + sway);
    float by = saturate(ay + (spider ? 0.10 : 0.145) + 0.04 * Hash21(float2(seed + sf, 8.2)));
    float2 p = float2(alongN * alongPx, sky * max(acrossPx - 1.0, 1.0));
    float2 a = float2(ax * alongPx, ay * max(acrossPx - 1.0, 1.0));
    float2 b = float2(bx * alongPx, by * max(acrossPx - 1.0, 1.0));
    float midT = 0.45 + 0.12 * Hash21(float2(ax * 17.0 + seed, by * 13.0));
    float midJ = (Hash21(float2(ax * 9.0 + seed, ay * 11.0)) * 2.0 - 1.0) * (spider ? 0.028 : 0.016);
    float2 m = float2(saturate(ax + (bx - ax) * midT + midJ) * alongPx,
                      saturate(ay + (by - ay) * midT + midJ * 0.35) * max(acrossPx - 1.0, 1.0));
    float d0 = DistSeg(p, a, m);
    float d1 = DistSeg(p, m, b);
    float d = min(d0, d1);
    float radius = mainR * (1.0 - 0.28 * prog);
    float invR = 1.0 / max(radius, 0.8);
    float dn = d * invR;
    if (dn < 1.85) {
      float core = exp(-dn * dn * 2.35);
      float mid = exp(-dn * dn * 1.15);
      float halo = exp(-dn * dn * 0.55);
      float e = strength * (0.92 + 0.18 * i) * (1.0 - prog * 0.12) *
                (core * 0.82 + mid * 0.58 + halo * 0.28);
      energy = max(energy, e);
    }
    if (branchChance > 0.45 && s >= 1 && s < 5) {
      float fx = saturate(bx + (branchChance * 2.0 - 1.0) * (spider ? 0.11 : 0.07));
      float fy = saturate(by + 0.08 + Hash21(float2(seed + sf, 9.9)) * 0.12);
      float2 c = float2(fx * alongPx, fy * max(acrossPx - 1.0, 1.0));
      float db = DistSeg(p, b, c);
      float dnB = db / max(mainR * 0.72, 0.7);
      if (dnB < 1.85) {
        float core = exp(-dnB * dnB * 2.35);
        float mid = exp(-dnB * dnB * 1.15);
        float halo = exp(-dnB * dnB * 0.55);
        float e = strength * 0.62 * (0.92 + 0.18 * i) * (core * 0.82 + mid * 0.58 + halo * 0.28);
        energy = max(energy, e);
      }
    }
    ax = bx;
    ay = by;
  }
  return saturate(energy);
}

float4 PaintWeather(float2 uv) {
  float alongN, acrossN, sky, alongF;
  bool vertical;
  MapEdge(uv, alongN, acrossN, sky, alongF, vertical);
  float tCalm = time * 0.40;
  float i = saturate(intensity);
  float ground = 1.0 - sky;
  float period = 4.8;
  float phase = frac(tCalm / period);
  float loop = floor(tCalm / period);
  float windSign = (edge == 1 || edge == 3) ? -1.0 : 1.0;

  float flashA = FlashEnv(phase, 0.12, 0.11, 1.1);
  float flashB = FlashEnv(phase, 0.34, 0.10, 2.7);
  float flashC = FlashEnv(phase, 0.58, 0.12, 4.3);
  float flashD = FlashEnv(phase, 0.82, 0.10, 6.1);
  float glowA = FlashEnv(phase, 0.13, 0.28, 1.2);
  float glowB = FlashEnv(phase, 0.35, 0.26, 2.8);
  float glowC = FlashEnv(phase, 0.59, 0.30, 4.4);
  float glowD = FlashEnv(phase, 0.83, 0.26, 6.2);

  float epicA = 0.16 + 0.12 * Hash21(float2(loop + 1.0, 3.1));
  float epicB = 0.40 + 0.12 * Hash21(float2(loop + 2.0, 5.7));
  float epicC = 0.64 + 0.12 * Hash21(float2(loop + 3.0, 8.2));
  float epicD = 0.86 + 0.09 * Hash21(float2(loop + 4.0, 9.4));

  float wrapA = min(abs(alongN - epicA), 1.0 - abs(alongN - epicA));
  float wrapB = min(abs(alongN - epicB), 1.0 - abs(alongN - epicB));
  float wrapC = min(abs(alongN - epicC), 1.0 - abs(alongN - epicC));
  float wrapD = min(abs(alongN - epicD), 1.0 - abs(alongN - epicD));
  float nearGlow = saturate(glowA * exp(-wrapA * 9.5) + glowB * exp(-wrapB * 10.0) +
                            glowC * exp(-wrapC * 9.0) + glowD * exp(-wrapD * 9.8));
  float nearCore = saturate(flashA * exp(-wrapA * 13.0) + flashB * exp(-wrapB * 13.5) +
                            flashC * exp(-wrapC * 12.5) + flashD * exp(-wrapD * 13.2));

  float3 rgb = float3(0.010 + 0.012 * i, 0.018 + 0.022 * i, 0.045 + 0.050 * i);
  float drift = tCalm * 0.10;
  float n1 = Noise2(float2(alongN * 1.15 + drift, sky * 1.05 + 0.15));
  float n2 = Noise2(float2(alongN * 2.6 - drift * 0.55, sky * 2.2 + 1.6));
  float n3 = Noise2(float2(alongN * 5.5 + drift * 0.35, sky * 4.0 + 3.5));
  float n4 = Noise2(float2(alongN * 0.55 + drift * 0.12, sky * 0.45 + 6.2));
  float fbm = n1 * 0.50 + n2 * 0.28 + n3 * 0.14 + n4 * 0.20;
  float ridge = 1.0 - abs(n2 * 2.0 - 1.0);
  fbm = saturate(fbm * 0.62 + ridge * 0.48);
  float cloud = Smoother((fbm - (0.38 - 0.05 * i)) / (0.28 + 0.05 * i));
  cloud *= Smoother(0.12 + 1.10 * ground + 0.40 * sky);
  cloud = saturate(cloud * (0.88 + 0.28 * i));

  float underside = Smoother(ground);
  float topLit = Smoother(sky);
  float rim = exp(-ground * 2.2) * cloud;
  float body = cloud * cloud;
  float micro = Noise2(float2(alongN * 14.0 - drift * 0.9, sky * 10.0 + 3.3)) * 0.55 +
                Noise2(float2(alongN * 28.0 + drift * 0.5, sky * 18.0 + 1.1)) * 0.45;
  float relief = 0.55 + 0.55 * micro;
  float slate = 0.16 + 0.18 * i;
  float shade = (0.22 + 0.58 * topLit + 0.28 * (1.0 - underside)) * relief;
  rgb += cloud * slate * float3(0.42, 0.58, 0.95) * shade;
  rgb += rim * float3(0.03 + 0.04 * i, 0.05 + 0.06 * i, 0.12 + 0.12 * i) * relief;

  float blueNoise = 0.45 + 0.55 * Noise2(float2(alongN * 2.2 + drift * 0.35, sky * 1.6 + 2.4));
  float ambient = body * (0.06 + 0.10 * i) * (0.40 + 0.60 * underside) * blueNoise;
  rgb += ambient * float3(0.18, 0.35, 0.85);

  if (cloud > 0.05 && (nearGlow > 0.02 || nearCore > 0.015)) {
    float bloom = body * (nearGlow * (0.38 + 0.40 * i) + nearCore * (0.28 + 0.32 * i));
    rgb += float3(0.70, 0.18, 0.88) * bloom;
    float hot = body * nearCore * nearCore * (0.22 + 0.28 * i);
    rgb += float3(0.48, 0.14, 0.32) * hot;
  }

  float shear = 1.30 * windSign;
  float rainU = alongN * 28.0 + sky * shear * 3.2 - tCalm * 1.3;
  float rainV = sky * 18.0 - alongN * shear * 2.6 + tCalm * 3.4;
  float streak = 1.0 - abs(Noise2(float2(rainU, rainV)) * 2.0 - 1.0);
  float drops = pow(streak, 6.0) * (0.08 + 0.12 * i) * (0.30 + 0.70 * cloud);
  rgb += float3(0.07, 0.08, 0.11) * drops;

  float breath = 0.94 + 0.06 * sin(tCalm * 0.55);
  float live = breath * (0.97 + 0.03 * Noise2(float2(alongF * 0.016 + tCalm * 0.14, sky * 3.0)));
  rgb *= live;

  float luma = dot(rgb, float3(0.30, 0.59, 0.11));
  if (luma > 0.58) rgb *= 0.58 / luma;

  float bolt = 0.0;
  bolt = max(bolt, BoltEnergy(alongN, sky, epicA, 11.0, flashA, i, true));
  if (flashA > 0.22) bolt = max(bolt, BoltEnergy(alongN, sky, epicA + 0.06, 13.0, flashA * 0.68, i, false));
  bolt = max(bolt, BoltEnergy(alongN, sky, epicB, 17.0, flashB, i, false));
  if (flashB > 0.30) bolt = max(bolt, BoltEnergy(alongN, sky, epicB - 0.05, 18.0, flashB * 0.50, i, true));
  bolt = max(bolt, BoltEnergy(alongN, sky, epicC, 19.0, flashC, i, true));
  if (flashC > 0.20) bolt = max(bolt, BoltEnergy(alongN, sky, epicC - 0.07, 23.0, flashC * 0.60, i, false));
  bolt = max(bolt, BoltEnergy(alongN, sky, epicD, 29.0, flashD, i, false));
  if (bolt > 0.012) {
    float mix = saturate(bolt);
    float3 boltRgb = float3(0.70 + 0.30 * mix, 0.40 + 0.55 * mix, 0.92 + 0.08 * mix);
    rgb = lerp(rgb, boltRgb, saturate(bolt));
  }

  float alpha = 0.95 + 0.04 * i;
  return Premul(rgb, alpha);
}

float Ramp(float p, float a, float b) {
  if (b <= a) return p >= b ? 1.0 : 0.0;
  return Smoother((p - a) / (b - a));
}

float Pulse(float p, float a, float b, float c, float d) {
  return Ramp(p, a, b) * (1.0 - Ramp(p, c, d));
}

float4 PaintSpace(float2 uv) {
  float alongN, acrossN, depth, alongF;
  bool vertical;
  MapEdge(uv, alongN, acrossN, depth, alongF, vertical);
  float tCalm = time * 0.20;
  float i = saturate(intensity);
  float period = 7.0;
  float phase = frac(tCalm / period);

  float actA = Pulse(phase, 0.00, 0.00, 0.18, 0.30);
  float actB = Pulse(phase, 0.16, 0.26, 0.46, 0.58);
  float actC = Pulse(phase, 0.28, 0.40, 0.82, 0.94);
  float arrive = Ramp(phase, 0.84, 0.98);
  float starsIn = Ramp(phase, 0.18, 0.38);

  float kCamXPanAmt = 0.92;
  float kCamXFlyAmt = 0.38;
  float kCamXArriveAmt = 0.30;
  float kWorldPeriod = kCamXPanAmt + kCamXFlyAmt + kCamXArriveAmt;
  float camXHold = 0.02 + 0.015 * sin(tCalm * 0.31);
  float camXPan = Ramp(phase, 0.18, 0.52);
  float camXFly = Ramp(phase, 0.52, 0.88) * kCamXFlyAmt;
  float camXArrive = arrive * kCamXArriveAmt;
  float camX = camXHold + camXPan * kCamXPanAmt + camXFly + camXArrive;
  float camYaw = saturate(Ramp(phase, 0.28, 0.52) * (1.0 - arrive));

  float nebPresence = saturate(0.10 + 0.88 * (actA * 0.95 + actB * 0.80) +
                               (1.0 - camYaw) * 0.22 * (1.0 - actC) + arrive * 0.90 -
                               actC * 0.55 * camYaw);
  nebPresence = clamp(nebPresence, 0.04, 1.15);
  float starHero = clamp(0.08 + 0.70 * starsIn + 0.40 * actB + 0.90 * actC + 0.06 * actA, 0.0, 1.20) *
                   (1.0 - arrive * 0.85);

  float breath = 0.90 + 0.10 * sin(tCalm * 0.48);
  float breath2 = 0.92 + 0.08 * sin(tCalm * 0.27 + 1.9);
  float alpha = 0.84 + 0.12 * i;

  float nebCoreWorld = 0.42;
  float nebCoreDepth = 0.52 + 0.05 * sin(tCalm * 0.17 + 0.8);
  float nebAlongSigma = 0.36;
  float nebDepthSigma = 0.62;
  float nebGain = (0.88 + 0.28 * i) * breath * nebPresence;
  float worldAlong = alongN + camX;

  float3 rgb = float3(0.004 + 0.006 * i, 0.004 + 0.005 * i, 0.008 + 0.010 * i);

  float nebShift = camYaw * 0.38;
  float dAlong0 = (worldAlong - nebCoreWorld - nebShift) / nebAlongSigma;
  float dAlong1 = (worldAlong - (nebCoreWorld + kWorldPeriod) - nebShift) / nebAlongSigma;
  float dDepth = (depth - nebCoreDepth) / nebDepthSigma;
  float env0 = exp(-(dAlong0 * dAlong0 + dDepth * dDepth * 0.55));
  float env1 = exp(-(dAlong1 * dAlong1 + dDepth * dDepth * 0.55)) * arrive;
  float envelope = max(env0, env1);
  float dAlong = (env1 > env0) ? dAlong1 : dAlong0;
  float localAlong = worldAlong - ((env1 > env0) ? kWorldPeriod : 0.0);

  float pxN = localAlong * 6.5 + depth * 1.8;
  float pyN = depth * 3.2 - localAlong * 1.1;
  float drift = tCalm * 0.028;
  float bulky = Noise2(float2(pxN * 0.55 + drift * 0.4, pyN * 0.55 - 1.7));
  float mid = Noise2(float2(pxN * 1.35 - drift * 0.6, pyN * 1.55 + 2.3));
  float fine = Noise2(float2(pxN * 3.4 + drift, pyN * 3.8 + 5.1));
  float filU = localAlong * 9.0 + depth * 4.5 + drift * 0.5;
  float filV = (-localAlong * 2.2 + depth * 1.4) * 0.55;
  float filament = Noise2(float2(filU, filV + 3.7));
  float filament2 = Noise2(float2(filU * 1.7 - 2.1, filV * 1.3 + 8.2));
  float ridge = 1.0 - abs(filament * 2.0 - 1.0);
  float ridge2 = 1.0 - abs(filament2 * 2.0 - 1.0);
  float wisps = pow(ridge, 2.4) * 0.65 + pow(ridge2, 3.1) * 0.45;
  float volume = pow(bulky, 1.35) * 0.55 + pow(mid, 1.8) * 0.35 + pow(fine, 2.4) * 0.22 + wisps * 0.40;

  float coreDist = sqrt(dAlong * dAlong * 0.85 + dDepth * dDepth * 0.35);
  float core = exp(-coreDist * coreDist * 2.8) * breath2;
  float halo = exp(-coreDist * coreDist * 0.85);
  float density = envelope * (0.35 + 0.85 * volume) * nebGain;
  float midGap = nebCoreWorld + 0.46;
  float pastCloud = Smoother(saturate((worldAlong - midGap) / 0.50)) *
                    (1.0 - Smoother(saturate((worldAlong - (nebCoreWorld + kWorldPeriod - 0.55)) / 0.45)));
  density *= 1.0 - pastCloud * (0.88 + 0.10 * actC) * (1.0 - arrive * 0.85);
  density *= 1.0 - actC * camYaw * 0.62;
  density = saturate(density);
  density = density * density * (3.0 - 2.0 * density);

  float shade = saturate(density * 0.90 + core * 0.60 + halo * 0.28);
  float3 bodyC = float3(0.62 + 0.32 * shade, 0.22 + 0.28 * shade, 0.05 + 0.08 * shade);
  float3 hotC = float3(1.00, 0.52 + 0.22 * mid, 0.10 + 0.08 * fine);
  float3 creamC = float3(1.00, 0.94, 0.82);
  float hotMix = saturate(density * 0.82 + halo * 0.40);
  float creamMix = saturate(core * (1.00 + 0.40 * i));
  float3 nr = lerp(lerp(bodyC, hotC, hotMix), creamC, creamMix);
  float grain = (fine - 0.5) * density * 0.14;
  nr += float3(grain * 0.9, grain * 0.45, grain * 0.15);
  float rift = pow(1.0 - abs(filament * 2.0 - 1.0), 3.2) * envelope * (0.55 + 0.35 * mid);
  float riftAmt = rift * (0.42 + 0.18 * i) * (0.55 + 0.45 * creamMix);
  nr *= float3(1.0 - riftAmt * 0.88, 1.0 - riftAmt * 0.92, 1.0 - riftAmt * 0.70);
  float recess = (1.0 - wisps) * envelope * 0.20 * (1.0 - creamMix);
  nr *= float3(1.0 - recess * 0.55, 1.0 - recess * 0.65, 1.0 - recess * 0.40);
  float amt = saturate(density * (0.62 + 0.48 * i) + creamMix * 0.42);
  rgb = lerp(rgb, nr, amt);
  float bloom = creamMix * creamMix * (0.32 + 0.28 * i);
  rgb += float3(0.65, 0.50, 0.26) * bloom;
  float sheen = 0.5 + 0.5 * sin(alongF * 0.006 + depth * 2.4 + tCalm * 0.55);
  float live = 0.94 + 0.04 * fine + 0.02 * sheen;
  rgb *= live * (0.88 + 0.12 * depth);

  // Stars as true pixel discs in cbuffer pixel space (UV already from SV_Position).
  float2 dim = max(float2(width, height), float2(1.0, 1.0));
  float2 fragPx = uv * dim;
  float fieldBoost = (0.40 + 0.55 * (1.0 - min(nebPresence, 1.0)) + 0.55 * starsIn +
                      0.35 * actB + 0.55 * actC + 0.25 * camYaw) * (0.70 + 0.40 * i);
  float rushSpeed = 0.040 + 0.20 * starHero;
  float vpAlongBias = (0.18 - 0.18 * camYaw) * (vertical ? 0.0 : 1.0);
  float2 vp = float2(0.50 + vpAlongBias * 0.5, 0.50);

  // Layer A — field stars (hard 1×1 / plus cores — CPU Put parity).
  // quality 1.0 ≈ 120; quality 1.5 ≈ 150 (CPU field count).
  int fieldCount = (int)(120.0 + 30.0 * saturate((quality - 1.0) * 2.0));
  fieldCount = clamp(fieldCount, 80, 150);
  [loop] for (int fi = 0; fi < 150; ++fi) {
    if (fi >= fieldCount) break;
    float id = (float)fi;
    float hx = Hash21(float2(id + 0.13, 2.77));
    float hy = Hash21(float2(id + 3.91, 5.41));
    float hBright = Hash21(float2(id + 7.07, 1.19));
    float hTw = Hash21(float2(id + 4.33, 9.01));
    float hSize = Hash21(float2(id + 1.67, 6.53));
    float sx = floor(frac(hx - camX * 0.42) * dim.x) + 0.5;
    float sy = floor(hy * dim.y) + 0.5;
    float2 d = abs(fragPx - float2(sx, sy));
    // Strict axis-aligned pixel hit — cannot elongate.
    float core = (d.x < 0.55 && d.y < 0.55) ? 1.0 : 0.0;
    // Plus-shaped bloom (same as CPU Put neighbors) — never a horizontal-only dash.
    if (hSize > 0.62 && fieldBoost > 0.2) {
      core = max(core, (abs(fragPx.x - (sx + 1.0)) < 0.55 && d.y < 0.55) ? 0.40 : 0.0);
      core = max(core, (abs(fragPx.x - (sx - 1.0)) < 0.55 && d.y < 0.55) ? 0.40 : 0.0);
      core = max(core, (d.x < 0.55 && abs(fragPx.y - (sy + 1.0)) < 0.55) ? 0.32 : 0.0);
      core = max(core, (d.x < 0.55 && abs(fragPx.y - (sy - 1.0)) < 0.55) ? 0.32 : 0.0);
    }
    float twinkle = 0.72 + 0.28 * sin(tCalm * (0.55 + hTw * 0.4) + hTw * 6.2831853);
    float amt = fieldBoost * twinkle * (0.55 + 0.50 * hBright) * (0.55 + 0.50 * starHero) * core;
    if (amt > 0.02) {
      float3 sc = float3(0.90 + 0.10 * hBright, 0.92 + 0.07 * hBright, 0.98 + 0.02 * hBright);
      rgb = lerp(rgb, sc, saturate(amt));
    }
  }

  // Layer B — depth rush toward vanishing point (CPU radial tunnel parity).
  // Stars spawn near VP and fly outward as nebula exits; short radial trails are
  // discrete pixels along the ray (not DistSeg lines).
  float halfW = max(dim.x * 0.5, 1.0);
  float halfH = max(dim.y * 0.5, 1.0);
  float iso = min(halfW, halfH);
  float maxReach = (max(halfW, halfH) / iso) * 1.15;
  float2 vpPx = float2((dim.x - 1.0) * vp.x, (dim.y - 1.0) * vp.y);
  float streakScale = 0.55 + 2.35 * starHero;
  // quality 1.0 ≈ 160; quality 1.5 ≈ 210 (CPU rush count).
  int rushCount = (int)(160.0 + 50.0 * saturate((quality - 1.0) * 2.0));
  rushCount = clamp(rushCount, 96, 210);
  [loop] for (int si = 0; si < 210; ++si) {
    if (si >= rushCount) break;
    float id = (float)si;
    float hAngle = Hash21(float2(id + 0.37, 1.71));
    float hPhase = Hash21(float2(id + 2.11, 4.93));
    float hSpeed = Hash21(float2(id + 5.57, 8.23));
    float hBright2 = Hash21(float2(id + 9.41, 3.17));
    float hWarm = Hash21(float2(id + 6.13, 7.77));
    float hSpread = Hash21(float2(id + 8.29, 2.41));
    float angle;
    if (!vertical) {
      // Prefer near-horizontal rays on the thin bar (readable depth streaks).
      float side = (hAngle < 0.5) ? 0.0 : 3.14159265;
      float spread = (0.10 + 0.22 * hSpread) * (1.0 - 0.35 * camYaw);
      angle = side + (hAngle * 2.0 - (hAngle < 0.5 ? 0.0 : 1.0)) * 2.0 * spread;
    } else {
      angle = hAngle * 6.2831853;
    }
    float speed = rushSpeed * (0.55 + hSpeed * 0.95);
    float u = frac(hPhase + tCalm * speed);
    float uEase = u * u * (0.65 + 0.35 * u);
    float radius = uEase * maxReach;
    float2 dir = float2(cos(angle), sin(angle));
    // Isotropic px scale (CPU iso) so motion reads as flying into depth.
    float2 tip = vpPx + dir * (radius * iso);
    float streakPx = (0.8 + uEase * (3.8 + hSpeed * 4.2) + starHero * 2.2) * streakScale;
    streakPx = min(streakPx, 28.0);
    float twinkle = 0.80 + 0.20 * sin(tCalm * (1.05 + hSpeed * 0.55) + hPhase * 6.2831853);
    float bright = (0.22 + 0.90 * uEase) * (0.55 + 0.45 * i) * (0.50 + 0.50 * hBright2) *
                   twinkle * (0.40 + 0.70 * starHero) * (0.72 + 0.28 * camYaw + 0.22 * actC);
    if (bright >= 0.030) {
      float warm = hWarm * 0.05;
      float3 sc = float3(saturate(0.88 + warm), saturate(0.92 + warm * 0.30), 1.0);
      float hit = 0.0;
      // Match CPU: one discrete stamp per streak pixel along the ray.
      int steps = 1 + (int)(streakPx + 0.5);
      steps = clamp(steps, 1, 24);
      [loop] for (int s = 0; s < steps; ++s) {
        float tS = (float)s / (float)max(steps, 1);
        float2 p = tip - dir * (streakPx * tS);
        float2 ip = floor(p) + 0.5;
        float2 d = abs(fragPx - ip);
        float fall = 1.0 - tS * 0.62;
        float cell = (d.x < 0.55 && d.y < 0.55) ? fall : 0.0;
        if (s == 0 && uEase > 0.22 && starHero > 0.35) {
          cell = max(cell, (abs(fragPx.x - (ip.x + 1.0)) < 0.55 && d.y < 0.55) ? 0.55 * fall : 0.0);
          cell = max(cell, (abs(fragPx.x - (ip.x - 1.0)) < 0.55 && d.y < 0.55) ? 0.55 * fall : 0.0);
          cell = max(cell, (d.x < 0.55 && abs(fragPx.y - (ip.y + 1.0)) < 0.55) ? 0.42 * fall : 0.0);
          cell = max(cell, (d.x < 0.55 && abs(fragPx.y - (ip.y - 1.0)) < 0.55) ? 0.42 * fall : 0.0);
        }
        hit = max(hit, cell);
      }
      float a = saturate(hit * bright);
      if (a > 0.02) rgb = lerp(rgb, sc, a);
    }
  }
  return Premul(rgb, alpha);
}

float4 PaintAurora(float2 uv) {
  float alongN, acrossN, sky, alongF;
  bool vertical;
  MapEdge(uv, alongN, acrossN, sky, alongF, vertical);
  float tCalm = time * 0.48;
  float i = saturate(intensity);
  float fall = 1.0 - sky;
  float alpha = 0.82 + 0.16 * i;
  float breath = 0.88 + 0.12 * sin(tCalm * 0.55);
  float breath2 = 0.90 + 0.10 * sin(tCalm * 0.31 + 1.9);

  float sway = 0.012 * sin(alongN * 6.2831853 * 0.9 + tCalm * 0.42) +
               0.007 * sin(alongN * 6.2831853 * 2.1 - tCalm * 0.28 + 1.3);

  float curtainField = 0.0;
  float hueField = 0.0;
  float hueW = 0.0;
  [unroll] for (int s = 0; s < 5; ++s) {
    float fs = (float)s;
    float home = Hash21(float2(fs + 0.37, 1.91));
    float phase = Hash21(float2(fs + 2.83, 6.41));
    float width0 = 0.032 + 0.058 * Hash21(float2(fs + 0.77, 3.55));
    float hue0 = Hash21(float2(fs + 4.12, 8.09));
    float spd = 0.014 + 0.030 * Hash21(float2(fs + 5.55, 9.17));
    float dir = Hash21(float2(fs + 6.02, 1.33)) > 0.5 ? 1.0 : -1.0;
    float wander = 0.10 * sin(tCalm * 0.13 + phase * 5.1 + home * 3.7) +
                   0.045 * sin(tCalm * (0.28 + phase * 0.22) + phase * 6.2831853);
    float pulse = 0.15 + 0.85 * (0.5 + 0.5 * sin(tCalm * (0.55 + phase * 0.40) + phase * 6.2831853));
    float hueNow = frac(hue0 + tCalm * (0.02 + 0.03 * phase));
    float width = width0 * (0.85 + 0.25 * (0.5 + 0.5 * sin(tCalm * 0.45 + phase * 4.0)));
    float pos = frac(home + dir * spd * tCalm + wander);
    float d = alongN - pos - sway;
    d -= round(d);
    float env = exp(-(d * d) / max(width * width, 1e-6));
    float contrib = env * pulse;
    curtainField = max(curtainField, contrib);
    hueField += hueNow * contrib;
    hueW += contrib;
  }
  curtainField = saturate(curtainField);
  float pillarHue = (hueW > 1e-4) ? (hueField / hueW) : 0.5;
  float rays = 0.55 + 0.45 * (0.5 + 0.5 * sin(alongN * 28.0 + tCalm * 0.15));

  float3 rgb = float3(0.012 + 0.010 * i, 0.010 + 0.008 * i, 0.040 + 0.025 * i);
  float starN = Hash21(float2(alongN * 97.0 + 0.13, sky * 41.0 + 3.7));
  float star = pow(saturate((starN - 0.88) / 0.12), 3.0) * (1.0 - curtainField) * sky;
  rgb += star * float3(0.55, 0.55, 0.70);

  float lengthMul = 0.70 + 0.45 * curtainField;
  float tip = exp(-fall * fall * (2.4 / lengthMul));
  float body = exp(-abs(fall - 0.35) * (2.2 - 0.6 * curtainField));
  float curtain = saturate(tip * 0.95 + body * 0.75);
  curtain *= curtainField * rays * breath;

  float flowV = fall * 5.5 - tCalm * 1.85;
  float flowU = alongN * 14.0 + sway * 40.0;
  float stream1 = 0.5 + 0.5 * sin(flowU * 1.7 + flowV * 2.1);
  float stream2 = 0.5 + 0.5 * sin(flowU * 2.9 - flowV * 1.4 + 2.4);
  float silk = 0.62 + 0.24 * stream1 + 0.14 * stream2;
  curtain *= silk;

  float hMix = saturate(pillarHue * 0.55 + fall * 0.45);
  float3 violet = float3(0.42, 0.12, 0.95);
  float3 magenta = float3(0.92, 0.18, 0.78);
  float3 pink = float3(0.95, 0.28, 0.48);
  float3 cr = (hMix < 0.55) ? lerp(violet, magenta, hMix / 0.55)
                            : lerp(magenta, pink, (hMix - 0.55) / 0.45);
  float amt = saturate(curtain * (0.55 + 0.55 * i) * breath2);
  rgb = lerp(rgb, cr, amt);
  float bloom = curtain * curtain * (0.12 + 0.16 * i);
  rgb += bloom * cr * float3(0.55, 0.35, 0.65);
  float wash = pow(fall, 2.2) * curtainField * (0.10 + 0.14 * i);
  rgb += float3(0.55, 0.12, 0.22) * wash;
  float voidKeep = 0.55 + 0.45 * curtainField;
  rgb *= float3(0.70 + 0.30 * voidKeep, 0.70 + 0.30 * voidKeep, 0.75 + 0.25 * voidKeep);
  return Premul(rgb, alpha);
}

float4 PSMain(VSOut inn) : SV_Target {
  // Derive UV from pixel position + cbuffer size. The fullscreen-triangle
  // interpolator shears UV (ddx/ddy not axis-aligned), which turned round
  // sparks into diagonal dashes.
  float2 dim = max(float2(width, height), float2(1.0, 1.0));
  float2 uv = inn.pos.xy / dim;
  if (themeId == 1) return PaintSea(uv);
  if (themeId == 2) return PaintWeather(uv);
  if (themeId == 3) return PaintSpace(uv);
  if (themeId == 4) return PaintAurora(uv);
  return PaintRainbow(uv);
}
