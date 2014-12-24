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
#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_SRC_AOA_AOA_LISTENER_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_SRC_AOA_AOA_LISTENER_H_

#include <map>

#include "utils/lock.h"
#include "utils/conditional_variable.h"

#include "transport_manager/transport_adapter/client_connection_listener.h"
#include "transport_manager/aoa/aoa_device.h"
#include "transport_manager/aoa/aoa_wrapper.h"

namespace transport_manager {
namespace transport_adapter {

class TransportAdapterController;

class AOAListener : public ClientConnectionListener {
 public:
  explicit AOAListener(TransportAdapterController* controller);

 protected:
  virtual TransportAdapter::Error Init();
  virtual void Terminate();
  virtual TransportAdapter::Error StartListening();
  virtual TransportAdapter::Error StopListening();
  virtual bool IsInitialised() const;

 private:
  static const std::string kPathToConfig;
  AOADeviceLife* life_;
  TransportAdapterController* controller_;
  sync_primitives::Lock life_lock_;
  sync_primitives::ConditionalVariable life_cond_;

  DeviceUID AddDevice(AOAWrapper::AOAHandle hdl);
  void LoopDevice(AOAWrapper::AOAHandle hdl);
  void StopDevice(AOAWrapper::AOAHandle hdl);
  void RemoveDevice(const DeviceUID& device_uid);

  std::string GetName(const std::string& unique_id);
  std::string GetUniqueId();

  class DeviceLife : public AOADeviceLife {
   public:
    explicit DeviceLife(AOAListener* parent);
    void Loop(AOAWrapper::AOAHandle hdl);
    void OnDied(AOAWrapper::AOAHandle hdl);
   private:
    AOAListener* parent_;
  };
};

}  // namespace transport_adapter
}  // namespace transport_manager



#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_SRC_AOA_AOA_LISTENER_H_
