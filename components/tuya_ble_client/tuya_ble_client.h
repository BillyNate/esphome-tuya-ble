#pragma once

#include <map>
#include <tuple>
#include "aes/esp_aes.h"
#include "esphome/core/log.h"
#include "esphome/components/esp32_ble_client/ble_client_base.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/md5/md5.h"
#include "esphome/components/tuya_ble_tracker/tuya_ble_tracker.h"
#include "esphome/components/tuya_ble_tracker/common.h"

namespace esphome {
namespace tuya_ble_client {

using md5::MD5Digest;

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

enum DataCollectionState {
  NO_DATA,
  COLLECTING,
  COLLECTED
};

/** UUID for Bluetooth GATT information service */
static std::string uuid_info_service = "00001910-0000-1000-8000-00805f9b34fb";
/** UUID for Bluetooth GATT notification characteristic */
static std::string uuid_notification_char = "00002b10-0000-1000-8000-00805f9b34fb";
/** UUID for Bluetooth GATT command characteristic */
static std::string uuid_write_char = "00002b11-0000-1000-8000-00805f9b34fb";

class TuyaBleClient : public esp32_ble_client::BLEClientBase, virtual public tuya_ble_tracker::TYBleClient {

  //DeviceInfoResolver *device_info_resolver = new DeviceInfoResolver();

  esp32_ble_client::BLECharacteristic *notification_char;
  esp32_ble_client::BLECharacteristic *write_char;

  std::map<uint64_t, tuya_ble_tracker::TYBleNode*> nodes{};
  std::vector<unsigned char> data_collected;
  uint8_t data_collection_incrementor = 0;
  uint32_t data_collection_expected_size = 0;

  std::function<void()> disconnect_callback;
  
  virtual void set_state(esp32_ble_tracker::ClientState st) override;

  public:
    // Solve ambiguous methods:
    void set_address(uint64_t address) { esp32_ble_client::BLEClientBase::set_address(address); }

    bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) { return esp32_ble_client::BLEClientBase::parse_device(device); }

    void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) { esp32_ble_client::BLEClientBase::gap_event_handler(event, param); }

    void connect() { esp32_ble_client::BLEClientBase::connect(); }

    void disconnect() { esp32_ble_client::BLEClientBase::disconnect(); }

    // Override existing methods:
    bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;

    void on_shutdown() override;

    void loop() override;

    // Declare our own methods:
    static void encrypt_data(uint32_t seq_num, TuyaBLECode code, unsigned char *data, size_t size, unsigned char *encrypted_data, size_t encrypted_size, unsigned char *key, unsigned char *iv, uint32_t response_to, uint8_t security_flag = 0x05);

    static std::tuple<uint32_t, TuyaBLECode, size_t, uint32_t> decrypt_data(unsigned char *encrypted_data, size_t encrypted_size, unsigned char *data, size_t size, unsigned char *key, unsigned char *iv);
  
    void register_node(uint64_t mac_address, tuya_ble_tracker::TYBleNode *tuyaBleNode);
    
    void set_disconnect_after(uint16_t disconnect_after);

    bool has_node(uint64_t mac_address);

    void connect_device(const esp32_ble_tracker::ESPBTDevice &device);

    tuya_ble_tracker::TYBleNode *get_node(uint64_t mac_address);

    void node_request_info(uint64_t mac_address);

    void node_switch(uint64_t mac_address, bool value);

    bool node_has_session_key(uint64_t mac_address);

    void disconnect_when_appropriate();

    void set_disconnect_callback(std::function<void()> &&f);

  protected:

    DataCollectionState data_collection_state = DataCollectionState::NO_DATA;

    bool should_disconnect = false;

    uint16_t disconnect_after = 0;

    uint32_t should_disconnect_timer = 0;

    static void write_to_char(esp32_ble_client::BLECharacteristic *write_char, unsigned char *encrypted_data, size_t encrypted_size);

    void write_data(TuyaBLECode code, uint32_t *seq_num, unsigned char *data, size_t size, unsigned char *key, uint32_t response_to = 0, int protocol_version = 3);

    void collect_data(unsigned char *data, size_t size);

    void process_data(uint64_t mac_address);

    void disconnect_check();
};

}  // namespace tuya_ble_client
}  // namespace esphome