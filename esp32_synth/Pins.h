#pragma once
#include <Arduino.h>

// ===== MUX #1 (pots) CD4051 =====
static constexpr uint8_t PIN_MUX_A = 18;
static constexpr uint8_t PIN_MUX_B = 19;
static constexpr uint8_t PIN_MUX_C = 21;

static constexpr uint8_t PIN_ADC_MUX1_Z = 34;  // ADC do MUX1 (pots)

// Pot direto (volume master)
static constexpr uint8_t PIN_ADC_VOL = 35;

// ===== MUX #2 (botões) CD4051 =====
// Reusa A/B/C do MUX1, e lê Z digital com pullup
static constexpr uint8_t PIN_MUX2_Z = 26; // GPIO digital INPUT_PULLUP

// ===== Encoder =====
static constexpr uint8_t PIN_ENC_CLK = 32;
static constexpr uint8_t PIN_ENC_DT  = 33;
static constexpr uint8_t PIN_ENC_SW  = 27;

// ===== Audio DAC =====
static constexpr uint8_t PIN_DAC_OUT = 25; // ESP32 DAC1 (GPIO25)

// ===== I2C OLED (se você usar) =====
static constexpr uint8_t PIN_I2C_SDA = 23;
static constexpr uint8_t PIN_I2C_SCL = 22;
static constexpr uint8_t OLED_ADDR   = 0x3C;

// ===== TM1638 (se você usar) =====
static constexpr uint8_t PIN_TM_STB = 4;
static constexpr uint8_t PIN_TM_CLK = 2;
static constexpr uint8_t PIN_TM_DIO = 15;