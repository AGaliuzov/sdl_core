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

#include "transport_manager/aoa/aoa_wrapper.h"

#include <string>

#include "utils/macro.h"
#include "utils/logger.h"
#include "utils/make_shared.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")
#define RETURN_ON_ERROR(value) \
  if (IsError(value)) { \
    PrintError(value); \
    return false; \
  }

bool AOAWrapper::AOAUsbInfo::operator<(const AOAWrapper::AOAUsbInfo& other) const {
  return object_name < other.object_name;
}

static void OnReceivedData(aoa_hdl_t* hdl, uint8_t* data, uint32_t sz,
                           uint32_t status, const void* udata) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (udata == NULL) {
    LOG4CXX_ERROR(logger_, "No object to send data to.");
    return;
  }

  const AOAWrapper* aoa_wrapper = reinterpret_cast<const AOAWrapper*>(udata);
  LOG4CXX_DEBUG(logger_, "AOA: received data from device " << hdl);
  const bool error = aoa_wrapper->IsError(status);
  if (error) {
    aoa_wrapper->PrintError(status);
  }

  if (!aoa_wrapper->IsHandleValid(hdl) || (status == AOA_EINVALIDHDL)) {
    aoa_wrapper->OnDied();
    return;
  }

  bool success = !error;
  protocol_handler::RawMessagePtr message;
  if (data) {
    message = utils::MakeShared<protocol_handler::RawMessage>(0, 0, data, sz);
  } else {
    LOG4CXX_ERROR(logger_, "AOA: data is null");
    success = false;
  }

  aoa_wrapper->OnMessageReceived(success, message);

  aoa_buffer_free(hdl, data);
  if (aoa_wrapper->SetCallback(AOA_TIMEOUT_INFINITY, AOA_Ept_Accessory_BulkIn)) {
    LOG4CXX_DEBUG(logger_, "AOA: data receive callback reregistered");
  } else {
    LOG4CXX_ERROR(logger_,
                  "AOA: could not reregister data receive callback, error = "
                  <<  strerror(errno));
  }
}

AOAWrapper::AOAWrapper()
  : hdl_(NULL),
    timeout_(AOA_TIMEOUT_INFINITY) {
}

AOAWrapper::AOAWrapper(uint32_t timeout)
  : hdl_(NULL),
    timeout_(timeout) {
}

bool AOAWrapper::HandleDevice(const AOAWrapper::AOAUsbInfo& aoa_usb_info) {
  LOG4CXX_AUTO_TRACE(logger_);
  usb_info_t usb_info;
  PrepareUsbInfo(aoa_usb_info, &usb_info);
  int ret;
  LOG4CXX_DEBUG(logger_, "Connecting to AOA path:" << aoa_usb_info.path);
  hdl_ = aoa_connect(&usb_info, AOA_FLAG_UNIQUE_DEVICE, &ret);
  RETURN_ON_ERROR(ret)
  return true;
}

bool AOAWrapper::SetCallback(AOAEndpoint endpoint) const {
  return SetCallback(timeout_, endpoint);
}

bool AOAWrapper::SetCallback(uint32_t timeout, AOAEndpoint endpoint) const {
  LOG4CXX_TRACE(logger_,
                "AOA: set callback " << hdl_ << ", endpoint " << endpoint);
  int ret;
  switch (endpoint) {
    case AOA_Ept_Accessory_BulkIn:
      ret = aoa_bulk_arx(hdl_,
                         &OnReceivedData,
                         this,
                         BitEndpoint(AOA_Ept_Accessory_BulkIn),
                         timeout,
                         0,
                         kBufferSize,
                         0);
      break;
    default:
      LOG4CXX_ERROR(
        logger_, "AOA: " << endpoint << " endpoint doesn't support callbacks")
      ;
      return false;
  }

  RETURN_ON_ERROR(ret);
  return true;
}

void AOAWrapper::OnMessageReceived(bool success,
                                   protocol_handler::RawMessagePtr message) const {
  LOG4CXX_AUTO_TRACE(logger_);

  if (connection_observer_) {
    connection_observer_->OnMessageReceived(success, message);
  }
}

AOAWrapper::AOAHandle AOAWrapper::GetHandle() const {
  return hdl_;
}

bool AOAWrapper::Subscribe(AOAConnectionObserver* observer) {
  LOG4CXX_TRACE(logger_, "AOA: subscribe on receive data " << hdl_);
  connection_observer_ = observer;
  return SetCallback(AOA_Ept_Accessory_BulkIn);
}

bool AOAWrapper::Unsubscribe() {
  LOG4CXX_AUTO_TRACE(logger_);
  connection_observer_ = 0;
  const int status = aoa_disconnect(hdl_);
  RETURN_ON_ERROR(status);
  return true;
}

bool AOAWrapper::IsHandleValid() const {
  return IsHandleValid(hdl_);
}

AOAVersion AOAWrapper::Version(uint16_t version) const {
  switch (version) {
    case 1:
      return AOA_Ver_1_0;
    case 2:
      return AOA_Ver_2_0;
    default:
      return AOA_Ver_Unknown;
  }
}

AOAVersion AOAWrapper::GetProtocolVesrion() const {
  LOG4CXX_TRACE(logger_, "AOA: get protocol version for handle " << hdl_);
  uint16_t version;
  int ret = aoa_get_protocol(hdl_, &version);
  if (IsError(ret)) {
    PrintError(ret);
  }
  return Version(version);
}

uint32_t AOAWrapper::BitEndpoint(AOAEndpoint endpoint) const {
  const uint32_t kUndefined = 0;
  switch (endpoint) {
    case AOA_Ept_Accessory_BulkIn:
      return AOA_EPT_ACCESSORY_BULKIN;
    case AOA_Ept_Accessory_BulkOut:
      return AOA_EPT_ACCESSORY_BULKOUT;
    case AOA_Ept_Accessory_Control:
      return AOA_EPT_ACCESSORY_CONTROL;
    default:
      return kUndefined;
  }
}

