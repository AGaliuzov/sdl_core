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

#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_AOA_CONNECTION_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_AOA_CONNECTION_H_

#include "transport_manager/transport_adapter/connection.h"
#include "transport_manager/aoa/aoa_wrapper.h"

namespace transport_manager {
namespace transport_adapter {

class TransportAdapterController;

class AOAConnection : public Connection {
 public:
  AOAConnection(const DeviceUID& device_uid,
                const ApplicationHandle& app_handle,
                TransportAdapterController* controller,
                AOAWrapper::AOAHandle aoa_handle);
  virtual ~AOAConnection();
  bool Init();

 protected:
  virtual TransportAdapter::Error SendData(::protocol_handler::RawMessagePtr message);
  virtual TransportAdapter::Error Disconnect();

 private:
  static const uint32_t kTimeout = 2000; // ms
  AOAWrapper* wrapper_;
  AOAConnectionObserver* observer_;
  DeviceUID device_uid_;
  ApplicationHandle app_handle_;
  TransportAdapterController* controller_;

  void OnMessageReceived(bool success, ::protocol_handler::RawMessagePtr message);
  void OnMessageTransmitted(bool success, ::protocol_handler::RawMessagePtr message);
  void OnDisconnected();
  void ReceiveDone(::protocol_handler::RawMessagePtr message);
  void ReceiveFailed();
  void TransmitDone(::protocol_handler::RawMessagePtr message);
  void TransmitFailed(::protocol_handler::RawMessagePtr message);
  void Abort();

  class ConnectionObserver : public AOAConnectionObserver {
   public:
    explicit ConnectionObserver(AOAConnection* const parent);
    void OnMessageReceived(bool success, ::protocol_handler::RawMessagePtr message);
    void OnMessageTransmitted(bool success, ::protocol_handler::RawMessagePtr message);
    void OnDisconnected();
   private:
    AOAConnection* parent_;
  };
};

}  // namespace transport_adapter
}  // namespace transport_manager

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_AOA_CONNECTION_H_
