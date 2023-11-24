#pragma once

#include <set>
#include "esphome/components/esp32_ble_client/ble_client_base.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/component.h"

#include "tuya_ble_client.h"

namespace esphome {
namespace tuya_ble {

using namespace esp32_ble_client;

class TuyaBleTracker : public esp32_ble_tracker::ESPBTDeviceListener, public Component {
  uint32_t start;
  uint32_t last_connection_attempt{0};
  void sort_devices();
  void remove_devices_that_are_not_available();

  public:
    TuyaBleTracker() { this->start = esphome::millis(); }
    bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

    void on_scan_end() override { ESP_LOGD("TuyaBle", "scan end"); }

    void register_client(TuyaBleClient *client) {
        ESP_LOGD("TuyaBle", "register_client");
        this->client = client;
    }
    void setup() override;
    void loop() override;

 protected:
  TuyaBleClient *client;
  std::set<uint64_t> found_devices{};
};

}  // namespace tuya_ble
}  // namespace esphome