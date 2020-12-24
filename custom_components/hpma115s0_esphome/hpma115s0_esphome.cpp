#include "hpma115s0_esphome.h"
#include "esphome/core/log.h"

namespace esphome {
  namespace hpma115s0_esphome {

    bool success;
    float p25;
    float p10;

    bool launchSuccess = false;

    long waitLast = 0; //Do not change
    long waitTime = 300; //Change to how long you want the module to wait before giving up COM in ms

    int comWaitStatus = 0; //0 - Off - Default, 1 - Available - Can use Com Line, 2 - Unavailable (Timeout - Exit Code), 3 - Waiting - Wait for response

    static const char *TAG = "hpma115s0";

    void hpma115S0CustomSensor::dump_config() {
      if (launchSuccess) {
        ESP_LOGCONFIG(TAG, "Sensor found! Will read and log values.");
      } else {
        ESP_LOGE(TAG, "HPMA115S0 Particle Sensor not found!");
      }
    }

    void hpma115S0CustomSensor::setup() {
      if (stop_autosend()) {
        ESP_LOGI(TAG, "Successfully Stopped AutoSend");
        if (start_measurement()) {
          ESP_LOGI(TAG, "Successfully Started Mesaurement");
          launchSuccess = true;
        } else {
          ESP_LOGI(TAG, "Starting Mesaurement Failed!");
        }
      } else {
        ESP_LOGI(TAG, "Stopping AutoSend Failed - Sensor may not be present.");
      }
    }

    void hpma115S0CustomSensor::loop() {
    }

    void hpma115S0CustomSensor::update() {
      if (launchSuccess) {
        success = read_values(&p25, &p10);
        if (success == 1) {
          this->pm_2_5_sensor_->publish_state(p25);
          this->pm_10_0_sensor_->publish_state(p10);
        } else {
          ESP_LOGW(TAG, "Read Value Failure");
        }
      } else {
        ESP_LOGI(TAG, "Not Updating! launchSuccess is false");
      }
    }

    float hpma115S0CustomSensor::get_setup_priority() const { return setup_priority::LATE; }

    int hpma115S0CustomSensor::comAvailableRead(int availableMustBeLowerThan) {
      if (comWaitStatus != 1 && comWaitStatus != 3) {
        waitLast = millis();
        comWaitStatus = 3; //Set status to waiting for availability
      }
      if (this->available() < availableMustBeLowerThan) {
        //Check for timeout
        if ((millis() - waitLast) >= waitTime) {
          comWaitStatus = 2; //Set comWaitStatus to Unavailable (Due To Timeout)
        }
      } else {
        comWaitStatus = 1; //Set comWaitStatus to Available
      }
      return comWaitStatus;
    }

    bool hpma115S0CustomSensor::read_values (float *p25, float *p10) {
      // Send the command 0x68 0x01 0x04 0x93
      byte read_particle[] = {0x68, 0x01, 0x04, 0x93 };
      this->write_array(read_particle, sizeof(read_particle));
      // A measurement can return 0X9696 for NACK
      // Or can return eight bytes if successful
      // We wait for the first two bytes
      while(comAvailableRead(1) == 3);
      if (comWaitStatus == 2) return 0;
      byte HEAD = this->read();

      while(comAvailableRead(1) == 3);
      if (comWaitStatus == 2) return 0;
      byte LEN = this->read();
      // Test the response
      if ((HEAD == 0x96) && (LEN == 0x96)){
        // NACK
        return 0;
      } else if ((HEAD == 0x40) && (LEN == 0x05)) {
        // The measuremet is valid, read the rest of the data 
        // wait for the next byte
        while(comAvailableRead(1) == 3);
        if (comWaitStatus == 2) return 0;
        byte COMD = this->read();
        while(comAvailableRead(1) == 3);
        if (comWaitStatus == 2) return 0;
        byte DF1 = this->read(); 
        while(comAvailableRead(1) == 3);
        if (comWaitStatus == 2) return 0;
        byte DF2 = this->read();     
        while(comAvailableRead(1) == 3);
        if (comWaitStatus == 2) return 0;
        byte DF3 = this->read();   
        while(comAvailableRead(1) == 3);
        if (comWaitStatus == 2) return 0;
        byte DF4 = this->read();     
        while(comAvailableRead(1) == 3);
        if (comWaitStatus == 2) return 0;
        byte CS = this->read();      
        // Now we shall verify the checksum
        if (((0x10000 - HEAD - LEN - COMD - DF1 - DF2 - DF3 - DF4) % 0XFF) != CS){
          //Checksum Fail
          return 0;
        } else {
          // Checksum OK, we compute PM2.5 and PM10 values
          *p25 = DF1 * 256 + DF2;
          *p10 = DF3 * 256 + DF4;
          return 1;
        }
      }
      return 0;
    }

