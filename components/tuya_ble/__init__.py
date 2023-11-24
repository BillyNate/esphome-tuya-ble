import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker, esp32_ble_client
from esphome.const import CONF_ID, CONF_MAC_ADDRESS

AUTO_LOAD = ["esp32_ble_client", "esp32_ble_tracker", "md5"]
DEPENDENCIES = ["esp32"]

tuya_ble_ns = cg.esphome_ns.namespace("tuya_ble")

TuyaBleTracker = tuya_ble_ns.class_("TuyaBleTracker", esp32_ble_tracker.ESPBTDeviceListener, cg.Component)
TuyaBleClient = tuya_ble_ns.class_("TuyaBleClient", esp32_ble_client.BLEClientBase)

CONF_LOCAL_KEY = 'local_key'

CLIENT_SCHEMA = esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(TuyaBleClient),
    }
).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TuyaBleTracker),
            cv.Optional("client", {}): CLIENT_SCHEMA,
            cv.Optional("devices", default=[]): cv.ensure_list(
                cv.Schema(
                    {
                        cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
                        cv.Required(CONF_LOCAL_KEY): cv.string,
                    }
                )
            ),
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    client_var = cg.new_Pvariable(config["client"][CONF_ID])
    
    for device in config.get("devices", []):
        cg.add(
            client_var.register_device(
                device[CONF_MAC_ADDRESS].as_hex,
                device[CONF_LOCAL_KEY],
            )
        )

    await cg.register_component(client_var, config["client"])
    cg.add(var.register_client(client_var))
    await esp32_ble_tracker.register_client(client_var, config["client"])