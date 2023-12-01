#pragma once

#include <set>
#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

namespace esphome {
namespace tuya_ble_tracker {

namespace espbt = esphome::esp32_ble_tracker;

struct TuyaBleDevice {
  unsigned char local_key[6];
  unsigned char login_key[16];
  unsigned char session_key[16];
  // There's supposedly also an auth_key, but since it's not used, it's not declared either
  uint32_t seq_num;
  uint32_t last_detected;
  int rssi;
};

class TYBleClient {
  espbt::ClientState state_;
  public:
    virtual struct TuyaBleDevice *get_device(uint64_t mac_address);
    virtual bool has_device(uint64_t mac_address) = 0;
    virtual void set_address(uint64_t address) = 0;
    virtual bool connected() { return this->state_ == espbt::ClientState::ESTABLISHED; }
    virtual void disconnect() = 0;
    virtual void set_disconnect_callback(std::function<void()> &&f);
    virtual bool parse_device(const espbt::ESPBTDevice &device);
    espbt::ClientState state() const { return state_; }
};

class TuyaBleTracker : public esp32_ble_tracker::ESPBTDeviceListener, public Component {
  uint32_t start;
  uint32_t last_connection_attempt{0};
  void sort_devices();
  void remove_devices_that_are_not_available();

  public:
    TuyaBleTracker() { this->start = esphome::millis(); }
    bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

    void on_scan_end() override { ESP_LOGD("TuyaBle", "scan end"); }

    void register_client(TYBleClient *client) {
        ESP_LOGD("TuyaBle", "register_client");
        this->client = client;
        this->has_client = true;
    }
    void setup() override;
    void loop() override;

 protected:
  TYBleClient *client;
  bool has_client = false;
  std::set<uint64_t> found_devices{};
};

}  // namespace tuya_ble
}  // namespace esphome