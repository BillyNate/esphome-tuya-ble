#include "tuya_ble_client.h"

namespace esphome {
namespace tuya_ble {

static const char *const TAG = "tuya_ble_client";

void TuyaBleClient::set_state(esp32_ble_tracker::ClientState st) {
  this->state_ = st;
  switch(st) {
    case esp32_ble_tracker::ClientState::INIT:
      ESP_LOGI(TAG, "INIT");
      break;
    case esp32_ble_tracker::ClientState::DISCONNECTING:
      ESP_LOGI(TAG, "DISCONNECTING");
      break;
    case esp32_ble_tracker::ClientState::IDLE:
      ESP_LOGI(TAG, "IDLE");
      break;
    case esp32_ble_tracker::ClientState::SEARCHING:
      ESP_LOGI(TAG, "SEARCHING");
      break;
    case esp32_ble_tracker::ClientState::DISCOVERED:
      ESP_LOGI(TAG, "DISCOVERED");
      break;
    case esp32_ble_tracker::ClientState::READY_TO_CONNECT:
      ESP_LOGI(TAG, "READY_TO_CONNECT");
      break;
    case esp32_ble_tracker::ClientState::CONNECTING:
      ESP_LOGI(TAG, "CONNECTING");
      break;
    case esp32_ble_tracker::ClientState::CONNECTED:
      ESP_LOGI(TAG, "CONNECTED");
      break;
    case esp32_ble_tracker::ClientState::ESTABLISHED:
      ESP_LOGI(TAG, "ESTABLISHED");
      break;
    default:
      ESP_LOGI(TAG, "Unknown state");
      break;
  }
}

void TuyaBleClient::encrypt_data(uint32_t seq_num, TuyaBLECode code, unsigned char *data, size_t size, unsigned char *encrypted_data, size_t encrypted_size, unsigned char *key, uint32_t response_to, uint8_t security_flag) {
  
  unsigned char iv[IV_SIZE]{0};// = { 0x7c, 0xbc, 0x56, 0x4c, 0x86, 0xcf, 0xac, 0x73, 0x8d, 0x28, 0x9f, 0x5d, 0xaa, 0xfa, 0x36, 0x6a };
  esp_fill_random(iv, 16);
  size_t inflated_size = META_SIZE + size + CRC_SIZE + (16 - ((META_SIZE + size + CRC_SIZE) % 16)); // 12 bytes of meta data + size of data + 2 bytes crc + padding
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
  /*
  std::stringstream buffer;
  for(int i=0; i<inflated_size; i++) {
    buffer << std::hex << std::setfill('0');
    buffer << std::setw(2) << static_cast<unsigned>(raw[i]);
  }
  std::string hexString = buffer.str();
  buffer.clear();
  buffer.str(std::string());

  ESP_LOGI(TAG, "%s", hexString.c_str());
  */
  encrypted_data[0] = (unsigned char)security_flag;
  memcpy(&encrypted_data[1], iv, IV_SIZE);

  esp_aes_context aes;
  esp_aes_init(&aes);
  esp_aes_setkey(&aes, (const unsigned char *) key, KEY_SIZE * 8);
  esp_aes_crypt_cbc(&aes, ESP_AES_ENCRYPT, sizeof(raw), iv, (uint8_t*)raw, (uint8_t*)&encrypted_data[sizeof(security_flag) + IV_SIZE]);
  esp_aes_free(&aes);
}

void TuyaBleClient::write_to_char(esp32_ble_client::BLECharacteristic *write_char, unsigned char *encrypted_data, size_t encrypted_size) {
  /*
  std::stringstream buffer;
  for(int i=0; i<encrypted_size; i++) {
    buffer << std::hex << std::setfill('0');
    buffer << std::setw(2) << static_cast<unsigned>(encrypted_data[i]);
  }
  std::string hexString2 = buffer.str();
  buffer.clear();

  ESP_LOGI(TAG, "%s", hexString2.c_str());
  */
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

  size_t encrypted_size = META_SIZE + size + CRC_SIZE + (16 - ((META_SIZE + size + CRC_SIZE) % 16)) + 1 + IV_SIZE;
  unsigned char encrypted_data[encrypted_size + 2]{0};
  uint8_t security_flag = 0x05;
  if(protocol_version == 2) {
    security_flag = 0x04; // adjust security flag for some reason
  }

  encrypt_data(*seq_num, code, data, size, &encrypted_data[2], encrypted_size, key, response_to, security_flag);

  encrypted_data[0] = (uint8_t)(encrypted_size);
  encrypted_data[1] = (protocol_version << 4);

  write_to_char(this->write_char, encrypted_data, encrypted_size + 2);

  (*seq_num)++;
}

void TuyaBleClient::register_device(uint64_t mac_address, const char *local_key, uint64_t login_key_a, uint64_t login_key_b) {

  struct TuyaBleDevice tuyaBleDevice = { .login_key = {0}, .seq_num = 1, .rssi = 0 };
  tuyaBleDevice.login_key[ 0] = (login_key_a >> 56) & 0xff;
  tuyaBleDevice.login_key[ 1] = (login_key_a >> 48) & 0xff;
  tuyaBleDevice.login_key[ 2] = (login_key_a >> 40) & 0xff;
  tuyaBleDevice.login_key[ 3] = (login_key_a >> 32) & 0xff;
  tuyaBleDevice.login_key[ 4] = (login_key_a >> 24) & 0xff;
  tuyaBleDevice.login_key[ 5] = (login_key_a >> 16) & 0xff;
  tuyaBleDevice.login_key[ 6] = (login_key_a >>  8) & 0xff;
  tuyaBleDevice.login_key[ 7] = (login_key_a >>  0) & 0xff;
  
  tuyaBleDevice.login_key[ 8] = (login_key_b >> 56) & 0xff;
  tuyaBleDevice.login_key[ 9] = (login_key_b >> 48) & 0xff;
  tuyaBleDevice.login_key[10] = (login_key_b >> 40) & 0xff;
  tuyaBleDevice.login_key[11] = (login_key_b >> 32) & 0xff;
  tuyaBleDevice.login_key[12] = (login_key_b >> 24) & 0xff;
  tuyaBleDevice.login_key[13] = (login_key_b >> 16) & 0xff;
  tuyaBleDevice.login_key[14] = (login_key_b >>  8) & 0xff;
  tuyaBleDevice.login_key[15] = (login_key_b >>  0) & 0xff;

  this->devices.insert(std::make_pair(mac_address, tuyaBleDevice));
  
  ESP_LOGI(TAG, "Added: %llu from config", mac_address);
  /*
  std::stringstream buffer;
  for(int i=0; i<16; i++) {
    buffer << std::hex << std::setfill('0');
    buffer << std::setw(2) << static_cast<unsigned>(tuyaBleDevice.login_key[i]);
  }
  std::string hexString = buffer.str();
  buffer.clear();
  buffer.str(std::string());

  ESP_LOGI(TAG, "Login key: %s", hexString.c_str());
  */
}

void TuyaBleClient::device_request_info(uint64_t mac_address) {
  this->notification_char = this->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw(uuid_info_service), esp32_ble_tracker::ESPBTUUID::from_raw(uuid_notification_char));
  this->write_char = this->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw(uuid_info_service), esp32_ble_tracker::ESPBTUUID::from_raw(uuid_write_char));

