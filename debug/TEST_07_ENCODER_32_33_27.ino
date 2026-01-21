/*
  TESTE 07 - ENCODER
  CLK=32 DT=33 SW=27
  - Mostra contador e sentido
  - Clique do encoder zera contador
*/

#include <Arduino.h>

static const uint8_t ENC_CLK = 32;
static const uint8_t ENC_DT  = 33;
static const uint8_t ENC_SW  = 27;

static int lastClk = 1;
static long count = 0;
static uint32_t lastPressMs = 0;

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(ENC_CLK, INPUT);
  pinMode(ENC_DT, INPUT);
  pinMode(ENC_SW, INPUT_PULLUP);

  Serial.println("TESTE ENCODER: gira para contar, clique zera.");
}

void loop() {
  int clk = digitalRead(ENC_CLK);
  if (clk != lastClk && clk == LOW) {
    int dt = digitalRead(ENC_DT);
    int dir = (dt != clk) ? +1 : -1;
    count += dir;

    Serial.print("DIR=");
    Serial.print(dir > 0 ? "CW" : "CCW");
    Serial.print("  COUNT=");
    Serial.println(count);
  }
  lastClk = clk;

  if (digitalRead(ENC_SW) == LOW) {
    uint32_t now = millis();
    if (now - lastPressMs > 250) {
      lastPressMs = now;
      count = 0;
      Serial.println("CLICK -> COUNT=0");
    }
  }
}
