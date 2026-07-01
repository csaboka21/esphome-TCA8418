# ESPHome TCA8418 Keypad

External ESPHome component for the TI TCA8418 keypad controller.

## What this component does

- Supports keypad matrices from 1x1 up to 8x14.
- Reads key events from the TCA8418 event FIFO.
- Emits both press and release events through a single automation trigger.
- Optional keycode-to-character mapping.
- Optional interrupt-driven mode via INT pin.
- Optional debounce and long-press detection.
- Optional binary sensor entities per key, by raw keycode or row/column position.
- LVGL keypad integration through mapped binary sensors.

## Repository layout

- components/tca8418_keypad/__init__.py
- components/tca8418_keypad/tca8418_keypad.h
- components/tca8418_keypad/tca8418_keypad.cpp

## Configuration

### Required

- rows: keypad row count, range 1..8.
- columns: keypad column count, range 1..14.

### Optional

- address: I2C address (default 0x34).
- interrupt_pin: GPIO input connected to TCA8418 INT (active low).
- debounce: ignore repeated key events in this window (default 12ms).
- long_press: sets long_press=true on release when held long enough (default 500ms).
- debounce_ms / long_press_ms: deprecated aliases still accepted for compatibility.
- keymap: map TCA8418 keycode to one character.
  - Key range: 1..80.
  - Value: single character string, or empty string to clear.
  - Example: 21: "F"

## Binary sensor platform

You can expose individual key states as `binary_sensor` entities.

- platform: `tca8418_keypad`
- keypad_id (required): ID of your keypad component.
- keycode (optional): Raw TCA8418 keycode (1..80).
- row and col (optional): Matrix position (row 0..7, col 0..13).

Provide either `keycode`, or `row` + `col`.

## ESPHome example

```yaml
external_components:
  - source: github://csaboka21/esphome-TCA8418@main
    components: [tca8418_keypad]

i2c:
  sda: GPIO8
  scl: GPIO9
  scan: true

tca8418_keypad:
  id: keypad
  rows: 4
  columns: 14
  address: 0x34
  # interrupt_pin: GPIO10
  debounce: 12ms
  long_press: 500ms
  keymap:
    1: "Q"
    2: "W"
    3: "E"
    11: "A"
    12: "S"
    13: "D"
  on_key:
    - then:
        - lambda: |-
            ESP_LOGI(
              "kbd",
              "row=%u col=%u pressed=%s char=%c keycode=%u long=%s",
              row,
              col,
              pressed ? "true" : "false",
              key_char ? static_cast<char>(key_char) : '?',
              keycode,
              long_press ? "true" : "false"
            );

binary_sensor:
  - platform: tca8418_keypad
    keypad_id: keypad
    id: kb_up
    keycode: 12

  - platform: tca8418_keypad
    keypad_id: keypad
    id: kb_down
    keycode: 22

  - platform: tca8418_keypad
    keypad_id: keypad
    id: kb_enter
    keycode: 32

lvgl:
  keypads:
    - up: kb_up
      down: kb_down
      enter: kb_enter
```

## on_key trigger variables

- row: decoded matrix row index.
- col: decoded matrix column index.
- pressed: true on press, false on release.
- key_char: mapped character code, or 0 when unmapped.
- keycode: raw TCA8418 keycode (1..80).
- long_press: true only for release events that exceeded configured long_press.

## Notes

- This project was created with the help of AI.
- Tested with M5Stack Cardputer Adv.
- The component can run in polling mode or interrupt mode.
- In interrupt mode, the loop only wakes when INT is asserted.
- Out-of-range key events are ignored and logged.

## LVGL Integration Notes

- LVGL `keypads` consumes binary sensor IDs.
- Map your physical keys to logical LVGL keys (for example `up`, `down`, `left`, `right`, `enter`, `esc`).
- For 3-button UIs, consider LVGL `encoders` mode with `left_button`, `right_button`, and `enter_button`.

## Troubleshooting

- No key events:
  - Verify I2C wiring and address.
  - Enable I2C scan and confirm the device is found.
  - If using interrupt_pin, check INT wiring and pull-up behavior.
- Unexpected row/column values:
  - Verify rows and columns match your physical matrix wiring.
  - Verify keymap keys are valid TCA8418 keycodes (1..80).
- Repeated or noisy events:
  - Increase debounce.
  - Confirm matrix diode/wiring characteristics.
  - Prefer interrupt_pin mode for cleaner event timing.
