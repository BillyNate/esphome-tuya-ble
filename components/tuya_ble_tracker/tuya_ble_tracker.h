#pragma once

#include <set>
#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "common.h"

namespace esphome {
namespace tuya_ble_tracker {

using namespace esphome::tuya_ble;

class TuyaBLETracker : public esp32_ble_tracker::ESPBTDeviceListener, public Component {
  uint32_t last_connection_attempt{0};
  
  public:
    bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

    void on_scan_end() override { ESP_LOGD("TuyaBLE", "Finished scan."); }

    void register_client(TYBLEClient *client) {
      ESP_LOGD("TuyaBLE", "Registering client");
      this->client = client;
      this->has_client = true;
    }
    void setup() override;
    void loop() override;

 protected:
  TYBLEClient *client;
  bool has_client = false;
};

}  // namespace tuya_ble_tracker
}  // namespace esphome
