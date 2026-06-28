#pragma once

#include <string>
#include <vector>

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace tca8418_keypad {

class TCA8418KeyEventTrigger
  : public Trigger<uint8_t, uint8_t, bool, uint8_t, uint8_t, bool> {};

class TCA8418Keypad : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_dimensions(uint8_t rows, uint8_t columns) {
    this->rows_ = rows;
    this->columns_ = columns;
  }

  void set_keymap_entries(const std::vector<int> &keycodes, const std::vector<int> &chars) {
    this->keymap_by_code_.assign(81, 0);
    const size_t count = keycodes.size() < chars.size() ? keycodes.size() : chars.size();
    for (size_t i = 0; i < count; i++) {
      const int code = keycodes[i];
      if (code >= 1 && code <= 80) {
        this->keymap_by_code_[static_cast<size_t>(code)] = static_cast<char>(chars[i]);
      }
    }
  }

  void set_interrupt_pin(InternalGPIOPin *interrupt_pin) { this->interrupt_pin_ = interrupt_pin; }

  void set_debounce_ms(uint32_t debounce_ms) { this->debounce_ms_ = debounce_ms; }

  void set_long_press_ms(uint32_t long_press_ms) { this->long_press_ms_ = long_press_ms; }

  void add_on_key_event_trigger(TCA8418KeyEventTrigger *trigger) {
    this->key_event_triggers_.push_back(trigger);
  }

  void setup() override;
  void update() override;
  void dump_config() override;

 protected:
  bool configure_keypad_();
  void handle_event_(uint8_t event);
  char lookup_key_(uint8_t keycode, uint8_t row, uint8_t col) const;

  uint8_t rows_{0};
  uint8_t columns_{0};
  InternalGPIOPin *interrupt_pin_{nullptr};
  uint32_t debounce_ms_{0};
  uint32_t long_press_ms_{0};
  std::vector<char> keymap_by_code_;
  std::vector<TCA8418KeyEventTrigger *> key_event_triggers_;
  std::vector<bool> key_pressed_state_;
  std::vector<uint32_t> key_last_event_ms_;
  std::vector<uint32_t> key_press_start_ms_;
};

}  // namespace tca8418_keypad
}  // namespace esphome
