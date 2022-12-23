#include "lacrosse_protocol.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {

namespace remote_base {

static const char *const TAG = "remote.lacrosse";

// https://www.f6fbb.org/domo/sensors/tx_signals.php

// TX3 - 0 = 1300us HIGH / 1000 LOW
//       1 =  500us HIGH / 1100 LOW

static const uint32_t TX3_BIT_ZERO_HIGH_US = 1300;
static const uint32_t TX3_BIT_ZERO_LOW_US = 1000;

static const uint32_t TX3_BIT_ONE_HIGH_US = 500;
static const uint32_t TX3_BIT_ONE_LOW_US = 1100;

static const uint32_t WS7K_SHORT_US = 400;
static const uint32_t WS7K_LONG_US = 800;

static const uint8_t SENSORS_MAX = 30;

optional<LacrosseData> LacrosseProtocol::decode(RemoteReceiveData src) {
  if (bIsTx3Protocol(src)) {
    return LacrosseProtocol::decodeTx(src);
  } else if (bIsWs7kProtocol(src)) {
    ESP_LOGD(TAG, "WS protocol");
    return LacrosseProtocol::decodeWs(src);
  } 
  return {};
}

optional<LacrosseData> LacrosseProtocol::decodeTx(RemoteReceiveData src) {

  // keep track of already seen sensors

  static uint8_t iSensors = 0;  
  static LacrosseDataStore aSensors[SENSORS_MAX];

  uint64_t packet = 0;
  LacrosseData out{
    .address = 0,
    .type = 0,
    .value = 0,
  };

  src.advance(8*2); // header already checked by bIsTx3Protocol

  out.type = this->readNibble(src);
  if (out.type==0xff) {
    return {};
  }

  if (out.type!=0x00 && out.type!=0x0E) {
    ESP_LOGD(TAG, "Unknown type: %d", out.type);
    return {};
  }

  uint8_t add_msb = 0;
  uint8_t add_lsb = 0;

  add_msb = this->readNibble(src);
  if (add_msb==0xff) {
    return {};
  }

  add_lsb = this->readNibble(src);
  if (add_msb==0xff) {
    return {};
  }

  out.address = add_msb << 3 | (add_lsb & 0xE) >> 1;

  uint8_t aDigits[] = { 0, 0, 0, 0, 0}; // 5 next nibbles are digits in BCD

  for( uint8_t iDigit = 0 ; iDigit<5; iDigit++ ) {
    uint8_t iTmp = this->readNibble(src);
    if (iTmp==0xff) {
      return {};
    } else {
      aDigits[iDigit] = iTmp;
    }
  }

  if (aDigits[0]==aDigits[3] && aDigits[1]==aDigits[4]) {
    if (out.type==0) { // temperature
      out.value = 10.0*aDigits[0] + aDigits[1] - 50.0 + (0.0+aDigits[2])/10;
    } else {
      out.value = 10.0*aDigits[0] + aDigits[1]  + (0.0+aDigits[2])/10;
    }

    // look for a free slot

    uint8_t iSameSlot = 0xff;
    for (uint8_t iSlot = 0; iSlot < SENSORS_MAX; iSlot++) { 
      if (out.address==aSensors[iSlot].address && out.type==aSensors[iSlot].type) {
        iSameSlot = iSlot;
        break;
      }
    }

    if (iSameSlot==0xff) { // first time we see this sensor
      iSameSlot=iSensors;
      aSensors[iSameSlot].address = out.address; 
      aSensors[iSameSlot].type = out.type; 
      aSensors[iSameSlot].value = out.value; 
      iSensors++;
      sprintf(out.buf, "TX%02X%01X=%.1f", out.address, out.type, out.value );
      ESP_LOGD(TAG, "NEW %s", out.buf );
      return out;
    } else if (iSameSlot!=0xff) { // sensor known
      if (aSensors[iSameSlot].value!=out.value) { // if new value
        sprintf(out.buf, "TX%02X%01X=%.1f", out.address, out.type, out.value );
        ESP_LOGD(TAG, "UPD %s", out.buf );
        aSensors[iSameSlot].value=out.value;
        return out;
      }
    }
    return {};
  }
  return {};
}

optional<LacrosseData> LacrosseProtocol::decodeWs(RemoteReceiveData src) {

  uint64_t packet = 0;
  LacrosseData out{
    .address = 0,
    .type = 0,
    .value = 0,
  };

  src.advance(10*2); // header already checked by bIsTx3Protocol

  out.type = this->readWsNibble(src);
  if (out.type==0xff) {
    return {};
  }

  out.address = this->readWsNibble(src);
  if (out.address==0xff) {
    return {};
  }

  ESP_LOGD(TAG, "Sensor Address: %d", out.address);
  ESP_LOGD(TAG, "Sensor Type: %d", out.type); // Physical quantity

  uint8_t iNumDigits;
  uint8_t aDigits[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // 10 next nibbles are digits in BCD

  switch (out.type) {

    case 0: // WS7000-27/28 - 7 blocks
     iNumDigits = 3;
     break;

    case 1: // WS7000-22/25 meteo sensor - 10 blocks
     iNumDigits = 6;
     break;

    case 2: // WS7000-16 rain sensor
     iNumDigits = 3;
     break;

    case 3: // WS7000-15 wind sensor - 10 blocks
     iNumDigits = 6;
     break;

    case 4: // WS7000-20 - 14 blocks - 12 remaining - 10 digits - XOR - SUM
     iNumDigits = 10;
     break;

    case 5: // WS2500-19 - 11 blocks - 9 remaining - 7 digits - XOR - SUM
     iNumDigits = 7;
     break; 

  }

  uint8_t iComputeXor =       out.type ^ out.address;
  uint8_t iComputeSum = ( 5 + out.type + out.address ) & 0xF;

  for( uint8_t iDigit = 0 ; iDigit<iNumDigits; iDigit++ ) {
    uint8_t iTmp = this->readWsNibble(src);
    if (iTmp==0xff) {
      return {};
    } else {
      aDigits[iDigit] = iTmp;
      iComputeXor =   iComputeXor ^ iTmp;
      iComputeSum = ( iComputeXor + iTmp ) & 0xF;
    }
  }

  uint8_t iCheckXor = this->readWsNibble(src);
  if (iCheckXor==0xff) {
    return {};
  }
  ESP_LOGV( TAG, "XOR: %02X %02X", iComputeXor, iCheckXor ); 
  if (iComputeXor!=iCheckXor) {
    ESP_LOGW( TAG, "XOR check failed"); 
    return {}
  }

  uint8_t iCheckSum = this->readWsNibble(src);
  if (iCheckSum==0xff) {
    return {};
  }
  ESP_LOGD( TAG, "SUM: %02X %02X", iComputeSum, iCheckSum ); 

  switch (out.type) {

    case 0: // WS7000-27/28 - 7 blocks
     break;

    case 1: // WS7000-22/25 meteo sensor - 10 blocks
     break;

    case 2: // WS7000-16 rain sensor
     break;

    case 3: // WS7000-15 wind sensor - 10 blocks
     break;

    case 4: { // WS7000-20 - 14 blocks - 12 remaining - 10 digits - XOR - SUM
      float fTemperature =                        10*aDigits[2] + aDigits[1] + ( 0.0 + aDigits[0] )/10;
      float fPression    = 200 + 100*aDigits[8] + 10*aDigits[7] + aDigits[6] + ( 0.0 + aDigits[9] )/10;
      float fHumidity    =                        10*aDigits[5] + aDigits[4] + ( 0.0 + aDigits[3] )/10;
      if (out.address & 0x8) {
       fTemperature = -fTemperature;
       out.address = out.address & 0x7; 
      }
      ESP_LOGV( TAG, "Pression: %f", fPression );
      ESP_LOGV( TAG, "Humidité: %f", fHumidity );
      ESP_LOGV( TAG, "Temperature: %f", fTemperature );
      sprintf(out.buf, "WS%01X%01XP=%.1f;WS%01X%01X0=%.1f;WS%01X%01XE=%.1f", out.address, out.type, fPression, out.address, out.type, fTemperature, out.address, out.type,fHumidity );
      break;
    }
    case 5: // WS2500-19 - 11 blocks - 9 remaining - 7 digits - XOR - SUM
     break; 

  }




  ESP_LOGD(TAG, "UPD %s", out.buf );
  return out;
}

void LacrosseProtocol::encode(RemoteTransmitData *dst, const LacrosseData &data) {
}

void LacrosseProtocol::dump(const LacrosseData &data) {
  ESP_LOGD(TAG, "Received Lacrosse: type=%d  address=%d" PRIX8, data.type, data.address);
}

uint8_t LacrosseProtocol::readNibble(RemoteReceiveData &src) {
  uint8_t _nibble = 0;
  for (uint8_t bit_counter = 0; bit_counter < 4; bit_counter++) {
   if (src.expect_item(TX3_BIT_ONE_HIGH_US, TX3_BIT_ONE_LOW_US)) {
      _nibble = (_nibble << 1) | 1;
    } else if (src.expect_item(TX3_BIT_ZERO_HIGH_US, TX3_BIT_ZERO_LOW_US)) {
      _nibble = (_nibble << 1) | 0;
    } else {
      return 0xff;
    }
  }
  return _nibble;
} 

// === Check if it is a TX3 (or TX4 ?) sensor

bool LacrosseProtocol::bIsTx3Protocol(RemoteReceiveData src) {
  uint8_t _byte = 0;
  for (uint8_t bit_counter = 0; bit_counter < 8; bit_counter++) {
   if (src.expect_item(TX3_BIT_ONE_HIGH_US, TX3_BIT_ONE_LOW_US)) {
      _byte = (_byte << 1) | 1;
    } else if (src.expect_item(TX3_BIT_ZERO_HIGH_US, TX3_BIT_ZERO_LOW_US)) {
      _byte = (_byte << 1) | 0;
    } else {
      return false;
    }
  }
  return (_byte==0x0A);
}

// ============================================================================
//
//    WS-7000 WS-2000 Protocol
//

bool LacrosseProtocol::bIsWs7kProtocol(RemoteReceiveData src) {
  uint8_t _byte = 0;
  for (uint8_t bit_counter = 0; bit_counter < 10; bit_counter++) {
    if (src.expect_item(WS7K_LONG_US, WS7K_SHORT_US)) {
      _byte++;
    } else if (src.expect_item(WS7K_SHORT_US, WS7K_LONG_US)) {
    } else {
      return false;
    } 
  }
  return (_byte==10); // 10 x 0 expected
}

uint8_t LacrosseProtocol::readWsNibble(RemoteReceiveData &src) {
  if (src.expect_item(WS7K_SHORT_US, WS7K_LONG_US)) {
    uint8_t _nibble = 0;
    for (uint8_t bit_counter = 0; bit_counter < 4; bit_counter++) {
     if (src.expect_item(WS7K_SHORT_US, WS7K_LONG_US)) {
        _nibble = (_nibble >> 1) | 8;
      } else if (src.expect_item(WS7K_LONG_US, WS7K_SHORT_US)) {
        _nibble = (_nibble >> 1) | 0;
      } else {
        ESP_LOGD( TAG, "WS not a bit" );
        return 0xff; // it was not a 1 neither a 0
      }
    }
    return _nibble;
  }
  ESP_LOGD( TAG, "WS not starting with one" );
  return 0xff; // this nibble did not start with a 1
} 


}  // namespace remote_base
}  // namespace esphome
