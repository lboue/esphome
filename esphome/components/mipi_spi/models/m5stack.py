from esphome.components.mipi import DriverChip

# Register M5Stack models
DriverChip(
    "M5STACK",
    width=320,
    height=240,
    cs_pin=14,
    dc_pin=27,
    reset_pin=33,
    invert_colors=True,
    data_rate="40MHz",
)

DriverChip(
    "M5CORES3",
    width=320,
    height=240,
    cs_pin=3,
    dc_pin=35,
    invert_colors=True,
    data_rate="40MHz",
)
