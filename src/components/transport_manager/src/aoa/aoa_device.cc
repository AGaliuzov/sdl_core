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

#include "transport_manager/aoa/aoa_device.h"

#include "utils/logger.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

AOADevice::AOADevice(const std::string& name,
                     const DeviceUID& unique_id)
    : Device(name, unique_id),
      handle_(0) {
}

AOADevice::AOADevice(AOAWrapper::AOAHandle handle,
                     const std::string& name,
                     const DeviceUID& unique_id)
    : Device(name, unique_id),
      handle_(handle) {
}

bool AOADevice::IsSameAs(const Device* other_device) const {
  const AOADevice* other_aoa_device =
      static_cast<const AOADevice*>(other_device);
  if (other_aoa_device) {
    return other_aoa_device->unique_device_id() == unique_device_id();
  } else {
    return false;
  }
}

ApplicationList AOADevice::GetApplicationList() const {
  // Device has got only one application
  const ApplicationList::size_type kSize = 1;
  const ApplicationList::value_type kNumberApplication = 1;
  return ApplicationList(kSize, kNumberApplication);
}

AOAWrapper::AOAHandle AOADevice::handle() const {
  return handle_;
}

void AOADevice::set_handle(AOAWrapper::AOAHandle handle) {
  handle_ = handle;
}

}  // namespace transport_adapter
}  // namespace transport_manager
