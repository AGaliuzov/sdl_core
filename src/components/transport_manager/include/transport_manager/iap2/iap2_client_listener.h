#pragma once

#include <dbus/dbus.h>
#include <dbus/TCDBusRawAPI.h>

#include "transport_manager/transport_adapter/client_connection_listener.h"
#include "transport_manager/transport_adapter/transport_adapter_controller.h"

namespace transport_manager {
namespace transport_adapter {

class IAP2ClientListener : public ClientConnectionListener {

public:
  IAP2ClientListener(TransportAdapterController *controller);

private:
  TransportAdapter::Error Init() final;
  void Terminate() final;
  bool IsInitialised() const final;
  TransportAdapter::Error StartListening() final;
  TransportAdapter::Error StopListening() final;

  TransportAdapter::Error InitIAP2FacilityDBusManager();
  TransportAdapter::Error RegisterListener();

  DBusMsgErrorCode OnReceiveDBusSignal(DBusMessage *message,
                                       const char *interface);

  DBusMsgErrorCode OnReceiveMethodCall(DBusMessage *message,
                                       const char *interface);

  TransportAdapterController *controller_;
};

} // namespace transport_adapter
} // namesapce transport_manager
