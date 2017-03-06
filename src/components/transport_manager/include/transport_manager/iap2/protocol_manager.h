#pragma once

#include<vector>
#include "transport_manager/iap2/protocol_descriptor.h"

namespace transport_manager {
namespace transport_adapter {

class ProtocolManager {
  typedef std::vector<ProtocolDescriptor> Descriptors;
public:
  ProtocolManager();

  ProtocolDescriptor PickProtocol();
  void ReleaseProtocol(uint8_t index);

private:
  Descriptors descriptors_;
};

} // namespace transport_adapter
} // namespace transport_manager
