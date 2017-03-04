#include "transport_manager/iap2/iap2_connection.h"

#include <iap2/iAP2NativeMsg.h>
#include <iap2/Iap2DbHandler.h>
#include <iap2/TCiAP2TestUiDBusMsgDefNames.h>
#include <iap2/Iap2Const.h>
#include <dbus/dbus.h>
#include <dbus/TCDBusRawAPI.h>

#include "utils/logger.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager");

IAP2Connection::IAP2Connection(IAP2Device *parent, ApplicationHandle app_handle,
                               TransportAdapterController *controller)
    : parent_(parent), app_handle_(app_handle), controller_(controller),
      session_index_(0) {}

TransportAdapter::Error
IAP2Connection::SendData(protocol_handler::RawMessagePtr message) {

  if (0 == session_index_) {
    LOG4CXX_WARN(logger_, "IAP session not established.");
    return TransportAdapter::Error::FAIL;
  }
  iAP2NativeMsg *msg = new iAP2NativeMsg();
  msg->addIntItem(eap_data_SessionIdentifier, IAP2_PARAM_NUMU16,
                  session_index_);
  msg->addBlobItem(eap_data_Data, IAP2_PARAM_BLOB, message->data(),
                   static_cast<int>(message->data_size()));

  uint8_t *payload = msg->getRawNativeMsg();
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
    delete msg;
  }
}

TransportAdapter::Error IAP2Connection::Disconnect() {}

} // namespace transport_adapter
} // namesapce transport_manager
