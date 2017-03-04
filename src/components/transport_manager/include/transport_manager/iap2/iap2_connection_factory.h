#pragma once

#include "transport_manager/transport_adapter/server_connection_factory.h"
#include "transport_manager/transport_adapter/transport_adapter_controller.h"

namespace transport_manager {
namespace transport_adapter {

/**
 * @brief The IAP2ConnectionFactory class
 */
class IAP2ConnectionFactory : public ServerConnectionFactory {
public:
  IAP2ConnectionFactory(
      TransportAdapterController *transport_adapter_controller);

private:
  /**
   * @brief Init
   * @return
   */
  TransportAdapter::Error Init() final;

  /**
   * @brief CreateConnection
   * @param device_handle
   * @param app_handle
   * @return
   */
  TransportAdapter::Error
  CreateConnection(const DeviceUID &device_handle,
                   const ApplicationHandle &app_handle) final;

  /**
   * @brief Terminate
   */
  void Terminate() final;

  /**
   * @brief IsInitialised
   * @return
   */
  bool IsInitialised() const final;


  TransportAdapterController* controller_;
  bool initialized_;
};

} // namespace transport_adapter
} // namesapce transport_manager
