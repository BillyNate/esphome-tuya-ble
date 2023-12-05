#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

namespace esphome {
namespace tuya_ble {

#define KEY_SIZE 0x10
#define IV_SIZE 0x10
#define AES_BLOCK_SIZE 0x10
#define META_SIZE 0x0C
#define CRC_SIZE 0x02
#define GATT_MTU 0x14

enum TuyaBLECode {
  FUN_SENDER_DEVICE_INFO = 0x0000,
  FUN_SENDER_PAIR = 0x0001,
  FUN_SENDER_DPS = 0x0002,
  FUN_SENDER_DEVICE_STATUS = 0x0003,

  FUN_SENDER_UNBIND = 0x0005,
  FUN_SENDER_DEVICE_RESET = 0x0006,

  FUN_SENDER_OTA_START = 0x000C,
  FUN_SENDER_OTA_FILE = 0x000D,
  FUN_SENDER_OTA_OFFSET = 0x000E,
  FUN_SENDER_OTA_UPGRADE = 0x000F,
  FUN_SENDER_OTA_OVER = 0x0010,

  FUN_SENDER_DPS_V4 = 0x0027,

  FUN_RECEIVE_DP = 0x8001,
  FUN_RECEIVE_TIME_DP = 0x8003,
  FUN_RECEIVE_SIGN_DP = 0x8004,
  FUN_RECEIVE_SIGN_TIME_DP = 0x8005,

  FUN_RECEIVE_DP_V4 = 0x8006,
  FUN_RECEIVE_TIME_DP_V4 = 0x8007,

  FUN_RECEIVE_TIME1_REQ = 0x8011,
  FUN_RECEIVE_TIME2_REQ = 0x8012
};

enum Security {
  AUTH_KEY = 0x01,
  LOGIN_KEY = 0x04,
  SESSION_KEY = 0x05,
};

class TYBleNode {
  public:
    unsigned char local_key[6];
    unsigned char login_key[16];
    unsigned char session_key[16];
    // There's supposedly also an auth_key, but since it's not used, it's not declared either
    uint32_t seq_num;
    uint32_t last_detected;
    int rssi;
};

class TYBleClient {
  esp32_ble_tracker::ClientState state_;
  public:
    virtual TYBleNode *get_node(uint64_t mac_address);
    virtual bool has_node(uint64_t mac_address) = 0;
    virtual void connect_device(const esp32_ble_tracker::ESPBTDevice &device);
    virtual void set_address(uint64_t address) = 0;
    virtual bool connected() { return this->state_ == esp32_ble_tracker::ClientState::ESTABLISHED; }
    virtual void disconnect() = 0;
    virtual bool node_has_session_key(uint64_t mac_address);
    virtual void set_disconnect_callback(std::function<void()> &&f);
    virtual bool parse_device(const esp32_ble_tracker::ESPBTDevice &device);
    esp32_ble_tracker::ClientState state() const { return state_; }
};

  std::string binary_to_string(unsigned char *data, size_t size);

}  // namespace tuya_ble
}  // namespace esphome
