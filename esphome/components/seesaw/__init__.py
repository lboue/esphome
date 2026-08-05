from esphome import pins
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_INPUT,
    CONF_INVERTED,
    CONF_MODE,
    CONF_NUMBER,
    CONF_OUTPUT,
    CONF_PULLUP,
)

CODEOWNERS = ["@lboue"]
DEPENDENCIES = ["i2c"]
MULTI_CONF = True

CONF_SEESAW = "seesaw"
CONF_SEESAW_ID = "seesaw_id"

seesaw_ns = cg.esphome_ns.namespace("seesaw")
Seesaw = seesaw_ns.class_("Seesaw", i2c.I2CDevice, cg.Component)
SeesawGPIOPin = seesaw_ns.class_("SeesawGPIOPin", cg.GPIOPin)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Seesaw),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x49))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)


def validate_mode(value):
    if not (value[CONF_INPUT] or value[CONF_OUTPUT]):
        raise cv.Invalid("Mode must be either input or output")
    if value[CONF_INPUT] and value[CONF_OUTPUT]:
        raise cv.Invalid("Mode must be either input or output")
    if value[CONF_PULLUP] and not value[CONF_INPUT]:
        raise cv.Invalid("Pullup only available with input")
    return value


SEESAW_PIN_SCHEMA = pins.gpio_base_schema(
    SeesawGPIOPin,
    cv.int_range(min=0, max=63),
    modes=[CONF_INPUT, CONF_OUTPUT, CONF_PULLUP],
    mode_validator=validate_mode,
).extend(
    {
        cv.Required(CONF_SEESAW): cv.use_id(Seesaw),
    }
)


@pins.PIN_SCHEMA_REGISTRY.register(CONF_SEESAW, SEESAW_PIN_SCHEMA)
async def seesaw_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_SEESAW])

    cg.add(var.set_parent(parent))
    cg.add(var.set_pin(config[CONF_NUMBER]))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config[CONF_MODE])))
    return var
