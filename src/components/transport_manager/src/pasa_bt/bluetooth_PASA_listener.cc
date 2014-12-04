/*
 * Copyright (c) 2014, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "transport_manager/pasa_bt/bluetooth_PASA_listener.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <string.h>
#include <sstream>

#include "utils/logger.h"
#include "utils/threads/thread.h"
#include "transport_manager/pasa_bt/bluetooth_PASA_transport_adapter.h"
#include "transport_manager/pasa_bt/bluetooth_PASA_device.h"
#include "transport_manager/pasa_bt/message_queue.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

namespace {
char* SplitToAddr(char* dev_list_entry) {
  char* bl_address = strtok(dev_list_entry, "()");
  if (bl_address != NULL) {
    bl_address = strtok(NULL, "()");
    return bl_address;
  } else {
    return NULL;
  }
}
}  // namespace

BluetoothPASAListener::BluetoothPASAListener(
    TransportAdapterController* controller)
    : controller_(controller),
      thread_(NULL),
      mq_from_sdl_(-1),
      mq_to_sdl_(-1) {
}

BluetoothPASAListener::~BluetoothPASAListener() {
  StopListening();
  Terminate();
}

void BluetoothPASAListener::ListeningThreadDelegate::threadMain() {
  parent_->Loop();
}

BluetoothPASAListener::ListeningThreadDelegate::ListeningThreadDelegate(
    BluetoothPASAListener* parent)
    : parent_(parent) {
}

void BluetoothPASAListener::Loop() {
  LOG4CXX_AUTO_TRACE(logger_);

  char buffer[MAX_QUEUE_MSG_SIZE];
  while (true) {
    const ssize_t length = mq_receive(mq_to_sdl_, buffer, sizeof(buffer), 0);
    if (length == -1) {
      LOG4CXX_WARN(logger_, "Can not receive message from PASA FW BT");
      continue;
    };
    switch (buffer[0]) {
      case SDL_MSG_BT_DEVICE_CONNECT: {
        LOG4CXX_INFO(logger_, "Received PASA FW BT SPP Connect Message");
        ConnectDevice(&buffer[1]);
      }
        break;
      case SDL_MSG_BT_DEVICE_SPP_DISCONNECT: {
        LOG4CXX_INFO(logger_, "Received PASA FW BT SPP Disconnect Message");
        DisconnectSPP(&buffer[1]);
      }
        break;
      case SDL_MSG_BT_DEVICE_DISCONNECT: {
        LOG4CXX_INFO(logger_, "Received PASA FW BT Disconnect Message");
        DisconnectDevice(&buffer[1]);
      }
        break;
      case SDL_MSG_BT_DEVICE_CONNECT_END: {
        LOG4CXX_INFO(logger_, "Received PASA FW BT SPP END Message");
        UpdateTotalApplicationList();
      }
        break;
      default:
        LOG4CXX_WARN(logger_, "Received PASA FW BT Unknown Message");
        break;
    }
  }
}

bool BluetoothPASAListener::IsInitialised() const {
  return mq_from_sdl_ != -1;
}

void BluetoothPASAListener::UpdateTotalApplicationList() {
  LOG4CXX_AUTO_TRACE(logger_);
  controller_->FindNewApplicationsRequest();
}

void BluetoothPASAListener::ConnectDevice(void *data) {
  LOG4CXX_AUTO_TRACE(logger_);
  const PBTDeviceConnectInfo pDeviceInfo =
      static_cast<PBTDeviceConnectInfo>(data);
  const DeviceUID device_id = MacToString(pDeviceInfo->mac);
  DeviceSptr device = controller_->FindDevice(device_id);
  if (!device) {
    device = new BluetoothPASADevice(pDeviceInfo->cDeviceName,
                                     pDeviceInfo->mac);
    controller_->AddDevice(device);
    LOG4CXX_DEBUG(
        logger_,
        "Bluetooth device created successfully: " << pDeviceInfo->cDeviceName);
  } else {
    LOG4CXX_DEBUG(logger_,
                  "Bluetooth device exists: " << pDeviceInfo->cDeviceName);
  }
  const BluetoothPASADevice::SCOMMChannel tChannel(pDeviceInfo->cSppQueName);
  BluetoothPASADevicePtr bt_device = DeviceSptr::static_pointer_cast<
      BluetoothPASADevice>(device);
  bt_device->AddChannel(tChannel);
  LOG4CXX_INFO(
      logger_,
      "Bluetooth channel " << pDeviceInfo->cSppQueName << " added to " << pDeviceInfo->cDeviceName);
  SendMsgQ(mq_from_sdl_, SDL_MSG_BT_DEVICE_CONNECT_ACK, 0, NULL);
}

void BluetoothPASAListener::DisconnectDevice(void *data) {
  LOG4CXX_AUTO_TRACE(logger_);
  const PBTDeviceDisconnectInfo pDeviceInfo =
      static_cast<PBTDeviceDisconnectInfo>(data);
  const DeviceUID device_id = MacToString(pDeviceInfo->mac);
  controller_->DisconnectDevice(device_id);
  SendMsgQ(mq_from_sdl_, SDL_MSG_BT_DEVICE_DISCONNECT_ACK, 0, NULL);
}

void BluetoothPASAListener::DisconnectSPP(void *data) {
  LOG4CXX_AUTO_TRACE(logger_);
  const PBTDeviceDisConnectSPPInfo pDeviceInfo =
      static_cast<PBTDeviceDisConnectSPPInfo>(data);
  const DeviceUID device_id = MacToString(pDeviceInfo->mac);
  DeviceSptr device = controller_->FindDevice(device_id);
  if (device) {
    BluetoothPASADevice::SCOMMChannel tChannel(pDeviceInfo->cSppQueName);
    ApplicationHandle disconnected_app;
    BluetoothPASADevicePtr bt_device = DeviceSptr::static_pointer_cast<
        BluetoothPASADevice>(device);
    const bool found = bt_device->RemoveChannel(tChannel, &disconnected_app);
    if (found) {
      controller_->AbortConnection(device->unique_device_id(),
                                   disconnected_app);
    }
  }
}

TransportAdapter::Error BluetoothPASAListener::Init() {
  LOG4CXX_AUTO_TRACE(logger_);
  mq_from_sdl_ = OpenMsgQ(PREFIX_STR_FROMSDLCOREBTADAPTER_QUEUE, true, true);
  return mq_from_sdl_ != -1 ? TransportAdapter::OK : TransportAdapter::FAIL;
}

void BluetoothPASAListener::Terminate() {
  LOG4CXX_AUTO_TRACE(logger_);
  CloseMsgQ(mq_from_sdl_);
  mq_from_sdl_ = -1;
}

TransportAdapter::Error BluetoothPASAListener::StartListening() {
  LOG4CXX_AUTO_TRACE(logger_);
  mq_to_sdl_ = OpenMsgQ(PREFIX_STR_TOSDLCOREBTADAPTER_QUEUE, false, true);
  if (mq_to_sdl_ != -1) {
    thread_ = threads::CreateThread("BT PASA", new ListeningThreadDelegate(this));
    thread_->start();
    LOG4CXX_INFO(logger_, "PASA incoming messages thread started");
    return TransportAdapter::OK;
  } else {
    LOG4CXX_ERROR(logger_, "PASA incoming messages thread start failed");
    return TransportAdapter::FAIL;
  }
}

TransportAdapter::Error BluetoothPASAListener::StopListening() {
  LOG4CXX_AUTO_TRACE(logger_);
  if (!thread_) {
    LOG4CXX_ERROR(logger_, "PASA incoming messages thread stop failed");
    return TransportAdapter::BAD_STATE;
  }
  thread_->join();
  delete thread_->delegate();
  threads::DeleteThread(thread_);
  thread_ = NULL;
  CloseMsgQ(mq_to_sdl_);
  mq_to_sdl_ = -1;
  LOG4CXX_INFO(logger_, "PASA incoming messages thread stopped");
  return TransportAdapter::OK;
}

}  // namespace transport_adapter
}  // namespace transport_manager
