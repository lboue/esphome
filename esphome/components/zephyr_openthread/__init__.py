import enum

from esphome import pins
import esphome.codegen as cg

# from esphome.components import gpio
from esphome.components.zephyr import (
    # zephyr_add_mcuboot_overlay,
    zephyr_add_overlay,
    zephyr_add_prj_conf,
)
import esphome.config_validation as cv

# from esphome.const import CONF_ID, CONF_ENABLE_IPV6, CONF_OTA, CONF_TRANSPORT, CONF_UDP
from esphome.const import CONF_ENABLE_IPV6, CONF_ID, CONF_OTA, CONF_UDP
from esphome.core import _LOGGER, CORE, coroutine_with_priority

from . import config_validation as ot_cv
from .const import (
    CONF_CHANNEL,
    CONF_FORCE_DATASET,
    CONF_NETWORK_KEY,
    CONF_NETWORK_NAME,
    CONF_PANID,
    CONF_PSKC,
    CONF_RADIO_TX_POWER,
    CONF_SHELL,
    CONF_TRANSPORT,
    CONF_XPANID,
    DEFAULT_CHANNEL,
    DEFAULT_FORCE_DATASET,
    DEFAULT_NETWORK_KEY,
    DEFAULT_NETWORK_NAME,
    DEFAULT_PANID,
    DEFAULT_PSKC,
    DEFAULT_RADIO_TX_POWER,
    DEFAULT_SHELL,
    DEFAULT_XPANID,
    zephyr_openthread_ns,
)

CODEOWNERS = ["@felipejfc"]
DEPENDENCIES = ["zephyr", "nrf52"]
AUTO_LOAD = ["mdns", "network"]

OpenThreadZephyr = zephyr_openthread_ns.class_("OpenThreadZephyr", cg.Component)
OpenThreadDNSComponent = zephyr_openthread_ns.class_(
    "OpenThreadDNSComponent", cg.Component
)

# Define new constant for factory reset pin
# TODO: Move this to constants, leaving it here for now to avoid breaking changes
CONF_FACTORY_RESET_PIN = "factory_reset_pin"

CONF_DEVICE_TYPE = "device_type"


# Define enum for device types
class DeviceType(enum.Enum):
    MTD = "mtd"
    FTD = "ftd"


DEFAULT_DEVICE_TYPE = DeviceType.FTD

DEVICE_TYPE_OPTIONS = {
    "mtd": DeviceType.MTD,
    "ftd": DeviceType.FTD,
}


# Create a custom validator for hex strings with a specific length
def hex_string_length(length):
    def validator(value):
        value = cv.string_strict(value)
        if not all(c in "0123456789ABCDEFabcdef" for c in value):
            raise cv.Invalid(f"Invalid hex string: {value}")
        if len(value) != length:
            raise cv.Invalid(f"Hex string must be exactly {length} characters long")
        return value

    return validator


def _validate_shell_depends_on_ftd(config):
    if config[CONF_SHELL] and config[CONF_DEVICE_TYPE] != DeviceType.FTD:
        raise cv.Invalid(
            f"{CONF_SHELL}=true is only supported when {CONF_DEVICE_TYPE}=FTD"
        )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(OpenThreadZephyr),
            cv.Optional(CONF_DEVICE_TYPE, default=DEFAULT_DEVICE_TYPE): cv.enum(
                DEVICE_TYPE_OPTIONS, upper=False
            ),
            cv.Optional(CONF_CHANNEL, default=DEFAULT_CHANNEL): cv.int_range(
                min=11, max=26
            ),
            cv.Optional(CONF_PANID, default=DEFAULT_PANID): cv.hex_uint16_t,
            cv.Optional(CONF_NETWORK_NAME, default=DEFAULT_NETWORK_NAME): cv.string,
            cv.Optional(CONF_XPANID, default=DEFAULT_XPANID): hex_string_length(16),
            cv.Optional(
                CONF_NETWORK_KEY, default=DEFAULT_NETWORK_KEY
            ): hex_string_length(32),
            cv.Optional(CONF_PSKC, default=DEFAULT_PSKC): hex_string_length(32),
            cv.Optional(
                CONF_RADIO_TX_POWER, default=DEFAULT_RADIO_TX_POWER
            ): cv.int_range(min=-20, max=20),
            cv.Optional(CONF_FORCE_DATASET, default=DEFAULT_FORCE_DATASET): cv.boolean,
            cv.Optional(CONF_SHELL, default=DEFAULT_SHELL): cv.boolean,
            cv.Optional(
                CONF_ENABLE_IPV6, default=True
            ): ot_cv.require_framework_version(cv.Version(1, 0, 0)),
            cv.Optional(CONF_FACTORY_RESET_PIN): pins.gpio_input_pin_schema,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate_shell_depends_on_ftd,
)


