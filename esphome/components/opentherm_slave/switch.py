import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.types import ConfigType

from . import CONF_OPENTHERM_SLAVE_ID, OpenthermSlaveHub

CONF_FAULT = "fault"
CONF_CH_ACTIVE = "ch_active"
CONF_DHW_ACTIVE = "dhw_active"
CONF_FLAME_ACTIVE = "flame_active"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
    cv.GenerateID(CONF_OPENTHERM_SLAVE_ID): cv.use_id(OpenthermSlaveHub),
    cv.Optional(CONF_FAULT): switch.switch_schema(switch.ApplySwitch, icon="mdi:alert"),
    cv.Optional(CONF_CH_ACTIVE): switch.switch_schema(
        switch.ApplySwitch, icon="mdi:radiator"
    ),
    cv.Optional(CONF_DHW_ACTIVE): switch.switch_schema(
        switch.ApplySwitch, icon="mdi:faucet"
    ),
    cv.Optional(CONF_FLAME_ACTIVE): switch.switch_schema(
        switch.ApplySwitch, icon="mdi:fire"
    ),
}


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_OPENTHERM_SLAVE_ID])
    switches = switch.sub_switches(config, parent=hub)
    await switches(CONF_FAULT, None, apply="set_fault(state)", publish="after")
    await switches(CONF_CH_ACTIVE, None, apply="set_ch_active(state)", publish="after")
    await switches(
        CONF_DHW_ACTIVE, None, apply="set_dhw_active(state)", publish="after"
    )
    await switches(
        CONF_FLAME_ACTIVE, None, apply="set_flame_active(state)", publish="after"
    )
