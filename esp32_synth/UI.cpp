#include "UI.h"
#include "Config.h"

void UI::begin(EventQueue* q, Sequencer* seq) {
  q_ = q;
  seq_ = seq;

  // UI mínima: não inicializa OLED nem TM1638 para evitar dependências agora.
  // Se você quiser debug, habilite Serial no setup() do .ino.
}

void UI::tick(const SynthParams& params, UiState state, uint8_t editIndex, EditLayer layer) {
  (void)params; (void)state; (void)editIndex; (void)layer;

  // UI mínima: não faz nada.
  // Depois você pode implementar OLED/TM1638 aqui sem mexer no resto do sistema.

  // Opcional: exemplo de throttle p/ debug (descomente se quiser)
  /*
  const uint32_t now = millis();
  if (now - lastPrintMs_ >= (1000u / UI_FPS)) {
    lastPrintMs_ = now;
    if (seq_) {
      Serial.print("BPM=");
      Serial.print(seq_->getBpm());
      Serial.print(" play=");
      Serial.print(seq_->isPlaying());
      Serial.print(" step=");
      Serial.println(seq_->playIndex());
    }
  }
  */
}