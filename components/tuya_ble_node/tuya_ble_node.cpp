#include "tuya_ble_node.h"

namespace esphome {
namespace tuya_ble_node {

static const char *const TAG = "tuya_ble_node";

void TuyaBleNode::set_local_key(const char *local_key) {

  memcpy(this->local_key, local_key, 6);

  MD5Digest *md5digest = new MD5Digest();
  
  md5digest->init();
  md5digest->add(local_key, 6);
  md5digest->calculate();
  md5digest->get_bytes(&this->login_key[0]);
  
  ESP_LOGV(TAG, "Got local key (%s), turned into login key (%s)", local_key, binary_to_string(this->login_key, 16).c_str());
}

}  // namespace tuya_ble_node
}  // namespace esphome
