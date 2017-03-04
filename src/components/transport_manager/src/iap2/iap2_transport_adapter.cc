#include "transport_manager/iap2/iap2_transport_adapter.h"

#include "transport_manager/iap2/iap2_connection_factory.h"
#include "transport_manager/iap2/iap2_client_listener.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager");

IAP2TransportAdapter::IAP2TransportAdapter(
    resumption::LastState &last_state, const TransportManagerSettings &settings)
    : TransportAdapterImpl(nullptr, new IAP2ConnectionFactory(this),
                           new IAP2ClientListener(this), last_state, settings) {
}

DeviceType IAP2TransportAdapter::GetDeviceType() const { return IAP2; }

void IAP2TransportAdapter::RunAppOnDevice(const std::string &device_uid,
                                          const std::string &bundle_id) {}

} // namespace transport_adapter
} // namesapce transport_manager
