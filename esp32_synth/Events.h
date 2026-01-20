#pragma once
#include <Arduino.h>

enum EventType : uint8_t {
  EVT_NONE = 0,
  EVT_BTN_DOWN,
  EVT_BTN_UP,
  EVT_STEP_BTN,    // botão do step (ex.: TM1638), id=0..7
  EVT_ENC_DELTA,   // value = +1/-1
  EVT_ENC_CLICK,   // click do encoder
};

enum ButtonId : uint8_t {
  BTN_SHIFT = 0,
  BTN_PLAY  = 1,
  BTN_EDIT  = 2,
  BTN_PATTERN = 3,
  BTN_LENGTH  = 4,
  BTN_ACCENT  = 5,
  BTN_SCALE   = 6,
  BTN_CLEAR   = 7,
};

struct Event {
  EventType type = EVT_NONE;
  uint8_t id = 0;
  int16_t value = 0;
};

class EventQueue {
public:
  void begin();
  bool push(const Event& e);
  bool pop(Event& out);
  void clear();

private:
  static constexpr uint8_t CAP = 32;
  volatile uint8_t head_ = 0;
  volatile uint8_t tail_ = 0;
  Event buf_[CAP];
};