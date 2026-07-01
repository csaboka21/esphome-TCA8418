import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_COL, CONF_ID, CONF_ROW

from .. import TCA8418Keypad, tca8418_keypad_ns

DEPENDENCIES = ["tca8418_keypad"]

CONF_KEYPAD_ID = "keypad_id"
CONF_KEYCODE = "keycode"

TCA8418KeypadBinarySensor = tca8418_keypad_ns.class_(
    "TCA8418KeypadBinarySensor", binary_sensor.BinarySensor
)


def _validate_button(config):
    has_position = CONF_ROW in config or CONF_COL in config
    has_keycode = CONF_KEYCODE in config

    if has_position and has_keycode:
        raise cv.Invalid("You can't provide both keycode and a row/col position")

    if has_position:
        if CONF_ROW not in config:
            raise cv.Invalid("Missing row")
        if CONF_COL not in config:
            raise cv.Invalid("Missing col")
    elif not has_keycode:
        raise cv.Invalid("Missing keycode or row/col position")

    return config


CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(TCA8418KeypadBinarySensor).extend(
        {
            cv.GenerateID(CONF_KEYPAD_ID): cv.use_id(TCA8418Keypad),
            cv.Optional(CONF_ROW): cv.int_range(min=0, max=7),
            cv.Optional(CONF_COL): cv.int_range(min=0, max=13),
            cv.Optional(CONF_KEYCODE): cv.int_range(min=1, max=80),
        }
    ),
    _validate_button,
)


async def to_code(config):
    if CONF_KEYCODE in config:
        var = cg.new_Pvariable(config[CONF_ID], config[CONF_KEYCODE])
    else:
        var = cg.new_Pvariable(config[CONF_ID], config[CONF_ROW], config[CONF_COL])

    await binary_sensor.register_binary_sensor(var, config)

    keypad = await cg.get_variable(config[CONF_KEYPAD_ID])
    cg.add(keypad.register_listener(var))
