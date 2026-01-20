#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Types.h"
#include "Sequencer.h"

// Mozzi DSP headers (não incluir MozziGuts.h aqui)
#include <Oscil.h>
#include <tables/saw2048_int8.h>
#include <ADSR.h>
#include <LowPassFilter.h>

class AudioEngine {
public:
  void begin();
  void setSequencer(Sequencer* seq) { seq_ = seq; }
  void setParams(const SynthParams& p) { params_ = p; }

  // Chamado 1x por tick de controle: atualiza cutoff/res/ADSR/volume (barato)
  void updateControlAudioSide();

  // Chamado quando o Sequencer inicia um NOVO step (retrigger correto)
  void onStep();

  // Chamado no updateAudio()
  int updateAudio();

private:
  Sequencer* seq_ = nullptr;
  SynthParams params_;

  Oscil<SAW2048_NUM_CELLS, MOZZI_AUDIO_RATE> osc_;
  LowPassFilter lpf_;
  ADSR<MOZZI_CONTROL_RATE, MOZZI_AUDIO_RATE> env_;

  uint8_t lastMidi_ = 60;

  static uint16_t mapCutoffHz(uint16_t v);
  static float midiToHz(uint8_t midi);
};