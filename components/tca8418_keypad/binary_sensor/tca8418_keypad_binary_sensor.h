#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/tca8418_keypad/tca8418_keypad.h"

namespace esphome {
namespace tca8418_keypad {

class TCA8418KeypadBinarySensor final : public TCA8418KeypadListener,
                                        public binary_sensor::BinarySensorInitiallyOff {
 public:
  explicit TCA8418KeypadBinarySensor(uint8_t keycode) : has_keycode_(true), keycode_(keycode) {}
  TCA8418KeypadBinarySensor(uint8_t row, uint8_t col) : has_keycode_(false), row_(row), col_(col) {}

  void keycode_pressed(uint8_t keycode) override {
    if (!this->has_keycode_) {
      return;
    }
    if (keycode == this->keycode_) {
      this->publish_state(true);
    }
  }

  void keycode_released(uint8_t keycode) override {
    if (!this->has_keycode_) {
      return;
    }
    if (keycode == this->keycode_) {
      this->publish_state(false);
    }
  }

  void button_pressed(uint8_t row, uint8_t col) override {
    if (this->has_keycode_) {
      return;
    }
    if (row == this->row_ && col == this->col_) {
      this->publish_state(true);
    }
  }

  void button_released(uint8_t row, uint8_t col) override {
    if (this->has_keycode_) {
      return;
    }
    if (row == this->row_ && col == this->col_) {
      this->publish_state(false);
    }
  }

 protected:
  bool has_keycode_;
  uint8_t keycode_{0};
  uint8_t row_{0};
  uint8_t col_{0};
};

}  // namespace tca8418_keypad
}  // namespace esphome
