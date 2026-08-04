import esphome.codegen as cg
from esphome.components import i2c, pn532
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@OttoWinter", "@jesserockz"]
DEPENDENCIES = ["i2c"]
MULTI_CONF = True

m5stack_nfc_ns = cg.esphome_ns.namespace("m5stack_nfc")
M5StackNFCComponent = m5stack_nfc_ns.class_(
    "M5StackNFCComponent",
    cg.PollingComponent,
    i2c.I2CDevice,
)

CONFIG_SCHEMA = cv.All(
    pn532.PN532_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(M5StackNFCComponent),
        }
    ).extend(i2c.i2c_device_schema(0x50))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # cg.add_library("m5stack/M5Unit-NFC", "^0.1.0")
