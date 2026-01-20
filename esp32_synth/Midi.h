#pragma once
#include <Arduino.h>
#include <math.h>

// Utilitários MIDI básicos
// (não envolve UART, apenas matemática)

namespace Midi {

  // Converte nota MIDI (0..127) para frequência em Hz
  // A4 (69) = 440 Hz
  inline float noteToFreq(uint8_t midiNote) {
    return 440.0f * powf(2.0f, ((int)midiNote - 69) / 12.0f);
  }

  // Versão inteira (útil para DSP leve)
  inline uint16_t noteToFreqHz(uint8_t midiNote) {
    float hz = noteToFreq(midiNote);
    if (hz < 20.0f) hz = 20.0f;
    if (hz > 8000.0f) hz = 8000.0f;
    return (uint16_t)(hz + 0.5f);
  }

}