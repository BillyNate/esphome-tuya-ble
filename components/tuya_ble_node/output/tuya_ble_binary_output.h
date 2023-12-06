#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/components/tuya_ble_tracker/common.h"

#include <esp_gattc_api.h>
namespace esphome {
namespace tuya_ble {

class TuyaBleBinaryOutput : public output::BinaryOutput, public Component {
  public:
    void dump_config() override;
    void loop() override {}
    float get_setup_priority() const override { return setup_priority::DATA; }

    void register_node(TYBleNode *node) {
      ESP_LOGD("tuya_ble_binary_output", "Node registered in output!");
      this->node = node;
    }

  protected:
    void write_state(bool state) override;
    TYBleNode *node;
};

}  // namespace ble_client
}  // namespace esphome
