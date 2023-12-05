#include "tuya_ble_client.h"

namespace esphome {
namespace tuya_ble_client {

static const char *const TAG = "tuya_ble_client";

void TuyaBleClient::set_state(esp32_ble_tracker::ClientState st) {
  esp32_ble_client::BLEClientBase::set_state(st);
  
  switch(st) {
    case esp32_ble_tracker::ClientState::INIT:
      ESP_LOGD(TAG, "INIT");
      break;
    case esp32_ble_tracker::ClientState::DISCONNECTING:
      ESP_LOGD(TAG, "DISCONNECTING");
      break;
    case esp32_ble_tracker::ClientState::IDLE:
      ESP_LOGD(TAG, "IDLE");
      break;
    case esp32_ble_tracker::ClientState::SEARCHING:
      ESP_LOGD(TAG, "SEARCHING");
      break;
    case esp32_ble_tracker::ClientState::DISCOVERED:
      ESP_LOGD(TAG, "DISCOVERED");
      break;
    case esp32_ble_tracker::ClientState::READY_TO_CONNECT:
      ESP_LOGD(TAG, "READY_TO_CONNECT");
      break;
    case esp32_ble_tracker::ClientState::CONNECTING:
      ESP_LOGD(TAG, "CONNECTING");
      break;
    case esp32_ble_tracker::ClientState::CONNECTED:
      ESP_LOGD(TAG, "CONNECTED");
      break;
    case esp32_ble_tracker::ClientState::ESTABLISHED:
      ESP_LOGD(TAG, "ESTABLISHED");
      break;
    default:
      ESP_LOGD(TAG, "Unknown state");
      break;
  }
}

void TuyaBleClient::encrypt_data(uint32_t seq_num, TuyaBLECode code, unsigned char *data, size_t size, unsigned char *encrypted_data, size_t encrypted_size, unsigned char *key, unsigned char *iv, uint32_t response_to, uint8_t security_flag) {

  size_t inflated_size = META_SIZE + size + CRC_SIZE + (AES_BLOCK_SIZE - ((META_SIZE + size + CRC_SIZE) % AES_BLOCK_SIZE)); // 12 bytes of meta data + size of data + 2 bytes crc + padding
  unsigned char raw[inflated_size]{0};

  if(inflated_size + sizeof(security_flag) + IV_SIZE > encrypted_size) {
    ESP_LOGE(TAG, "Not enough space allocated for encrypted data!");
    return;
  }

  memcpy(&raw[12], data, size);

  raw[0] = (seq_num >> 24) & 0xff;
  raw[1] = (seq_num >> 16) & 0xff;
  raw[2] = (seq_num >>  8) & 0xff;
  raw[3] = (seq_num >>  0) & 0xff;

  raw[4] = (response_to >> 24) & 0xff;
  raw[5] = (response_to >> 16) & 0xff;
  raw[6] = (response_to >>  8) & 0xff;
  raw[7] = (response_to >>  0) & 0xff;

  raw[8] = (code >> 8) & 0xff;
  raw[9] = (code >> 0) & 0xff;

  raw[10] = (size >> 8) & 0xff;
  raw[11] = (size >> 0) & 0xff;

  uint16_t crc = crc16(raw, size + 12);

  raw[12 + size] = (crc >> 8) & 0xff;
  raw[13 + size] = (crc >> 0) & 0xff;
  
  ESP_LOGV(TAG, "%s", binary_to_string(raw, inflated_size).c_str());
  
  encrypted_data[0] = (unsigned char)security_flag;
  memcpy(&encrypted_data[1], iv, IV_SIZE);

  esp_aes_context aes;
  esp_aes_init(&aes);
  esp_aes_setkey(&aes, (const unsigned char *) key, KEY_SIZE * 8);
  esp_aes_crypt_cbc(&aes, ESP_AES_ENCRYPT, sizeof(raw), iv, (uint8_t*)raw, (uint8_t*)&encrypted_data[sizeof(security_flag) + IV_SIZE]);
  esp_aes_free(&aes);
}

std::tuple<uint32_t, TuyaBLECode, size_t, uint32_t> TuyaBleClient::decrypt_data(unsigned char *encrypted_data, size_t encrypted_size, unsigned char *data, size_t size, unsigned char *key, unsigned char *iv) {

  uint8_t security_flag = encrypted_data[0];
  uint32_t seq_num;
  TuyaBLECode code;
  size_t decrypted_size;
  uint32_t response_to;
  size_t deflated_size = encrypted_size - sizeof(security_flag) - IV_SIZE;

  esp_aes_context aes;
  esp_aes_init(&aes);
  esp_aes_setkey(&aes, (const unsigned char *)key, KEY_SIZE * 8);
  esp_aes_crypt_cbc(&aes, ESP_AES_DECRYPT, size, iv, (uint8_t*)encrypted_data, (uint8_t*)data);
  esp_aes_free(&aes);

  seq_num = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
  response_to = (data[4] << 24) + (data[5] << 16) + (data[6] << 8) + data[7];
  code = (TuyaBLECode)((data[8] << 8) + data[9]);
  decrypted_size = (data[10] << 8) + data[11];

  ESP_LOGV(TAG, "%s", binary_to_string(data, size).c_str());

  return std::make_tuple(seq_num, code, decrypted_size, response_to);
}

void TuyaBleClient::write_to_char(esp32_ble_client::BLECharacteristic *write_char, unsigned char *encrypted_data, size_t encrypted_size) {
  
  ESP_LOGV(TAG, "%s", binary_to_string(encrypted_data, encrypted_size).c_str());
  
  size_t pack_size = GATT_MTU - 1;

  for(int i=0; i<ceil(encrypted_size / (double)pack_size); i++) {
    unsigned char packet[GATT_MTU]{0};
    size_t data_size = pack_size;

    packet[0] = i;
    if(i * pack_size + pack_size > encrypted_size) {
      data_size = encrypted_size - (i * pack_size);
    }
    memcpy(&packet[1], &encrypted_data[i*pack_size], data_size);
    
    auto status = write_char->write_value(packet, data_size + 1, ESP_GATT_WRITE_TYPE_NO_RSP);
  }
}

void TuyaBleClient::write_data(TuyaBLECode code, uint32_t *seq_num, unsigned char *data, size_t size, unsigned char *key, uint32_t response_to, int protocol_version) {

  size_t encrypted_size = META_SIZE + size + CRC_SIZE + (AES_BLOCK_SIZE - ((META_SIZE + size + CRC_SIZE) % AES_BLOCK_SIZE)) + 1 + IV_SIZE;
  unsigned char encrypted_data[encrypted_size + 2]{0};
  uint8_t security_flag = Security::SESSION_KEY;
  if(protocol_version == 2) {
    security_flag = Security::LOGIN_KEY; // adjust security flag for some reason
  }

  unsigned char iv[IV_SIZE]{0};
  esp_fill_random(iv, IV_SIZE);
  encrypt_data(*seq_num, code, data, size, &encrypted_data[2], encrypted_size, key, (unsigned char *)iv, response_to, security_flag);

  encrypted_data[0] = (uint8_t)(encrypted_size);
  encrypted_data[1] = (protocol_version << 4);

  write_to_char(this->write_char, encrypted_data, encrypted_size + 2);

  (*seq_num)++;
}

void TuyaBleClient::collect_data(unsigned char *data, size_t size) {

  ESP_LOGV(TAG, "%s", binary_to_string(data, size).c_str());

  size_t data_starts_at = 1;
  size_t concatenated_length = 0;

  if(data[0] != 0x00 && data[0] != this->data_collection_incrementor + 1) {
    ESP_LOGW(TAG, "Received data packet with incorrect incrementor. Expected %i, got %i. Data rejected.", this->data_collection_incrementor + 1, data[0]);
    return;
  }
  
  this->data_collection_incrementor = data[0];

  if(data[0] == 0x00) { // if new sequence of packets is received; reset (do not wait for potential existing sequence to finish)
    this->data_collection_state = DataCollectionState::COLLECTING;
    this->data_collection_expected_size = data[1];
    if(data[1] >= 128) {
      data_starts_at ++;
      if(data[2] > 1) {
        ESP_LOGE(TAG, "Length (%i) of received data above 255. No code written to handle this!", data[1]);
        // TODO: Instead of this error message, the int from data[1] should be read as variable length and handled accordingly
      }
    }
    data_starts_at += 2;

    this->data_collected.clear();
    this->data_collected.reserve(this->data_collection_expected_size);
  }
  else {
    concatenated_length = this->data_collection_incrementor * 19 - 2;
    if(this->data_collection_expected_size > 127) {
      concatenated_length --;
    }
  }

  if(concatenated_length + (size - data_starts_at) > this->data_collection_expected_size) {
    ESP_LOGW(TAG, "Not enough space allocated for received data!");
    return;
  }
  memcpy(&this->data_collected[concatenated_length], &data[data_starts_at], size - data_starts_at);
  concatenated_length += (size - data_starts_at);
  ESP_LOGD(TAG, "Collected %i/%i", concatenated_length, this->data_collection_expected_size);
  if(concatenated_length >= this->data_collection_expected_size)
  {
    ESP_LOGD(TAG, "Data collected!");
    this->data_collection_state = DataCollectionState::COLLECTED;
  }
}

void TuyaBleClient::process_data(uint64_t mac_address) {
  if(this->data_collection_state != DataCollectionState::COLLECTED || this->data_collection_expected_size <= IV_SIZE + 1) { // Should be a multiple of 16 as well?
    ESP_LOGW(TAG, "Attempt to process received data aborted");
    return;
  }
  
  tuya_ble_tracker::TYBleNode *node = this->get_node(mac_address);

  uint8_t security_flag = (uint8_t)this->data_collected[0];

  unsigned char *key;
  if(security_flag == Security::LOGIN_KEY) {
    key = node->login_key;
  }
  else if(security_flag == Security::AUTH_KEY) {
    //TODO: set auth key?
  }
  else {
    key = node->session_key;
  }
  
  
  unsigned char first_decrypted_part[AES_BLOCK_SIZE]{0};
  size_t start_pos = sizeof(security_flag) + IV_SIZE;
  uint32_t seq_num;
  TuyaBLECode code;
  size_t decrypted_size;
  uint32_t response_to;
  std::tie(seq_num, code, decrypted_size, response_to) = decrypt_data(&this->data_collected[start_pos], this->data_collection_expected_size - start_pos, first_decrypted_part, AES_BLOCK_SIZE, key, &this->data_collected[1]);
  // get length now that it's decrypted...

  switch(code)
  {
    case TuyaBLECode::FUN_SENDER_DEVICE_INFO:
      decrypted_size = 32; // Only need the srand actually, but the auth_key is in here as well
      break;

    default:
      decrypted_size = this->data_collection_expected_size - sizeof(security_flag) - IV_SIZE - AES_BLOCK_SIZE;
      break;
  }

  if(decrypted_size > 0) {
    unsigned char decrypted_data[decrypted_size]{0};
    decrypt_data(&this->data_collected[start_pos + AES_BLOCK_SIZE], this->data_collection_expected_size - start_pos - AES_BLOCK_SIZE, decrypted_data, decrypted_size, key, &this->data_collected[1]);
  
    switch(code) {
      case TuyaBLECode::FUN_SENDER_DEVICE_INFO:
        {
          MD5Digest *md5digest = new MD5Digest();
    
          md5digest->init();
          md5digest->add(node->local_key, 6);
          md5digest->add(&decrypted_data[2], 6);
          md5digest->calculate();
          md5digest->get_bytes(&node->session_key[0]);
          ESP_LOGD(TAG, "Session key set!");

          ESP_LOGV(TAG, "%s", binary_to_string(node->session_key, KEY_SIZE).c_str());
        }
        break;

      default:
        break;
    }
  }
  
  this->data_collected.clear();
  this->data_collection_state = DataCollectionState::NO_DATA;
}

void TuyaBleClient::register_node(uint64_t mac_address, tuya_ble_tracker::TYBleNode *tuyaBleNode) {

  this->nodes.insert(std::make_pair(mac_address, tuyaBleNode));
  
  ESP_LOGD(TAG, "Added: %llu from config", mac_address);
}

void TuyaBleClient::set_disconnect_after(uint16_t disconnect_after) {

  this->disconnect_after = disconnect_after;
}

void TuyaBleClient::node_request_info(uint64_t mac_address) {
  this->notification_char = this->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw(uuid_info_service), esp32_ble_tracker::ESPBTUUID::from_raw(uuid_notification_char));
  this->write_char = this->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw(uuid_info_service), esp32_ble_tracker::ESPBTUUID::from_raw(uuid_write_char));

