#include "transport_manager/iap2/iap2_transport_adapter.h"


namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager");

IAP2TransportAdapter::IAP2TransportAdapter(resumption::LastState& last_state,
        const TransportManagerSettings& settings)
    : TransportAdapterImpl (nullptr, nullptr, nullptr, last_state, settings) {
}

DeviceType IAP2TransportAdapter::GetDeviceType() const {
    return  IAP2;
}

void IAP2TransportAdapter::RunAppOnDevice(const std::string& device_uid,
                                          const std::string& bundle_id) {
}


} // namespace transport_adapter
} // namesapce transport_manager
