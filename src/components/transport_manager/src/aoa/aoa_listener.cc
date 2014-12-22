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
#include "transport_manager/aoa/aoa_listener.h"

#include <sstream>

#include "utils/logger.h"

#include "transport_manager/transport_adapter/transport_adapter_controller.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

const std::string AOAListener::kPathToConfig = "";  // default on QNX /etc/mm/aoa.conf

AOAListener::AOAListener(TransportAdapterController* controller)
    : life_(NULL),
      controller_(controller) {
}

TransportAdapter::Error AOAListener::Init() {
  return TransportAdapter::OK;
}

void AOAListener::Terminate() {
}

bool AOAListener::IsInitialised() const {
  return true;
}

std::string AOAListener::GetName(const std::string& unique_id) {
  return "AOA device " + unique_id;
}

std::string AOAListener::GetUniqueId() {
  static int counter = 0;
  ++counter;
  std::ostringstream stream;
  stream << counter;
  return "#_" + stream.str() + "_aoa";
}

void AOAListener::AddDevice(AOAWrapper::AOAHandle hdl) {
  LOG4CXX_TRACE(logger_, "AOA: add new device " << hdl);
  const std::string unique_id = GetUniqueId();
  const std::string name = GetName(unique_id);
  DeviceSptr aoa_device(new AOADevice(hdl, name, unique_id));
  controller_->AddDevice(aoa_device);
  DeviceUID device_uid = aoa_device->unique_device_id();
  controller_->ApplicationListUpdated(device_uid);
}

void AOAListener::RemoveDevice(AOAWrapper::AOAHandle hdl) {
  LOG4CXX_TRACE(logger_, "AOA: remove device " << hdl);
}

void AOAListener::LoopDevice(AOAWrapper::AOAHandle hdl) {
  LOG4CXX_TRACE(logger_, "AOA: loop of life device " << hdl);
  sync_primitives::AutoLock locker(life_lock_);
  while (AOAWrapper::IsHandleValid(hdl)) {
    LOG4CXX_TRACE(logger_, "AOA: wait cond " << hdl);
    life_cond_.Wait(locker);
    // It does nothing because this method is called from libaoa thread so
    // if it returns from the method then thread will stop
    // and device will be disconnected
  }
}

void AOAListener::StopDevice(AOAWrapper::AOAHandle hdl) {
  LOG4CXX_TRACE(logger_, "AOA: stop device " << hdl);
  life_cond_.Broadcast();
}

AOAListener::DeviceLife::DeviceLife(AOAListener* parent)
    : parent_(parent) {
}

void AOAListener::DeviceLife::Loop(AOAWrapper::AOAHandle hdl) {
  parent_->AddDevice(hdl);
  parent_->LoopDevice(hdl);
  parent_->RemoveDevice(hdl);
}

void AOAListener::DeviceLife::OnDied(AOAWrapper::AOAHandle hdl) {
  parent_->StopDevice(hdl);
}

TransportAdapter::Error AOAListener::StartListening() {
  LOG4CXX_AUTO_TRACE(logger_);
  life_ = new DeviceLife(this);
  bool ret;
  if (kPathToConfig.empty()) {
    ret = AOAWrapper::Init(life_);
  } else {
    ret = AOAWrapper::Init(life_, kPathToConfig);
  }
  return ret ? TransportAdapter::OK : TransportAdapter::FAIL;
}

TransportAdapter::Error AOAListener::StopListening() {
  AOAWrapper::Shutdown();
  delete life_;
  life_ = NULL;
  return TransportAdapter::OK;
}

}  // namespace transport_adapter
}  // namespace transport_manager