  tuya_ble_tracker::TYBleNode *node = this->get_node(mac_address);
  node->seq_num = 1;

  ESP_LOGD(TAG, "Listen for notifications");
  esp_err_t status = esp_ble_gattc_register_for_notify(this->get_gattc_if(), this->get_remote_bda(), this->notification_char->handle);
  if(status) {
    ESP_LOGW(TAG, "[%d] [%s] esp_ble_gattc_register_for_notify failed, status=%d", this->get_conn_id(), this->address_str_.c_str(), status);
  }

  // About to get DEVICE_INFO, this should be limited to whenever session_key is unusable:
  if(!this->node_has_session_key(mac_address)) { // TODO: OR when session_key is expired
    ESP_LOGD(TAG, "Requesting device info...");

    TuyaBLECode code = TuyaBLECode::FUN_SENDER_DEVICE_INFO;
    size_t data_size = 0;
    unsigned char data[data_size]{0};
    int protocol_version = 2; // protocol version is usually 3

    this->write_data(code, &node->seq_num, data, data_size, node->login_key, 0, protocol_version);
  }
}

void TuyaBleClient::node_switch(uint64_t mac_address, bool value) {
  tuya_ble_tracker::TYBleNode *node = this->get_node(mac_address);

  size_t dp_size = 4;
  unsigned char data[dp_size] = { 0x14, 0x01, 0x01, (unsigned char)value };

  this->write_data(TuyaBLECode::FUN_SENDER_DPS, &node->seq_num, data, dp_size, node->session_key);
}

