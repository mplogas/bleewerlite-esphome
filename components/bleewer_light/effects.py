import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.light.effects import register_monochromatic_effect

from . import bleewer_light_ns

NeewerEffect = bleewer_light_ns.class_("NeewerEffect", cg.Component)

# Common parameter keys
CONF_EFFECT_ID = "effect_id"
CONF_BRIGHTNESS = "brightness"
CONF_SPEED = "speed"
CONF_TEMP = "color_temp"
CONF_GM = "gm"
CONF_SPARKS = "sparks"
CONF_BRIGHT_MIN = "brightness_min"
CONF_BRIGHT_MAX = "brightness_max"
CONF_HUE = "hue"
CONF_SATURATION = "saturation"
CONF_SPECIAL_OPTIONS = "special_options"
CONF_TEMP_MIN = "temp_min"
CONF_TEMP_MAX = "temp_max"

# Effect ID mapping (user-facing name -> protocol effect number)
EFFECT_IDS = {
    "lightning": 1,
    "paparazzi": 2,
    "defective_bulb": 3,
    "explosion": 4,
    "welding": 5,
    "cct_flash": 6,
    "hue_flash": 7,
    "cct_pulse": 8,
    "hue_pulse": 9,
    "cop_car": 10,
    "candlelight": 11,
    "hue_loop": 12,
    "cct_loop": 13,
    "intensity_loop": 14,
    "tv_screen": 15,
    "fireworks": 16,
    "party": 17,
}


def _neewer_effect_schema(effect_name, extra_params=None):
    """Build a schema for a Neewer effect with common + effect-specific params."""
    schema = {
        cv.Optional(CONF_BRIGHTNESS, default=100): cv.int_range(min=0, max=100),
        cv.Optional(CONF_SPEED, default=5): cv.int_range(min=1, max=10),
    }
    if extra_params:
        schema.update(extra_params)
    return schema


async def _neewer_effect_to_code(config, effect_id, effect_name):
    """Common codegen for all Neewer effects."""
    var = cg.new_Pvariable(effect_id, config["name"])
    cg.add(var.set_effect_id(EFFECT_IDS[effect_name]))

    if CONF_BRIGHTNESS in config:
        cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))
    if CONF_SPEED in config:
        cg.add(var.set_speed(config[CONF_SPEED]))
    if CONF_TEMP in config:
        cg.add(var.set_temp(config[CONF_TEMP]))
    if CONF_GM in config:
        cg.add(var.set_gm(config[CONF_GM]))
    if CONF_SPARKS in config:
        cg.add(var.set_sparks(config[CONF_SPARKS]))
    if CONF_BRIGHT_MIN in config:
        cg.add(var.set_bright_min(config[CONF_BRIGHT_MIN]))
    if CONF_BRIGHT_MAX in config:
        cg.add(var.set_bright_max(config[CONF_BRIGHT_MAX]))
    if CONF_HUE in config:
        cg.add(var.set_hue(config[CONF_HUE]))
    if CONF_SATURATION in config:
        cg.add(var.set_saturation(config[CONF_SATURATION]))
    if CONF_SPECIAL_OPTIONS in config:
        cg.add(var.set_special_options(config[CONF_SPECIAL_OPTIONS]))
    if CONF_TEMP_MIN in config:
        cg.add(var.set_temp_min(config[CONF_TEMP_MIN]))
    if CONF_TEMP_MAX in config:
        cg.add(var.set_temp_max(config[CONF_TEMP_MAX]))

    return var


# --- Register each effect ---

@register_monochromatic_effect(
    "neewer_lightning", NeewerEffect, "Lightning",
    _neewer_effect_schema("lightning", {
        cv.Optional(CONF_TEMP, default=56): cv.int_range(min=20, max=85),
    }),
)
async def neewer_lightning_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "lightning")


@register_monochromatic_effect(
    "neewer_paparazzi", NeewerEffect, "Paparazzi",
    _neewer_effect_schema("paparazzi", {
        cv.Optional(CONF_TEMP, default=56): cv.int_range(min=20, max=85),
        cv.Optional(CONF_GM, default=50): cv.int_range(min=0, max=100),
    }),
)
async def neewer_paparazzi_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "paparazzi")


@register_monochromatic_effect(
    "neewer_defective_bulb", NeewerEffect, "Defective Bulb",
    _neewer_effect_schema("defective_bulb", {
        cv.Optional(CONF_TEMP, default=56): cv.int_range(min=20, max=85),
        cv.Optional(CONF_GM, default=50): cv.int_range(min=0, max=100),
    }),
)
async def neewer_defective_bulb_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "defective_bulb")


@register_monochromatic_effect(
    "neewer_explosion", NeewerEffect, "Explosion",
    _neewer_effect_schema("explosion", {
        cv.Optional(CONF_TEMP, default=56): cv.int_range(min=20, max=85),
        cv.Optional(CONF_GM, default=50): cv.int_range(min=0, max=100),
        cv.Optional(CONF_SPARKS, default=4): cv.int_range(min=1, max=10),
    }),
)
async def neewer_explosion_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "explosion")


@register_monochromatic_effect(
    "neewer_welding", NeewerEffect, "Welding",
    _neewer_effect_schema("welding", {
        cv.Optional(CONF_BRIGHT_MIN, default=20): cv.int_range(min=0, max=100),
        cv.Optional(CONF_BRIGHT_MAX, default=100): cv.int_range(min=0, max=100),
        cv.Optional(CONF_TEMP, default=56): cv.int_range(min=20, max=85),
        cv.Optional(CONF_GM, default=50): cv.int_range(min=0, max=100),
    }),
)
async def neewer_welding_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "welding")


