#pragma once
#include <Arduino.h>

// =======================================================
// Sequencer v2
// - determinístico
// - tick-based
// - sem dependência de hardware / Mozzi / UI
// =======================================================

class Sequencer {
public:
  // -------- lifecycle --------
  void begin();                 // inicializa steps e timing
  void restart(bool retrigger); // reseta transporte (step 0)

  // -------- clock --------
  // Deve ser chamado 1x por tick (control-rate)
  // Retorna true SOMENTE quando um novo step inicia
  bool tick();

  // -------- tempo --------
  void setBpm(uint16_t bpm);    // 30..300
  uint16_t getBpm() const;

  void setSwing(uint8_t pct);   // 50..75
  uint8_t getSwing() const;

  // -------- step editing --------
  static constexpr uint8_t STEP_COUNT = 8;

  void setStepActive(uint8_t step, bool on);
  void toggleStep(uint8_t step);
  bool isStepActive(uint8_t step) const;

  void setStepNote(uint8_t step, uint8_t midi);
  uint8_t getStepNote(uint8_t step) const;

  void setStepAccent(uint8_t step, bool on);
  bool getStepAccent(uint8_t step) const;

  // -------- playback state (outputs) --------
  uint8_t currentStep() const;
  uint8_t currentNote() const;
  bool    currentGate() const;     // true se step ativo
  bool    currentAccent() const;

private:
  // -------- internal structures --------
  struct Step {
    bool    active = false;
    uint8_t note   = 60;
    bool    accent = false;
  };

  // -------- musical state --------
  Step    steps_[STEP_COUNT];
  uint8_t currentStep_ = 0;

  // -------- tempo state --------
  uint16_t bpm_   = 120;
  uint8_t  swing_ = 50;

  // -------- tick engine --------
  uint16_t baseTicksPerStep_ = 0;
  int16_t  swingOffsetTicks_ = 0;
  uint16_t tickCounter_      = 0;

  // -------- helpers --------
  void recalcStepTiming();
  uint16_t ticksForCurrentStep() const;
};