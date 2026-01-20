#pragma once
#include <Arduino.h>
#include "Config.h"

struct Step {
  bool active = false;
  uint8_t note = 60;               // MIDI 0..127
  uint8_t accent = 0;              // 0/1 (por enquanto)
  uint8_t gate = DEFAULT_GATE_PCT; // 1..100 (% do step)
};

enum UiState : uint8_t {
  UI_PLAY = 0,
  UI_EDIT_STEP = 1,
};

enum EditLayer : uint8_t {
  LAYER_NOTE = 0,
  LAYER_GATE = 1,
  LAYER_ACCENT = 2,
};

struct SynthParams {
  // Pots no range 0..1023 (ADC 10-bit)
  uint16_t cutoff = 0;
  uint16_t resonance = 0;
  uint16_t attack = 0;
  uint16_t decay = 0;
  uint16_t sustain = 0;
  uint16_t release = 0;
  uint16_t swing = 50; // já “pré-mapeado” para 50..75 no Controls.cpp
  uint16_t pot7 = 0;

  uint16_t volume = 1023; // master
};