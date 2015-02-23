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

#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_DEVICE_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_DEVICE_H_

#include "transport_manager/transport_adapter/device.h"

namespace transport_manager {
namespace transport_adapter {

/**
 * @brief Class representing device maintained by QNX MME subsystem
 *
 * Currently only Apple Inc. devices connected over USB via iAP/iAP2 protocol are supported
 **/
class MmeDevice : public Device {
 public:
   /**
    * @enum Protocol
    *
    * suppported protocols for MME devices
    *
    * \a IAP - iAP device
    * \a iAP2 - iAP2 device
    * \a UnknownProtocol - unsupported protocol
    */
  typedef enum {
    UnknownProtocol,
    IAP,
    IAP2
  } Protocol;

  /**
   * @brief Constructor
   *
   * @param mount_point Path device is mounted to
   * @param name User-friendly device name.
   * @param unique_device_id device unique identifier.
   */
  MmeDevice(const std::string& mount_point, const std::string& name,
            const DeviceUID& unique_device_id);

  /**
   * @brief Path device is mounted to
   */
  const std::string& mount_point() const {
    return mount_point_;
  }

  /**
   * @brief Initialize MME device
   * @return \a true on success, \a false otherwise
   */
  virtual bool Init() = 0;

  /**
   * @brief Protocol device is connected over
   */
  virtual Protocol protocol() const = 0;

 protected:
  /**
   * @brief Compare devices.
   *
   * This method checks whether two Device structures
   * refer to the same device.
   *
   * @param other_device Device to compare with.
   *
   * @return true if devices are equal, false otherwise.
   **/
  virtual bool IsSameAs(const Device* other_device) const;

  /**
   * @brief Callback for connection timeout
   * @param protocol_name Connection protocol
   * (default implementation does nothing)
   */
  virtual void OnConnectionTimeout(const std::string& protocol_name);

 private:
  std::string mount_point_;

  friend class ProtocolConnectionTimer;
};

/**
 * @typedef Smart pointer to MME device
 */
typedef utils::SharedPtr<MmeDevice> MmeDevicePtr;

/**
 * @brief Output MME device protocol to stream
 * @tparam Stream Output stream class
 * @param stream Output stream
 * @param protocol MME device protocol
 * @return Output stream
 */
template<typename Stream>
Stream& operator << (Stream& stream, MmeDevice::Protocol protocol) {
  switch (protocol) {
    case MmeDevice::IAP:
      stream << "iAP";
      break;
    case MmeDevice::IAP2:
      stream << "iAP2";
      break;
    case MmeDevice::UnknownProtocol:
      stream << "unknown protocol";
      break;
  }
  return stream;
}

}  // namespace transport_adapter
}  // namespace transport_manager

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_DEVICE_H_
