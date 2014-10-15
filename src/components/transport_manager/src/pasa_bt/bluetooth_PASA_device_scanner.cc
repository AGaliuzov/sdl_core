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

#include "transport_manager/pasa_bt/bluetooth_PASA_device_scanner.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <string.h>
#include <sstream>
#include "transport_manager/pasa_bt/bluetooth_PASA_transport_adapter.h"
#include "transport_manager/pasa_bt/bluetooth_PASA_device.h"
#include "transport_manager/pasa_bt/message_queue.h"

#include "utils/logger.h"

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
}  //  namespace

BluetoothPASADeviceScanner::BluetoothPASADeviceScanner(
  TransportAdapterController* controller, bool auto_repeat_search,
  int auto_repeat_pause_sec)
  : controller_(controller),
    bt_device_scanner_thread_(NULL),
    bt_PASA_msg_thread_(NULL),
    thread_started_(false),
    shutdown_requested_(false),
    device_scan_requested_(false),
    device_scan_requested_cv_(),
    auto_repeat_search_(auto_repeat_search),
    auto_repeat_pause_sec_(auto_repeat_pause_sec) ,
    mPASAFWSendHandle(OpenMsgQ(PREFIX_STR_FROMSDLCOREBTADAPTER_QUEUE, true, true)),
    mq_ToSDL(NULL){
  bt_device_scanner_thread_ = threads::CreateThread("BT PASA Scanner", new  DeviceScannerDelegate(this));
  bt_PASA_msg_thread_ = threads::CreateThread("BT PASA Framwork Messages", new  PASAMessageDelegate(this));
}


BluetoothPASADeviceScanner::DeviceScannerDelegate::DeviceScannerDelegate(
    BluetoothPASADeviceScanner* scanner)
  : scanner_(scanner) {
}

void BluetoothPASADeviceScanner::DeviceScannerDelegate::threadMain() {
  LOG4CXX_TRACE_ENTER(logger_);
  scanner_->Thread();
  LOG4CXX_TRACE_EXIT(logger_);
}

BluetoothPASADeviceScanner::PASAMessageDelegate::PASAMessageDelegate(
    BluetoothPASADeviceScanner* scanner)
  : scanner_(scanner) {
}

void BluetoothPASADeviceScanner::PASAMessageDelegate::threadMain() {
  LOG4CXX_TRACE_ENTER(logger_);
  scanner_->mq_ToSDL = OpenMsgQ(PREFIX_STR_TOSDLCOREBTADAPTER_QUEUE, false, true);

  char buffer[MAX_QUEUE_MSG_SIZE];
  while (!scanner_->shutdown_requested_) {
    const ssize_t length = mq_receive(scanner_->mq_ToSDL, buffer, sizeof(buffer), 0);
    if (length == -1) {
      continue;
    };
    switch (buffer[0]) {
      case SDL_MSG_BT_DEVICE_CONNECT: {
          LOG4CXX_INFO(logger_, "Received PASA FW BT SPP Connect Message");
          scanner_->connectBTDevice(&buffer[1]);
        }
        break;
      case SDL_MSG_BT_DEVICE_SPP_DISCONNECT: {
          LOG4CXX_INFO(logger_, "Received PASA FW BT SPP Disconnect Message");
          scanner_->disconnectBTDeviceSPP(&buffer[1]);
        }
        break;
      case SDL_MSG_BT_DEVICE_DISCONNECT: {
          LOG4CXX_INFO(logger_, "Received PASA FW BT Disconnect Message");
          scanner_->disconnectBTDevice(&buffer[1]);
        }
        break;
      case SDL_MSG_BT_DEVICE_CONNECT_END: {
          LOG4CXX_INFO(logger_, "Received PASA FW BT SPP END Message");
          scanner_->UpdateTotalDeviceList();
          scanner_->UpdateTotalApplicationList();
        }
        break;
      default:
        LOG4CXX_WARN(logger_, "Received PASA FW BT Unknown Message");
        break;
    }
  }
  LOG4CXX_TRACE_EXIT(logger_);
}

bool BluetoothPASADeviceScanner::IsInitialised() const {
  return thread_started_;
}

