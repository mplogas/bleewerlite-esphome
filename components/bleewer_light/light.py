import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client, light
from esphome.const import CONF_ID, CONF_OUTPUT_ID

from . import bleewer_light_ns, BLEewerLight, DEPENDENCIES  # noqa: F401
from . import effects as neewer_effects  # noqa: F401 — registers effects with ESPHome

CONF_CCT_MIN = "color_temp_min"
CONF_CCT_MAX = "color_temp_max"
CONF_CCT_ONLY = "cct_only"
CONF_INFINITY_MODE = "infinity_mode"
CONF_MODEL_NAME = "model_name"

CONFIG_SCHEMA = cv.All(
    light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(BLEewerLight),
            cv.Optional(CONF_MODEL_NAME): cv.string,
            cv.Optional(CONF_CCT_MIN): cv.int_range(min=1000, max=10000),
            cv.Optional(CONF_CCT_MAX): cv.int_range(min=1000, max=10000),
            cv.Optional(CONF_CCT_ONLY): cv.boolean,
            cv.Optional(CONF_INFINITY_MODE): cv.int_range(min=0, max=2),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(ble_client.BLE_CLIENT_SCHEMA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    await ble_client.register_ble_node(var, config)

    if CONF_MODEL_NAME in config:
        cg.add(var.set_model_name(config[CONF_MODEL_NAME]))
    if CONF_CCT_MIN in config:
        cg.add(var.set_cct_max_mireds(1000000.0 / config[CONF_CCT_MIN]))
    if CONF_CCT_MAX in config:
        cg.add(var.set_cct_min_mireds(1000000.0 / config[CONF_CCT_MAX]))
    if CONF_CCT_ONLY in config:
        cg.add(var.set_cct_only(config[CONF_CCT_ONLY]))
    if CONF_INFINITY_MODE in config:
        cg.add(var.set_infinity_mode(config[CONF_INFINITY_MODE]))
