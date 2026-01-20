#pragma once
#include <Arduino.h>

// Utilitários genéricos pequenos
// (helpers matemáticos, clamp, map seguro, etc.)

// clamp para int
template <typename T>
inline T clamp(T v, T lo, T hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// map seguro para inteiros (evita overflow estranho)
inline int mapInt(int x, int in_min, int in_max, int out_min, int out_max) {
  if (in_max == in_min) return out_min;
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// map seguro para uint16_t
inline uint16_t mapU16(uint16_t x,
                       uint16_t in_min, uint16_t in_max,
                       uint16_t out_min, uint16_t out_max) {
  if (in_max == in_min) return out_min;
  return (uint16_t)(
    (uint32_t)(x - in_min) * (uint32_t)(out_max - out_min) /
    (uint32_t)(in_max - in_min) + out_min
  );
}