uint32_t AOAWrapper::GetBufferMaximumSize(AOAEndpoint endpoint) const {
  uint32_t size;
  int ret = aoa_get_bufsz(hdl_, BitEndpoint(endpoint), &size);
  if (IsError(ret)) {
    PrintError(ret);
  }
  return size;
}

bool AOAWrapper::IsValueInMask(uint32_t bitmask, uint32_t value) const {
  return (bitmask & value) == value;
}

std::vector<AOAMode> AOAWrapper::CreateModesList(uint32_t modes_mask) const {
  std::vector<AOAMode> list;
  if (IsValueInMask(modes_mask, AOA_MODE_ACCESSORY)) {
    list.push_back(AOA_Mode_Accessory);
  }
  if (IsValueInMask(modes_mask, AOA_MODE_AUDIO)) {
    list.push_back(AOA_Mode_Audio);
  }
  if (IsValueInMask(modes_mask, AOA_MODE_DEBUG)) {
    list.push_back(AOA_Mode_Debug);
  }
  return list;
}

std::vector<AOAMode> AOAWrapper::GetModes() const {
  uint32_t modes;
  int ret = aoa_get_mode_mask(hdl_, &modes);
  if (IsError(ret)) {
    PrintError(ret);
  }
  return CreateModesList(modes);
}

std::vector<AOAEndpoint> AOAWrapper::CreateEndpointsList(
  uint32_t endpoints_mask) const {
  std::vector<AOAEndpoint> list;
  if (IsValueInMask(endpoints_mask, AOA_EPT_ACCESSORY_BULKIN)) {
    list.push_back(AOA_Ept_Accessory_BulkIn);
  }
  if (IsValueInMask(endpoints_mask, AOA_EPT_ACCESSORY_BULKOUT)) {
    list.push_back(AOA_Ept_Accessory_BulkOut);
  }
  if (IsValueInMask(endpoints_mask, AOA_EPT_ACCESSORY_CONTROL)) {
    list.push_back(AOA_Ept_Accessory_Control);
  }
  return list;
}

std::vector<AOAEndpoint> AOAWrapper::GetEndpoints() const {
  uint32_t endpoints;
  int ret = aoa_get_endpoint_mask(hdl_, &endpoints);
  if (IsError(ret)) {
    PrintError(ret);
  }
  return CreateEndpointsList(endpoints);
}

bool AOAWrapper::SendMessage(::protocol_handler::RawMessagePtr message) const {
  LOG4CXX_TRACE(logger_, "AOA: send to bulk endpoint");
  DCHECK(message);

  if (!IsHandleValid()) {
    OnDied();
    return false;
  }

  uint8_t* data = NULL;
  if (AOA_EOK != aoa_buffer_alloc(hdl_, &data, message->data_size())) {
    LOG4CXX_ERROR(logger_, "AOA: unable to allocate buffer.");
    return false;
  }

  size_t length = message->data_size();
  ::memcpy(data, message->data(), length);
  const int ret = aoa_bulk_tx(hdl_, AOA_EPT_ACCESSORY_BULKOUT, timeout_, data,
                              &length, AOA_FLAG_MANAGED_BUF);

  if (AOA_EOK != aoa_buffer_free(hdl_, data)) {
    LOG4CXX_ERROR(logger_, "AOA: unable to deallocate buffer.");
  }

  RETURN_ON_ERROR(ret);

  return true;
}

bool AOAWrapper::SendControlMessage(uint16_t request, uint16_t value,
                                    uint16_t index,
                                    ::protocol_handler::RawMessagePtr message) const {
  LOG4CXX_TRACE(logger_, "AOA: send to control endpoint");
  DCHECK(message);

  if (!IsHandleValid()) {
    OnDied();
    return false;
  }

  uint8_t* data = message->data();
  size_t length = message->data_size();
  const int ret = aoa_control_tx(hdl_, AOA_EPT_ACCESSORY_CONTROL, timeout_,
                                 request, value, index, data, &length,
                                 AOA_FLAG_MANAGED_BUF);
  RETURN_ON_ERROR(ret);
  return true;
}

bool AOAWrapper::IsError(int ret) const {
  return ret != AOA_EOK;
}

void AOAWrapper::PrintError(int ret) const {
  LOG4CXX_ERROR(logger_, "AOA: error " << ret << " - " << aoa_err2str(ret));
}

bool AOAWrapper::IsHandleValid(AOAWrapper::AOAHandle hdl) const {
  LOG4CXX_TRACE(logger_, "AOA: check handle " << hdl);
  if (hdl != hdl_) {
    LOG4CXX_TRACE(logger_, "Inappropriate handle has been received. Expected: "
                  << hdl_ << " Received " << hdl);
  }
  bool valid;
  const int ret = aoa_get_valid(hdl, &valid);
  RETURN_ON_ERROR(ret);
  return valid;
}

void AOAWrapper::OnDied() const {
  LOG4CXX_AUTO_TRACE(logger_);
  if (connection_observer_) {
    connection_observer_->OnDisconnected();
  }
}

void AOAWrapper::PrepareUsbInfo(const AOAUsbInfo& aoa_usb_info,
                                usb_info_s* usb_info) {
  usb_info->path = aoa_usb_info.path.c_str();
  usb_info->busno = aoa_usb_info.busno;
  usb_info->devno = aoa_usb_info.devno;
  usb_info->iface = aoa_usb_info.iface;
}

}  // namespace transport_adapter
}  // namespace transport_manager
