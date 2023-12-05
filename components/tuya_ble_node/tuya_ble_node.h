#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/md5/md5.h"
#include "esphome/components/tuya_ble/helpers.h"
#include "esphome/components/tuya_ble_tracker/tuya_ble_tracker.h"

namespace esphome {
namespace tuya_ble_node {

using md5::MD5Digest;

class TuyaBleNode : public tuya_ble_tracker::TYBleNode, public Component {

  public:
    void set_local_key(const char *local_key);
    void register_client(tuya_ble_tracker::TYBleClient *client) {
      ESP_LOGD("tuya_ble_node", "Client registered in node!");
      this->client = client;
    }

  protected:
    tuya_ble_tracker::TYBleClient *client;
};

}  // namespace tuya_ble_node
}  // namespace esphome
