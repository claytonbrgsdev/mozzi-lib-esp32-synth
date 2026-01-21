/*
  TESTE 01 - AUDIO
  ESP32 + Mozzi -> DAC interno GPIO25
  Gera uma onda senoidal constante (440Hz)
*/

#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_INTERNAL_DAC
#define MOZZI_CONTROL_RATE 128

#include <MozziConfigValues.h>
#include <Mozzi.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

Oscil<SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> osc(SIN2048_DATA);

void setup() {
  osc.setFreq(440.0f);
  startMozzi(MOZZI_CONTROL_RATE);
}

void updateControl() {
  // nada
}

AudioOutput updateAudio() {
  int16_t s = (int16_t)osc.next();       // -128..127
  return MonoOutput::from16Bit(s << 8);  // escala p/ 16-bit
}

void loop() {
  audioHook();
}
