/*
  ✅ TESTE MUX2 (CD4051) - ESP32
  A/B/C compartilhados com MUX1: 18,19,21
  Z/COM do MUX2: GPIO26 (ADC2)
*/

#include <Arduino.h>

static const uint8_t MUX_A = 18;
static const uint8_t MUX_B = 19;
static const uint8_t MUX_C = 21;

// Z/COM do MUX2 (ADC)
static const uint8_t MUX2_Z = 26;

static inline void muxSelect(uint8_t ch) {
  digitalWrite(MUX_A, (ch >> 0) & 1);
  digitalWrite(MUX_B, (ch >> 1) & 1);
  digitalWrite(MUX_C, (ch >> 2) & 1);
}

static int readMux2(uint8_t ch) {
  muxSelect(ch);
  delayMicroseconds(80);
  (void)analogRead(MUX2_Z);   // descarte (ajuda na troca de canal)
  delayMicroseconds(80);
  return analogRead(MUX2_Z);  // 0..4095
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);
  pinMode(MUX_C, OUTPUT);

  pinMode(MUX2_Z, INPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(MUX2_Z, ADC_11db);

  Serial.println();
  Serial.println("=== TESTE MUX2 (ANALOG) ===");
  Serial.println("A=18 B=19 C=21 | Z(MUX2)=26");
  Serial.println("Imprimindo CH0..CH7 a cada 200ms");
  Serial.println();
}

void loop() {
  static uint32_t t0 = 0;
  if (millis() - t0 < 200) return;
  t0 = millis();

  for (uint8_t ch = 0; ch < 8; ch++) {
    int v = readMux2(ch);
    Serial.print("CH"); Serial.print(ch);
    Serial.print(": "); Serial.print(v);

    // Só rótulos (ajuste se quiser)
    if (ch == 0) Serial.print("  (BTN0)");
    if (ch == 1) Serial.print("  (BTN1)");
    if (ch == 2) Serial.print("  (BTN2)");
    if (ch == 3) Serial.print("  (BTN3)");
    if (ch == 4) Serial.print("  (RES LADDER KEYBOARD)");
    if (ch == 5) Serial.print("  (SPDT)");
    if (ch == 6) Serial.print("  (POT6)");
    if (ch == 7) Serial.print("  (POT7)");
    Serial.println();
  }
  Serial.println("----------------------------");
}
