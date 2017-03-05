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

  void InitIAP2FacilityDBusManager();
  TransportAdapter::Error RegisterListener();

  DBusMsgErrorCode OnReceiveDBusSignal_(DBusMessage *message,
                                       const char *interface);

  DBusMsgErrorCode OnReceiveMethodCall_(DBusMessage *message,
                                       const char *interface);
  void AppleDeviceManagerSignalDBusProcess(unsigned int id,
                                           DBusMessage *message);

  void DBusMethodiAP2Connected(DBusMessage* message);

  void OnDeviceConnected();
  void OnDeviceDisconnected();

  void OnEAPStart();
  void OnEAPStop();
  void OnEAPData();

  void iAP2Notification(DBusMessage *message);
  void iAP2EventConnected();

  TransportAdapterController *controller_;
  bool initialized_;
};

} // namespace transport_adapter
} // namesapce transport_manager
