import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_CHANNEL, ICON_ROTATE_RIGHT, STATE_CLASS_MEASUREMENT

from .. import CONF_SEESAW_ID, Seesaw, seesaw_ns

SeesawEncoderSensor = seesaw_ns.class_(
    "SeesawEncoderSensor", sensor.Sensor, cg.PollingComponent
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SeesawEncoderSensor),
            cv.GenerateID(CONF_SEESAW_ID): cv.use_id(Seesaw),
            # Bounded to 4 -- no current Adafruit Seesaw board exposes more encoders.
            cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=3),
        }
    )
    .extend(
        sensor.sensor_schema(
            SeesawEncoderSensor,
            icon=ICON_ROTATE_RIGHT,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        )
    )
    .extend(cv.polling_component_schema("100ms"))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_SEESAW_ID])
    cg.add(var.set_channel(config[CONF_CHANNEL]))
