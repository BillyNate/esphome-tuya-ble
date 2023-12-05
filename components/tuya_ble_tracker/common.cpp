#include "common.h"

namespace esphome {
namespace tuya_ble {

  std::string binary_to_string(unsigned char *data, size_t size) {
    std::stringstream buffer;
    for(int i=0; i<size; i++) {
      buffer << std::hex << std::setfill('0');
      buffer << std::setw(2) << static_cast<unsigned>(data[i]);
    }
    std::string hexString = buffer.str();
    buffer.clear();
    return hexString;
  }

}  // namespace tuya_ble
}  // namespace esphome
