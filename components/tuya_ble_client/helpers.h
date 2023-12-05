#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>

namespace esphome {
namespace tuya_ble {

  std::string binary_to_string(unsigned char *data, size_t size);

}  // namespace tuya_ble
}  // namespace esphome
