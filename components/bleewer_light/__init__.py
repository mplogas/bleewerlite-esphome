import esphome.codegen as cg
from esphome.components import ble_client, light

CODEOWNERS = ["@marc"]
DEPENDENCIES = ["ble_client", "esp32"]

bleewer_light_ns = cg.esphome_ns.namespace("bleewer_light")
BLEewerLight = bleewer_light_ns.class_(
    "BLEewerLight",
    cg.Component,
    ble_client.BLEClientNode,
    light.LightOutput,
)
