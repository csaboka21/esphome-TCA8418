#include "tca8418_keypad.h"

#include <algorithm>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tca8418_keypad {

static const char *const TAG = "tca8418_keypad";

static const uint8_t REG_CFG = 0x01;
static const uint8_t REG_INT_STAT = 0x02;
static const uint8_t REG_KEY_LCK_EC = 0x03;
static const uint8_t REG_KEY_EVENT_A = 0x04;
static const uint8_t REG_KP_GPIO1 = 0x1D;
static const uint8_t REG_KP_GPIO2 = 0x1E;
static const uint8_t REG_KP_GPIO3 = 0x1F;

void TCA8418Keypad::setup() {
  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
    this->interrupt_pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
    this->disable_loop();
  }

  const size_t key_slots = static_cast<size_t>(this->rows_) * this->columns_;
  this->key_pressed_state_.assign(key_slots, false);
  this->key_last_event_ms_.assign(key_slots, 0);
  this->key_press_start_ms_.assign(key_slots, 0);

  if (!this->configure_keypad_()) {
    ESP_LOGE(TAG, "Failed to configure TCA8418 keypad controller");
    this->mark_failed();
    return;
  }

  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->attach_interrupt(&TCA8418Keypad::gpio_intr_, this, gpio::INTERRUPT_FALLING_EDGE);
    // INT is active-low/open-drain; if already low, no falling edge will occur.
    if (!this->interrupt_pin_->digital_read()) {
      this->interrupt_pending_ = true;
      this->enable_loop();
    }
    ESP_LOGI(TAG, "Interrupt enabled on pin %d", this->interrupt_pin_->get_pin());
  }
}

void TCA8418Keypad::loop() {
  if (this->interrupt_pin_ != nullptr) {
    if (!this->interrupt_pending_) {
      return;
    }
    ESP_LOGI(TAG, "Interrupt on pin %d", this->interrupt_pin_->get_pin());
    this->process_events_();
    if (!this->interrupt_pending_) {
      this->disable_loop();
    }
    return;
  }
  // Polling mode: check for queued events each loop iteration.
  this->process_events_();
}


void TCA8418Keypad::process_events_() {
  this->interrupt_pending_ = false;

  uint8_t int_stat = 0;
  if (!this->read_byte(REG_INT_STAT, &int_stat)) {
    ESP_LOGW(TAG, "Unable to read interrupt status");
    this->status_set_warning();
    return;
  }

  if ((int_stat & 0x01) == 0) {
    return;
  }

  uint8_t key_lck_ec = 0;
  if (!this->read_byte(REG_KEY_LCK_EC, &key_lck_ec)) {
    ESP_LOGW(TAG, "Unable to read key event count");
    this->status_set_warning();
    return;
  }

  const uint8_t event_count = key_lck_ec & 0x0F;
  const uint8_t read_count = std::min<uint8_t>(event_count, 10);

  for (uint8_t i = 0; i < read_count; i++) {
    uint8_t event = 0;
    if (!this->read_byte(REG_KEY_EVENT_A + i, &event)) {
      ESP_LOGW(TAG, "Unable to read key event %u", i);
      this->status_set_warning();
      break;
    }

    if ((event & 0x7F) == 0) {
      continue;
    }

    this->handle_event_(event);
  }

  if (!this->write_byte(REG_INT_STAT, 0x01)) {
    ESP_LOGW(TAG, "Unable to clear key event interrupt");
    this->status_set_warning();
  }
}

void IRAM_ATTR TCA8418Keypad::gpio_intr_(TCA8418Keypad *self) {
  self->interrupt_pending_ = true;
  if (self->interrupt_pin_ != nullptr) {
    self->enable_loop_soon_any_context();
  }
}

