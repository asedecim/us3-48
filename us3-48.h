#pragma once
#include "esphome.h"
#include <vector>

class UPSComponent : public Component, public uart::UARTDevice {
 public:
  UPSComponent(uart::UARTComponent *parent) : UARTDevice(parent) {}

  void setup() override {
    ESP_LOGD("ups", "ИБП компонент запущен");
    last_request_ = millis();
  }

  void loop() override {
    // Чтение входящих данных
    while (available()) {
      uint8_t b;
      read_byte(&b);
      buffer_.push_back(b);
    }
    
    // Парсинг ответа на команду 0x41 (основные параметры)
    for (int i = 0; i < (int)buffer_.size() - 10; i++) {
      if (buffer_[i] == 0xBE && buffer_[i+1] == 0xC1 && buffer_[i+10] == 0xAA) {
        uint16_t raw_urms = (buffer_[i+2] << 8) | buffer_[i+3];
        uint16_t raw_irms = (buffer_[i+4] << 8) | buffer_[i+5];
        uint16_t raw_pout = (buffer_[i+6] << 8) | buffer_[i+7];
        uint16_t raw_ubat = (buffer_[i+8] << 8) | buffer_[i+9];
        
        if (выходное_напряжение_) выходное_напряжение_->publish_state(raw_urms / 10.0);
        if (выходной_ток_) выходной_ток_->publish_state(raw_irms / 100.0);
        if (полная_мощность_) полная_мощность_->publish_state(raw_pout);
        if (напряжение_батареи_) напряжение_батареи_->publish_state(raw_ubat / 10.0);
        
        buffer_.erase(buffer_.begin(), buffer_.begin() + i + 11);
        break;
      }
    }
    
    // Парсинг ответа на команду 0x4A (статус)
    for (int i = 0; i < (int)buffer_.size() - 5; i++) {
      if (buffer_[i] == 0xBE && buffer_[i+1] == 0xCA && buffer_[i+5] == 0xAA) {
        uint8_t status1 = buffer_[i+2];
        uint8_t status2 = buffer_[i+3];
        uint8_t status3 = buffer_[i+4];
        
        if (режим_ведомый_) режим_ведомый_->publish_state(status1 & 0x01);
        if (режим_3ф_ведомый_) режим_3ф_ведомый_->publish_state(status1 & 0x02);
        if (идёт_генерация_) идёт_генерация_->publish_state(status1 & 0x04);
        if (перегрев_) перегрев_->publish_state(status1 & 0x08);
        if (батарея_в_норме_) батарея_в_норме_->publish_state(status1 & 0x10);
        if (фатальная_ошибка_) фатальная_ошибка_->publish_state(status1 & 0x20);
        if (пропуск_сети_) пропуск_сети_->publish_state(status1 & 0x40);
        if (питание_от_генератора_) питание_от_генератора_->publish_state(status1 & 0x80);
        
        if (светодиод_статус1_) светодиод_статус1_->publish_state(status2 & 0x01);
        if (светодиод_статус2_) светодиод_статус2_->publish_state(status2 & 0x02);
        if (светодиод_статус3_) светодиод_статус3_->publish_state(status2 & 0x04);
        if (светодиод_перегрузка_) светодиод_перегрузка_->publish_state(status2 & 0x08);
        if (светодиод_батарея_низко_) светодиод_батарея_низко_->publish_state(status2 & 0x10);
        if (светодиод_bulk_) светодиод_bulk_->publish_state(status2 & 0x20);
        if (светодиод_абсорбция_) светодиод_абсорбция_->publish_state(status2 & 0x40);
        if (светодиод_float_) светодиод_float_->publish_state(status2 & 0x80);
        
        uint8_t генератор_состояние = status3 & 0x03;
        if (генератор_состояние_ == 0) {
          if (генератор_выключен_) генератор_выключен_->publish_state(true);
        } else if (генератор_состояние == 1) {
          if (генератор_запуск_) генератор_запуск_->publish_state(true);
        } else if (генератор_состояние == 2) {
          if (генератор_работает_) генератор_работает_->publish_state(true);
        } else if (генератор_состояние == 3) {
          if (генератор_останов_) генератор_останов_->publish_state(true);
        }
        
        if (ошибка_запуска_генератора_) ошибка_запуска_генератора_->publish_state(status3 & 0x04);
        
        buffer_.erase(buffer_.begin(), buffer_.begin() + i + 6);
        break;
      }
    }
    
    // Парсинг ответа на команду 0x47 (внутренняя температура)
    for (int i = 0; i < (int)buffer_.size() - 4; i++) {
      if (buffer_[i] == 0xBE && buffer_[i+1] == 0xC7 && buffer_[i+4] == 0xAA) {
        uint16_t raw_temp = (buffer_[i+2] << 8) | buffer_[i+3];
        // Код = (0.75+0.01*(T-25))/2.5*1024
        // T = (код * 2.5 / 1024 - 0.75) / 0.01 + 25
        float temp = (raw_temp * 2.5 / 1024.0 - 0.75) / 0.01 + 25.0;
        if (внутренняя_температура_) внутренняя_температура_->publish_state(temp);
        
        buffer_.erase(buffer_.begin(), buffer_.begin() + i + 5);
        break;
      }
    }
    
    // Парсинг ответа на команду 0x77 (температура батарей)
    for (int i = 0; i < (int)buffer_.size() - 4; i++) {
      if (buffer_[i] == 0xBE && buffer_[i+1] == 0xF7 && buffer_[i+4] == 0xAA) {
        uint8_t temp = buffer_[i+2];
        if (температура_батарей_) температура_батарей_->publish_state(temp);
        
        buffer_.erase(buffer_.begin(), buffer_.begin() + i + 5);
        break;
      }
    }
    
    if (buffer_.size() > 1024) buffer_.clear();
    
    // Отправка команд по расписанию
    unsigned long now = millis();
    
    if (now - last_request_a_ > 3000) {
      uint8_t cmd[] = {0xBE, 0x41, 0xAA};  // Команда A
      write_array(cmd, 3);
      last_request_a_ = now;
    }
    
    if (now - last_request_j_ > 10000) {
      uint8_t cmd[] = {0xBE, 0x4A, 0xAA};  // Команда J
      write_array(cmd, 3);
      last_request_j_ = now;
    }
    
    if (now - last_request_g_ > 30000) {
      uint8_t cmd[] = {0xBE, 0x47, 0xAA};  // Команда G
      write_array(cmd, 3);
      last_request_g_ = now;
    }
    
    if (now - last_request_tempbat_ > 60000) {
      uint8_t cmd[] = {0xBE, 0x77, 0xAA};  // Команда 0x77
      write_array(cmd, 3);
      last_request_tempbat_ = now;
    }
  }

