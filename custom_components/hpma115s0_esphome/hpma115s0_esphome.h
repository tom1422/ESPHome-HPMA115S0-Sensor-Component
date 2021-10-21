#pragma once

#include "esphome.h"
#include "HardwareSerial.h"
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

#include <string>
#include <sstream>

//https://github.com/tom1422/ESPHome-HPMA115S0-Sensor-Component -- VERSION 2

//TX 9
//RX 10
//HardwareSerial hpmaSerial(9600, SERIAL_8N1, 10, 9);

namespace esphome {
  namespace hpma115s0_esphome {

    class hpma115S0CustomSensor : public PollingComponent, public uart::UARTDevice {
    public:
      hpma115S0CustomSensor() = default;

      void set_pm_2_5_sensor(sensor::Sensor *pm_2_5_sensor) { pm_2_5_sensor_ = pm_2_5_sensor; }
      void set_pm_10_0_sensor(sensor::Sensor *pm_10_0_sensor) { pm_10_0_sensor_ = pm_10_0_sensor; }
      
      void setup() override;
      void dump_config() override;
      void loop() override;
      void update() override;
      float get_setup_priority() const override;

	  int comWait(bool start, int minDataToRead);

      bool read_values(float *p25, float *p10);
      bool stop_measurement();
      bool start_measurement();
      bool stop_autosend();
      bool enable_autosend();

    protected:
      sensor::Sensor *pm_2_5_sensor_{nullptr};
      sensor::Sensor *pm_10_0_sensor_{nullptr};
    };

  }
}