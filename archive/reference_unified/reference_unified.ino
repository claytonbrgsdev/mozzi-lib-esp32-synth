/*
  🎹 ESP32 SYNTH - Firmware otimizado (performance / jitter)
  HARDWARE MAP:
  - Audio: DAC Interno (GPIO 25) via Mozzi INTERNAL_DAC
  - Mux 1: 18, 19, 21 (Sel) | 34 (Z) -> 8 pots
  - Pot direto: 35 (volume master)
  - Encoder: 32 (CLK), 33 (DT), 27 (SW)
  - OLED I2C: SDA 23, SCL 22 (addr 0x3C)
  - TM1638: STB 4, CLK 2, DIO 15
*/

#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_INTERNAL_DAC
#define MOZZI_CONTROL_RATE 128

#include <MozziConfigValues.h>
#include <Mozzi.h>

#include <Oscil.h>
#include <tables/saw2048_int8.h>
#include <LowPassFilter.h>
#include <ADSR.h>
#include <mozzi_midi.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TM1638plus.h>

/* ------------------- PINOS ------------------- */
// MUX1
static const uint8_t MUX_PIN_A = 18;
static const uint8_t MUX_PIN_B = 19;
static const uint8_t MUX_PIN_C = 21;
static const uint8_t MUX_PIN_Z = 34;

// Pot direto
static const uint8_t POT_DIRECT_PIN = 35;

// Encoder
static const uint8_t ENC_CLK = 32;
static const uint8_t ENC_DT  = 33;
static const uint8_t ENC_SW  = 27;

// TM1638
static const uint8_t TM_STB = 4;
static const uint8_t TM_CLK = 2;
static const uint8_t TM_DIO = 15;

// OLED
static const uint8_t OLED_SDA = 23;
static const uint8_t OLED_SCL = 22;
static const uint8_t OLED_ADDR = 0x3C;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

/* ------------------- OBJETOS ------------------- */
Oscil<SAW2048_NUM_CELLS, MOZZI_AUDIO_RATE> aOsc(SAW2048_DATA);
LowPassFilter kFilter;
ADSR<MOZZI_CONTROL_RATE, MOZZI_AUDIO_RATE> env;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TM1638plus tm(TM_STB, TM_CLK, TM_DIO, false);

/* ------------------- SEQUENCER ------------------- */
struct Step {
  bool active;
  uint8_t note;    // MIDI 0..127
  uint8_t accent;  // 0/1 (reservado)
};

static Step sequence[8];
static uint8_t currentStep = 0;

// BPM em "batidas" por minuto (cada step = 1 batida)
static uint16_t bpm = 120;

// tick-based timing (estável)
static uint16_t baseTicksPerStep = 64; // calculado
static uint16_t tickCounter = 0;

// Swing 50..75% (50 = reto)
static uint8_t swingPct = 50;  // vem do MUX1[6]
static int16_t swingOffsetTicks = 0; // calculado

/* ------------------- CONTROLES ------------------- */
static int potValues[8] = {0};
static int potCached[8] = {0};

// round-robin MUX
static uint8_t muxScanCh = 0;

// Volume master
static uint8_t volume = 200;

// Encoder
static int lastEncClk = 1;
static int editMode = 0; // 0=BPM, 1=NOTE, 2=SWING
static uint32_t lastBtnMs = 0;

// TM1638
static uint8_t lastButtons = 0;

// UI
static bool displayDirty = true;
static uint8_t uiTick = 0;

/* ------------------- HELPERS ------------------- */
static inline void muxSelect(uint8_t ch) {
  digitalWrite(MUX_PIN_A, (ch >> 0) & 1);
  digitalWrite(MUX_PIN_B, (ch >> 1) & 1);
  digitalWrite(MUX_PIN_C, (ch >> 2) & 1);
}

// leitura “barata”: 1 canal por tick
static void readMuxRoundRobin() {
  muxSelect(muxScanCh);
  // descarte curto ajuda a estabilizar troca de canal
  (void)mozziAnalogRead(MUX_PIN_Z);
  int v = mozziAnalogRead(MUX_PIN_Z);

  // smoothing IIR leve (barato)
  potValues[muxScanCh] = (potValues[muxScanCh] * 3 + v) >> 2;

  muxScanCh = (muxScanCh + 1) & 0x07;
}

