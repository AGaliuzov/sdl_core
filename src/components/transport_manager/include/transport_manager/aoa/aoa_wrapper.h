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

#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_AOA_WRAPPER_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_AOA_WRAPPER_H_

#include <stdint.h>
#include <vector>
#include <string>

#include "protocol/common.h"

struct aoa_hdl_s;
struct usb_info_s;

namespace transport_manager {
namespace transport_adapter {

enum AOAVersion {
  AOA_Ver_Unknown,
  AOA_Ver_1_0,
  AOA_Ver_2_0
};
enum AOAMode {
  AOA_Mode_Accessory,
  AOA_Mode_Audio,
  AOA_Mode_Debug
};
enum AOAEndpoint {
  AOA_Ept_Accessory_BulkIn,
  AOA_Ept_Accessory_BulkOut,
  AOA_Ept_Accessory_Control
};

class AOADeviceLife;
class AOAConnectionObserver;

class AOAWrapper {
 public:
  typedef aoa_hdl_s* AOAHandle;
  struct AOAUsbInfo {
    std::string path; /* Path to the USB stack */
    int aoa_version;
    uint32_t devno; /* Device number */
    uint32_t busno; /* Device bus number */
    std::string manufacturer;
    uint32_t vendor_id;
    std::string product;
    uint32_t product_id;
    std::string serial_number;
    std::string stackno;
    uint32_t iface; /* Device interface */
  };

  static bool Init(AOADeviceLife* life);
  static bool Init(AOADeviceLife* life, const std::string& config_path);
  static bool Init(AOADeviceLife* life, const AOAWrapper::AOAUsbInfo& aoa_usb_info);
  static bool Shutdown();
  static bool IsHandleValid(AOAWrapper::AOAHandle hdl);
  static void OnDied(AOAWrapper::AOAHandle hdl);
  static inline bool IsError(int ret);
  static inline void PrintError(int ret);

  explicit AOAWrapper(AOAHandle hdl);
  AOAWrapper(AOAHandle hdl, uint32_t timeout);
  ~AOAWrapper();

  bool IsHandleValid() const;
  AOAVersion GetProtocolVesrion() const;
  uint32_t GetBufferMaximumSize(AOAEndpoint endpoint) const;
  std::vector<AOAMode> GetModes() const;
  std::vector<AOAEndpoint> GetEndpoints() const;
  bool Subscribe(AOAConnectionObserver* observer);
  bool Unsubscribe();
  bool SendMessage(::protocol_handler::RawMessagePtr message) const;
  bool SendControlMessage(uint16_t request, uint16_t value, uint16_t index,
                          ::protocol_handler::RawMessagePtr message) const;
  ::protocol_handler::RawMessagePtr ReceiveMessage() const;
  ::protocol_handler::RawMessagePtr ReceiveControlMessage(uint16_t request, uint16_t value,
                                      uint16_t index) const;

  static const uint32_t kBufferSize = 32768;

 private:
  static AOADeviceLife* life_;

  AOAHandle hdl_;
  uint8_t* buffer_;
  uint32_t timeout_;
  AOAConnectionObserver* connection_observer_;

  static bool Init(AOADeviceLife* life, const char* config_path,
                   usb_info_s* usb_info);
  static void PrepareUsbInfo(const AOAUsbInfo& aoa_usb_info,
                             usb_info_s* usb_info);

  inline AOAVersion Version(uint16_t version) const;
  inline static uint32_t BitEndpoint(AOAEndpoint endpoint);
  inline bool IsValueInMask(uint32_t bitmask, uint32_t value) const;
  std::vector<AOAMode> CreateModesList(uint32_t modes_mask) const;
  std::vector<AOAEndpoint> CreateEndpointsList(uint32_t endpoints_mask) const;
  bool SetCallback(AOAEndpoint endpoint) const;
  bool UnsetCallback(AOAEndpoint endpoint) const;
};

class AOADeviceLife {
 public:
  virtual void Loop(AOAWrapper::AOAHandle handle) = 0;
  virtual void OnDied(AOAWrapper::AOAHandle handle) = 0;
  virtual ~AOADeviceLife() {
  }
};

class AOAConnectionObserver {
 public:
  virtual void OnMessageReceived(bool success, ::protocol_handler::RawMessagePtr message) = 0;
  virtual void OnMessageTransmitted(bool success, ::protocol_handler::RawMessagePtr message) = 0;
  virtual void OnDisconnected() = 0;
  virtual ~AOAConnectionObserver() {
  }
};

}  // namespace transport_adapter
}  // namespace transport_manager

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_AOA_WRAPPER_H_
