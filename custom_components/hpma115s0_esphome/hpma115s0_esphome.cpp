#include "esphome/core/log.h"
#include "hpma115s0_esphome.h"

namespace esphome {
namespace hpma115S0_esphome {

#define byte uint8_t
static const char *const TAG = "hpma115s0.sensor";

void HPMA115S0Component::setup() {
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

void HPMA115S0Component::dump_config() {
  if (launchSuccess) {
    ESP_LOGCONFIG(TAG, "Sensor found after %i tries. Will read and log values.", setupTries);
  } else {
    ESP_LOGE(TAG, "Error! HPMA115S0 Particle Sensor not found! Failed with: %i AND %i.", setup_SAS, setup_SM);
  }
}

float HPMA115S0Component::get_setup_priority() const { return setup_priority::LATE; }

void HPMA115S0Component::update() {
  if (launchSuccess) {
    if (read_values(&p25, &p10)) {
      this->pm_2_5_sensor_->publish_state(p25);
      this->pm_10_0_sensor_->publish_state(p10);
    } else {
      ESP_LOGE(TAG, "Read Values Failed - See Previous Message");
    }
  } else {
    ESP_LOGI(TAG, "Not Updating. HPMA115S0 sensor was not found.");
  }
}


int HPMA115S0Component::comWait(bool start, int minDataToRead) { //3 - Keep Waiting, 2 - Timeout, 1 - Success, 0 - Start Condition (Ignore)
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

byte calculateChecksum(byte HEAD, byte LEN, byte* messageBuffer) {
  int sum = 0;
  for ( int index = 0; index < LEN; index++) {
    sum += messageBuffer[index];
  }
  return (0x10000 - HEAD - LEN - sum) % 0x100;
}

bool HPMA115S0Component::read_values(float *p25, float *p10) {
  while(this->available() >= 1){
    char getData = this->read();
  }

  byte read_particle[] = {0x68, 0x01, 0x04, 0x93 };
  this->write_array(read_particle, sizeof(read_particle));

  for(comWait(true, 2); comWait(false, 2) == 3;); //Wait for 2 pieces of data to be recieved
  if (not this->available() >= 2) { //2 or more pieces of data have been recieved
    ESP_LOGE(TAG, "Read Values Failed - Serial Timeout to sensor");
    ESP_LOGD(TAG, "Available: %i", this->available());
    return false;
  }

  byte HEAD = this->read();
  byte LEN = this->read();
  if (HEAD != 0x40 or (LEN != 0x05 and LEN !=0x0D)) {
    ESP_LOGE(TAG, "Invalid Header - Check debug data if this happens again");
    ESP_LOGE(TAG, "HEAD %i LEN %i", HEAD, LEN);
    return false;
  }

  for(comWait(true, LEN+1); comWait(false, LEN+1) == 3;); //Wait for other data to be recieved
  if (not this->available() >= LEN+1) { // HEAD + LEN + CHECKSUM or more pieces of data have been recieved in total
    ESP_LOGE(TAG, "Most likely NACK as only 2 bytes recieved - Check debug data if this happens again");
    ESP_LOGE(TAG, "HEAD %i LEN %i", HEAD, LEN);
    return false;
  }

  // Read the message, including the command
  byte messageBuffer[LEN];
  if (not this->read_array(messageBuffer, LEN)) {
    ESP_LOGE(TAG, "Read Values Failed - Serial Buffer Error");
    return false;
  }

  // Read the checksum
  byte CS = this->read();
  if (CS != calculateChecksum(HEAD, LEN, messageBuffer)) {
    ESP_LOGE(TAG, "Checksum Mismatch - Check debug data if this happens again");
    return false;
  }

  // At this point, messageBuffer has either 5 or 0x0D (13) pieces in it.
  if (LEN == 0x05) {
    *p25 = messageBuffer[1] * 256 + messageBuffer[2];
    *p10 = messageBuffer[3] * 256 + messageBuffer[4];
  } else {
    *p25 = messageBuffer[3] * 256 + messageBuffer[4];
    *p10 = messageBuffer[7] * 256 + messageBuffer[8];
  }
  return true;
}

bool HPMA115S0Component::start_measurement(void) {
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

bool HPMA115S0Component::stop_measurement(void) {
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

bool HPMA115S0Component::stop_autosend(void) {
  //Only manual polling
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
  } else if ((read1 == 0x96) && (read2 == 0x96)) {
    // NACK
    setup_SAS = 3;
    return false; //ERR - NACK Recieved
  } else {
    setup_SAS = 4;
    return false; //ERR - Data Malformed
  }
}

bool HPMA115S0Component::enable_autosend(void)  { //Not used
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

}  // namespace hm3301
}  // namespace esphome
