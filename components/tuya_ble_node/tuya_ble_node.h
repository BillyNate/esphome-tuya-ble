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

class TuyaBLENode : public TYBLENode, public Component {
  
  std::deque<struct TYBLECommand> command_queue;

  public:
    bool has_command();

    bool has_session_key();

    void issue_command();

    void set_device_id(std::string device_id);

    void set_local_key(const char *local_key);

    void set_max_queued(uint8_t max);

    void set_uuid(std::string uuid);

    void pair();

    void request_info();

    void reset_session_key();

    void toggle(bool value);

    void register_client(TYBLEClient *client) {
      ESP_LOGD("tuya_ble_node", "Client registered in node!");
      this->client = client;
      this->has_client = true;
    }

  protected:
    TYBLEClient *client;
    bool has_client = false;
    uint8_t max_queued = 1;

    void enqueue_command(TYBLECommand *command);
};

}  // namespace tuya_ble_node
}  // namespace esphome