static void recalcStepTiming() {
  // cada step = 1 beat => ticks/step = CONTROL_RATE * 60 / bpm
  // com arredondamento:
  uint32_t t = (uint32_t)MOZZI_CONTROL_RATE * 60u;
  baseTicksPerStep = (uint16_t)((t + (bpm / 2)) / bpm);
  if (baseTicksPerStep < 2) baseTicksPerStep = 2;

  // swing: 50% = 0 offset; 66% ~ triplet feel; 75% bem puxado.
  // offset em ticks aplicado alternando: step par alonga, ímpar encurta.
  // offset = base * (swingPct-50)/100
  int16_t delta = (int16_t)swingPct - 50;
  swingOffsetTicks = (int16_t)((int32_t)baseTicksPerStep * delta / 100);
}

static void triggerStep(uint8_t stepIdx) {
  if (!sequence[stepIdx].active) return;

  // mtof usa float internamente, mas aqui só roda no evento de step
  float f = mtof(sequence[stepIdx].note);
  aOsc.setFreq(f);

  // Accent (se quiser usar depois)
  // ex: env.setADLevels(255, sequence[stepIdx].accent ? 220 : 150);

  env.noteOn();
}

// Encoder quadrature simples + botão sem delay()
static void readEncoderNonBlocking() {
  int clk = digitalRead(ENC_CLK);
  if (clk != lastEncClk && clk == LOW) {
    int dt = digitalRead(ENC_DT);

    // direção
    int dir = (dt != clk) ? +1 : -1;

    if (editMode == 0) {
      int nbpm = (int)bpm + dir * 1;
      if (nbpm < 30) nbpm = 30;
      if (nbpm > 300) nbpm = 300;
      bpm = (uint16_t)nbpm;
      recalcStepTiming();
    } else if (editMode == 1) {
      int nn = (int)sequence[currentStep].note + dir;
      if (nn < 0) nn = 0;
      if (nn > 127) nn = 127;
      sequence[currentStep].note = (uint8_t)nn;
    } else if (editMode == 2) {
      int ns = (int)swingPct + dir;
      if (ns < 50) ns = 50;
      if (ns > 75) ns = 75;
      swingPct = (uint8_t)ns;
      recalcStepTiming();
    }
    displayDirty = true;
  }
  lastEncClk = clk;

  // botão do encoder (debounce por tempo)
  if (digitalRead(ENC_SW) == LOW) {
    uint32_t now = millis();
    if (now - lastBtnMs > 220) {
      lastBtnMs = now;
      editMode++;
      if (editMode > 2) editMode = 0;
      displayDirty = true;
    }
  }
}

static void updateTM1638Throttled() {
  // ler botões a cada ~2ms*? => aqui a cada 4 ticks (128Hz => ~31ms)
  // suficiente para responsividade sem pesar.
  static uint8_t tmTick = 0;
  tmTick++;
  if (tmTick < 4) return;
  tmTick = 0;

  uint8_t b = tm.readButtons();
  uint8_t edges = (uint8_t)(b & ~lastButtons);
  lastButtons = b;

  // toggle steps no rising edge
  if (edges) {
    for (uint8_t i = 0; i < 8; i++) {
      if ((edges >> i) & 1) {
        sequence[i].active = !sequence[i].active;
        displayDirty = true;
      }
    }
  }

  // LEDs: 1 = cursor, demais = ativos
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t on = (i == currentStep) ? 1 : (sequence[i].active ? 1 : 0);
    tm.setLED(i, on);
  }

  // 7-seg: mostra BPM / NOTE / SWING conforme modo
  if (editMode == 0) tm.displayIntNum(bpm, false);
  else if (editMode == 1) tm.displayIntNum(sequence[currentStep].note, false);
  else tm.displayIntNum(swingPct, false);
}

