# TODO

- [ ] Confirm and document the exact Cardputer Advanced 4x14 physical key position map (row/col -> legend).
- [ ] Replace example keymap with hardware-accurate Cardputer Advanced defaults.
- [x] Add optional `interrupt_pin` support so events are read on IRQ instead of polling-only.
- [x] Add optional `debounce_ms` and `long_press_ms` processing helpers in component logic.
- [x] Add optional `on_key_press` and `on_key_release` dedicated triggers (in addition to `on_key`).
- [x] Keep trigger surface centered on `on_key`, `on_key_press`, and `on_key_release`.
- [x] Add `dump_config()` output for key matrix mode (`keymap` vs fallback labels).
- [x] Add validation error messages with row/column index for malformed `keymap` rows.
- [x] Add an example that maps string `key` values to actions.
- [x] Add a minimal test YAML set for 4x14 and 8x10 schema validation cases.
- [x] Add troubleshooting section in README for no-key events, wrong row/col, and ghosting.
