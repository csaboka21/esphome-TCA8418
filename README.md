# ESPHome TCA8418 Keypad Component

This repository contains an ESPHome external component for the TI TCA8418 keypad controller.

## Features

- Configurable matrix size up to 8 rows x 14 columns
- Reads key press and key release events from the hardware event FIFO
- Optional character keymap translation per matrix position
- Optional IRQ-based event handling with interrupt_pin
- Optional debounce and long-press detection
- Single automation hook: on_key (press and release events)

## Folder Layout

- components/tca8418_keypad/__init__.py
- components/tca8418_keypad/tca8418_keypad.h
- components/tca8418_keypad/tca8418_keypad.cpp

## Configuration

- rows (required): Matrix row count, 1..8.
- columns (required): Matrix column count, 1..14.
- keymap (optional): Dictionary that maps TCA8418 keycode (1..80) to a char.
  - Format: `keycode: "c"`
  - Value must be a single character or empty string.
  - Example: `21: "F"`, `11: "\t"`, `1: "`"`
- interrupt_pin (optional): GPIO input connected to TCA8418 INT pin (active low). When set, polling checks are skipped until IRQ is asserted.
- debounce_ms (optional, default 0ms): Ignore repeated events for the same key within this window.
- long_press_ms (optional, default 0ms): Marks release event as long_press when key hold duration exceeds this threshold.

## Example ESPHome YAML

Use this component from your node configuration:

external_components:
  - source:
      type: local
      path: ./components

i2c:
  sda: GPIO8
  scl: GPIO9
  scan: true

tca8418_keypad:
  id: kb
  rows: 4
  columns: 14
  address: 0x34
  debounce_ms: 12ms
  long_press_ms: 500ms
  # interrupt_pin: GPIO10
  keymap:
    1: "`"
    2: "1"
    3: "2"
    4: "3"
    11: "\t"
    21: "F"
    22: "S"
    31: "C"
  on_key:
    - then:
        - lambda: |-
            ESP_LOGI(
              "kbd",
              "row=%u col=%u pressed=%s key=%c raw=%u long=%s",
              row,
              col,
              pressed ? "true" : "false",
              key_char ? static_cast<char>(key_char) : '?',
              keycode,
              long_press ? "true" : "false"
            );

## Callback Variables

- row: matrix row index.
- col: matrix column index.
- pressed: true for key press, false for key release.
- key_char: mapped char code from keymap, or 0 when unmapped.
- keycode: raw TCA8418 event code (1..80).
- long_press: true when a release event exceeded long_press_ms.

## Notes

- This project was created with the help of AI.
- Tested with M5Stack Cardputer Adv.
- The component can run in polling mode or interrupt mode.
- In interrupt mode, the loop only wakes when INT is asserted.
- Out-of-range key events are ignored and logged.

## Troubleshooting

- No key events:
  - Verify i2c SDA/SCL pin mapping and address 0x34.
  - Enable i2c scan and confirm TCA8418 appears.
  - If using interrupt_pin, ensure it is wired to INT and configured as input.
- Wrong row/col values:
  - Check rows and columns match your physical matrix.
  - Verify keymap keys are valid TCA8418 keycodes (1..80).
- Ghosting or duplicate presses:
  - Increase debounce_ms.
  - Confirm matrix diode/wiring characteristics.
  - Prefer interrupt_pin mode for cleaner event timing.

## Notes

- For Cardputer Advanced wiring, set i2c SDA/SCL pins to your board pinout.
- If your matrix uses fewer rows or columns, set rows and columns accordingly.
