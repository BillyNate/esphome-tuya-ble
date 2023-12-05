#include "tuya_ble_node.h"

namespace esphome {
namespace tuya_ble_node {

static const char *const TAG = "tuya_ble_node";

bool TuyaBleNode::has_session_key() {
  return !std::all_of(this->session_key, this->session_key + KEY_SIZE, [](unsigned char x) { return x == '\0'; });
}

void TuyaBleNode::set_local_key(const char *local_key) {

  memcpy(this->local_key, local_key, 6);

  MD5Digest *md5digest = new MD5Digest();
  
  md5digest->init();
  md5digest->add(local_key, 6);
  md5digest->calculate();
  md5digest->get_bytes(&this->login_key[0]);
  
  ESP_LOGV(TAG, "Got local key (%s), turned into login key (%s)", local_key, binary_to_string(this->login_key, 16).c_str());
}

void TuyaBleNode::request_info() {
  
  // About to get DEVICE_INFO, this should be limited to whenever session_key is unusable:
  if(!this->has_session_key()) { // TODO: OR when session_key is expired
    ESP_LOGD(TAG, "Requesting device info...");

    this->client->write_data(TuyaBLECode::FUN_SENDER_DEVICE_INFO, &this->seq_num, {0}, 0, this->login_key, 0, 2);
  }
}

void TuyaBleNode::toggle(bool value) {

  size_t dp_size = 4;
  unsigned char data[dp_size] = { 0x14, 0x01, 0x01, (unsigned char)value };

  this->client->write_data(TuyaBLECode::FUN_SENDER_DPS, &this->seq_num, data, dp_size, this->session_key);
}

}  // namespace tuya_ble_node
}  // namespace esphome
