from .ili import ST7789V

# M5StickC Plus preset using ST7789V panel
ST7789V.extend(
    "M5STICKC-PLUS",
    height=240,
    width=135,
    offset_height=52,
    offset_width=40,
    cs_pin=5,
    dc_pin=23,
    reset_pin=18,
    invert_colors=True,
    rotation=270,
)

models = {}