void BluetoothPASADeviceScanner::UpdateTotalDeviceList() {
  LOG4CXX_TRACE_ENTER(logger_);
  DeviceVector devices;
  devices_lock_.Acquire();
  devices.insert(devices.end(), found_devices_with_sdl_.begin(),
                 found_devices_with_sdl_.end());
  devices_lock_.Release();
  controller_->SearchDeviceDone(devices);
  LOG4CXX_TRACE_EXIT(logger_);
}

void BluetoothPASADeviceScanner::UpdateTotalApplicationList() {
  LOG4CXX_TRACE_ENTER(logger_);
  controller_->FindNewApplicationsRequest();
  LOG4CXX_TRACE_EXIT(logger_);
}

void BluetoothPASADeviceScanner::connectBTDevice(void *data) {
  const PBTDeviceConnectInfo pDeviceInfo =
      static_cast<PBTDeviceConnectInfo>(data);
  BluetoothPASADevice* device = NULL;
  sync_primitives::AutoLock lock(devices_lock_);
  for (DeviceVector::const_iterator i = found_devices_with_sdl_.begin();
       i != found_devices_with_sdl_.end(); ++i) {
    BluetoothPASADevice* existing_device = static_cast<BluetoothPASADevice*>(i->get());
    if (0 == memcmp(existing_device->mac(), pDeviceInfo->mac, sizeof(pDeviceInfo->mac))) {
      LOG4CXX_DEBUG(logger_, "Bluetooth device exists: " << pDeviceInfo->cDeviceName);
      LOG4CXX_INFO(logger_, "Bluetooth channel " << pDeviceInfo->cSppQueName
                   << " added to "<< pDeviceInfo->cDeviceName << " " << existing_device);
      device = existing_device;
      break;
    }
  }
  if (!device) {
    device = new BluetoothPASADevice(pDeviceInfo->cDeviceName, pDeviceInfo->mac);
    LOG4CXX_INFO(logger_, "Bluetooth device created successfully: "<< pDeviceInfo->cDeviceName);
    found_devices_with_sdl_.push_back(device);
  }
  const BluetoothPASADevice::SCOMMChannel tChannel(pDeviceInfo->cSppQueName);
  device->AddChannel(tChannel);
  SendMsgQ(mPASAFWSendHandle, SDL_MSG_BT_DEVICE_CONNECT_ACK, 0, NULL);
}

void BluetoothPASADeviceScanner::disconnectBTDevice(void *data) {
  const PBTDeviceDisconnectInfo pDeviceInfo =
      static_cast<PBTDeviceDisconnectInfo>(data);
  const DeviceUID device_id = MacToString(pDeviceInfo->mac);
  controller_->DisconnectDevice(device_id);
  SendMsgQ(mPASAFWSendHandle, SDL_MSG_BT_DEVICE_DISCONNECT_ACK, 0, NULL);
}

void BluetoothPASADeviceScanner::disconnectBTDeviceSPP(void *data) {
  const PBTDeviceDisConnectSPPInfo pDeviceInfo =
      static_cast<PBTDeviceDisConnectSPPInfo>(data);
  sync_primitives::AutoLock lock(devices_lock_);
  for (DeviceVector::iterator i = found_devices_with_sdl_.begin();
       i != found_devices_with_sdl_.end(); ++i) {
    BluetoothPASADevice* device = static_cast<BluetoothPASADevice*>(i->get());
    if (0 == memcmp(device->mac(), pDeviceInfo->mac, sizeof(pDeviceInfo->mac))) {
      BluetoothPASADevice::SCOMMChannel tChannel(pDeviceInfo->cSppQueName);
      ApplicationHandle disconnected_app;
      const bool found = device->RemoveChannel(tChannel, &disconnected_app);
      if (found) {
        controller_->AbortConnection(device->unique_device_id(), disconnected_app);
      }
      break;
    }
  }
}

void BluetoothPASADeviceScanner::Thread() {
  LOG4CXX_TRACE_ENTER(logger_);
  LOG4CXX_DEBUG(logger_, "Bluetooth adapter main thread initialized");
  if (auto_repeat_search_) {
    while (!shutdown_requested_) {
      device_scan_requested_ = false;
      TimedWaitForDeviceScanRequest();
    }
  } else {  // search only on demand
    while (true) {
      {
        sync_primitives::AutoLock auto_lock(device_scan_requested_lock_);
        while (!(device_scan_requested_ || shutdown_requested_)) {
          device_scan_requested_cv_.Wait(auto_lock);
        }
      }
      if (shutdown_requested_) {
        break;
      }
      device_scan_requested_ = false;
    }
  }
  LOG4CXX_INFO(logger_, "PASA Bluetooth device scanner thread finished");
  LOG4CXX_TRACE_EXIT(logger_);
}

