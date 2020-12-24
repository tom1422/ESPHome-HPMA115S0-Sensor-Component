# ESPHome-HPMA115S0-Sensor-Component
Integration for the HPMA115S0 Dust and Particlee Sensor into ESPHome

## How to install
1. Download this repo
2. Extract the "custom_components" folder and place it in your esphome directory (where your .yaml files are works for me)
3. Add this to your ESPHome Project and you should be done:
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
