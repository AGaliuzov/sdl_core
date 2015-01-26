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

#include "transport_manager/aoa/aoa_transport_adapter.h"

#include "utils/logger.h"
#include "transport_manager/aoa/aoa_connection_factory.h"
#ifdef CUSTOMER_PASA
#  include "transport_manager/aoa/pps_listener.h"
#else
#  include "transport_manager/aoa/aoa_listener.h"
#endif

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

AOATransportAdapter::AOATransportAdapter()
    : TransportAdapterImpl(0, new AOAConnectionFactory(this),
#ifdef CUSTOMER_PASA
                           new PPSListener(this)),
#else
                           new AOAListener(this)),
#endif  // CUSTOMER_PASA
      initialised_(false) {
}

AOATransportAdapter::~AOATransportAdapter() {
  initialised_ = false;
}

TransportAdapter::Error AOATransportAdapter::Init() {
  LOG4CXX_AUTO_TRACE(logger_);
  TransportAdapter::Error error = TransportAdapterImpl::Init();
  if (error != TransportAdapter::OK) {
    LOG4CXX_WARN(logger_, "AOA: Init error " << error);
    return error;
  }
  initialised_ = true;
  LOG4CXX_DEBUG(logger_, "AOA: Init success");
  return TransportAdapter::OK;
}

DeviceType AOATransportAdapter::GetDeviceType() const {
  return "pasa-aoa";
}

bool AOATransportAdapter::IsInitialised() const {
  return initialised_ && TransportAdapterImpl::IsInitialised();
}

void AOATransportAdapter::ApplicationListUpdated(
    const DeviceUID& device_handle) {
  ConnectDevice(device_handle);
}

}  // namespace transport_adapter
}  // namespace transport_manager

