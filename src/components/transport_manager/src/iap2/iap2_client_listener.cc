#include "transport_manager/iap2/iap2_client_listener.h"

#include <string>

#include <iap2/Iap2DbHandler.h>
#include <iap2/TCiAP2TestUiDBusMsgDefNames.h>
#include <iap2/Iap2Const.h>

#include "transport_manager/iap2/iap2_device.h"
#include "transport_manager/iap2/protocol_manager.h"

#include "transport_manager/common.h"
#include "utils/logger.h"
#include "utils/make_shared.h"

namespace transport_manager {
namespace transport_adapter {

namespace {
const uint8_t iap2_ncm_num = 0;

DBusMsgErrorCode OnReceiveMethodCall(DBusMessage *message,
                                     const char *interface) {
  return CallBackWrapper::OnReceiveMethodCallEvent(message, interface);
}

DBusMsgErrorCode OnReceiveDBusSignal(DBusMessage *message,
                                     const char *interface) {

  return CallBackWrapper::OnReceiveDBusSignalEvent(message, interface);
}

} // namespace
CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager");

IAP2ClientListener::IAP2ClientListener(TransportAdapterController *controller)
    : controller_(controller), initialized_(false) {
  protocol_manager_ = utils::MakeShared<ProtocolManager>();
}

IAP2ClientListener::~IAP2ClientListener() {}

TransportAdapter::Error IAP2ClientListener::Init() {
  TransportAdapter::Error error = TransportAdapter::OK;
  InitIAP2FacilityDBusManager();

  error = RegisterListener();
  if (TransportAdapter::OK != error) {
    return error;
  }

  return error;
}

void IAP2ClientListener::Terminate() {}

bool IAP2ClientListener::IsInitialised() const { return initialized_; }

TransportAdapter::Error IAP2ClientListener::StartListening() {
  return TransportAdapter::OK;
}

TransportAdapter::Error IAP2ClientListener::StopListening() {
  return TransportAdapter::OK;
}

void IAP2ClientListener::InitIAP2FacilityDBusManager() {
  using namespace std::placeholders;

  SetDBusPrimaryOwner(IAP2_FACILITY_DEST_NAME);

  CallBackWrapper::OnReceiveMethodCallEvent =
      std::bind(&IAP2ClientListener::OnReceiveMethodCallEvent, this, _1, _2);

  CallBackWrapper::OnReceiveMethodCallEvent =
      std::bind(&IAP2ClientListener::OnReceiveDBusSignalEvent, this, _1, _2);

  SetCallBackFunctions(OnReceiveDBusSignal, OnReceiveMethodCall);
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
IAP2ClientListener::OnReceiveDBusSignalEvent(DBusMessage *message,
                                             const char *interface) {
  DBusMsgErrorCode error = ErrorCodeNoError;

  if (nullptr == message || nullptr == interface) {
    LOG4CXX_ERROR(logger_, "Message arguments are not valid: "
                               << "Message adress: " << message
                               << "Message interface" << interface);
    return ErrorCodeUnknown;
  }

  unsigned int index;
  unsigned int stop = 0;

  if (strcmp(interface, IAP2_PROCESS_INTERFACE) == 0) {
    for (index = SignaliAP2EventParcel; index < TotalSignaliAP2Events && !stop;
         index++) {
      if (dbus_message_is_signal(message, IAP2_PROCESS_INTERFACE,
                                 g_signaliAP2EventNames[index])) {
        if (index < TotalSignaliAP2Events) {
          iAP2Notification(message);
        }
        stop = 1;
      }
    }
  }

  else if (strcmp(interface, ADM_MANAGER_INTERFACE) == 0) {
    for (index = SignalAppleDeviceManageriAP1Connected;
         index < TotalSignalAppleDeviceManagerEvents && !stop; index++) {
      if (dbus_message_is_signal(message, ADM_MANAGER_INTERFACE,
                                 g_signalADMEventNames[index])) {
        AppleDeviceManagerSignalDBusProcess(index, message);
        stop = 1;
      }
    }
  }

  if (!stop) {
    error = ErrorCodeUnknown;
  }

  return error;
}

DBusMsgErrorCode
IAP2ClientListener::OnReceiveMethodCallEvent(DBusMessage *message,
                                             const char *interface) {
  DBusMsgErrorCode error = ErrorCodeNoError;

  if (nullptr == message || nullptr == interface) {
    LOG4CXX_ERROR(logger_, "Message arguments are not valid: "
                               << "Message adress: " << message
                               << "Message interface" << interface);
    return ErrorCodeUnknown;
  }

  if (strcmp(interface, IAP2_FACILITY_FEATURE_INTERFACE) == 0) {
    fprintf(stderr, "test %s interface %s\n", __func__, interface);
    if (dbus_message_is_method_call(message, IAP2_FACILITY_FEATURE_INTERFACE,
                                    METHOD_PLAYER_IAP2_NOTIFY)) {
      iAP2Notification(message);
    } else if (dbus_message_is_method_call(message,
                                           IAP2_FACILITY_FEATURE_INTERFACE,
                                           METHOD_PLAYER_IAP2_CONNECTED)) {
      DBusMethodiAP2Connected(message);
    }
  }
  return error;
}

void IAP2ClientListener::AppleDeviceManagerSignalDBusProcess(
    unsigned int id, DBusMessage *message) {
  if (id < TotalSignalAppleDeviceManagerEvents) {

    LOG4CXX_INFO(logger_, "RECEIVED SIGNAL[%s(%d)] "
                              << g_signalADMEventNames[id] << "id: " << id);

    if (message != NULL) {
      int ret = 1;
      int fWithEaNative = 0;
      switch (id) {
      case SignalAppleDeviceManageriAP2Connected:
        ret = GetArgumentFromDBusMessage(message, DBUS_TYPE_INT32,
                                         &fWithEaNative, DBUS_TYPE_INVALID);
        break;
      default:
        ret = GetArgumentFromDBusMessage(message, DBUS_TYPE_INVALID);
        break;
      }

      if (ret) {
        switch (id) {
        case SignalAppleDeviceManageriAP2Connected:
          OnDeviceConnected();
          break;
        case SignalAppleDeviceManageriAP2Disconnected:
          OnDeviceDisconnected();
          break;
        default:
          LOG4CXX_ERROR(logger_, "Unknown signal id: " << id);
          break;
        }
      } else {
        LOG4CXX_ERROR(logger_, "GetArgumentFromDBusMessage failed");
      }
    }
  } else {
    LOG4CXX_INFO(logger_, "RECEIVED SIGNAL: " << id);
  }
}

void IAP2ClientListener::DBusMethodiAP2Connected(DBusMessage *message) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (nullptr == message) {
    LOG4CXX_ERROR(logger_, "Nll message received from dbus");
    return;
  }

  int hwConnect = 0;
  int ncmNumber = -1;
  int supportEaNative = 0;
  if (GetArgumentFromDBusMessage(message, DBUS_TYPE_INT32, &hwConnect,
                                 DBUS_TYPE_INT32, &ncmNumber, DBUS_TYPE_INT32,
                                 &supportEaNative, DBUS_TYPE_INVALID)) {
    if (hwConnect != 0) {
      if (ncmNumber == iap2_ncm_num) // || ncmNumber == CARPLAY_NCM_NUM)
      {
        iAP2EventConnected();
      }
    }
    LOG4CXX_DEBUG(logger_, "hwConnect : " << hwConnect << ", ncmNumber "
                                          << ncmNumber);
  } else {
    LOG4CXX_ERROR(logger_, "GetArgumentFromDBusMessage failed");
  }
  //
}

void IAP2ClientListener::OnDeviceConnected() {
  DeviceSptr device = utils::MakeShared<IAP2Device>(std::string("FakeName"),
                                                    DeviceUID("FakeID"));

  controller_->AddDevice(device);
}

void IAP2ClientListener::OnDeviceDisconnected() {
  controller_->DeviceDisconnected("FakeUUID", DisconnectDeviceError());
}

void IAP2ClientListener::OnEAPStart(iAP2NativeMsg *native_message) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (nullptr == native_message) {
    LOG4CXX_ERROR(logger_, "Native message is null. Nothing to parse");
    return;
  }