bool TuyaBleClient::node_has_session_key(uint64_t mac_address) {
  tuya_ble_tracker::TYBleNode *node = this->get_node(mac_address);

  return !std::all_of(node->session_key, node->session_key + KEY_SIZE, [](unsigned char x) { return x == '\0'; });
}

bool TuyaBleClient::has_node(uint64_t mac_address) {
  return this->nodes.count(mac_address) > 0;
}

tuya_ble_tracker::TYBleNode *TuyaBleClient::get_node(uint64_t mac_address) {
  return this->nodes[mac_address];
}

void TuyaBleClient::on_shutdown() {
}

bool TuyaBleClient::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  ESP_LOGV(TAG, "[%d] [%s] gattc_event_handler: event=%d gattc_if=%d", this->connection_index_, this->address_str_.c_str(), event, gattc_if);

  if (!esp32_ble_client::BLEClientBase::gattc_event_handler(event, gattc_if, param))
    return false;
  
  uint64_t mac_address = this->get_address();

  switch (event) {
    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGD(TAG, "Disconnected!");

      if(this->node_has_session_key(mac_address)) {
        this->set_address(0);
      }
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
    case ESP_GATTC_OPEN_EVT: {
      if(esp32_ble_client::BLEClientBase::state_ == esp32_ble_tracker::ClientState::ESTABLISHED) {
        this->node_request_info(mac_address);
      }
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      ESP_LOGD(TAG, "Notification received!");
      this->collect_data(param->notify.value, param->notify.value_len);

      if(this->data_collection_state == DataCollectionState::COLLECTED) {

        this->process_data(mac_address);
        if(this->node_has_session_key(mac_address)) {
          this->disconnect_when_appropriate();
        }
      }
      break;
    }
  }
  return true;
}

