#pragma once

#include <cstdint>
#include <string>

namespace transport_manager {
namespace transport_adapter {

enum class ProtocolStatus {
  kAcquired,
  kReleased
};

class ProtocolDescriptor
{
public:
  ProtocolDescriptor(uint32_t index, std::string name);
  ProtocolStatus status() const;
  void set_tatus(ProtocolStatus status);
  uint8_t index() const;
private:
  uint32_t index_;
  std::string name_;
  ProtocolStatus status_;
};

} // namespace transport_adapter
} // namespace transport_manager
