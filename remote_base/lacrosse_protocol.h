#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "remote_base.h"

namespace esphome {
namespace remote_base {

// for sending back an answer

struct LacrosseData
{
    uint8_t address;
    uint8_t type;
    float value;
    char buf[80];
    bool operator==(const LacrosseData &rhs) const { return type == rhs.type && address == rhs.address; }
};

// to keep record of previous sensors values

struct LacrosseDataStore
{
    uint8_t address;
    uint8_t type;
    float value;
};

class LacrosseProtocol : public RemoteProtocol<LacrosseData> {
 public:
  void encode(RemoteTransmitData *dst, const LacrosseData &data) override;
  optional<LacrosseData> decode(RemoteReceiveData src) override;
  void dump(const LacrosseData &data) override;
 private:
  uint8_t readNibble(RemoteReceiveData &src);  
  uint8_t readWsNibble(RemoteReceiveData &src);  
  bool bIsTx3Protocol(RemoteReceiveData src);
  bool bIsWs7kProtocol(RemoteReceiveData src);
  optional<LacrosseData> decodeTx(RemoteReceiveData src);
  optional<LacrosseData> decodeWs(RemoteReceiveData src);
};


DECLARE_REMOTE_PROTOCOL(Lacrosse)


template<typename... Ts> class LacrosseAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(std::string, data)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    LacrosseData data{};
    data.value = this->data_.value(x...);
    LacrosseProtocol().encode(dst, data);
  }
};


}  // namespace remote_base
}  // namespace esphome
