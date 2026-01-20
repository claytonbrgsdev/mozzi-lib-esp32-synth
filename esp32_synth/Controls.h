#pragma once
#include <Arduino.h>

#include "Config.h"
#include "Pins.h"
#include "Types.h"
#include "Events.h"

class Controls {
public:
  void begin(EventQueue* q);
  void tick(SynthParams& params); // chamado em updateControl()

  bool shiftHeld() const { return shiftHeld_; }

private:
  EventQueue* q_ = nullptr;

  // scanning indices
  uint8_t mux1Chan_ = 0; // pots (0..7)
  uint8_t mux2Chan_ = 0; // buttons (0..MUX2_BUTTON_COUNT-1)

  // debounce integrator para MUX2
  uint8_t btnInt_[MUX2_BUTTON_COUNT] = {0};
  bool btnStable_[MUX2_BUTTON_COUNT] = {false};

  bool shiftHeld_ = false;

  // Encoder
  uint8_t encPrev_ = 0;
  bool encSwPrev_ = false;

  // Volume pot slow polling
  uint8_t volDivider_ = 0;

  // helpers
  void setMuxChannel(uint8_t ch);
  uint16_t readAdcFast(uint8_t pin);
  uint16_t iir(uint16_t prev, uint16_t x);

  void scanMux1Pots(SynthParams& params);
  void scanMux2Buttons();
  void scanEncoder();
};