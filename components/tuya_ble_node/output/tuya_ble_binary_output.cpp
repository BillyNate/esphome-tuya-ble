#include "tuya_ble_binary_output.h"
#include "esphome/core/log.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

namespace esphome {
namespace tuya_ble {

static const char *const TAG = "tuya_ble_binary_output";

void TuyaBleBinaryOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "BLE Binary Output");
  //ESP_LOGCONFIG(TAG, "  MAC address        : %s", this->parent_->address_str().c_str());
  LOG_BINARY_OUTPUT(this);
}

void TuyaBleBinaryOutput::write_state(bool state) {
  ESP_LOGV(TAG, "write_state: %i", state);

  this->node->toggle(state);
}

}  // namespace tuya_ble
}  // namespace esphome
