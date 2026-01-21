/*
  TESTE 06 - OLED SSD1306 I2C
  SDA=23 SCL=22  ADDR=0x3C
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static const uint8_t OLED_SDA  = 23;
static const uint8_t OLED_SCL  = 22;
static const uint8_t OLED_ADDR = 0x3C;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  delay(200);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000); // 400kHz

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Falha no OLED");
    for(;;) {}
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("OLED OK");
  display.println("SDA=23 SCL=22");
  display.display();
}

void loop() {
  static int x = 0;
  static int dir = 1;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Animacao simples");

  display.fillCircle(x, 40, 4, WHITE);
  display.display();

  x += dir * 3;
  if (x <= 4) dir = 1;
  if (x >= 124) dir = -1;

  delay(50);
}
