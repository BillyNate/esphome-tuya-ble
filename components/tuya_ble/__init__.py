import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker, esp32_ble_client, tuya_ble_tracker
from esphome.const import CONF_ID, CONF_MAC_ADDRESS

AUTO_LOAD = ["esp32_ble_client", "md5"]
DEPENDENCIES = ["tuya_ble_tracker", "esp32"]

tuya_ble_ns = cg.esphome_ns.namespace("tuya_ble")

TuyaBleClient = tuya_ble_ns.class_("TuyaBleClient", esp32_ble_client.BLEClientBase)

CONF_LOCAL_KEY = 'local_key'

# ESPHome BLE_CLIENT is set to MULTI_CONF=3, let's keep that:
MULTI_CONF = 3

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TuyaBleClient),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Required(CONF_LOCAL_KEY): cv.string,
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(tuya_ble_tracker.TUYA_BLE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(
        var.register_device(
            config[CONF_MAC_ADDRESS].as_hex,
            config[CONF_LOCAL_KEY],
        )
    )

    await esp32_ble_tracker.register_client(var, config)
    await tuya_ble_tracker.register_client(var, config)
