#pragma once

#include <algorithm>
#include <iterator>
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

using namespace esphome::tuya_ble;
using md5::MD5Digest;

enum DataCollectionState {
  NO_DATA,
  EXPECTING,
  COLLECTING,
  COLLECTED
};

/** UUID for Bluetooth GATT information service */
static std::string uuid_info_service = "00001910-0000-1000-8000-00805f9b34fb";
/** UUID for Bluetooth GATT notification characteristic */
static std::string uuid_notification_char = "00002b10-0000-1000-8000-00805f9b34fb";
/** UUID for Bluetooth GATT command characteristic */
static std::string uuid_write_char = "00002b11-0000-1000-8000-00805f9b34fb";

class TuyaBLEClient : public esp32_ble_client::BLEClientBase, virtual public TYBLEClient {

  //DeviceInfoResolver *device_info_resolver = new DeviceInfoResolver();

  esp32_ble_client::BLECharacteristic *notification_char;
  esp32_ble_client::BLECharacteristic *write_char;

  std::map<uint64_t, TYBLENode*> nodes;
  std::map<uint64_t, TYBLENode*>::iterator nodes_i = nodes.begin();
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

    bool connected() { return esp32_ble_client::BLEClientBase::state_ == esp32_ble_tracker::ClientState::ESTABLISHED; }

    void disconnect() { esp32_ble_client::BLEClientBase::disconnect(); }

    esp32_ble_tracker::ClientState state() const { return esp32_ble_client::BLEClientBase::state(); }

    // Override existing methods:
    bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;

    void on_shutdown() override;

    void loop() override;

    // Declare our own methods:
    static void encrypt_data(uint32_t seq_num, TuyaBLECode code, unsigned char *data, size_t size, unsigned char *encrypted_data, size_t encrypted_size, unsigned char *key, unsigned char *iv, uint32_t response_to, uint8_t security_flag = 0x05);

    static std::tuple<uint32_t, TuyaBLECode, size_t, uint32_t> decrypt_data(unsigned char *encrypted_data, size_t encrypted_size, unsigned char *data, size_t size, unsigned char *key, unsigned char *iv);
  
    void register_node(uint64_t mac_address, TYBLENode *tuyaBLENode);
    
    void set_disconnect_after(uint16_t disconnect_after);

    bool has_node(uint64_t mac_address);

    void connect_mac_address(const uint64_t mac_address);

    TYBLENode *get_node(uint64_t mac_address);

    void disconnect_when_appropriate();

    void set_disconnect_callback(std::function<void()> &&f);

    // write_data should actually not be public, but it needs to be accessible by BLENode. Maybe setting it as a callback would be a better solution...
    void write_data(TuyaBLECode code, uint32_t *seq_num, unsigned char *data, size_t size, unsigned char *key, uint32_t response_to = 0, int protocol_version = 3);

  protected:

    DataCollectionState data_collection_state = DataCollectionState::NO_DATA;

    bool should_disconnect = false;

    uint16_t disconnect_after = 0;

    uint32_t should_disconnect_timer = 0;

    static void write_to_char(esp32_ble_client::BLECharacteristic *write_char, unsigned char *encrypted_data, size_t encrypted_size);

    void collect_data(unsigned char *data, size_t size);

    void process_data(TYBLENode *node);

    void register_for_notifications();

    void disconnect_check();
};

}  // namespace tuya_ble_client
}  // namespace esphome
