import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import tuya_ble_tracker, tuya_ble_client
from esphome.const import CONF_ID, CONF_MAC_ADDRESS

AUTO_LOAD = ["md5"]
DEPENDENCIES = ["tuya_ble_client", "esp32"]

tuya_ble_node_ns = cg.esphome_ns.namespace("tuya_ble_node")

TuyaBLENode = tuya_ble_node_ns.class_("TuyaBLENode", cg.Component)

CONF_LOCAL_KEY = 'local_key'
CONF_MAX_QUEUED = 'max_queued'

MULTI_CONF = True

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TuyaBLENode),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Required(CONF_LOCAL_KEY): cv.string,
            cv.Optional(CONF_MAX_QUEUED, default=1): cv.int_range(1, 10),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(tuya_ble_client.TUYA_BLE_CLIENT_SCHEMA)
)

CONF_TUYA_BLE_NODE_ID = "tuya_ble_node_id"

TUYA_BLE_NODE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TUYA_BLE_NODE_ID): cv.use_id(TuyaBLENode),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_local_key(config[CONF_LOCAL_KEY]))
    cg.add(var.set_max_queued(config[CONF_MAX_QUEUED]))

    await tuya_ble_client.register_tuya_node(var, config)

    parent = await cg.get_variable(config[tuya_ble_client.CONF_TUYA_BLE_CLIENT_ID])
    cg.add(var.register_client(parent))
