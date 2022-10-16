#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace hpma115S0_esphome {

static const uint8_t SELECT_COMM_CMD = 0X88;

class HPMA115S0Component : public PollingComponent, public uart::UARTDevice {
 public:
  HPMA115S0Component() = default;

  void set_pm_2_5_sensor(sensor::Sensor *pm_2_5_sensor) { pm_2_5_sensor_ = pm_2_5_sensor; }
  void set_pm_10_0_sensor(sensor::Sensor *pm_10_0_sensor) { pm_10_0_sensor_ = pm_10_0_sensor; }

  void set_pm_4_0_sensor(sensor::Sensor *pm_4_0_sensor) { pm_4_0_sensor_ = pm_4_0_sensor; }
  void set_pm_1_0_sensor(sensor::Sensor *pm_1_0_sensor) { pm_1_0_sensor_ = pm_1_0_sensor; }


  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void update() override;

 protected:
  sensor::Sensor *pm_2_5_sensor_{nullptr};
  sensor::Sensor *pm_10_0_sensor_{nullptr};

  sensor::Sensor *pm_4_0_sensor_{nullptr};
  sensor::Sensor *pm_1_0_sensor_{nullptr};


  bool success;
  float p25 = 0;
  float p10 = 0;
  float p4 = 0;
  float p1 = 0;

  bool launchSuccess = false;
  long waitLast = 0; //Do not change
  long waitTime = 300; //Change to how long you want the module to wait before giving up COM in ms (Default 300)
  int setup_SAS = 0; //1-Success, 2-Timeout, 3-NACK, 4-Malformed
  int setup_SM = 0; //1-Success, 2-Timeout, 3-NACK, 4-Malformed
  int setupTries = 0;

  //For debugging
  // long startupDelay = 8000;
  // long setupTime = 0;
  // void setupLate();


  int comWait(bool start, int minDataToRead);
  bool read_values(float *p25, float *p10, float *p4, float *p1);
  bool start_measurement(void);
  bool stop_measurement(void);
  bool stop_autosend(void);
  bool enable_autosend(void);
};

}  // namespace hpma115S0_esphome
}  // namespace esphome
