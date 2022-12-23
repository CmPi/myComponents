#pragma once

#include "esphome.h"

typedef enum{   	//For configuration registers
  SHUTDOWN,    		// shutdwon mode to reduce power consumption <3.5uA
  COMPARATOR,    	// Bit 0 = operate OS in comparator mode, 1= INTERRUPT MODE
  OS_POLARITY,    	// Polarity bit ;Bit 0 = Active low output, Bit 1 = Active high
  FAULT_QUEUE_0,    // Fault indication bits
  FAULT_QUEUE_1,    // Fault indication bits
  DATA_FORMAT,      //Data Format
  TIME_OUT,          //Time out
  ONE_SHOT           //1= One shot, 0 = Continuos
}configuration;


/**
 * @brief chip register definition
 */

#define MAX30205_ADDRESS        0x4c  // my MAX30205 component
#define MAX30205_ADDRESS1       0x49  // 8bit address converted to 7bit
#define MAX30205_ADDRESS2       0x48  // 8bit address converted to 7bit

// Registers
#define MAX30205_TEMPERATURE    0x00  /**< temperature register */
#define MAX30205_CONFIGURATION  0x01  /**< configure register */
#define MAX30205_THYST          0x02   /**< thyst register */
#define MAX30205_TOS            0x03   /**< tos register */

static const char *const TAG_MAX30205 = "max30205.sensor";
static const char *const TAG = "max30205.sensor";

class myMax30205 : public sensor::Sensor, public PollingComponent, public i2c::I2CDevice
{

public:

  float fLastTemperature = 0;      // Last temperature
  uint8_t sensorAddress = MAX30205_ADDRESS;

public:
  // constructor
  myMax30205() : PollingComponent(60000)
  {
  }

  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)

  void setup() override
  {
    begin();
  }

/*
  void loop() override
  {
  }
*/
  void dump_config()
  {
    LOG_SENSOR("", "MAX30205", this);
    LOG_I2C_DEVICE(this);
    if (this->is_failed())
    {
      ESP_LOGE(TAG_MAX30205, "Communication with BH1750 failed!");
    }
    LOG_UPDATE_INTERVAL(this);
  }

  void update() override
  {
    float fCurrentTemperature = getTemperature();
    if (fCurrentTemperature!=this->fLastTemperature) 
    {
      publish_state(fCurrentTemperature);
      this->fLastTemperature = fCurrentTemperature;
    }
  }


float get_setup_priority() const override 
{ 
  return esphome::setup_priority::BUS; 
}

protected:

bool scanAvailableSensors(void){
  bool sensorFound = false;

  Wire.beginTransmission (MAX30205_ADDRESS1);
  if (Wire.endTransmission () == 0){
    sensorAddress = MAX30205_ADDRESS1;
    sensorFound = true;
  }
  
  Wire.beginTransmission (MAX30205_ADDRESS2);
  if(Wire.endTransmission () == 0){
    sensorAddress = MAX30205_ADDRESS2;
    sensorFound = true;
  }

  return sensorFound;
}


float getTemperature(void){
	uint8_t readRaw[2] = {0};
  I2CreadBytes( sensorAddress, MAX30205_TEMPERATURE, &readRaw[0] ,2); // read two bytes
	int16_t raw = readRaw[0] << 8 | readRaw[1];  //combine two bytes
  float  _temperature = ceil( 10 * raw  * 0.00390625 ) / 10;     // convert to temperature
	return  _temperature;
}


void shutdown(void){
  uint8_t reg = I2CreadByte(sensorAddress, MAX30205_CONFIGURATION);  // Get the current register
  I2CwriteByte(sensorAddress, MAX30205_CONFIGURATION, reg | 0x80);
}

void begin(void){
  I2CwriteByte(sensorAddress, MAX30205_CONFIGURATION, 0x00); //mode config
  I2CwriteByte(sensorAddress, MAX30205_THYST , 		    0x00); // set threshold
  I2CwriteByte(sensorAddress, MAX30205_TOS, 			    0x00); //
}

void printRegisters(void){
  Serial.println(I2CreadByte(sensorAddress, MAX30205_TEMPERATURE),  BIN);
  Serial.println(I2CreadByte(sensorAddress, MAX30205_CONFIGURATION),  BIN);
  Serial.println(I2CreadByte(sensorAddress, MAX30205_THYST), BIN);
  Serial.println(I2CreadByte(sensorAddress, MAX30205_TOS), BIN);
}

// Wire.h read and write protocols
void I2CwriteByte(uint8_t address, uint8_t subAddress, uint8_t data){
	Wire.beginTransmission(address);  // Initialize the Tx buffer
	Wire.write(subAddress);           // Put slave register address in Tx buffer
	Wire.write(data);                 // Put data in Tx buffer
	Wire.endTransmission();           // Send the Tx buffer
}

uint8_t I2CreadByte(uint8_t address, uint8_t subAddress){
	uint8_t data; // `data` will store the register data
	Wire.beginTransmission(address);
	Wire.write(subAddress);
	Wire.endTransmission(false);
	Wire.requestFrom(address, (uint8_t) 1);
	data = Wire.read();
	return data;
}

void I2CreadBytes(uint8_t address, uint8_t subAddress, uint8_t * dest, uint8_t count){
	Wire.beginTransmission(address);   // Initialize the Tx buffer
	// Next send the register to be read. OR with 0x80 to indicate multi-read.
	Wire.write(subAddress);
	Wire.endTransmission(false);
	uint8_t i = 0;
	Wire.requestFrom(address, count);  // Read bytes from slave register address
	while (Wire.available())
	{
    uint8_t bRead;
		dest[i++] = Wire.read();
	}
}










};
