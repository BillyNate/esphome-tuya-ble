#include "tuya_ble_tracker.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace tuya_ble_tracker {

static const char *const TAG = "tuya_ble_tracker";

bool TuyaBleTracker::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {

  if(!this->has_client) {
    ESP_LOGW(TAG, "No client registered!");
    return false;
  }

  uint64_t mac_address = device.address_uint64();

  if(!this->client->has_device(mac_address)) {
    ESP_LOGV(TAG, "Found BLE device %s - %s. RSSI: %d dB (rejected)", device.get_name().c_str(), device.address_str().c_str(), device.get_rssi());
    return false;
  }

  TuyaBleDevice *ble_device = this->client->get_device(mac_address);
  ble_device->last_detected = esphome::millis();
  ble_device->rssi = device.get_rssi();

  if(this->found_devices.count(mac_address) < 1) {

    ESP_LOGD(TAG, "Found BLE device %s - %s. RSSI: %d dB (total devices: %d)", device.get_name().c_str(), device.address_str().c_str(), ble_device->rssi, this->found_devices.size());

    if(std::all_of(ble_device->session_key, ble_device->session_key + 16, [](unsigned char x) { return x == '\0'; })) {
      ESP_LOGD(TAG, "Device has no session key yet!");
    }
    else {
      ESP_LOGD(TAG, "Device already has a session key!");
    }

    this->client->set_address(mac_address);
    this->client->parse_device(device);
    this->last_connection_attempt = esphome::millis();
    this->found_devices.insert(mac_address);
  }

  return true;
}

void TuyaBleTracker::setup() {
  Component::setup();

  ESP_LOGD(TAG, "setup");

  if(this->has_client) {
    this->client->set_disconnect_callback([this]() { ESP_LOGD(TAG, "disconnected"); });
  }
}

void TuyaBleTracker::loop() {
  if(this->has_client) {
    //ESP_LOGD(TAG, "Connection state: %i, millis: %i, last_connection_attempt: %i, connected: %i", this->client->state(), esphome::millis(), this->last_connection_attempt, this->client->connected());
    if(this->client->state() == espbt::ClientState::CONNECTING && esphome::millis() > this->last_connection_attempt + 20000) {
      if(!this->client->connected()) {
        ESP_LOGD(TAG, "Failed to connect");
        this->client->disconnect();
        this->client->set_address(0);
      }
    }
  }
}

}  // namespace tuya_ble
}  // namespace esphome