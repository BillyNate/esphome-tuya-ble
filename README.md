# ESPHome component: Tuya BLE

An ESPhome component (https://esphome.io/components/external_components.html#git) for controlling BLE (Bluetooth) enabled devices from Tuya.

For an example yaml see [`tuya_ble_light-example_config.yaml`](tuya_ble_light-example_config.yaml).

### Prerequisite
In order to be able to connect to a device the mac address of the device is needed and the corresponding local key from Tuya.  
If the device id and uuid are set in the config as well, a pairing request will be issued.  
To obtain the device's local key you will need a Tuya IoT dev account and some tools. Please search the internet for more information on how to obtain a local key.  
This component is focused around the ESP32.

### Current state
This component is *not* fully functional! It only supports basic setup for controlling a light (like an ESPHome binary output).  
So connecting to a device is possible and encrypted data can be send and received. But any other feature still needs to be added.  

### Development of this component would not have been possible without:
- [ESPHome](https://github.com/esphome/esphome)
- [Home Assistant support for Tuya BLE devices](https://github.com/PlusPlus-ua/ha_tuya_ble)
- [ESPHome component: AwoX BLE mesh (mqtt) hub](https://github.com/fsaris/EspHome-AwoX-BLE-mesh-hub)
