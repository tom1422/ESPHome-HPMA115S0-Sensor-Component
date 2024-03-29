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
    if (read_values(&p25, &p10, &p4, &p1)) {
      this->pm_2_5_sensor_->publish_state(p25);
      this->pm_10_0_sensor_->publish_state(p10);
      if(aqi_2_5_sensor_) (*aqi_2_5_sensor_).publish_state(calcAQI2_5());
      if(aqi_10_0_sensor_) (*aqi_10_0_sensor_).publish_state(calcAQI10());
      if(pm_4_0_sensor_) (*pm_4_0_sensor_).publish_state(p4);
      if(pm_1_0_sensor_) (*pm_1_0_sensor_).publish_state(p1);
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

bool HPMA115S0Component::read_values(float *p25, float *p10, float *p4, float *p1) {
  while(this->available() >= 1){
    char getData = this->read();
  }

  byte read_particle[] = {0x68, 0x01, 0x04, 0x93 };
  this->write_array(read_particle, sizeof(read_particle));

  for(comWait(true, 2); comWait(false, 2) == 3;); //Wait for 2 pieces of data to be recieved
  if (not (this->available() >= 2)) { //2 or more pieces of data have been recieved
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
  if (not (this->available() >= LEN+1)) { // HEAD + LEN + CHECKSUM or more pieces of data have been recieved in total
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
    *p4 = messageBuffer[5] * 256 + messageBuffer[6];
    *p1 = messageBuffer[1] * 256 + messageBuffer[2];
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

// equation from
// https://www.airnow.gov/sites/default/files/2020-05/aqi-technical-assistance-document-sept2018.pdf
float HPMA115S0Component::calcAQI2_5() const
{
    if (p25 <   0.0) { return -1.0; }
    if (p25 <  12.0) { return (p25) / 12.0 * 50.0; }
    if (p25 <  35.4) { return (p25 -  12.0) * 50.0 / ( 35.4 - 12.0) +  50.0; }
    if (p25 <  55.4) { return (p25 -  35.4) * 50.0 / ( 55.4 - 35.4) + 100.0; }
    if (p25 < 150.4) { return (p25 -  55.4) * 50.0 / (150.4 - 55.4) + 150.0; }
    if (p25 < 250.4) { return (p25 - 150.4) + 200.0; }
    if (p25 < 350.4) { return (p25 - 250.4) + 300.0; }
    return (p25 - 350.4) + 400.0;
}

// equation from
// https://www.airnow.gov/sites/default/files/2020-05/aqi-technical-assistance-document-sept2018.pdf
float HPMA115S0Component::calcAQI10() const
{
    if (p10 < 0.0)   { return -1.0; }
    if (p10 < 54.0)  { return (p10) / 54.0 * 50.0; }
    if (p10 < 154.0) { return (p10 -  54.0) *  50.0 / (154.0 - 54.0) + 50.0; }
    if (p10 < 254.0) { return (p10 - 154.0) *  50.0 / (254.0 - 154.0) + 100.0; }
    if (p10 < 354.0) { return (p10 - 254.0) *  50.0 / (354.0 - 254.0) + 150.0; }
    if (p10 < 424.0) { return (p10 - 354.0) * 100.0 / (424.0 - 354.0) + 200.0; }
    if (p10 < 504.0) { return (p10 - 424.0) * 100.0 / (504.0 - 424.0) + 300.0; }
    return (p10 - 504.0) + 400.0;
}

}  // namespace hpma115S0_esphome
}  // namespace esphome
