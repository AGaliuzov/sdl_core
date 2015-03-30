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
#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_AOA_DYNAMIC_DEVICE_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_AOA_DYNAMIC_DEVICE_H_

#include <map>
#include <string>

#include "utils/lock.h"
#include "utils/conditional_variable.h"

#include "transport_manager/aoa/aoa_device.h"

namespace transport_manager {
namespace transport_adapter {

class TransportAdapterController;

class AOADynamicDevice : public AOADevice {
 public:
  AOADynamicDevice(const std::string& name, const DeviceUID& unique_id,
                   const AOAWrapper::AOAUsbInfo& info,
                   TransportAdapterController* controller);
  ~AOADynamicDevice();

  bool Init();

 private:
  AOADeviceLife* life_;
  TransportAdapterController* controller_;
  AOAWrapper::AOAUsbInfo aoa_usb_info_;
  sync_primitives::Lock life_lock_;
  sync_primitives::ConditionalVariable life_cond_;
  DeviceUID lastDevice_;

  void AddDevice(AOAWrapper::AOAHandle handle);
  void LoopDevice(AOAWrapper::AOAHandle handle);
  void StopDevice(AOAWrapper::AOAHandle handle);

  class DeviceLife : public AOADeviceLife {
   public:
    explicit DeviceLife(AOADynamicDevice* parent);
    void Loop(AOAWrapper::AOAHandle handle);
    void OnDied(AOAWrapper::AOAHandle handle);
   private:
    AOADynamicDevice* parent_;
  };
};

typedef utils::SharedPtr<AOADynamicDevice> AOADynamicDeviceSPtr;

}  // namespace transport_adapter
}  // namespace transport_manager

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_AOA_DYNAMIC_DEVICE_H_
