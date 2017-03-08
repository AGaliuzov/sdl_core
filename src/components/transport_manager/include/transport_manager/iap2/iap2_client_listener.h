#pragma once

#include <functional>
#include <vector>
#include <dbus/dbus.h>
#include <dbus/TCDBusRawAPI.h>


#include <iap2/iAP2NativeMsg.h>

#include "transport_manager/transport_adapter/client_connection_listener.h"
#include "transport_manager/transport_adapter/transport_adapter_controller.h"
#include "utils/shared_ptr.h"

namespace transport_manager {
namespace transport_adapter {

struct CallBackWrapper;
class ProtocolManager;

class IAP2ClientListener : public ClientConnectionListener {

public:
  IAP2ClientListener(TransportAdapterController *controller);
  ~IAP2ClientListener();
private:
  TransportAdapter::Error Init() final;
  void Terminate() final;
  bool IsInitialised() const final;
  TransportAdapter::Error StartListening() final;
  TransportAdapter::Error StopListening() final;

  void InitIAP2FacilityDBusManager();
  TransportAdapter::Error RegisterListener();

  DBusMsgErrorCode OnReceiveDBusSignalEvent(DBusMessage *message,
                                       const char *interface);

  DBusMsgErrorCode OnReceiveMethodCallEvent(DBusMessage *message,
                                       const char *interface);

  void AppleDeviceManagerSignalDBusProcess(unsigned int id,
                                           DBusMessage *message);

  void DBusMethodiAP2Connected(DBusMessage* message);

  void OnDeviceConnected();
  void OnDeviceDisconnected();

  void OnEAPStart(iAP2NativeMsg* native_message);
  void OnEAPStop();
  void OnEAPData();

  void OnHubConnected(int session_index);

  void iAP2Notification(DBusMessage *message);
  void iAP2EventConnected();

  TransportAdapterController *controller_;
  bool initialized_;

  utils::SharedPtr<ProtocolManager> protocol_manager_;

  friend Wrapper;
};

struct CallBackWrapper {
  typedef std::function<DBusMsgErrorCode(DBusMessage *message,
                                        const char *interface)> CallBack;

  static CallBack OnReceiveMethodCallEvent;
  static CallBack OnReceiveDBusSignalEvent;
};

} // namespace transport_adapter
} // namesapce transport_manager
