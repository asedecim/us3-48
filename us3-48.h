#pragma once
#include "esphome.h"
#include <vector>

class Us3_48Component : public Component, public uart::UARTDevice {
 public:
  Us3_48Component(uart::UARTComponent *parent) : UARTDevice(parent) {}

  void setup() override {
    ESP_LOGD("us3_48", "ИБП компонент запущен");
    last_request_ = millis();
  }

  void loop() override {
    // ... весь код без изменений ...
  }

  // Все set_методы оставляем как есть
  void set_выходное_напряжение(sensor::Sensor *s) { выходное_напряжение_ = s; }
  // ... остальные методы ...

 private:
  std::vector<uint8_t> buffer_;
  unsigned long last_request_ = 0;
  sensor::Sensor *выходное_напряжение_ = nullptr;
  // ... остальные переменные ...
};