  Iap2ObjItem *item = nullptr;
  int protocolIndex = 0;
  int sessionIndex = 0;
  std::string protocolName;
  Iap2ObjGroup *rootGroup = native_message->getRootGroup();
  for (int i = 0; i < rootGroup->getItemCount(); i++) {
    item = rootGroup->getObjItem(i);
    switch (item->mParamType) {
    case IAP2_PARAM_NUMU8: {
      protocolIndex = item->getItemInt();

      switch (item->mParamId) {
      case eap_start_ProtocolIdentifier:
        LOG4CXX_DEBUG(logger_, "id = " << item->mParamId
                                       << " ProtocolIdentifier "
                                       << protocolIndex);
        break;
      }
    } break;
    case IAP2_PARAM_NUMU16: {
      sessionIndex = item->getItemInt();
      switch (item->mParamId) {
      case eap_start_SessionIdentifier:
        LOG4CXX_DEBUG(logger_, "id = " << item->mParamId << "SessionIdentifier "
                                       << sessionIndex);
        break;
      }
    } break;
    case IAP2_PARAM_STRING: {
      int strLen = 0;
      protocolName = reinterpret_cast<char *>(item->getItemString(&strLen));
      switch (item->mParamId) {
      case eap_start_ProtocolName:
        if (!protocolName.empty())
          LOG4CXX_DEBUG(logger_, "id = " << item->mParamId << "protocolName "
                                         << protocolName);
        break;
      }
    } break;
    default:
      break;
    }
  }
  if (!protocolName.empty()) {
    const ProtocolDescriptor &hub_prot = protocol_manager_->hub_protocol();
    if (protocolName == hub_prot.name()) {
      OnHubConnected(sessionIndex);
    }
  }
}

void IAP2ClientListener::OnEAPStop() {}

void IAP2ClientListener::OnEAPData() {}

void IAP2ClientListener::OnHubConnected(int session_index) {
  if (0 == session_index) {
    LOG4CXX_WARN(logger_, "IAP session not established.");
    return;
  }

  ProtocolDescriptor descriptor = protocol_manager_->PickProtocol();

  char prot_to_reconnect[1] = {static_cast<char>(descriptor.index())};
  iAP2NativeMsg msg = iAP2NativeMsg();
  msg.addIntItem(eap_data_SessionIdentifier, IAP2_PARAM_NUMU16, session_index);
  msg.addBlobItem(eap_data_Data, IAP2_PARAM_BLOB, prot_to_reconnect, 1);

  uint8_t *payload = msg.getRawNativeMsg();
  if (payload != NULL) {
    int length = 0;
    memcpy(&length, payload, 4);
    length += 4; // (AG)taken from sample. not sure why do we need +4.
    LOG4CXX_DEBUG(logger_, "Create dbus message with size: " << length);

    DBusMessage *dbus_msg = CreateDBusMsgMethodCall(
        IAP2_PROCESS_DBUS_NAME, IAP2_PROCESS_OBJECT_PATH,
        IAP2_PROCESS_INTERFACE, METHOD_iAP2_EAP_SEND_DATA, DBUS_TYPE_ARRAY,
        DBUS_TYPE_INT32, &payload, length, DBUS_TYPE_INVALID);

    if (dbus_msg != NULL) {
      DBusPendingCall *pending = NULL;
      if (!SendDBusMessage(dbus_msg, &pending)) {
        LOG4CXX_ERROR(logger_, "Failed to send DBus message.");
      }
      dbus_message_unref(dbus_msg);
    } else {
      LOG4CXX_ERROR(logger_, "Failed to create dbus message.");
    }
  }
}

void IAP2ClientListener::iAP2Notification(DBusMessage *message) {
  if (nullptr == message) {
    LOG4CXX_ERROR(logger_, "Null message from iAP. Skip processing.");
    return;
  }

  int notification_type, arg1, arg2, length;
  void *notification_data;

  if (GetArgumentFromDBusMessage(
          message, DBUS_TYPE_INT32, &notification_type, DBUS_TYPE_INT32, &arg1,
          DBUS_TYPE_INT32, &arg2, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
          &notification_data, &length, DBUS_TYPE_INVALID)) {
  }

  iAP2NativeMsg native_message =
      iAP2NativeMsg(static_cast<uint8_t *>(notification_data), length);

  switch (notification_type) {
  case IAP2_NOTI_EAP_START:
    OnEAPStart(&native_message);
    break;
  case IAP2_NOTI_EAP_STOP:
    OnEAPStop();
    break;
  case IAP2_NOTI_EAP_DATA:
    OnEAPData();
    break;
  default:
    LOG4CXX_INFO(logger_, "Signal: " << notification_data
                                     << "can't be processed.");
  }
}

void IAP2ClientListener::iAP2EventConnected() { LOG4CXX_AUTO_TRACE(logger_); }

} // namespace transport_adapter
} // namesapce transport_manager
