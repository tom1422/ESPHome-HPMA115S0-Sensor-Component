#include "hpma115s0_esphome.h"
#include "esphome/core/log.h"

namespace esphome {
  namespace hpma115s0_esphome {

    bool success;
    float p25;
    float p10;

    bool launchSuccess = false;

    long waitLast = 0; //Do not change
    long waitTime = 300; //Change to how long you want the module to wait before giving up COM in ms (Default 300)
	
	std::string debugString = "";
	std::string configString = "";
	
	int setup_SAS = 0; //1-Success, 2-Timeout, 3-NACK, 4-Malformed
	int setup_SM = 0; //1-Success, 2-Timeout, 3-NACK, 4-Malformed
	int setupTries = 0;

    static const char *TAG = "hpma115s0";

    void hpma115S0CustomSensor::dump_config() {
      if (launchSuccess) {
		configString = "Sensor found after " + to_string(setupTries) + " tries. Will read and log values.";
        ESP_LOGCONFIG(TAG, configString.c_str());
      } else {
		configString = "Error! HPMA115S0 Particle Sensor not found! Failed with: " + to_string(setup_SAS)  + " AND " + to_string(setup_SM) + ".";
        ESP_LOGE(TAG, configString.c_str());
      }
    }

    void hpma115S0CustomSensor::setup() {
      while (setupTries < 3) { // Loop 3 times in case sensor had a malfunction
	    setupTries++;
		//Clear buffer before trying
	    while(this->available()){
		  char getData = this->read();
	    }
        if (stop_autosend()) {
		  if (start_measurement()) {
			launchSuccess = true;
            break;
		  }
        }
      } 
    }

    void hpma115S0CustomSensor::loop() {
    }

    void hpma115S0CustomSensor::update() {
      if (launchSuccess) {
		if (read_values(&p25, &p10)) {
			this->pm_2_5_sensor_->publish_state(p25);
            this->pm_10_0_sensor_->publish_state(p10);
		} else {
			char toSend[200] = "Read Values Failed - See Previous Message - Debug info: ";
			ESP_LOGE(TAG, strcat(toSend, debugString.c_str()));
		}
      } else {
        ESP_LOGI(TAG, "Not Updating. HPMA115S0 sensor was not found.");
      }
    }
	
	template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }

    float hpma115S0CustomSensor::get_setup_priority() const { return setup_priority::LATE; }
	
	int hpma115S0CustomSensor::comWait(bool start, int minDataToRead) { //3 - Keep Waiting, 2 - Timeout, 1 - Success, 0 - Start Condition (Ignore)
	  if (start) {
	    waitLast = millis();
		return 0;
	  } else if (this->available() >= minDataToRead) {
		return 1;
	  } else if ((millis() - waitLast) >= waitTime) {
		return 2;
	  }
	  return 3;
	}

    bool hpma115S0CustomSensor::read_values (float *p25, float *p10) {
	  while(this->available() >= 1){
	    char getData = this->read();
	  }
	  
      byte read_particle[] = {0x68, 0x01, 0x04, 0x93 };
      this->write_array(read_particle, sizeof(read_particle));
	  

	  for(comWait(true, 2); comWait(false, 2) == 3;); //Wait for 2 pieces of data to be recieved
	  
	  if (this->available() >= 2) { //2 or more pieces of data have been recieved
	    byte HEAD = this->read();
	    byte LEN = this->read();
	    for(comWait(true, 6); comWait(false, 6) == 3;); //Wait for other 6 to be recieved
        if (this->available() >= 6) { //8 or more pieces of data have been recieved in total
		  byte COMD = this->read();
		  byte DF1 = this->read();
		  byte DF2 = this->read();
		  byte DF3 = this->read();
		  byte DF4 = this->read();
		  byte CS = this->read();
	      if ((HEAD == 0x40) && (LEN == 0x05)) {
			// The header is valid, process rest of the data     
			// Verify checksum
			if (((0x10000 - HEAD - LEN - COMD - DF1 - DF2 - DF3 - DF4) % 0XFF) == CS){
			  // Checksum OK, we compute PM2.5 and PM10 values
			  *p25 = DF1 * 256 + DF2;
			  *p10 = DF3 * 256 + DF4;
			  return true;
			} else {
			  //Checksum Fail
			  ESP_LOGE(TAG, "Checksum Mismatch - Check debug data if this happens again");
			  debugString = "HEAD: " + to_string((int)HEAD) + " LEN: " + to_string((int)LEN) + " COMD: " + to_string((int)COMD) + " DF1: " + to_string((int)DF1) + " DF2: " + to_string((int)DF2) + " DF3: " + to_string((int)DF3) + " DF4: " + to_string((int)DF4) + " CS: " + to_string((int)CS);
		      return false;
			}
		  } else {
			ESP_LOGE(TAG, "Invalid Header - Check debug data if this happens again");
			debugString = "HEAD: " + to_string((int)HEAD) + " LEN: " + to_string((int)LEN) + " COMD: " + to_string((int)COMD) + " DF1: " + to_string((int)DF1) + " DF2: " + to_string((int)DF2) + " DF3: " + to_string((int)DF3) + " DF4: " + to_string((int)DF4) + " CS: " + to_string((int)CS);
		    return false;
		  }
		} else {  
		  ESP_LOGE(TAG, "Most likely NACK as only 2 bytes recieved - Check debug data if this happens again");
		  debugString = "HEAD: " + to_string((int)HEAD) + " LEN: " + to_string((int)LEN);
		  return false;
		}
      } else {
        ESP_LOGE(TAG, "Read Values Failed - Serial Timeout to sensor");
		debugString = "Available: " + to_string(this->available());
		return false;
	  }
    }

