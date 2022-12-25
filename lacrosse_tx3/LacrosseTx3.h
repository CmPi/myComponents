#include "esphome.h"

static const char *const TAG = "custom.lacrosse";


class LacrosseTx3Sensor : public PollingComponent {

  protected:
    std::string address_;

  public:
    Sensor *temperature_sensor = new Sensor();
    Sensor *humidity_sensor = new Sensor();

    LacrosseTx3Sensor( const std::string &address ) : PollingComponent(15000) { 
      address_ = address; 
    }

    void setup() override {
    }

    void update() override {

      if (sBuffer->value().size()>0) {

        ESP_LOGD( TAG, "Buffer not empty :  %s", sBuffer->value().c_str() );
        ESP_LOGD( TAG, "My Address :  %s",  this->address_.c_str() );

       size_t begin =  sBuffer->value().find(this->address_);
       if (begin != std::string::npos) {
         ESP_LOGD( TAG, "BINGO ! FOUND:  %s at %d", this->address_.c_str(), begin );
         std::string sValue;
         std::string sType = sBuffer->value().substr(begin+4,1);
         size_t end  =  sBuffer->value().find(";",begin);
         if (end != std::string::npos) {
           ESP_LOGD( TAG, "BINGO ! FOUND ; at %d", end );
           sValue = sBuffer->value().substr(begin+6,end-begin-6);
         } else {
           sValue.append(sBuffer->value().c_str()); 
           sBuffer->value().assign(sBuffer->value().substr(0,begin));   
           ESP_LOGD( TAG, "New buffer content :  %s", sBuffer->value().c_str() );
         }
         ESP_LOGD( TAG, "sValue  %s ", to_string(sValue).c_str() );
         float fValue = 0.0;
         fValue = std::stof(sValue);
         if (sType=="0") {
           this->temperature_sensor->publish_state(fValue);
         } else if (sType=="E") {
           this->humidity_sensor->publish_state(fValue);
         } 
      }
    }
  }
};