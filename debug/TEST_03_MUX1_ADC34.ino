/*
  TESTE 03 - MUX 1 (CD4051)
  A=18 B=19 C=21
  Z/COM = GPIO34 (ADC1)
*/

#include <Arduino.h>

static const uint8_t MUX_A = 18;
static const uint8_t MUX_B = 19;
static const uint8_t MUX_C = 21;
static const uint8_t MUX_Z = 34;

static inline void muxSelect(uint8_t ch) {
  digitalWrite(MUX_A, (ch >> 0) & 1);
  digitalWrite(MUX_B, (ch >> 1) & 1);
  digitalWrite(MUX_C, (ch >> 2) & 1);
}

static int readMux(uint8_t ch) {
  muxSelect(ch);
  delayMicroseconds(80);
  (void)analogRead(MUX_Z);
  delayMicroseconds(80);
  return analogRead(MUX_Z);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);
  pinMode(MUX_C, OUTPUT);
  pinMode(MUX_Z, INPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(MUX_Z, ADC_11db);

  Serial.println("TESTE MUX1: CH0..CH7 (200ms)");
}

void loop() {
  for (uint8_t ch = 0; ch < 8; ch++) {
    int v = readMux(ch);
    Serial.print("CH"); Serial.print(ch);
    Serial.print(": "); Serial.println(v);
  }
  Serial.println("---");
  delay(200);
}
