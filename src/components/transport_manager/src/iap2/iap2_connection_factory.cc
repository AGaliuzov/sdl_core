#include "transport_manager/iap2/iap2_connection_factory.h"

namespace transport_manager {
namespace transport_adapter {

IAP2ConnectionFactory::IAP2ConnectionFactory(
    TransportAdapterController *transport_adapter_controller)
    : controller_(transport_adapter_controller), initialized_(false) {}

TransportAdapter::Error IAP2ConnectionFactory::Init() {
  return TransportAdapter::OK;
}

TransportAdapter::Error
IAP2ConnectionFactory::CreateConnection(const DeviceUID &device_handle,
                                        const ApplicationHandle &app_handle) {
  return TransportAdapter::OK;
}

void IAP2ConnectionFactory::Terminate() {}

bool IAP2ConnectionFactory::IsInitialised() const { return true; }

} // namespace transport_adapter
} // namesapce transport_manager
