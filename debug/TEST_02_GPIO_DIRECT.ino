/*
  TESTE 02 - GPIOs DIRETAS
  - Pisca o LED da placa (se existir) ou um LED externo no GPIO26
  - Lê um botão no GPIO27 (INPUT_PULLUP) e imprime no Serial
*/

#include <Arduino.h>

static const uint8_t LED_PIN = 26;   // troque se quiser
static const uint8_t BTN_PIN = 27;   // troque se quiser (INPUT_PULLUP)

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  Serial.println("TESTE GPIO: LED piscando e leitura de botao (LOW=pressionado).");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(250);
  digitalWrite(LED_PIN, LOW);
  delay(250);

  int b = digitalRead(BTN_PIN);
  Serial.print("BTN="); Serial.println(b == LOW ? "PRESSED" : "RELEASED");
}
