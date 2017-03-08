#include "transport_manager/iap2/protocol_manager.h"

#include <algorithm>
#include "utils/logger.h"
namespace transport_manager {
namespace transport_adapter {

namespace {
uint8_t kInvalidProtocol = 255;
std::string kEmptyProtocol = "";

std::string hub_protocol_name = "com.smartdevicelink.prot0";
} // namespace;

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager");

ProtocolManager::ProtocolManager() {
  // TODO(AGaliuzov): Read the protocols from config file all add statically.
}

ProtocolDescriptor ProtocolManager::hub_protocol() const {
  return ProtocolDescriptor(0, hub_protocol_name);
}

ProtocolDescriptor ProtocolManager::PickProtocol() {
  LOG4CXX_AUTO_TRACE(logger_);
  auto descriptor_it =
      std::find_if(std::begin(descriptors_), std::end(descriptors_),
                   [](const ProtocolDescriptor &descriptor) {
                     return descriptor.status() == ProtocolStatus::kReleased;
                   });

  if (descriptor_it == std::end(descriptors_)) {
    LOG4CXX_ERROR(logger_, "There are no free protocols.");
    return ProtocolDescriptor(kInvalidProtocol, kEmptyProtocol);
  }

  descriptor_it->set_tatus(ProtocolStatus::kAcquired);
  return *descriptor_it;
}

void ProtocolManager::ReleaseProtocol(uint8_t index) {
  LOG4CXX_AUTO_TRACE(logger_);
  auto descriptor_it =
      std::find_if(std::begin(descriptors_), std::end(descriptors_),
                   [index](const ProtocolDescriptor &descriptor) {
                     return descriptor.index() == index;
                   });
  if (descriptor_it == std::end(descriptors_)) {
    LOG4CXX_ERROR(logger_, "There are no free protocols.");
    return;
  }

  descriptor_it->set_tatus(ProtocolStatus::kReleased);
}

} // namespace transport_adapter
} // namespace transport_manager
