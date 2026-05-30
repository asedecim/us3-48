#pragma once
#include "esphome.h"
#include <vector>

class UPSComponent : public Component, public uart::UARTDevice {
 public:
  UPSComponent(uart::UARTComponent *parent) : UARTDevice(parent) {}

  void setup() override {
    ESP_LOGD("ups", "UPS started");
  }

  void loop() override {
    while (available()) {
      uint8_t b;
      read_byte(&b);
      buffer_.push_back(b);
    }
    
    for (int i = 0; i < (int)buffer_.size() - 10; i++) {
      if (buffer_[i] == 0xBE && buffer_[i+1] == 0xC1 && buffer_[i+10] == 0xAA) {
        uint16_t v = (buffer_[i+2] << 8) | buffer_[i+3];
        uint16_t a = (buffer_[i+4] << 8) | buffer_[i+5];
        uint16_t p = (buffer_[i+6] << 8) | buffer_[i+7];
        uint16_t b = (buffer_[i+8] << 8) | buffer_[i+9];
        
        if (voltage_sensor_) voltage_sensor_->publish_state(v / 10.0);
        if (current_sensor_) current_sensor_->publish_state(a / 100.0);
        if (power_sensor_) power_sensor_->publish_state(p);
        if (battery_sensor_) battery_sensor_->publish_state(b / 10.0);
        
        buffer_.erase(buffer_.begin(), buffer_.begin() + i + 11);
        break;
      }
    }
    
    if (buffer_.size() > 512) buffer_.clear();
    
    if (millis() - last_request_ > 3000) {
      uint8_t cmd[] = {0xBE, 0x41, 0xAA};
      write_array(cmd, 3);
      last_request_ = millis();
    }
  }

  void set_voltage_sensor(sensor::Sensor *s) { voltage_sensor_ = s; }
  void set_current_sensor(sensor::Sensor *s) { current_sensor_ = s; }
  void set_power_sensor(sensor::Sensor *s) { power_sensor_ = s; }
  void set_battery_sensor(sensor::Sensor *s) { battery_sensor_ = s; }

 private:
  std::vector<uint8_t> buffer_;
  unsigned long last_request_ = 0;
  sensor::Sensor *voltage_sensor_ = nullptr;
  sensor::Sensor *current_sensor_ = nullptr;
  sensor::Sensor *power_sensor_ = nullptr;
  sensor::Sensor *battery_sensor_ = nullptr;
};
