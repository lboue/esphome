import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID, STATE_CLASS_MEASUREMENT
from esphome.types import ConfigType

from . import CONF_OPENTHERM_SLAVE_ID, OpenthermSlaveHub

CONF_CH_SETPOINT = "ch_setpoint"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_OPENTHERM_SLAVE_ID): cv.use_id(OpenthermSlaveHub),
    cv.Optional(CONF_CH_SETPOINT): sensor.sensor_schema(
        kind=sensor.TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT
    ),
}


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_OPENTHERM_SLAVE_ID])
    sensors = sensor.sub_sensors(config)
    await sensors(CONF_CH_SETPOINT, hub.set_ch_setpoint_sensor)
