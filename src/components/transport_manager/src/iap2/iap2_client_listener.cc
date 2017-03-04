#include "transport_manager/iap2/iap2_client_listener.h"

#include <functional>

#include <iap2/iAP2NativeMsg.h>
#include <iap2/Iap2DbHandler.h>
#include <iap2/TCiAP2TestUiDBusMsgDefNames.h>
#include <iap2/Iap2Const.h>

#include "utils/logger.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager");

IAP2ClientListener::IAP2ClientListener(TransportAdapterController *controller)
    : controller_(controller) {}

TransportAdapter::Error IAP2ClientListener::Init() {
  TransportAdapter::Error error = TransportAdapter::OK;
  error = InitIAP2FacilityDBusManager();
  if (TransportAdapter::OK != error) {
    return error;
  }

  error = RegisterListener();
  if (TransportAdapter::OK != error) {
    return error;
  }
}

void IAP2ClientListener::Terminate() {}

bool IAP2ClientListener::IsInitialised() const {}

TransportAdapter::Error IAP2ClientListener::StartListening() {}

TransportAdapter::Error IAP2ClientListener::StopListening() {}

TransportAdapter::Error IAP2ClientListener::InitIAP2FacilityDBusManager() {
  using namespace std::placeholders;

  SetDBusPrimaryOwner(IAP2_FACILITY_DEST_NAME);

  auto signal_func = std::bind<DBusMsgErrorCode>(
      &IAP2ClientListener::OnReceiveDBusSignal, this, _1, _2);

  auto method_func = std::bind<DBusMsgErrorCode>(
      &IAP2ClientListener::OnReceiveMethodCall, this, _1, _2);

  SetCallBackFunctions(signal_func, method_func);
  AddMethodInterface(IAP2_FACILITY_FEATURE_INTERFACE);
  AddMethodInterface(IAP2_FACILITY_EA_NATIVE_INTERFACE);
  AddSignalInterface(IAP2_PROCESS_INTERFACE);
  AddSignalInterface(ADM_MANAGER_INTERFACE);
  InitializeRawDBusConnection("TC iAP2 Facility DBUS");
}

TransportAdapter::Error IAP2ClientListener::RegisterListener() {
  DBusMessage *message;
  const char *dest = IAP2_FACILITY_DEST_NAME;
  const char *objectPath = IAP2_FACILITY_OBJECT_PATH;
  const char *interfaceName = IAP2_FACILITY_FEATURE_INTERFACE;
  const char *methodNotifyName = METHOD_PLAYER_IAP2_NOTIFY;
  const char *methodConnectedName = METHOD_PLAYER_IAP2_CONNECTED;

  message = CreateDBusMsgMethodCall(
      IAP2_PROCESS_DBUS_NAME, IAP2_PROCESS_OBJECT_PATH, IAP2_PROCESS_INTERFACE,
      METHOD_iAP2_INIT_APP, DBUS_TYPE_STRING, &dest, DBUS_TYPE_STRING,
      &objectPath, DBUS_TYPE_STRING, &interfaceName, DBUS_TYPE_STRING,
      &methodNotifyName, DBUS_TYPE_STRING, &methodConnectedName,
      DBUS_TYPE_INVALID);

  if (message != NULL) {
    DBusPendingCall *pending = NULL;
    if (!SendDBusMessage(message, &pending)) {
      LOG4CXX_ERROR(logger_, "Failed to send DBus for iap2");
      return TransportAdapter::FAIL;
    }
    dbus_message_unref(message);
  } else {
    LOG4CXX_ERROR(logger_, "Failed to create DBus message for iap2");
    return TransportAdapter::FAIL;
  }

  return TransportAdapter::OK;
}

DBusMsgErrorCode
IAP2ClientListener::OnReceiveDBusSignal(DBusMessage *message,
                                        const char *interface) {}

DBusMsgErrorCode
IAP2ClientListener::OnReceiveMethodCall(DBusMessage *message,
                                        const char *interface) {}

} // namespace transport_adapter
} // namesapce transport_manager
