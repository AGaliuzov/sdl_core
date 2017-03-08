#include "transport_manager/iap2/protocol_descriptor.h"

namespace transport_manager {
namespace transport_adapter {

ProtocolDescriptor::ProtocolDescriptor(uint32_t index, std::string name)
    : index_(std::move(index)), name_(name),
      status_(ProtocolStatus::kReleased) {}

ProtocolStatus ProtocolDescriptor::status() const { return status_; }

void ProtocolDescriptor::set_tatus(ProtocolStatus status) { status_ = status; }

uint8_t ProtocolDescriptor::index() const { return index_; }

std::string ProtocolDescriptor::name() const { return name_; }

} // namespace transport_adapter
} // namespace transport_manager
