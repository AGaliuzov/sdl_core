#pragma once

#include "utils/logger.h"
#include "transport_manager/transport_adapter/transport_adapter_impl.h"
#include "transport_manager/transport_manager_settings.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager");

/**
 * @brief The IAP2TransportAdapter class adds ability to connect to the certain
 * iap2 device. Creates iap2 listener and iap2 connection factory.
 */
class IAP2TransportAdapter: public TransportAdapterImpl {
 public:

    IAP2TransportAdapter(resumption::LastState& last_state,
                         const TransportManagerSettings& settings);

    /**
     * @brief GetDeviceType allows to obtain device type.
     *
     * @return IAP2 device type.
     */
    DeviceType GetDeviceType() const final;

    /**
     * @brief RunAppOnDevice allows to perfoarm LAUNCH_APP iap feature.
     *
     * @param device_uid the device where appropriate application will
     * be executed.
     *
     * @param bundle_id the certain application id to run.
     */
    void RunAppOnDevice(const std::string& device_uid,
                        const std::string& bundle_id) final;
};

} // namespace transport_adapter
} // namesapce transport_manager