    bool hpma115S0CustomSensor::start_measurement (void) {
      ESP_LOGI(TAG, "Attempting to Start Measurement for HPMA115S0");
      // First, we send the command
      uint8_t start_measurement[] = {0x68, 0x01, 0x01, 0x96 };
      this->write_array(start_measurement, sizeof(start_measurement));
      //Then we wait for the response
      while(comAvailableRead(2) == 3);
      if (comWaitStatus == 2) return 0;

      byte read1 = this->read();
      byte read2 = this->read();
      // Test the response
      if ((read1 == 0xA5) && (read2 == 0xA5)) {
        // ACK
        return 1;
      } else if ((read1 == 0x96) && (read2 == 0x96)) {
        // NACK
        return 0;
      } else return 0;
    }

    bool hpma115S0CustomSensor::stop_measurement (void) {
      ESP_LOGI(TAG, "Attempting to Stop Measurement for HPMA115S0");
      // First, we send the command
      byte stop_measurement[] = {0x68, 0x01, 0x02, 0x95 };
      this->write_array(stop_measurement, sizeof(stop_measurement));
      //Then we wait for the response
      while(comAvailableRead(2) == 3);
      if (comWaitStatus == 2) return 0;

      char read1 = this->read();
      char read2 = this->read();
      // Test the response
      if ((read1 == 0xA5) && (read2 == 0xA5)){
        // ACK
        return 1;
      } else if ((read1 == 0x96) && (read2 == 0x96)) {
        // NACK
        return 0;
      } else return 0;
    }

    bool hpma115S0CustomSensor::stop_autosend(void) {
      // Stop auto send
      ESP_LOGI(TAG, "Attempting to Stop Autosend for HPMA115S0");
      
      byte stop_autosend[] = {0x68, 0x01, 0x20, 0x77 };
      this->write_array(stop_autosend, sizeof(stop_autosend));
      
      //Then we wait for the response
      while(comAvailableRead(2) == 3);
      if (comWaitStatus == 2) return 0;

      byte read1 = this->read();
      byte read2 = this->read();
      // Test the response
      if ((read1 == 0xA5) && (read2 == 0xA5)){
        // ACK
        return 1;
      }
      else if ((read1 == 0x96) && (read2 == 0x96)) {
        // NACK
        return 0;
      } else return 0;
    }
    
    bool hpma115S0CustomSensor::enable_autosend(void) {
      ESP_LOGI(TAG, "Attempting to Enable Autosend for HPMA115S0");
      // Start auto send
      byte start_autosend[] = {0x68, 0x01, 0x40, 0x57 };
      this->write_array(start_autosend, sizeof(start_autosend));
      //Then we wait for the response
      while(comAvailableRead(2) == 3);
      if (comWaitStatus == 2) return 0;

      byte read1 = this->read();
      byte read2 = this->read();
      // Test the response
      if ((read1 == 0xA5) && (read2 == 0xA5)){
        // ACK
        return 1;
      } else if ((read1 == 0x96) && (read2 == 0x96)) {
        // NACK
        return 0;
      } else return 0;
    }

  }
}
