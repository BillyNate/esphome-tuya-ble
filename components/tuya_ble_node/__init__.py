import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import tuya_ble_tracker, tuya_ble
from esphome.const import CONF_ID, CONF_MAC_ADDRESS

AUTO_LOAD = ["md5"]
DEPENDENCIES = ["tuya_ble", "esp32"]

tuya_ble_node_ns = cg.esphome_ns.namespace("tuya_ble_node")

TuyaBleNode = tuya_ble_node_ns.class_("TuyaBleNode", cg.Component)

CONF_LOCAL_KEY = 'local_key'

MULTI_CONF = True

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TuyaBleNode),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Required(CONF_LOCAL_KEY): cv.string,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(tuya_ble.TUYA_BLE_CLIENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_local_key(config[CONF_LOCAL_KEY]))

    await tuya_ble.register_tuya_node(var, config)

    parent = await cg.get_variable(config[tuya_ble.CONF_TUYA_BLE_CLIENT_ID])
    cg.add(var.register_client(parent))
