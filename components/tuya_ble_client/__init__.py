import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker, esp32_ble_client, tuya_ble_tracker
from esphome.const import CONF_ID, CONF_MAC_ADDRESS, CONF_TIMEOUT

AUTO_LOAD = ["esp32_ble_client", "md5"]
DEPENDENCIES = ["tuya_ble_tracker", "esp32"]

tuya_ble_client_ns = cg.esphome_ns.namespace("tuya_ble_client")

TuyaBLEClient = tuya_ble_client_ns.class_("TuyaBLEClient", esp32_ble_client.BLEClientBase)

CONF_LOCAL_KEY = 'local_key'

# ESPHome BLE_CLIENT is set to MULTI_CONF=3, let's keep that:
MULTI_CONF = 3

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TuyaBLEClient),
            cv.Optional(
                CONF_TIMEOUT, default="2000ms"
            ): cv.positive_time_period_milliseconds,
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(tuya_ble_tracker.TUYA_BLE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

CONF_TUYA_BLE_CLIENT_ID = "tuya_ble_client_id"

TUYA_BLE_CLIENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TUYA_BLE_CLIENT_ID): cv.use_id(TuyaBLEClient),
    }
)

async def register_tuya_node(var, config):
    client = await cg.get_variable(config[CONF_TUYA_BLE_CLIENT_ID])
    cg.add(
        client.register_node(
            config[CONF_MAC_ADDRESS].as_hex,
            var,
        )
    )
    return var

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_disconnect_after(config[CONF_TIMEOUT]))

    await esp32_ble_tracker.register_client(var, config)
    await tuya_ble_tracker.register_client(var, config)