  // Сенсоры (все переменные из документации, названия на кириллице)
  void set_выходное_напряжение(sensor::Sensor *s) { выходное_напряжение_ = s; }
  void set_выходной_ток(sensor::Sensor *s) { выходной_ток_ = s; }
  void set_полная_мощность(sensor::Sensor *s) { полная_мощность_ = s; }
  void set_напряжение_батареи(sensor::Sensor *s) { напряжение_батареи_ = s; }
  void set_внутренняя_температура(sensor::Sensor *s) { внутренняя_температура_ = s; }
  void set_температура_батарей(sensor::Sensor *s) { температура_батарей_ = s; }
  
  // Бинарные сенсоры статуса
  void set_режим_ведомый(binary_sensor::BinarySensor *s) { режим_ведомый_ = s; }
  void set_режим_3ф_ведомый(binary_sensor::BinarySensor *s) { режим_3ф_ведомый_ = s; }
  void set_идёт_генерация(binary_sensor::BinarySensor *s) { идёт_генерация_ = s; }
  void set_перегрев(binary_sensor::BinarySensor *s) { перегрев_ = s; }
  void set_батарея_в_норме(binary_sensor::BinarySensor *s) { батарея_в_норме_ = s; }
  void set_фатальная_ошибка(binary_sensor::BinarySensor *s) { фатальная_ошибка_ = s; }
  void set_пропуск_сети(binary_sensor::BinarySensor *s) { пропуск_сети_ = s; }
  void set_питание_от_генератора(binary_sensor::BinarySensor *s) { питание_от_генератора_ = s; }
  
