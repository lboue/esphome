import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_NUM_LEDS, CONF_OUTPUT_ID, CONF_PIN

from .. import CONF_SEESAW_ID, Seesaw, seesaw_ns

SeesawLightOutput = seesaw_ns.class_("SeesawLightOutput", light.AddressableLight)

CONFIG_SCHEMA = light.ADDRESSABLE_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_SEESAW_ID): cv.use_id(Seesaw),
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(SeesawLightOutput),
        # The Seesaw GPIO pin the NeoPixel data line is wired to on-chip -- board-specific,
        # e.g. 18 on the Adafruit Quad Rotary Encoder Breakout.
        cv.Required(CONF_PIN): cv.int_range(min=0, max=63),
        cv.Required(CONF_NUM_LEDS): cv.int_range(min=1, max=64),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_SEESAW_ID])
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(hub))
    cg.add(var.set_pin(config[CONF_PIN]))
    cg.add(var.set_num_leds(config[CONF_NUM_LEDS]))
