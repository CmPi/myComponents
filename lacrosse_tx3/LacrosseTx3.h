#include "esphome.h"

static const char *const TAG = "custom.lacrosse";


class LacrosseTx3Sensor : public PollingComponent {

  protected:
    std::string address_;

  public:
    Sensor *temperature_sensor = new Sensor();
    Sensor *humidity_sensor = new Sensor();

  LacrosseTx3Sensor() : PollingComponent(15000) { }

  void set_address( const std::string &address ) {
    address_ = address; 
  }

  void setup() override {
 //   bmp.begin();
    }

  void update() override {

    if (sBuffer->value().size()>0) {
      ESP_LOGD( TAG, "Buffer not empty :  %s", sBuffer->value().c_str() );
      ESP_LOGD( TAG, "My Address :  %s", address_ );
    }
 



//    temperature_sensor->publish_state(temperature);
//    pressure_sensor->publish_state(pressure / 100.0);
  }
};