#include "tuya_ble_tracker.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace tuya_ble {

static const char *const TAG = "tuya_ble_tracker";

bool TuyaBleTracker::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {

  uint64_t mac_address = device.address_uint64();

  if(!this->connection->has_device(mac_address)) {
    return false;
  }

  TuyaBleDevice *ble_device = this->connection->get_device(mac_address);
  ble_device->last_detected = esphome::millis();
  ble_device->rssi = device.get_rssi();

  if(this->found_devices.count(mac_address) < 1) {

    ESP_LOGI(TAG, "Found BLE device %s - %s. RSSI: %d dB (total devices: %d)", device.get_name().c_str(), device.address_str().c_str(), ble_device->rssi, this->found_devices.size());

    /*
    if(!ble_device->session_key) {
      // Connect once and get necessary data
    }
    */
    this->connection->set_address(mac_address);
    this->connection->parse_device(device);

    this->set_timeout("connecting", 20000, [this, device]() { // Move this to the client loop() by using a start time and check if taken too long and thus disconnecting?
      if (this->connection->connected()) {
        return;
      }
      ESP_LOGI(TAG, "Failed to connect %s => rssi: %d", device.address_str().c_str(), device.get_rssi());
      this->connection->disconnect();
      this->connection->set_address(0);
    });

    this->found_devices.insert(mac_address);
  }

  return true;
}

void TuyaBleTracker::setup() {
  Component::setup();

  ESP_LOGI(TAG, "setup");

  this->connection->set_disconnect_callback([this]() { ESP_LOGI(TAG, "disconnected"); });
}

void TuyaBleTracker::loop() {
  
}

}  // namespace tuya_ble
}  // namespace esphome