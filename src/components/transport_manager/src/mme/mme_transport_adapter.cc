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

#include "transport_manager/mme/mme_transport_adapter.h"
#include "transport_manager/mme/mme_connection_factory.h"
#include "transport_manager/mme/mme_client_listener.h"

namespace transport_manager {
namespace transport_adapter {

MmeTransportAdapter::MmeTransportAdapter()
    : TransportAdapterImpl(NULL,
                           new MmeConnectionFactory(this),
                           new MmeClientListener(this)),
      initialised_(false) {
}

DeviceType MmeTransportAdapter::GetDeviceType() const {
  return "sdl-mme";
}

bool MmeTransportAdapter::IsInitialised() const {
  return initialised_;
}

TransportAdapter::Error MmeTransportAdapter::Init() {
  TransportAdapter::Error error = TransportAdapterImpl::Init();
  if (TransportAdapter::OK == error) {
    initialised_ = true;
  }
#if QNX_BARE_SYSTEM_WORKAROUND
  device_scanner_->Scan();
#endif
  return error;
}

void MmeTransportAdapter::ApplicationListUpdated(
    const DeviceUID& device_handle) {
  ConnectDevice(device_handle);
}

bool MmeTransportAdapter::ToBeAutoConnected(DeviceSptr device) const {
  return true;
}

}  // namespace transport_adapter
}  // namespace transport_manager
