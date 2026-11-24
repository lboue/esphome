from esphome import pins
import esphome.codegen as cg
from esphome.components.const import CONF_OUT_PIN
from esphome.components.esp32 import include_builtin_idf_component
import esphome.config_validation as cv
from esphome.const import CONF_ID, PLATFORM_ESP32
from esphome.types import ConfigType

CODEOWNERS = ["@TODO_GITHUB_HANDLE"]
DOMAIN = "opentherm_slave"
MULTI_CONF = False

CONF_IN_PIN = "in_pin"
CONF_OPENTHERM_SLAVE_ID = "opentherm_slave_id"

opentherm_slave_ns = cg.esphome_ns.namespace("opentherm_slave")
OpenthermSlaveHub = opentherm_slave_ns.class_("OpenthermSlaveHub", cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(OpenthermSlaveHub),
            cv.Required(CONF_IN_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_OUT_PIN): pins.internal_gpio_output_pin_schema,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on([PLATFORM_ESP32]),
)


async def to_code(config: ConfigType) -> None:
    include_builtin_idf_component("esp_driver_gptimer")

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    await cg.add_pin(var.set_in_pin, config[CONF_IN_PIN])
    await cg.add_pin(var.set_out_pin, config[CONF_OUT_PIN])
