#pragma once

// ===== Mozzi rates =====
// Defina aqui para todos os módulos poderem usar SEM incluir MozziGuts.h
#ifndef MOZZI_CONTROL_RATE
  #define MOZZI_CONTROL_RATE 128
#endif

#ifndef MOZZI_AUDIO_RATE
  #define MOZZI_AUDIO_RATE 16384
#endif

// Taxas e performance
#define UI_FPS               16
#define TM1638_POLL_MS       30
#define ADC_IIR_SHIFT        3

// Sequencer
#define STEPS_COUNT          8
#define SWING_MIN            50
#define SWING_MAX            75

// Gate/envelope
#define DEFAULT_GATE_PCT     80

// MUX2 buttons (canais 0..4)
#define MUX2_BUTTON_COUNT    5

// Debounce integrator
#define BTN_INT_MAX          8
#define BTN_INT_ON_TH        6
#define BTN_INT_OFF_TH       2