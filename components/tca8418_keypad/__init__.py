import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import i2c
from esphome import pins

DEPENDENCIES = ["i2c"]

CONF_ROWS = "rows"
CONF_COLUMNS = "columns"
CONF_KEYMAP = "keymap"
CONF_INTERRUPT_PIN = "interrupt_pin"
CONF_DEBOUNCE_MS = "debounce_ms"
CONF_LONG_PRESS_MS = "long_press_ms"
CONF_ON_KEY_PRESS = "on_key_press"
CONF_ON_KEY_RELEASE = "on_key_release"
CONF_ON_KEY = "on_key"
CONF_TRIGGER_ID = "trigger_id"
CONF_ID = "id"


tca8418_keypad_ns = cg.esphome_ns.namespace("tca8418_keypad")
TCA8418Keypad = tca8418_keypad_ns.class_(
    "TCA8418Keypad", cg.PollingComponent, i2c.I2CDevice
)
TCA8418KeyEventTrigger = tca8418_keypad_ns.class_(
    "TCA8418KeyEventTrigger",
    automation.Trigger.template(cg.uint8, cg.uint8, cg.bool_, cg.uint8, cg.uint8, cg.bool_),
)
TCA8418KeyPressTrigger = tca8418_keypad_ns.class_(
    "TCA8418KeyPressTrigger",
    automation.Trigger.template(cg.uint8, cg.uint8, cg.uint8, cg.uint8),
)
TCA8418KeyReleaseTrigger = tca8418_keypad_ns.class_(
    "TCA8418KeyReleaseTrigger",
    automation.Trigger.template(cg.uint8, cg.uint8, cg.uint8, cg.uint8, cg.bool_),
)


def _validate_keymap(config):
    keymap = config.get(CONF_KEYMAP)
    if keymap is None:
        return config

    if len(keymap) == 0:
        raise cv.Invalid("keymap must contain at least one keycode->char entry")

    for keycode, label in keymap.items():
        if len(label) > 1:
            raise cv.Invalid(
                f"keymap value for keycode {keycode} must be a single character or empty"
            )

    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TCA8418Keypad),
            cv.Required(CONF_ROWS): cv.int_range(min=1, max=8),
            cv.Required(CONF_COLUMNS): cv.int_range(min=1, max=14),
            cv.Optional(CONF_KEYMAP): cv.Schema(
                {cv.int_range(min=1, max=80): cv.string_strict}
            ),
            cv.Optional(CONF_INTERRUPT_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_DEBOUNCE_MS, default="0ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_LONG_PRESS_MS, default="0ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_ON_KEY): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TCA8418KeyEventTrigger),
                }
            ),
            cv.Optional(CONF_ON_KEY_PRESS): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TCA8418KeyPressTrigger),
                }
            ),
            cv.Optional(CONF_ON_KEY_RELEASE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TCA8418KeyReleaseTrigger),
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("10ms"))
    .extend(i2c.i2c_device_schema(0x34)),
    _validate_keymap,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_dimensions(config[CONF_ROWS], config[CONF_COLUMNS]))
    cg.add(var.set_debounce_ms(config[CONF_DEBOUNCE_MS].total_milliseconds))
    cg.add(var.set_long_press_ms(config[CONF_LONG_PRESS_MS].total_milliseconds))

    if CONF_INTERRUPT_PIN in config:
        interrupt_pin = await cg.gpio_pin_expression(config[CONF_INTERRUPT_PIN])
        cg.add(var.set_interrupt_pin(interrupt_pin))

    if CONF_KEYMAP in config:
        sorted_items = sorted(config[CONF_KEYMAP].items())
        keycodes = [item[0] for item in sorted_items]
        chars = [ord(item[1]) if len(item[1]) == 1 else 0 for item in sorted_items]
        cg.add(var.set_keymap_entries(keycodes, chars))

    for conf in config.get(CONF_ON_KEY, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.add_on_key_event_trigger(trigger))
        await automation.build_automation(
            trigger,
            [
                (cg.uint8, "row"),
                (cg.uint8, "col"),
                (cg.bool_, "pressed"),
                (cg.uint8, "key_char"),
                (cg.uint8, "keycode"),
                (cg.bool_, "long_press"),
            ],
            conf,
        )

    for conf in config.get(CONF_ON_KEY_PRESS, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.add_on_key_press_trigger(trigger))
        await automation.build_automation(
            trigger,
            [
                (cg.uint8, "row"),
                (cg.uint8, "col"),
                (cg.uint8, "key_char"),
                (cg.uint8, "keycode"),
            ],
            conf,
        )

    for conf in config.get(CONF_ON_KEY_RELEASE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.add_on_key_release_trigger(trigger))
        await automation.build_automation(
            trigger,
            [
                (cg.uint8, "row"),
                (cg.uint8, "col"),
                (cg.uint8, "key_char"),
                (cg.uint8, "keycode"),
                (cg.bool_, "long_press"),
            ],
            conf,
        )