static void updateOLEDThrottled() {
  // atualiza OLED a cada ~8 ticks => 128Hz/8 = 16Hz (bom e leve)
  uiTick++;
  if (uiTick < 8) return;
  uiTick = 0;

  if (!displayDirty) return;

  // opcional: só redesenha barras se mudou “o bastante”
  bool barsChanged = false;
  for (uint8_t i = 0; i < 8; i++) {
    int dv = potValues[i] - potCached[i];
    if (dv < 0) dv = -dv;
    if (dv > 40) { barsChanged = true; break; }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print("BPM:"); display.print(bpm);

  display.setCursor(64, 0);
  if (editMode == 0) display.print("MODE:TEMPO");
  else if (editMode == 1) display.print("MODE:NOTA");
  else display.print("MODE:SWING");

  display.setCursor(0, 12);
  display.print("STEP:"); display.print(currentStep + 1);
  display.print(" N:"); display.print(sequence[currentStep].note);
  display.print(" S:"); display.print(swingPct);

  // barras (se quiser ainda mais leve: só desenhar quando barsChanged)
  if (barsChanged) {
    for (uint8_t i = 0; i < 8; i++) potCached[i] = potValues[i];
  }
  for (uint8_t i = 0; i < 8; i++) {
    int v = barsChanged ? potCached[i] : potCached[i];
    int h = map(v, 0, 4095, 0, 25);
    display.fillRect(i * 16, 63 - h, 14, h, WHITE);
  }

  display.display();
  displayDirty = false;
}

/* ------------------- SETUP ------------------- */
void setup() {
  Serial.begin(115200);

  pinMode(MUX_PIN_A, OUTPUT);
  pinMode(MUX_PIN_B, OUTPUT);
  pinMode(MUX_PIN_C, OUTPUT);
  pinMode(MUX_PIN_Z, INPUT);

  pinMode(ENC_CLK, INPUT);
  pinMode(ENC_DT, INPUT);
  pinMode(ENC_SW, INPUT_PULLUP);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    for(;;) {}
  }

  tm.displayBegin();
  tm.reset();
  tm.brightness(2);

  // sequência default
  for (uint8_t i = 0; i < 8; i++) {
    sequence[i].active = true;
    sequence[i].note = 36 + i * 2;
    sequence[i].accent = 0;
  }

  // synth
  kFilter.setResonance(140);
  kFilter.setCutoffFreq(120);
  aOsc.setFreq(220);

  env.setADLevels(255, 160);
  // setTimes(attack, decay, sustain_ms, release)
  env.setTimes(20, 120, 50, 180);

  recalcStepTiming();

  startMozzi(MOZZI_CONTROL_RATE);
}

/* ------------------- CONTROL ------------------- */
void updateControl() {
  // 1) Ler MUX 1 round-robin (barato)
  readMuxRoundRobin();

  // 2) Pot direto (volume) - pode ser mais lento também; aqui a cada 2 ticks
  static uint8_t vTick = 0;
  vTick++;
  if ((vTick & 1) == 0) {
    int directPot = mozziAnalogRead(POT_DIRECT_PIN);
    volume = (uint8_t)map(directPot, 0, 4095, 0, 255);
  }

  // 3) Encoder (non-blocking)
  readEncoderNonBlocking();

  // 4) Atualizar params do synth (barato: só usa ints)
  uint8_t cutoff = (uint8_t)map(potValues[0], 0, 4095, 10, 255);
  uint8_t res    = (uint8_t)map(potValues[1], 0, 4095, 0, 255);
  uint16_t attk  = (uint16_t)map(potValues[2], 0, 4095, 5, 800);
  uint16_t dec   = (uint16_t)map(potValues[3], 0, 4095, 5, 900);
  uint8_t sus    = (uint8_t)map(potValues[4], 0, 4095, 0, 255);
  uint16_t rel   = (uint16_t)map(potValues[5], 0, 4095, 5, 1500);

  // Swing via pot MUX1[6]
  swingPct = (uint8_t)map(potValues[6], 0, 4095, 50, 75);
  recalcStepTiming();

  kFilter.setCutoffFreq(cutoff);
  kFilter.setResonance(res);
  env.setADLevels(255, sus);
  env.setTimes(attk, dec, 40, rel);
  env.update();

  // 5) Sequencer tick-based com swing
  // alterna ticks por step:
  // step par: base + swingOffset
  // step ímpar: base - swingOffset
  uint16_t targetTicks = baseTicksPerStep;
  if ((currentStep & 1) == 0) targetTicks = (uint16_t)((int32_t)baseTicksPerStep + swingOffsetTicks);
  else                        targetTicks = (uint16_t)((int32_t)baseTicksPerStep - swingOffsetTicks);
  if (targetTicks < 2) targetTicks = 2;

  tickCounter++;
  if (tickCounter >= targetTicks) {
    tickCounter = 0;
    currentStep = (currentStep + 1) & 0x07;
    triggerStep(currentStep);
    displayDirty = true;
  }

  // 6) UI throttled (não pesa no áudio)
  updateTM1638Throttled();
  updateOLEDThrottled();
}

/* ------------------- AUDIO ------------------- */
AudioOutput updateAudio() {
  int16_t s = (int16_t)aOsc.next();     // -128..127
  int16_t f = (int16_t)kFilter.next(s); // filtrado

  // env.next() retorna 0..255
  int16_t e = (int16_t)env.next();
  int32_t sig = (int32_t)f * e;         // ~ 16*8 bits
  sig >>= 8;

  sig = (sig * volume) >> 8;

  // saída mono (DAC interno)
  return MonoOutput::from16Bit((int16_t)(sig << 8));
}

/* ------------------- LOOP ------------------- */
void loop() {
  audioHook();
}