void TCA8418Keypad::dump_config() {
  ESP_LOGCONFIG(TAG, "TCA8418 Keypad:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Rows: %u", this->rows_);
  ESP_LOGCONFIG(TAG, "  Columns: %u", this->columns_);
  if (this->interrupt_pin_ != nullptr) {
    LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
  }
  ESP_LOGCONFIG(TAG, "  Debounce: %u ms", this->debounce_ms_);
  ESP_LOGCONFIG(TAG, "  Long Press Threshold: %u ms", this->long_press_ms_);
  const bool has_keymap = std::any_of(this->keymap_by_code_.begin(), this->keymap_by_code_.end(),
                                      [](char c) { return c != 0; });
  const char *matrix_mode = "none";
  if (has_keymap) {
    matrix_mode = "keycode_map";
  }
  ESP_LOGCONFIG(TAG, "  Matrix Mode: %s", matrix_mode);
}

bool TCA8418Keypad::configure_keypad_() {
  if (this->rows_ == 0 || this->columns_ == 0) {
    ESP_LOGE(TAG, "rows and columns must be configured");
    return false;
  }

  const uint8_t kp_gpio1 = static_cast<uint8_t>((1U << this->rows_) - 1U);
  const uint8_t kp_gpio2 =
      this->columns_ <= 8 ? static_cast<uint8_t>((1U << this->columns_) - 1U) : 0xFF;
  const uint8_t kp_gpio3 =
      this->columns_ > 8 ? static_cast<uint8_t>((1U << (this->columns_ - 8U)) - 1U) : 0x00;

  if (!this->write_byte(REG_KP_GPIO1, kp_gpio1)) {
    return false;
  }
  if (!this->write_byte(REG_KP_GPIO2, kp_gpio2)) {
    return false;
  }
  if (!this->write_byte(REG_KP_GPIO3, kp_gpio3)) {
    return false;
  }

  // Enable auto-increment and keypad event interrupts.
  if (!this->write_byte(REG_CFG, 0x81)) {
    return false;
  }

  // Clear any stale pending interrupts before normal operation.
  if (!this->write_byte(REG_INT_STAT, 0x0F)) {
    return false;
  }

  return true;
}

void TCA8418Keypad::handle_event_(uint8_t event) {
  const uint32_t now = millis();
  const bool pressed = (event & 0x80) != 0;
  const uint8_t keycode = event & 0x7F;

  if (keycode == 0) {
    return;
  }

  const uint8_t key_index = keycode - 1;
  const uint8_t row = key_index / 10;
  const uint8_t col = key_index % 10;

  if (row >= this->rows_ || col >= this->columns_) {
    ESP_LOGW(TAG, "Ignoring out-of-range key event code=%u row=%u col=%u", keycode, row, col);
    return;
  }

  const char key_char = this->lookup_key_(keycode, row, col);
  const size_t index = static_cast<size_t>(row) * this->columns_ + col;

  if (index < this->key_last_event_ms_.size() && this->debounce_ms_ > 0) {
    if ((now - this->key_last_event_ms_[index]) < this->debounce_ms_) {
      ESP_LOGV(TAG, "Ignoring debounced event row=%u col=%u", row, col);
      return;
    }
  }

  bool long_press = false;
  if (index < this->key_pressed_state_.size()) {
    if (pressed) {
      this->key_pressed_state_[index] = true;
      this->key_press_start_ms_[index] = now;
    } else {
      if (this->key_pressed_state_[index] && this->long_press_ms_ > 0) {
        long_press = (now - this->key_press_start_ms_[index]) >= this->long_press_ms_;
      }
      this->key_pressed_state_[index] = false;
    }

    this->key_last_event_ms_[index] = now;
  }

  ESP_LOGD(TAG, "Key event row=%u col=%u pressed=%s key=%c raw=%u long=%s", row, col,
           pressed ? "true" : "false", key_char, keycode,
           long_press ? "true" : "false");

  for (TCA8418KeyEventTrigger *trigger : this->key_event_triggers_) {
    trigger->trigger(row, col, pressed, key_char, keycode, long_press);
  }
}

char TCA8418Keypad::lookup_key_(uint8_t keycode, uint8_t row, uint8_t col) const {
  if (keycode < this->keymap_by_code_.size() && this->keymap_by_code_[keycode] != 0) {
    return this->keymap_by_code_[keycode];
  }

  return 0x00;
}
}  // namespace tca8418_keypad
}  // namespace esphome
