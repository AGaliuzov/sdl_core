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

#include "transport_manager/aoa/aoa_dynamic_device.h"

#include "utils/logger.h"

#include "transport_manager/aoa/aoa_wrapper.h"
#include "transport_manager/transport_adapter/transport_adapter_controller.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

AOADynamicDevice::AOADynamicDevice(const std::string& name,
                                   const DeviceUID& unique_id,
                                   const AOAWrapper::AOAUsbInfo& info,
                                   TransportAdapterController* controller)
    : AOADevice(name, unique_id),
      life_(new DeviceLife(this)),
      controller_(controller),
      aoa_usb_info_(info) {
  LOG4CXX_AUTO_TRACE(logger_);
  LOG4CXX_DEBUG(logger_, "AOA: device " << unique_device_id());
}

AOADynamicDevice::~AOADynamicDevice() {
  LOG4CXX_AUTO_TRACE(logger_);
  LOG4CXX_DEBUG(logger_, "AOA: device " << unique_device_id());
  life_cond_.NotifyOne();
  delete life_;
}

bool AOADynamicDevice::StartHandling() {
  return AOAWrapper::HandleDevice(life_, aoa_usb_info_);
}

void AOADynamicDevice::AddDevice(AOAWrapper::AOAHandle handle) {
  LOG4CXX_AUTO_TRACE(logger_);
  set_handle(handle);
  controller_->ApplicationListUpdated(unique_device_id());
}

void AOADynamicDevice::LoopDevice(AOAWrapper::AOAHandle handle) {
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock locker(life_lock_);
  while (AOAWrapper::IsHandleValid(handle)) {
    LOG4CXX_TRACE(logger_, "AOA: wait cond " << handle);
    if (!life_cond_.Wait(locker)) {
      break;
    }
    // It does nothing because this method is called from libaoa thread so
    // if it returns from the method then thread will stop
    // and device will be disconnected
  }
}

void AOADynamicDevice::StopDevice(AOAWrapper::AOAHandle handle) {
  LOG4CXX_TRACE(logger_, "AOA: stop device " << handle);
  life_cond_.Broadcast();
}

bool AOADynamicDevice::Ack() {
  LOG4CXX_AUTO_TRACE(logger_);
  if (!AOAWrapper::IsHandleValid(handle())) {
    LOG4CXX_DEBUG(logger_, "Device is about to be die.");
    life_->OnDied(handle());
    return false;
  }
  return true;
}

AOADynamicDevice::DeviceLife::DeviceLife(AOADynamicDevice* parent)
    : parent_(parent) {
}

void AOADynamicDevice::DeviceLife::Loop(AOAWrapper::AOAHandle handle) {
  parent_->AddDevice(handle);
  parent_->LoopDevice(handle);
}

void AOADynamicDevice::DeviceLife::OnDied(AOAWrapper::AOAHandle handle) {
  parent_->StopDevice(handle);
}

}  // namespace transport_adapter
}  // namespace transport_manager