  TuyaBleDevice *device = this->get_device(mac_address);

  ESP_LOGD(TAG, "Listen for notifications");
  esp_err_t status = esp_ble_gattc_register_for_notify(this->get_gattc_if(), this->get_remote_bda(), this->notification_char->handle);
  if(status) {
    ESP_LOGW(TAG, "[%d] [%s] esp_ble_gattc_register_for_notify failed, status=%d", this->get_conn_id(), this->address_str_.c_str(), status);
  }

  // About to get DEVICE_INFO, this should be limited to whenever session_key is unusable:

  TuyaBLECode code = TuyaBLECode::FUN_SENDER_DEVICE_INFO;
  size_t data_size = 0;
  unsigned char data[data_size]{0};
  int protocol_version = 2; // protocol version is usually 3

  this->write_data(code, &device->seq_num, data, data_size, device->login_key, 0, protocol_version);
}

bool TuyaBleClient::has_device(uint64_t mac_address) {
  return this->devices.count(mac_address) > 0;
}

TuyaBleDevice *TuyaBleClient::get_device(uint64_t mac_address) {
  return &this->devices[mac_address];
}

void TuyaBleClient::on_shutdown() {
}

bool TuyaBleClient::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  ESP_LOGI(TAG, "[%d] [%s] gattc_event_handler: event=%d gattc_if=%d", this->connection_index_, this->address_str_.c_str(), event, gattc_if);

  if (!esp32_ble_client::BLEClientBase::gattc_event_handler(event, gattc_if, param))
    return false;
  
  switch (event) {
    case ESP_GATTC_DISCONNECT_EVT: {
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
    case ESP_GATTC_OPEN_EVT: {
      if(this->state_ == esp32_ble_tracker::ClientState::ESTABLISHED) {
        this->device_request_info(this->get_address());
      }
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      std::string notification = std::string((char *) param->notify.value, param->notify.value_len);
      ESP_LOGI(TAG, "Notification received!");
      break;
    }
  }
  return true;
}

void TuyaBleClient::set_disconnect_callback(std::function<void()> &&f) { this->disconnect_callback = std::move(f); }
/*
void TuyaBleClient::loop() {
  esp32_ble_client::BLEClientBase::loop();
}
*/
}  // namespace tuya_ble
}  // namespace esphome