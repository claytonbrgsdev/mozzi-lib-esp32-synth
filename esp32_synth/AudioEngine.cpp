#include "AudioEngine.h"
#include <math.h>

void AudioEngine::begin() {
  osc_.setTable(SAW2048_DATA);
  osc_.setFreq(220.0f);

  env_.setADLevels(255, 160);
  env_.setTimes(20, 120, 0, 180);
  env_.noteOff();

  lpf_.setResonance(140);
  lpf_.setCutoffFreq(120);

  lastMidi_ = 60;
}

uint16_t AudioEngine::mapCutoffHz(uint16_t v) {
  // params_.cutoff vem 0..1023 (seu Controls atual)
  const float x = (float)v / 1023.0f;
  float hz = 40.0f + (x * x) * 5960.0f;
  if (hz < 40.0f) hz = 40.0f;
  if (hz > 6000.0f) hz = 6000.0f;
  return (uint16_t)(hz + 0.5f);
}

float AudioEngine::midiToHz(uint8_t midi) {
  // Igual ao mtof do unificado (A4=440, nota 69)
  return 440.0f * powf(2.0f, ((int)midi - 69) / 12.0f);
}

void AudioEngine::updateControlAudioSide() {
  // Filtro
  const uint16_t cutoffHz = mapCutoffHz(params_.cutoff);
  lpf_.setCutoffFreq(cutoffHz);

  // Resonance 0..1023 -> 0..255
  const uint8_t res = (uint8_t)map(params_.resonance, 0, 1023, 0, 255);
  lpf_.setResonance(res);

  // ADSR
  const uint16_t a = (uint16_t)map(params_.attack,  0, 1023, 5, 800);
  const uint16_t d = (uint16_t)map(params_.decay,   0, 1023, 5, 900);
  const uint16_t r = (uint16_t)map(params_.release, 0, 1023, 5, 1500);
  const uint8_t  s = (uint8_t) map(params_.sustain, 0, 1023, 0, 255);

  env_.setADLevels(255, s);
  env_.setTimes(a, d, 0, r);

  // Importante: noteOn/noteOff NÃO ficam aqui.
  // O retrigger correto fica em onStep().
}

void AudioEngine::onStep() {
  if (!seq_) return;

  const uint8_t midi = seq_->currentNote();
  const bool gate = seq_->currentGate();
  const bool accent = seq_->currentAccent();
  (void)accent; // reservado (depois você pode usar p/ amplitude/cutoff)

  // Só recalcula freq quando muda (barato, e ainda mais barato que todo tick)
  if (midi != lastMidi_) {
    osc_.setFreq(midiToHz(midi));
    lastMidi_ = midi;
  }

  // REGRAS de step (match unificado em espírito):
  // - se step ativo: retrigger sempre
  // - se step inativo: noteOff (para rest real)
  if (gate) env_.noteOn();
  else      env_.noteOff();
}

int AudioEngine::updateAudio() {
  const int8_t sig = osc_.next();
  const int16_t filtered = lpf_.next(sig);

  const uint8_t e = env_.next();
  int16_t shaped = (filtered * (int16_t)e) >> 8;

  // Volume 0..1023
  shaped = (shaped * (int16_t)params_.volume) >> 10;

  return (int)shaped;
}