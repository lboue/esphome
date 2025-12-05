from .ili import ST7789V

# M5StickC Plus preset using ST7789V panel
ST7789V.extend(
    "M5STICKC-PLUS",
    width=135,
    height=240,
    offset_width=52,
    offset_height=40,
    cs_pin=5,
    dc_pin=23,
    reset_pin=18,
    invert_colors=True,
)

models = {}