@coroutine_with_priority(1000)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Configure OpenThread parameters
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    cg.add(var.set_panid(config[CONF_PANID]))
    cg.add(var.set_network_name(config[CONF_NETWORK_NAME]))
    cg.add(var.set_xpanid(config[CONF_XPANID]))
    cg.add(var.set_network_key(config[CONF_NETWORK_KEY]))
    cg.add(var.set_pskc(config[CONF_PSKC]))
    cg.add(var.set_radio_tx_power(config[CONF_RADIO_TX_POWER]))
    cg.add(var.set_force_dataset(config[CONF_FORCE_DATASET]))

    # Setup factory reset pin if configured
    if CONF_FACTORY_RESET_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_FACTORY_RESET_PIN])
        cg.add(var.set_factory_reset_pin(pin))

    # Get mDNS component reference if available
    if "mdns" in CORE.config:
        mdns = await cg.get_variable(CORE.config["mdns"]["id"])
        cg.add(var.set_mdns(mdns))

    # Get MCUmgr component reference if configured for UDP OTA
    if CONF_OTA in CORE.config:
        for ota_config in CORE.config[CONF_OTA]:
            # Check if this OTA entry is for zephyr_mcumgr
            if ota_config.get("platform") == "zephyr_mcumgr":
                # Check if UDP transport is used for this entry
                if ota_config.get(CONF_TRANSPORT) == CONF_UDP:
                    # Get the ID of this specific zephyr_mcumgr OTA component
                    mcumgr_ota_id = ota_config.get(CONF_ID)
                    if mcumgr_ota_id:
                        _LOGGER.debug(
                            f"Found zephyr_mcumgr OTA component with UDP transport (ID: {mcumgr_ota_id}). Setting dependency."
                        )
                        mcumgr = await cg.get_variable(mcumgr_ota_id)
                        cg.add(var.set_mcumgr(mcumgr))
                        break
                    else:
                        _LOGGER.warning(
                            "Found zephyr_mcumgr OTA with UDP but no ID?"
                        )  # Should not happen with proper config validation
                else:
                    _LOGGER.debug(
                        f"Found zephyr_mcumgr OTA component but transport is not UDP ({ota_config.get(CONF_TRANSPORT)}). Skipping dependency."
                    )
    else:
        _LOGGER.debug("No OTA configuration found. Skipping mcumgr dependency.")

    # Configure Zephyr for OpenThread
    # Enable OpenThread
    zephyr_add_prj_conf("NET_L2_OPENTHREAD", True)
    zephyr_add_prj_conf("INIT_STACKS", True)

    # Set device type based on config
    zephyr_add_prj_conf("OPENTHREAD_MTD", config[CONF_DEVICE_TYPE] == DeviceType.MTD)
    zephyr_add_prj_conf("OPENTHREAD_FTD", config[CONF_DEVICE_TYPE] == DeviceType.FTD)

    # Shell configuration - make it configurable
    shell_enabled = config[CONF_SHELL]
    zephyr_add_prj_conf("OPENTHREAD_SHELL", shell_enabled)
    zephyr_add_prj_conf("OPENTHREAD_PING_SENDER", shell_enabled)
    zephyr_add_prj_conf("SHELL", shell_enabled)
    zephyr_add_prj_conf("SHELL_BACKEND_SERIAL", shell_enabled)

    # CLI settings - ensure CLI libraries are properly included
    if shell_enabled:
        zephyr_add_prj_conf("OPENTHREAD_CLI_MAX_LINE_LENGTH", 128)

    # Network stack with IPv6 support
    zephyr_add_prj_conf("NETWORKING", True)
    zephyr_add_prj_conf("NET_IPV6", True)
    zephyr_add_prj_conf("NET_IPV4", True)
    zephyr_add_prj_conf("NET_TCP", True)
    zephyr_add_prj_conf("NET_UDP", True)

    zephyr_add_prj_conf("OPENTHREAD_SLAAC", True)
    zephyr_add_prj_conf("NET_IF_UNICAST_IPV6_ADDR_COUNT", 6)
    zephyr_add_prj_conf("NET_SOCKETS", True)
    zephyr_add_prj_conf("NET_SOCKETS_POLL_MAX", 4)
    zephyr_add_prj_conf("NET_SOCKETS_POSIX_NAMES", True)

    # TCP configuration - disable RFC6528 ISN generation to avoid mbedtls_md5 dependency
    zephyr_add_prj_conf("NET_TCP_ISN_RFC6528", False)

    # Enable hostname
    zephyr_add_prj_conf("NET_HOSTNAME_ENABLE", True)

    # Configure DNS resolver settings for Zephyr
    zephyr_add_prj_conf("DNS_RESOLVER", True)
    zephyr_add_prj_conf("DNS_SD", True)
    zephyr_add_prj_conf("DNS_SERVER_IP_ADDRESSES", False)
    zephyr_add_prj_conf("DNS_RESOLVER_MAX_SERVERS", 2)
    zephyr_add_prj_conf("DNS_RESOLVER_ADDITIONAL_BUF_CTR", 2)
    zephyr_add_prj_conf("DNS_RESOLVER_ADDITIONAL_QUERIES", 2)

    # Setup SRP services if mDNS is enabled
    if "mdns" in CORE.config:
        zephyr_add_prj_conf("CONFIG_OPENTHREAD_SRP_CLIENT", True)
        zephyr_add_prj_conf("CONFIG_OPENTHREAD_DNS_CLIENT", True)
        _LOGGER.debug("mDNS component found, enabling OpenThread SRP client features.")
    else:
        _LOGGER.debug(
            "mDNS component not found, OpenThread SRP client features disabled."
        )

    # Stack sizes
    zephyr_add_prj_conf("MAIN_STACK_SIZE", 10240)

    # OpenThread settings
    zephyr_add_prj_conf("OPENTHREAD_CHANNEL", config[CONF_CHANNEL])
    panid_int = int(config[CONF_PANID])  # Already an integer from cv.hex_uint16_t
    zephyr_add_prj_conf("OPENTHREAD_PANID", panid_int)
    zephyr_add_prj_conf("OPENTHREAD_NETWORK_NAME", config[CONF_NETWORK_NAME])
    zephyr_add_prj_conf("OPENTHREAD_XPANID", config[CONF_XPANID])
    zephyr_add_prj_conf("OPENTHREAD_NETWORKKEY", config[CONF_NETWORK_KEY])
    zephyr_add_prj_conf("OPENTHREAD_MANUAL_START", True)
    zephyr_add_prj_conf("OPENTHREAD_DEFAULT_TX_POWER", config[CONF_RADIO_TX_POWER])

    # Add build flags - ensure proper spacing between flags
    cg.add_build_flag("-DUSE_OPENTHREAD")
    cg.add_build_flag("-DUSE_ZEPHYR_OPENTHREAD")
    cg.add_build_flag("-DUSE_IPV6")
    cg.add_build_flag("-DUSE_ZEPHYR")
    cg.add_build_flag("-DUSE_ZEPHYR_NETWORKING")

    # Add device tree overlay for OpenThread
    zephyr_add_overlay("""
&ieee802154 {
    status = "okay";
};
    """)
