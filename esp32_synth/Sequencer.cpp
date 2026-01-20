#include "Sequencer.h"
#include "Config.h"   // para MOZZI_CONTROL_RATE (definido como macro no seu projeto)

// ------------------------
// Helpers / clamp local
// ------------------------
static inline uint16_t clampU16(uint16_t v, uint16_t lo, uint16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline uint8_t clampU8(uint8_t v, uint8_t lo, uint8_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ------------------------
// Public API
// ------------------------
void Sequencer::begin() {
  // Default: espelha a ideia do reference_unified
  // (você poderá trocar o padrão depois sem alterar arquitetura)

  for (uint8_t i = 0; i < STEP_COUNT; i++) {
    steps_[i].active = true;
    steps_[i].note   = (uint8_t)(36 + i * 2);
    steps_[i].accent = false;
  }

  currentStep_ = 0;
  tickCounter_ = 0;

  bpm_   = 120;
  swing_ = 50;

  recalcStepTiming();

  // Em begin(), não “toca” nada (Sequencer não gera som).
  // O AudioEngine consultará currentGate/currentNote quando tick() avançar.
}

void Sequencer::restart(bool retrigger) {
  (void)retrigger; // Sequencer não toca, então retrigger é “semântico” para quem usa.

  currentStep_ = 0;
  tickCounter_ = 0;

  // timing já está calculado; nada mais a fazer aqui.
}

void Sequencer::setBpm(uint16_t bpm) {
  bpm_ = clampU16(bpm, 30, 300);
  recalcStepTiming();
}

uint16_t Sequencer::getBpm() const {
  return bpm_;
}

void Sequencer::setSwing(uint8_t pct) {
  swing_ = clampU8(pct, 50, 75);
  recalcStepTiming();
}

uint8_t Sequencer::getSwing() const {
  return swing_;
}

// -------- step editing --------
void Sequencer::setStepActive(uint8_t step, bool on) {
  if (step >= STEP_COUNT) return;
  steps_[step].active = on;
}

void Sequencer::toggleStep(uint8_t step) {
  if (step >= STEP_COUNT) return;
  steps_[step].active = !steps_[step].active;
}

bool Sequencer::isStepActive(uint8_t step) const {
  if (step >= STEP_COUNT) return false;
  return steps_[step].active;
}

void Sequencer::setStepNote(uint8_t step, uint8_t midi) {
  if (step >= STEP_COUNT) return;
  steps_[step].note = midi; // clamp opcional; deixo livre 0..127
}

uint8_t Sequencer::getStepNote(uint8_t step) const {
  if (step >= STEP_COUNT) return 60;
  return steps_[step].note;
}

void Sequencer::setStepAccent(uint8_t step, bool on) {
  if (step >= STEP_COUNT) return;
  steps_[step].accent = on;
}

bool Sequencer::getStepAccent(uint8_t step) const {
  if (step >= STEP_COUNT) return false;
  return steps_[step].accent;
}

// -------- playback outputs --------
uint8_t Sequencer::currentStep() const {
  return currentStep_;
}

uint8_t Sequencer::currentNote() const {
  return steps_[currentStep_].note;
}

bool Sequencer::currentGate() const {
  return steps_[currentStep_].active;
}

bool Sequencer::currentAccent() const {
  return steps_[currentStep_].accent;
}

// ------------------------
// Timing internals
// ------------------------
void Sequencer::recalcStepTiming() {
  // Espelha reference_unified:
  // baseTicksPerStep = (CONTROL_RATE * 60) / bpm com arredondamento simples
  // e swingOffset = base * (swing - 50) / 100

  // Proteção extra
  uint16_t bpm = clampU16(bpm_, 30, 300);

  const uint32_t t = (uint32_t)MOZZI_CONTROL_RATE * 60u;
  baseTicksPerStep_ = (uint16_t)((t + (bpm / 2)) / bpm);
  if (baseTicksPerStep_ < 2) baseTicksPerStep_ = 2;

  const int16_t delta = (int16_t)swing_ - 50; // 0..25
  swingOffsetTicks_ = (int16_t)((int32_t)baseTicksPerStep_ * (int32_t)delta / 100);
}

uint16_t Sequencer::ticksForCurrentStep() const {
  // step par alonga, ímpar encurta (igual ao reference_unified)
  int32_t target = (int32_t)baseTicksPerStep_;

  if ((currentStep_ & 1u) == 0u) target += (int32_t)swingOffsetTicks_;
  else                          target -= (int32_t)swingOffsetTicks_;

  if (target < 2) target = 2;
  if (target > 65535) target = 65535;

  return (uint16_t)target;
}

bool Sequencer::tick() {
  // Avança o "tempo" do sequencer em 1 tick.
  // Retorna true SOMENTE quando inicia um novo step.

  tickCounter_++;

  const uint16_t targetTicks = ticksForCurrentStep();

  if (tickCounter_ >= targetTicks) {
    tickCounter_ = 0;

    // Avança step 0..7
    currentStep_++;
    if (currentStep_ >= STEP_COUNT) currentStep_ = 0;

    // Novo step começou
    return true;
  }

  return false;
}