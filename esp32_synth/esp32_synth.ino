#include <Arduino.h>

#include "Config.h"     // antes
#include <MozziGuts.h>  // SOMENTE AQUI (apenas no .ino)

#include "Pins.h"
#include "Types.h"
#include "Events.h"
#include "Sequencer.h"
#include "AudioEngine.h"
#include "Controls.h"
#include "UI.h"

// ===== Singletons =====
static EventQueue  gEvents;
static Sequencer   gSeq;     // Sequencer v2 (tick-based, sempre rodando)
static AudioEngine gAudio;
static Controls    gControls;
static UI          gUI;

static SynthParams gParams;

// UI/edição (mantém seu modelo atual)
static UiState   gUiState   = UI_PLAY;      // UI_PLAY / UI_EDIT_STEP
static EditLayer gLayer     = LAYER_NOTE;   // vamos usar NOTE e ACCENT (GATE é ignorado no v2)
static uint8_t   gEditIndex = 0;

static bool gShiftHeld = false;

// ----- MAPA DOS 5 BOTOES DO MUX2 -----
// canais 0..4 => ids lógicos
static uint8_t mapMux2ChannelToButtonId(uint8_t ch) {
  switch (ch) {
    case 0: return BTN_SHIFT;   // pino 13 do 4051
    case 1: return BTN_PLAY;    // pino 14 (no v2: restart)
    case 2: return BTN_EDIT;    // pino 15
    case 3: return BTN_ACCENT;  // pino 12
    case 4: return BTN_CLEAR;   // pino 4
    default: return 255;
  }
}

static void applyLayerCycle() {
  // No Sequencer v2 não existe gate por step (ainda).
  // Então cycle fica: NOTE <-> ACCENT (ignora LAYER_GATE se existir no enum).
  if (gLayer == LAYER_NOTE) gLayer = LAYER_ACCENT;
  else gLayer = LAYER_NOTE;
}

static void handleEvent(const Event& e) {
  switch (e.type) {

    case EVT_BTN_DOWN: {
      // EVT_BTN_* vem com id = canal do mux2 (0..4)
      uint8_t bid = mapMux2ChannelToButtonId(e.id);
      if (bid == 255) break;

      if (bid == BTN_SHIFT) {
        gShiftHeld = true;
        break;
      }

      if (bid == BTN_PLAY) {
        // No Sequencer v2 (modo "unified"): não há play/pause.
        // BTN_PLAY vira RESTART imediato no step 0.
        // SHIFT+PLAY também faz restart (mesma ação, por enquanto).
        gSeq.restart(true);
        gAudio.onStep(); // garante retrigger do step 0 (se ativo)
        break;
      }

      if (bid == BTN_EDIT) {
        if (gShiftHeld) {
          // SHIFT+EDIT: troca layer rapidamente
          applyLayerCycle();
        } else {
          // alterna modo edit
          gUiState = (gUiState == UI_PLAY) ? UI_EDIT_STEP : UI_PLAY;
        }
        break;
      }

      if (bid == BTN_ACCENT) {
        // Vai direto para layer ACCENT
        gLayer = LAYER_ACCENT;
        break;
      }

      if (bid == BTN_CLEAR) {
        if (gShiftHeld) {
          // clear pattern inteiro (v2)
          for (uint8_t i = 0; i < Sequencer::STEP_COUNT; i++) {
            gSeq.setStepActive(i, false);
            gSeq.setStepAccent(i, false);
            // nota mantém (como referência), mas você pode zerar se quiser:
            // gSeq.setStepNote(i, 60);
          }
        } else {
          // clear step selecionado (em edit) ou step atual (em play)
          uint8_t idx = (gUiState == UI_EDIT_STEP) ? gEditIndex : gSeq.currentStep();
          gSeq.setStepActive(idx, false);
          gSeq.setStepAccent(idx, false);
        }
        break;
      }

    } break;

    case EVT_BTN_UP: {
      uint8_t bid = mapMux2ChannelToButtonId(e.id);
      if (bid == BTN_SHIFT) gShiftHeld = false;
    } break;

    case EVT_STEP_BTN: {
      // TM1638 step button (0..7)
      uint8_t step = e.id & 0x07;

      if (gUiState == UI_PLAY) {
        // Em play: toggle active
        gSeq.toggleStep(step);
      } else {
        // Em edit:
        if (gShiftHeld) {
          // SHIFT: toggle active sem mudar o cursor de edição
          gSeq.toggleStep(step);
        } else {
          // Seleciona step para editar
          gEditIndex = step;
        }
      }
    } break;

    case EVT_ENC_DELTA: {
      int16_t d = e.value;

      if (gUiState == UI_PLAY) {
        // Encoder no PLAY: BPM
        int nbpm = (int)gSeq.getBpm() + (int)d;
        if (nbpm < 30) nbpm = 30;
        if (nbpm > 300) nbpm = 300;
        gSeq.setBpm((uint16_t)nbpm);
      } else {
        // EDIT: layer define o que muda
        if (gLayer == LAYER_NOTE) {
          int n = (int)gSeq.getStepNote(gEditIndex) + (int)d;
          if (n < 0) n = 0;
          if (n > 127) n = 127;
          gSeq.setStepNote(gEditIndex, (uint8_t)n);
        } else if (gLayer == LAYER_ACCENT) {
          if (d != 0) {
            bool a = gSeq.getStepAccent(gEditIndex);
            gSeq.setStepAccent(gEditIndex, !a);
          }
        } else {
          // Se existir LAYER_GATE no enum, ignoramos no v2 por enquanto.
        }
      }
    } break;

    case EVT_ENC_CLICK: {
      // Click:
      // - se estiver em edit: alterna layer (NOTE <-> ACCENT)
      // - se estiver em play: entra em edit
      if (gUiState == UI_EDIT_STEP) {
        applyLayerCycle();
      } else {
        gUiState = UI_EDIT_STEP;
      }
    } break;

    default: break;
  }
}

// ===== Mozzi hooks =====
void setup() {
  gEvents.begin();

  // Sequencer v2
  gSeq.begin();

  // Controls/UI
  gControls.begin(&gEvents);
  gUI.begin(&gEvents, &gSeq);

  // Audio
  gAudio.begin();
  gAudio.setSequencer(&gSeq);

  // Garante um disparo inicial coerente (opcional, mas útil)
  gAudio.onStep();

  startMozzi();
}

void updateControl() {
  // 1) scan controles
  gControls.tick(gParams);

  // 2) aplica swing do pot (v2)
  // (assumindo que gParams.swing já está em 50..75 como você vinha fazendo)
  gSeq.setSwing((uint8_t)gParams.swing);

  // 3) sequencer tick
  // Quando iniciar um novo step -> retrigger correto no áudio
  if (gSeq.tick()) {
    gAudio.onStep();
  }

  // 4) processa eventos
  Event e;
  while (gEvents.pop(e)) {
    handleEvent(e);
  }

  // 5) UI tick
  gUI.tick(gParams, gUiState, gEditIndex, gLayer);

  // 6) Audio-side param update (cutoff/res/ADSR/volume)
  gAudio.setParams(gParams);
  gAudio.updateControlAudioSide();
}

int updateAudio() {
  return gAudio.updateAudio();
}

void loop() {
  audioHook();
}