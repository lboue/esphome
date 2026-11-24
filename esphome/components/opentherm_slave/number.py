import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_ID, UNIT_PERCENT
from esphome.types import ConfigType

from . import CONF_OPENTHERM_SLAVE_ID, OpenthermSlaveHub

CONF_RELATIVE_MODULATION_LEVEL = "relative_modulation_level"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_OPENTHERM_SLAVE_ID): cv.use_id(OpenthermSlaveHub),
    cv.Optional(CONF_RELATIVE_MODULATION_LEVEL): number.number_schema(
        number.ApplyNumber, unit_of_measurement=UNIT_PERCENT, icon="mdi:percent"
    ),
}


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_OPENTHERM_SLAVE_ID])
    numbers = number.sub_numbers(config, parent=hub)
    await numbers(
        CONF_RELATIVE_MODULATION_LEVEL,
        None,
        min_value=0,
        max_value=100,
        step=1,
        apply="set_modulation_level(value)",
        publish="after",
    )
