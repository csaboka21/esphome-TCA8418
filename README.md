# ESPHome TCA8418 Keypad

External ESPHome component for the TI TCA8418 keypad controller.

## What this component does

- Supports keypad matrices from 1x1 up to 8x14.
- Reads key events from the TCA8418 event FIFO.
- Emits both press and release events through a single automation trigger.
- Optional keycode-to-character mapping.
- Optional interrupt-driven mode via INT pin.
- Optional debounce and long-press detection.

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
- keymap: map TCA8418 keycode to one character.
  - Key range: 1..80.
  - Value: single character string, or empty string to clear.
  - Example: 21: "F"

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
- The component can run in polling mode or interrupt mode.
- In interrupt mode, the loop only wakes when INT is asserted.
- Out-of-range key events are ignored and logged.

## Troubleshooting

- No key events:
  - Verify I2C wiring and address.
  - Enable I2C scan and confirm the device is found.
  - If using interrupt_pin, check INT wiring and pull-up behavior.
- Unexpected row/column values:
  - Verify rows and columns match your physical matrix wiring.
- Repeated or noisy events:
  - Increase debounce.
