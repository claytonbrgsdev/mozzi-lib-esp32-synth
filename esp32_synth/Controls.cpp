#include "Controls.h"

void Controls::begin(EventQueue* q) {
  q_ = q;

  pinMode(PIN_MUX_A, OUTPUT);
  pinMode(PIN_MUX_B, OUTPUT);
  pinMode(PIN_MUX_C, OUTPUT);

  // Z do MUX2 em GPIO digital com pullup
  pinMode(PIN_MUX2_Z, INPUT_PULLUP);

  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);

  // init encoder
  uint8_t a = (uint8_t)digitalRead(PIN_ENC_CLK);
  uint8_t b = (uint8_t)digitalRead(PIN_ENC_DT);
  encPrev_ = (a << 1) | b;
  encSwPrev_ = (digitalRead(PIN_ENC_SW) == LOW);

  // init debounce arrays (somente 0..4)
  for (uint8_t i = 0; i < MUX2_BUTTON_COUNT; i++) {
    btnInt_[i] = 0;
    btnStable_[i] = false;
  }

  mux1Chan_ = 0;
  mux2Chan_ = 0;
  volDivider_ = 0;
  shiftHeld_ = false;
}

void Controls::setMuxChannel(uint8_t ch) {
  digitalWrite(PIN_MUX_A, (ch & 0x01) ? HIGH : LOW);
  digitalWrite(PIN_MUX_B, (ch & 0x02) ? HIGH : LOW);
  digitalWrite(PIN_MUX_C, (ch & 0x04) ? HIGH : LOW);
}

uint16_t Controls::readAdcFast(uint8_t pin) {
  return (uint16_t)analogRead(pin);
}

uint16_t Controls::iir(uint16_t prev, uint16_t x) {
  int32_t diff = (int32_t)x - (int32_t)prev;
  return (uint16_t)((int32_t)prev + (diff >> ADC_IIR_SHIFT));
}

void Controls::scanMux1Pots(SynthParams& params) {
  // 1 canal por tick no MUX1
  setMuxChannel(mux1Chan_);
  uint16_t raw = readAdcFast(PIN_ADC_MUX1_Z);

  switch (mux1Chan_) {
    case 0: params.cutoff    = iir(params.cutoff, raw); break;
    case 1: params.resonance = iir(params.resonance, raw); break;
    case 2: params.attack    = iir(params.attack, raw); break;
    case 3: params.decay     = iir(params.decay, raw); break;
    case 4: params.sustain   = iir(params.sustain, raw); break;
    case 5: params.release   = iir(params.release, raw); break;
    case 6: {
      uint16_t sm = iir(params.swing, raw);
      uint8_t s = (uint8_t)map(sm, 0, 1023, SWING_MIN, SWING_MAX);
      params.swing = s;
    } break;
    case 7: params.pot7      = iir(params.pot7, raw); break;
  }

  mux1Chan_ = (uint8_t)((mux1Chan_ + 1) & 0x07);
}

void Controls::scanMux2Buttons() {
  // Você tem botões apenas nos canais 0..4 do MUX2.
  // Faz round-robin apenas nesses 5 canais.
  setMuxChannel(mux2Chan_);

  // Botão fecha para GND => down = LOW
  bool downNow = (digitalRead(PIN_MUX2_Z) == LOW);

  // Debounce por integrador (0..BTN_INT_MAX)
  uint8_t &integ = btnInt_[mux2Chan_];

  if (downNow) {
    if (integ < BTN_INT_MAX) integ++;
  } else {
    if (integ > 0) integ--;
  }

  bool stable = btnStable_[mux2Chan_];

  // Histerese: liga em >= BTN_INT_ON_TH, desliga em <= BTN_INT_OFF_TH
  if (!stable && integ >= BTN_INT_ON_TH) {
    btnStable_[mux2Chan_] = true;
    if (q_) q_->push({EVT_BTN_DOWN, mux2Chan_, 1});
  } else if (stable && integ <= BTN_INT_OFF_TH) {
    btnStable_[mux2Chan_] = false;
    if (q_) q_->push({EVT_BTN_UP, mux2Chan_, 0});
  }

  // SHIFT held: como SHIFT está no canal 0 (ver mapeamento no .ino)
  // aqui a gente atualiza o cache só se esse canal for o do shift,
  // mas também dá para recalcular a cada tick lendo btnStable_[0].
  // Vou recalcular sempre (barato).
  shiftHeld_ = btnStable_[0];

  // Avança canal 0..4
  mux2Chan_++;
  if (mux2Chan_ >= MUX2_BUTTON_COUNT) mux2Chan_ = 0;
}

void Controls::scanEncoder() {
  uint8_t a = (uint8_t)digitalRead(PIN_ENC_CLK);
  uint8_t b = (uint8_t)digitalRead(PIN_ENC_DT);
  uint8_t cur = (a << 1) | b;

  if (cur != encPrev_) {
    int8_t delta = 0;

    // cw: 00->01->11->10->00
    if ((encPrev_ == 0b00 && cur == 0b01) ||
        (encPrev_ == 0b01 && cur == 0b11) ||
        (encPrev_ == 0b11 && cur == 0b10) ||
        (encPrev_ == 0b10 && cur == 0b00)) {
      delta = +1;
    }
    // ccw: 00->10->11->01->00
    else if ((encPrev_ == 0b00 && cur == 0b10) ||
             (encPrev_ == 0b10 && cur == 0b11) ||
             (encPrev_ == 0b11 && cur == 0b01) ||
             (encPrev_ == 0b01 && cur == 0b00)) {
      delta = -1;
    }

    encPrev_ = cur;
    if (delta != 0 && q_) q_->push({EVT_ENC_DELTA, 0, delta});
  }

  bool swDown = (digitalRead(PIN_ENC_SW) == LOW);
  if (swDown && !encSwPrev_) {
    encSwPrev_ = true;
    if (q_) q_->push({EVT_ENC_CLICK, 0, 1});
  } else if (!swDown && encSwPrev_) {
    encSwPrev_ = false;
  }
}

void Controls::tick(SynthParams& params) {
  scanMux1Pots(params);
  scanMux2Buttons();
  scanEncoder();

  // Volume mais lento
  volDivider_++;
  if (volDivider_ >= 8) {
    volDivider_ = 0;
    uint16_t raw = readAdcFast(PIN_ADC_VOL);
    params.volume = iir(params.volume, raw);
  }
}