void TuyaBleClient::connect_device(const esp32_ble_tracker::ESPBTDevice &device) {

  this->set_address(device.address_uint64());
  this->parse_device(device);
}

void TuyaBleClient::disconnect_when_appropriate() {
  this->should_disconnect = true;
  this->should_disconnect_timer = esphome::millis();
}

void TuyaBleClient::disconnect_check() {
  //ESP_LOGD(TAG, "disconnect_check. should_disconnect: %i, data_collection_state: %i, should_disconnect_timer: %i, millis: %i", this->should_disconnect, this->data_collection_state, this->should_disconnect_timer, esphome::millis());
  if(this->should_disconnect && this->data_collection_state == DataCollectionState::NO_DATA && esphome::millis() > this->should_disconnect_timer + this->disconnect_after) {
    //this->set_address(0);
    this->disconnect();
    //this->set_state(esp32_ble_tracker::ClientState::IDLE);
    this->should_disconnect = false;
  }
}

void TuyaBleClient::set_disconnect_callback(std::function<void()> &&f) { this->disconnect_callback = std::move(f); }

void TuyaBleClient::loop() {
  this->disconnect_check();
  // Prevent continuous reconnecting
  if(esp32_ble_client::BLEClientBase::state_ == esp32_ble_tracker::ClientState::READY_TO_CONNECT && this->get_address() != 0) {
    if(this->has_node(this->get_address())) {
      if(this->node_has_session_key(this->get_address())) { // TODO: OR when session_key is expired
        return;
      }
    }
  }
  esp32_ble_client::BLEClientBase::loop();
}

}  // namespace tuya_ble_client
}  // namespace esphome
