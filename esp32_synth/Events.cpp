#include "Events.h"

void EventQueue::begin() {
  clear();
}

void EventQueue::clear() {
  head_ = 0;
  tail_ = 0;
}

bool EventQueue::push(const Event& e) {
  const uint8_t next = (uint8_t)((head_ + 1) % CAP);
  if (next == tail_) return false; // cheio
  buf_[head_] = e;
  head_ = next;
  return true;
}

bool EventQueue::pop(Event& out) {
  if (tail_ == head_) return false; // vazio
  out = buf_[tail_];
  tail_ = (uint8_t)((tail_ + 1) % CAP);
  return true;
}