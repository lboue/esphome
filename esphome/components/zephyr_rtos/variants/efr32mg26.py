import esphome.codegen as cg
from esphome.components.silabs.const import VARIANT_EFR32MG26
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADVANCED,
    CONF_BOARD,
    CONF_FRAMEWORK,
    CONF_SOURCE,
    ThreadModel,
    Toolchain,
)
from esphome.types import ConfigType

from ..const import (
    ADVANCED_SCHEMA,
    BOOTLOADER_MCUBOOT,
    CONF_COMMANDER_VERSION,
    CONF_RUNNER,
)
from ..partitions import BootLayout
from . import (
    MAINLINE,
    SILABS,
    ZephyrVariant,
    qualify_board,
    resolve_framework_version,
    set_core_data,
)

# xG26-EK2709A (BRD2709A), Silicon Labs' EFR32xG26 Explorer Kit. Bare name; no
# SoC/qualifier segment needed -- upstream's board.yml declares a single SoC for
# this board, so qualify_board() is a no-op here (soc= is left unset below).
_DEFAULT_BOARD = "xg26_ek2709a"

_ADVANCED_SCHEMA = ADVANCED_SCHEMA.extend(
    {
        cv.Optional(CONF_COMMANDER_VERSION): cv.string_strict,
    }
)

# GPIO -> Silicon Labs IADC analog-input macro. Built from this SoC's own bonded
# pin counts (ngpios: A=8, B=4, C=10, D=6, confirmed against
# efr32mg26b510f3200im48.dtsi), unlike EFR32MG24 which bonds out the full
# PA0-PD15 range.
_BONDED_PINS_BY_PORT = {0: 8, 1: 4, 2: 10, 3: 6}  # port index -> ngpios
_ADC_AIN_MAP = {
    port * 16 + pin: f"IADC_INPUT_P{chr(ord('A') + port)}{pin}"
    for port, count in _BONDED_PINS_BY_PORT.items()
    for pin in range(count)
}

# https://github.com/zephyrproject-rtos/zephyr/blob/main/include/zephyr/dt-bindings/pinctrl/silabs/xg26-pinctrl.h
# Full crossbar within the bonded pin set above -- every USART/EUSART TX/RX/CLK signal
# has a macro for each bonded pin, no exclusions (same as EFR32MG24/BG27). Shared by
# every free-mux GPIO signal (UART, SPI).
_GPIO_MATRIX_PINS = frozenset(_ADC_AIN_MAP)

# Registry entries — collected by variants/__init__.py
VARIANT_NAME = VARIANT_EFR32MG26
VARIANT = ZephyrVariant(
    # Resets from the start of flash (xg26_ek2709a.dts boot_partition@0).
    boot=BootLayout(0x0),
    # Mainline stays default; Silicon Labs' vendor SDK (SILABS) is available as an alt
    # (framework: type: silabs) pending real hardware testing.
    sdk=MAINLINE,
    sdk_name="zephyr",
    alt_sdks={"silabs": SILABS},
    family="silabs",
    valid_toolchains=(Toolchain.ZEPHYR,),
    toolchain="arm-zephyr-eabi",
    # Both confirmed on real BRD2709A hardware (see issue #239): bt_hci_silabs brings
    # up BLE (HCI init + advertising received over the air), and the ieee802154 driver
    # brings up 802.15.4/OpenThread (ot scan received live beacons from other Thread
    # networks). ieee802154 support for this board landed upstream 2026-06-03 (commit
    # 03fd471a306, "boards: silabs: Add ieee802154 support"), included in the v4.5.0
    # tag -- MAINLINE's own default_version -- so no source: override is needed. No
    # Zigbee support in this fork yet either way.
    transports=frozenset({"ble", "openthread"}),
    # Zephyr's hal_silabs blob check verifies the HAL's entire manifest once BT or
    # 802.15.4 is enabled, not just this chip's -- same reasoning as efr32mg24.
    blobs=("hal_silabs", ".*", ".blobs_hal_silabs_ready"),
    gpio_port_width=16,
    gpio_port_labels=("a", "b", "c", "d"),
    # No scratch partition in the board's flash layout (boot/image-0/image-1/storage
    # only, confirmed against xg26_explorer_kit.dtsi), same shape as efr32mg24/nrf52 --
    # move/offset need no scratch, "swap" would.
    swap_methods=frozenset({"move", "offset"}),
    adc_ain_map=_ADC_AIN_MAP,
    # Only usart0 exists at SoC level on this board -- no usart1/UART1 (usart1 is
    # present on-chip but disabled in the board DTS).
    uart_node_labels={},
    uart_valid_pins={"tx": _GPIO_MATRIX_PINS, "rx": _GPIO_MATRIX_PINS},
    spi_valid_pins={
        "clk": _GPIO_MATRIX_PINS,
        "mosi": _GPIO_MATRIX_PINS,
        "miso": _GPIO_MATRIX_PINS,
    },
)


