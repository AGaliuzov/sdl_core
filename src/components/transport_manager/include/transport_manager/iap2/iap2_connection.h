#pragma once

#include "transport_manager/transport_adapter/connection.h"
#include "transport_manager/transport_adapter/transport_adapter_controller.h"
#include "transport_manager/iap2/iap2_device.h"



namespace transport_manager {
namespace transport_adapter {

class IAP2Connection : public Connection {
public:
  IAP2Connection(IAP2Device* parent, ApplicationHandle app_handle,
             TransportAdapterController* controller);

private:
  TransportAdapter::Error
  SendData(protocol_handler::RawMessagePtr message) final;

  TransportAdapter::Error Disconnect() final;

  IAP2Device* parent_;
  ApplicationHandle app_handle_;
  TransportAdapterController* controller_;

  int session_index_;
};

} // namespace transport_adapter
} // namesapce transport_manager