    bool hpma115S0CustomSensor::start_measurement (void) {
      ESP_LOGI(TAG, "Attempting to Start Measurement for HPMA115S0");
      // First, we send the command
      uint8_t start_measurement[] = {0x68, 0x01, 0x01, 0x96 };
      this->write_array(start_measurement, sizeof(start_measurement));
      //Then we wait for the response

	  for(comWait(true, 2); comWait(false, 2) == 3;);
	  if (this->available() < 2) {
		setup_SM = 2;
		return false;
	  }

      byte read1 = this->read();
      byte read2 = this->read();
      // Test the response
      if ((read1 == 0xA5) && (read2 == 0xA5)) {
        // ACK
		setup_SM = 1;
        return true;
      } else if ((read1 == 0x96) && (read2 == 0x96)) {
        // NACK
		setup_SM = 3;
        return false;
      } else {
		//Malformed
		setup_SM = 4;
		return false;
	  }
    }

    bool hpma115S0CustomSensor::stop_measurement (void) {
      ESP_LOGI(TAG, "Attempting to Stop Measurement for HPMA115S0");
      // First, we send the command
      byte stop_measurement[] = {0x68, 0x01, 0x02, 0x95 };
      this->write_array(stop_measurement, sizeof(stop_measurement));
      //Then we wait for the response
      for(comWait(true, 2); comWait(false, 2) == 3;);
	  if (this->available() < 2) {
		return false;
	  }

      char read1 = this->read();
      char read2 = this->read();
      // Test the response
      if ((read1 == 0xA5) && (read2 == 0xA5)){
        // ACK
        return true;
      } else if ((read1 == 0x96) && (read2 == 0x96)) {
        // NACK
        return false;
      } else return false;
    }

    bool hpma115S0CustomSensor::stop_autosend(void) { //Only manual polling
      // Stop auto send
      ESP_LOGI(TAG, "Attempting to Stop Autosend for HPMA115S0");
      
      byte stop_autosend[] = {0x68, 0x01, 0x20, 0x77 };
      this->write_array(stop_autosend, sizeof(stop_autosend));
      
      //Then we wait for the response
      for(comWait(true, 2); comWait(false, 2) == 3;);
	  if (this->available() < 2) {
		setup_SAS = 2;
		return false;
	  }

      byte read1 = this->read();
      byte read2 = this->read();
      // Test the response
      if ((read1 == 0xA5) && (read2 == 0xA5)){
        // ACK
		setup_SAS = 1;
        return true; //SUC - Successfully Stopped Autosend
      }
      else if ((read1 == 0x96) && (read2 == 0x96)) {
        // NACK
		setup_SAS = 3;
        return false; //ERR - NACK Recieved
      } else {
		setup_SAS = 4;
		return false; //ERR - Data Malformed
	  }
    }
    
    bool hpma115S0CustomSensor::enable_autosend(void) { //Not used currently
      ESP_LOGI(TAG, "Attempting to Enable Autosend for HPMA115S0");
      // Start auto send
      byte start_autosend[] = {0x68, 0x01, 0x40, 0x57 };
      this->write_array(start_autosend, sizeof(start_autosend));
      //Then we wait for the response
      for(comWait(true, 2); comWait(false, 2) == 3;);
	  if (this->available() < 2) {
		return false;
	  }

      byte read1 = this->read();
      byte read2 = this->read();
      // Test the response
      if ((read1 == 0xA5) && (read2 == 0xA5)){
        // ACK
        return true;
      } else if ((read1 == 0x96) && (read2 == 0x96)) {
        // NACK
        return false;
      } else return false;
    }

  }
}
