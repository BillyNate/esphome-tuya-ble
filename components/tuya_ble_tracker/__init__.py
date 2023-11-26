import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker

DEPENDENCIES = ["esp32_ble_tracker"]

tuya_ble_tracker_ns = cg.esphome_ns.namespace("tuya_ble_tracker")

TuyaBleTracker = tuya_ble_tracker_ns.class_("TuyaBleTracker", esp32_ble_tracker.ESPBTDeviceListener, cg.Component)

CONF_TUYA_BLE_TRACKER_ID = 'tuya_ble_tracker_id'

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_TUYA_BLE_TRACKER_ID): cv.declare_id(TuyaBleTracker),
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

TUYA_BLE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TUYA_BLE_TRACKER_ID): cv.use_id(TuyaBleTracker),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_TUYA_BLE_TRACKER_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

async def register_client(var, config):
    paren = await cg.get_variable(config[CONF_TUYA_BLE_TRACKER_ID])
    cg.add(paren.register_client(var))
    return var
