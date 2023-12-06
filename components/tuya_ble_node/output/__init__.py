import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.components import tuya_ble_node
from esphome.const import CONF_ID

from .. import tuya_ble_node_ns

DEPENDENCIES = ["tuya_ble_node"]

TuyaBleBinaryOutput = tuya_ble_node_ns.class_(
    "TuyaBleBinaryOutput", output.BinaryOutput, cg.Component
)

CONFIG_SCHEMA = cv.All(
    output.BINARY_OUTPUT_SCHEMA.extend(
        {
            cv.Required(CONF_ID): cv.declare_id(TuyaBleBinaryOutput),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(tuya_ble_node.TUYA_BLE_NODE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    
    await output.register_output(var, config)
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[tuya_ble_node.CONF_TUYA_BLE_NODE_ID])
    cg.add(var.register_node(parent))