void BluetoothPASADeviceScanner::TimedWaitForDeviceScanRequest() {
  LOG4CXX_TRACE_ENTER(logger_);
  if (auto_repeat_pause_sec_ != 0) {
    sync_primitives::AutoLock auto_lock(device_scan_requested_lock_);
    while (!(device_scan_requested_ || shutdown_requested_)) {
      const sync_primitives::ConditionalVariable::WaitStatus wait_status =
          device_scan_requested_cv_.WaitFor(auto_lock, auto_repeat_pause_sec_ * 1000);
      if (wait_status == sync_primitives::ConditionalVariable::kTimeout) {
        LOG4CXX_INFO(logger_, "Bluetooth scanner timeout, performing scan");
        device_scan_requested_ = true;
      }
    }
  }
  LOG4CXX_TRACE_EXIT(logger_);
}

TransportAdapter::Error BluetoothPASADeviceScanner::Init() {
  LOG4CXX_TRACE_ENTER(logger_);

  found_devices_with_sdl_.clear();
  DCHECK(bt_PASA_msg_thread_);
  if(!bt_PASA_msg_thread_->start()) {
    LOG4CXX_ERROR( logger_, "PASA incoming messages thread start failed");
    LOG4CXX_TRACE_EXIT(logger_);
    return TransportAdapter::FAIL;
  }
  LOG4CXX_DEBUG(logger_, "PASA incoming messages thread started");
 DCHECK(bt_PASA_msg_thread_);
  if(!bt_device_scanner_thread_->start()) {
    LOG4CXX_ERROR( logger_, "BT device scanner thread start failed");
    LOG4CXX_TRACE_EXIT(logger_);
    return TransportAdapter::FAIL;
  }
  LOG4CXX_DEBUG(logger_, "Bluetooth device scanner thread started");

  thread_started_ = true;
  LOG4CXX_TRACE_EXIT(logger_);
  return TransportAdapter::OK;
}

void BluetoothPASADeviceScanner::Terminate() {
  LOG4CXX_TRACE_ENTER(logger_);
  shutdown_requested_ = true;
  CloseMsgQ(mq_ToSDL);
  //TODO(EZamakhov): check stability
  CloseMsgQ(mPASAFWSendHandle);

  DCHECK(bt_PASA_msg_thread_);
  if (bt_device_scanner_thread_->is_running()) {
    LOG4CXX_INFO(logger_, "Waiting for PASA bluetooth device scanner threads termination");
    {
      sync_primitives::AutoLock auto_lock(device_scan_requested_lock_);
      device_scan_requested_ = false;
      device_scan_requested_cv_.NotifyOne();
    }
    bt_PASA_msg_thread_->stop();
    bt_device_scanner_thread_->stop();
    LOG4CXX_INFO(logger_, "PASA Bluetooth device scanner threads finished.");
  }
  LOG4CXX_TRACE_EXIT(logger_);
}

TransportAdapter::Error BluetoothPASADeviceScanner::Scan() {
  LOG4CXX_TRACE_ENTER(logger_);
  if ((!thread_started_) || shutdown_requested_) {
    LOG4CXX_INFO(logger_, "bad state");
    return TransportAdapter::BAD_STATE;
  }
  TransportAdapter::Error ret = TransportAdapter::OK;
  {
    sync_primitives::AutoLock auto_lock(device_scan_requested_lock_);
    if (false == device_scan_requested_) {
      LOG4CXX_INFO(logger_, "Requesting device Scan");
      device_scan_requested_ = true;
      device_scan_requested_cv_.NotifyOne();
    } else {
      ret = TransportAdapter::BAD_STATE;
      LOG4CXX_INFO(logger_, "Device Scan is currently in progress");
    }
  }

  LOG4CXX_TRACE_EXIT(logger_);
  return ret;
}

}  // namespace transport_adapter
}  // namespace transport_manager

