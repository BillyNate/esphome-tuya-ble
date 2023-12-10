#pragma once

#include <deque>
#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/md5/md5.h"
#include "esphome/components/tuya_ble_tracker/common.h"
#include "esphome/components/tuya_ble_tracker/tuya_ble_tracker.h"

namespace esphome {
namespace tuya_ble_node {

using namespace esphome::tuya_ble;
using md5::MD5Digest;

class TuyaBleNode : public TYBleNode, public Component {
  
  std::deque<struct TYBleCommand> command_queue;

  public:
    bool has_command();

    bool has_session_key();

    void issue_command();

    void set_local_key(const char *local_key);

    void request_info();

    void toggle(bool value);

    void register_client(TYBleClient *client) {
      ESP_LOGD("tuya_ble_node", "Client registered in node!");
      this->client = client;
      this->has_client = true;
    }

  protected:
    TYBleClient *client;
    bool has_client = false;

    void enqueue_command(TYBleCommand *command);
};

}  // namespace tuya_ble_node
}  // namespace esphome