  void set_светодиод_статус1(binary_sensor::BinarySensor *s) { светодиод_статус1_ = s; }
  void set_светодиод_статус2(binary_sensor::BinarySensor *s) { светодиод_статус2_ = s; }
  void set_светодиод_статус3(binary_sensor::BinarySensor *s) { светодиод_статус3_ = s; }
  void set_светодиод_перегрузка(binary_sensor::BinarySensor *s) { светодиод_перегрузка_ = s; }
  void set_светодиод_батарея_низко(binary_sensor::BinarySensor *s) { светодиод_батарея_низко_ = s; }
  void set_светодиод_bulk(binary_sensor::BinarySensor *s) { светодиод_bulk_ = s; }
  void set_светодиод_абсорбция(binary_sensor::BinarySensor *s) { светодиод_абсорбция_ = s; }
  void set_светодиод_float(binary_sensor::BinarySensor *s) { светодиод_float_ = s; }
  
  void set_генератор_выключен(binary_sensor::BinarySensor *s) { генератор_выключен_ = s; }
  void set_генератор_запуск(binary_sensor::BinarySensor *s) { генератор_запуск_ = s; }
  void set_генератор_работает(binary_sensor::BinarySensor *s) { генератор_работает_ = s; }
  void set_генератор_останов(binary_sensor::BinarySensor *s) { генератор_останов_ = s; }
  void set_ошибка_запуска_генератора(binary_sensor::BinarySensor *s) { ошибка_запуска_генератора_ = s; }

 private:
  std::vector<uint8_t> buffer_;
  unsigned long last_request_a_ = 0;
  unsigned long last_request_j_ = 0;
  unsigned long last_request_g_ = 0;
  unsigned long last_request_tempbat_ = 0;
  
  sensor::Sensor *выходное_напряжение_ = nullptr;
  sensor::Sensor *выходной_ток_ = nullptr;
  sensor::Sensor *полная_мощность_ = nullptr;
  sensor::Sensor *напряжение_батареи_ = nullptr;
  sensor::Sensor *внутренняя_температура_ = nullptr;
  sensor::Sensor *температура_батарей_ = nullptr;
  
  binary_sensor::BinarySensor *режим_ведомый_ = nullptr;
  binary_sensor::BinarySensor *режим_3ф_ведомый_ = nullptr;
  binary_sensor::BinarySensor *идёт_генерация_ = nullptr;
  binary_sensor::BinarySensor *перегрев_ = nullptr;
  binary_sensor::BinarySensor *батарея_в_норме_ = nullptr;
  binary_sensor::BinarySensor *фатальная_ошибка_ = nullptr;
  binary_sensor::BinarySensor *пропуск_сети_ = nullptr;
  binary_sensor::BinarySensor *питание_от_генератора_ = nullptr;
  
  binary_sensor::BinarySensor *светодиод_статус1_ = nullptr;
  binary_sensor::BinarySensor *светодиод_статус2_ = nullptr;
  binary_sensor::BinarySensor *светодиод_статус3_ = nullptr;
  binary_sensor::BinarySensor *светодиод_перегрузка_ = nullptr;
  binary_sensor::BinarySensor *светодиод_батарея_низко_ = nullptr;
  binary_sensor::BinarySensor *светодиод_bulk_ = nullptr;
  binary_sensor::BinarySensor *светодиод_абсорбция_ = nullptr;
  binary_sensor::BinarySensor *светодиод_float_ = nullptr;
  
  binary_sensor::BinarySensor *генератор_выключен_ = nullptr;
  binary_sensor::BinarySensor *генератор_запуск_ = nullptr;
  binary_sensor::BinarySensor *генератор_работает_ = nullptr;
  binary_sensor::BinarySensor *генератор_останов_ = nullptr;
  binary_sensor::BinarySensor *ошибка_запуска_генератора_ = nullptr;
};
