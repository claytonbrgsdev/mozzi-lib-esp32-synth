/*
  TESTE 05 - TM1638
  STB=4  CLK=2  DIO=15
  - Faz "corrida" nos LEDs
  - Mostra no display o indice do LED
  - Lê botões e mostra o bitmask no display (hex)
*/

#include <Arduino.h>
#include <TM1638plus.h>

static const uint8_t TM_STB = 4;
static const uint8_t TM_CLK = 2;
static const uint8_t TM_DIO = 15;

TM1638plus tm(TM_STB, TM_CLK, TM_DIO, false);

void setup() {
  Serial.begin(115200);
  delay(200);

  tm.displayBegin();
  tm.reset();
  tm.brightness(2);

  Serial.println("TESTE TM1638: LEDs correndo + botoes.");
}

void loop() {
  static uint8_t pos = 0;

  // LED chase
  for (uint8_t i = 0; i < 8; i++) tm.setLED(i, (i == pos) ? 1 : 0);
  tm.displayIntNum(pos + 1, false);

  // botões
  uint8_t b = tm.readButtons();
  if (b) {
    Serial.print("Buttons bitmask = 0x");
    Serial.println(b, HEX);
    tm.displayHex(0, b);  // mostra bits em hex no 7-seg
  }

  pos = (pos + 1) & 0x07;
  delay(120);
}
