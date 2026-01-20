#pragma once
#include <Arduino.h>
#include "Types.h"
#include "Events.h"
#include "Sequencer.h"

class UI {
public:
  void begin(EventQueue* q, Sequencer* seq);
  void tick(const SynthParams& params, UiState state, uint8_t editIndex, EditLayer layer);

private:
  EventQueue* q_ = nullptr;
  Sequencer* seq_ = nullptr;

  uint32_t lastPrintMs_ = 0;
};