def config_schema(config: ConfigType) -> ConfigType:
    config = dict(config)
    if CONF_BOARD not in config:
        config[CONF_BOARD] = _DEFAULT_BOARD
    config[CONF_BOARD] = qualify_board(VARIANT, config[CONF_BOARD])
    config[CONF_FRAMEWORK][CONF_ADVANCED] = _ADVANCED_SCHEMA(
        config[CONF_FRAMEWORK].get(CONF_ADVANCED, {})
    )
    _, framework_ver, sdk_name, _ = resolve_framework_version(
        VARIANT, "efr32mg26", config, "EFR32MG26 support"
    )
    if (
        CONF_COMMANDER_VERSION in config[CONF_FRAMEWORK][CONF_ADVANCED]
        and sdk_name != "silabs"
    ):
        raise cv.Invalid(
            f"'{CONF_COMMANDER_VERSION}' only applies with framework: type: silabs "
            f"(current: {sdk_name!r})",
            [CONF_FRAMEWORK, CONF_ADVANCED, CONF_COMMANDER_VERSION],
        )
    set_core_data(
        VARIANT_NAME,
        config[CONF_BOARD],
        BOOTLOADER_MCUBOOT,
        framework_ver,
        config,
        framework_type=sdk_name,
        sdk_source=config[CONF_FRAMEWORK].get(CONF_SOURCE),
        runner=config[CONF_FRAMEWORK][CONF_ADVANCED].get(CONF_RUNNER),
    )
    return config


async def to_code(config: ConfigType) -> None:
    from .. import (
        zephyr_add_prj_conf,
        zephyr_add_sysbuild_conf,
        zephyr_setup_preferences,
    )

    cg.add_build_flag("-DUSE_ZEPHYR_VARIANT_EFR32MG26")
    cg.add_define("ESPHOME_BOARD", config[CONF_BOARD])
    cg.add_define("ESPHOME_VARIANT", "EFR32MG26")
    cg.add_define(ThreadModel.MULTI_ATOMICS)
    zephyr_setup_preferences()
    zephyr_add_prj_conf("REBOOT", True)
    zephyr_add_prj_conf("HWINFO", True)

    # Same reasoning as efr32mg24/nrf52: xg26_ek2709a's board DTS hardcodes the
    # boot/image-0/image-1/storage partition layout and `zephyr,code-partition =
    # &slot0_partition` unconditionally (not gated behind any Kconfig), so the app
    # can only ever run via MCUboot jumping to slot0 -- this is required to boot at
    # all, regardless of whether OTA is configured.
    zephyr_add_sysbuild_conf("BOOTLOADER_MCUBOOT", True)
    # sysbuild's own BOOT_SIGNATURE_TYPE choice overrides a per-image setting.
    zephyr_add_sysbuild_conf("BOOT_SIGNATURE_TYPE_ECDSA_P256", True)
