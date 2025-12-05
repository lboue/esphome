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
    initsequence=(
        (0x11,),  # Sleep out
        (0x3A, 0x55),  # Set color mode to 16-bit
        (0x36, 0x00),  # Set memory access control
        (0x21,),  # Display inversion on
        (0x29,),  # Display on
    ),
)
