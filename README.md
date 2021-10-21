# ESPHome-HPMA115S0-Sensor-Component
Integration for the HPMA115S0 Dust and Particlee Sensor into ESPHome

## How to install
1. Download this repo through main branch or the releases page.
2. Extract the "custom_components" folder and place it in your esphome directory (where your .yaml files are works for me)
3. Add the sensor configuration to your ESPHome Project:
```
  - platform: hpma115s0_esphome
    pm_2_5:
      name: "Particulate Matter <2.5µm Concentration"
      unit_of_measurement: "<2.5µm"
      icon:  "mdi:fan"
    pm_10_0:
      name: "Particulate Matter <10.0µm Concentration"
      unit_of_measurement: "<10.0µm"
      icon: "mdi:fan"
    update_interval: 5s
```
4. Next add the uart configuration near the top of your ESPHome Project file:
```
uart:
  id: uart_bus
  tx_pin: 9 #Change tx_pin and rx_pin to the ones you're using
  rx_pin: 10
  baud_rate: 9600
```

## Issues
Please report issues/improvements in the issues tab along with any debug info you find. Thank you.

## Releases
To see version history, [click here](https://github.com/tom1422/ESPHome-HPMA115S0-Sensor-Component/releases).
