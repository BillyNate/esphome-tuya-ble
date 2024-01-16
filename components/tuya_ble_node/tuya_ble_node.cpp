#include "tuya_ble_node.h"

namespace esphome {
namespace tuya_ble_node {

static const char *const TAG = "tuya_ble_node";

void TuyaBLENode::enqueue_command(TYBLECommand *command) {
  
  while(this->command_queue.size() >= this->max_queued) {
    this->command_queue.pop_back();
  }

  this->command_queue.push_front(*command);
  
  ESP_LOGV(TAG, "enqueue_command: %s", binary_to_string(&command->data[0], command->data.size()).c_str());
}

bool TuyaBLENode::has_command() {
  return this->command_queue.size() > 0;
}

bool TuyaBLENode::has_session_key() {
  return !std::all_of(this->session_key, this->session_key + KEY_SIZE, [](unsigned char x) { return x == '\0'; });
}

void TuyaBLENode::issue_command() {
  if(!this->has_client) {
    ESP_LOGW(TAG, "No client registered at node");
    return;
  }

  if(!this->has_command()) {
    ESP_LOGW(TAG, "No commands to issue");
  }

  TYBLECommand *command = &this->command_queue.back();
  ESP_LOGV(TAG, "issue_command: %s", binary_to_string(&command->data[0], command->data.size()).c_str());
  this->client->write_data(command->code, &this->seq_num, &command->data[0], command->data.size(), command->key, command->response_to, command->protocol_version);
  this->command_queue.pop_back();
}

void TuyaBLENode::set_device_id(std::string device_id) {
  this->device_id = device_id;
}

void TuyaBLENode::set_local_key(const char *local_key) {

  memcpy(this->local_key, local_key, 6);

  MD5Digest *md5digest = new MD5Digest();
  
  md5digest->init();
  md5digest->add(local_key, 6);
  md5digest->calculate();
  md5digest->get_bytes(&this->login_key[0]);
  
  ESP_LOGV(TAG, "Got local key (%s), turned into login key (%s)", local_key, binary_to_string(this->login_key, 16).c_str());
}

void TuyaBLENode::set_max_queued(uint8_t max) {
  this->max_queued = max;
}

void TuyaBLENode::set_uuid(std::string uuid) {
  this->uuid = uuid;
}

void TuyaBLENode::pair() {
  ESP_LOGD(TAG, "Pairing device...");

  // https://github.com/airy10/ha_tuya_ble/blob/LightStrip/custom_components/tuya_ble/tuya_ble/tuya_ble.py#L315
  size_t data_size = 44;
  size_t uuid_size = this->uuid.size();
  size_t device_id_size = this->device_id.size();
  unsigned char data[data_size]{0};

  if(device_id_size == 0 || uuid_size == 0) {
    ESP_LOGE(TAG, "Cannot pair if device_id and uuid are not set");
    return;
  }

  if(device_id_size + 6 + uuid_size > data_size) {
    ESP_LOGE(TAG, "Size of device_id + uuid is too big");
    return;
  }
  
  memcpy(data, this->uuid.c_str(), uuid_size);
  memcpy(&data[uuid_size], this->local_key, 6);
  memcpy(&data[uuid_size + 6], this->device_id.c_str(), device_id_size);

  this->client->write_data(TuyaBLECode::FUN_SENDER_PAIR, &this->seq_num, data, data_size, this->session_key);
}

void TuyaBLENode::request_info() {
  
  // About to get DEVICE_INFO, this should be limited to whenever session_key is unusable:
  if(!this->has_session_key()) { // TODO: OR when session_key is expired
    ESP_LOGD(TAG, "Requesting device info...");

    this->client->write_data(TuyaBLECode::FUN_SENDER_DEVICE_INFO, &this->seq_num, {0}, 0, this->login_key, 0, 2);
  }
}

void TuyaBLENode::reset_session_key() {
  std::fill(this->session_key, this->session_key + KEY_SIZE, 0);
}

void TuyaBLENode::toggle(bool value) {

  if(!this->has_client) {
    ESP_LOGW(TAG, "No client registered at node");
    return;
  }

  TYBLECommand command = {
    TuyaBLECode::FUN_SENDER_DPS,
    { 0x14, 0x01, 0x01, (unsigned char)value },
    this->session_key, // Since we don't mind if session_key gets updated, we directly use the pointer to this node's session_key
    0,
    3,
  };

  this->enqueue_command(&command);
}

}  // namespace tuya_ble_node
}  // namespace esphome