@register_monochromatic_effect(
    "neewer_cct_flash", NeewerEffect, "CCT Flash",
    _neewer_effect_schema("cct_flash", {
        cv.Optional(CONF_TEMP, default=56): cv.int_range(min=20, max=85),
        cv.Optional(CONF_GM, default=50): cv.int_range(min=0, max=100),
    }),
)
async def neewer_cct_flash_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "cct_flash")


@register_monochromatic_effect(
    "neewer_hue_flash", NeewerEffect, "Hue Flash",
    _neewer_effect_schema("hue_flash", {
        cv.Optional(CONF_HUE, default=0): cv.int_range(min=0, max=360),
        cv.Optional(CONF_SATURATION, default=100): cv.int_range(min=0, max=100),
    }),
)
async def neewer_hue_flash_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "hue_flash")


@register_monochromatic_effect(
    "neewer_cct_pulse", NeewerEffect, "CCT Pulse",
    _neewer_effect_schema("cct_pulse", {
        cv.Optional(CONF_TEMP, default=56): cv.int_range(min=20, max=85),
        cv.Optional(CONF_GM, default=50): cv.int_range(min=0, max=100),
    }),
)
async def neewer_cct_pulse_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "cct_pulse")


@register_monochromatic_effect(
    "neewer_hue_pulse", NeewerEffect, "Hue Pulse",
    _neewer_effect_schema("hue_pulse", {
        cv.Optional(CONF_HUE, default=0): cv.int_range(min=0, max=360),
        cv.Optional(CONF_SATURATION, default=100): cv.int_range(min=0, max=100),
    }),
)
async def neewer_hue_pulse_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "hue_pulse")


@register_monochromatic_effect(
    "neewer_cop_car", NeewerEffect, "Cop Car",
    _neewer_effect_schema("cop_car", {
        cv.Optional(CONF_SPECIAL_OPTIONS, default=2): cv.int_range(min=1, max=3),
    }),
)
async def neewer_cop_car_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "cop_car")


@register_monochromatic_effect(
    "neewer_candlelight", NeewerEffect, "Candlelight",
    _neewer_effect_schema("candlelight", {
        cv.Optional(CONF_BRIGHT_MIN, default=20): cv.int_range(min=0, max=100),
        cv.Optional(CONF_BRIGHT_MAX, default=80): cv.int_range(min=0, max=100),
        cv.Optional(CONF_TEMP, default=32): cv.int_range(min=20, max=85),
        cv.Optional(CONF_GM, default=50): cv.int_range(min=0, max=100),
        cv.Optional(CONF_SPARKS, default=4): cv.int_range(min=1, max=10),
    }),
)
async def neewer_candlelight_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "candlelight")


@register_monochromatic_effect(
    "neewer_hue_loop", NeewerEffect, "Hue Loop",
    _neewer_effect_schema("hue_loop", {
        cv.Optional(CONF_HUE, default=0): cv.int_range(min=0, max=360),
    }),
)
async def neewer_hue_loop_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "hue_loop")


@register_monochromatic_effect(
    "neewer_cct_loop", NeewerEffect, "CCT Loop",
    _neewer_effect_schema("cct_loop", {
        cv.Optional(CONF_TEMP_MIN, default=32): cv.int_range(min=20, max=85),
        cv.Optional(CONF_TEMP_MAX, default=56): cv.int_range(min=20, max=85),
    }),
)
async def neewer_cct_loop_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "cct_loop")


@register_monochromatic_effect(
    "neewer_intensity_loop", NeewerEffect, "Intensity Loop",
    _neewer_effect_schema("intensity_loop", {
        cv.Optional(CONF_BRIGHT_MIN, default=20): cv.int_range(min=0, max=100),
        cv.Optional(CONF_BRIGHT_MAX, default=100): cv.int_range(min=0, max=100),
        cv.Optional(CONF_TEMP, default=56): cv.int_range(min=20, max=85),
    }),
)
async def neewer_intensity_loop_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "intensity_loop")


@register_monochromatic_effect(
    "neewer_tv_screen", NeewerEffect, "TV Screen",
    _neewer_effect_schema("tv_screen", {
        cv.Optional(CONF_BRIGHT_MIN, default=20): cv.int_range(min=0, max=100),
        cv.Optional(CONF_BRIGHT_MAX, default=100): cv.int_range(min=0, max=100),
        cv.Optional(CONF_TEMP, default=56): cv.int_range(min=20, max=85),
        cv.Optional(CONF_GM, default=50): cv.int_range(min=0, max=100),
    }),
)
async def neewer_tv_screen_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "tv_screen")


@register_monochromatic_effect(
    "neewer_fireworks", NeewerEffect, "Fireworks",
    _neewer_effect_schema("fireworks", {
        cv.Optional(CONF_SPECIAL_OPTIONS, default=2): cv.int_range(min=1, max=3),
        cv.Optional(CONF_SPARKS, default=4): cv.int_range(min=1, max=10),
    }),
)
async def neewer_fireworks_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "fireworks")


@register_monochromatic_effect(
    "neewer_party", NeewerEffect, "Party",
    _neewer_effect_schema("party", {
        cv.Optional(CONF_SPECIAL_OPTIONS, default=2): cv.int_range(min=1, max=3),
    }),
)
async def neewer_party_to_code(config, effect_id):
    return await _neewer_effect_to_code(config, effect_id, "party")
