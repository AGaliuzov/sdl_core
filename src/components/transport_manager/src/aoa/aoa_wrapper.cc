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

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

namespace {
  aoa_hdl_t* active_handle = NULL;
}

static void OnConnectedDevice(aoa_hdl_t *hdl, const void *udata) {
  LOG4CXX_AUTO_TRACE(logger_);
  LOG4CXX_DEBUG(logger_, "AOA: connected device " << hdl);

  if (NULL == active_handle) {
    LOG4CXX_DEBUG(logger_, "AOA: assign active handle " << hdl);
    active_handle = hdl;
  }

  AOADeviceLife* life = AOAWrapper::life_keeper_.BindHandle2Life(hdl);
  if (life) {
    life->Loop(hdl);
  }
  AOAWrapper::life_keeper_.ReleaseLife(hdl);
}

static void OnReceivedData(aoa_hdl_t *hdl, uint8_t *data, uint32_t sz,
                           uint32_t status, const void *udata) {
  LOG4CXX_AUTO_TRACE(logger_);

  LOG4CXX_DEBUG(logger_, "AOA: received data from device " << hdl);
  bool error = AOAWrapper::IsError(status);
  if (error) {
    AOAWrapper::PrintError(status);
  }

  if (NULL == udata) { return; }

  AOAConnectionObserver* const * p =
      static_cast<AOAConnectionObserver* const *>(udata);
  AOAConnectionObserver* observer = *p;

  if (!AOAWrapper::IsHandleValid(hdl) || (status == AOA_EINVALIDHDL)) {
    if (active_handle == hdl) {
      LOG4CXX_DEBUG(logger_, "AOA: reset active_handle to NULL");
      active_handle = NULL;
    }
    if (AOAWrapper::life_keeper_.LifeExists(hdl)) {
      AOAWrapper::OnDied(hdl);
      observer->OnDisconnected();
    }
    return;
  }

  if (NULL == active_handle) {
    LOG4CXX_DEBUG(logger_, "AOA: re-assign active handle " << hdl);
    active_handle = hdl;
  }

  if (hdl == active_handle) {
    bool success = !error;
    ::protocol_handler::RawMessagePtr message;
    if (data) {
      message = new ::protocol_handler::RawMessage(0, 0, data, sz);
    } else {
      LOG4CXX_ERROR(logger_, "AOA: data is null");
      success = false;
    }
    observer->OnMessageReceived(success, message);
  }

  aoa_buffer_free(hdl, data);
  if (AOAWrapper::SetCallback(hdl, udata, AOA_TIMEOUT_INFINITY, AOA_Ept_Accessory_BulkIn)) {
    LOG4CXX_DEBUG(logger_, "AOA: data receive callback reregistered");
  }
  else {
    LOG4CXX_ERROR(logger_, "AOA: could not reregister data receive callback, error = ");
  }
}

LifeKeeper AOAWrapper::life_keeper_;
AOAConnectionObserver* AOAWrapper::connection_observer_ = NULL;

AOAWrapper::AOAWrapper(AOAHandle hdl)
    : hdl_(hdl),
      timeout_(AOA_TIMEOUT_INFINITY) {
}

AOAWrapper::AOAWrapper(AOAHandle hdl, uint32_t timeout)
    : hdl_(hdl),
      timeout_(timeout) {
}

bool AOAWrapper::Init(AOADeviceLife *life) {
  LOG4CXX_TRACE(logger_, "AOA: init default");
  return Init(life, NULL, NULL);
}

bool AOAWrapper::Init(AOADeviceLife* life, const std::string& config_path) {
  LOG4CXX_TRACE(logger_, "AOA: init with path to config");
  return Init(life, config_path.c_str(), NULL);
}

bool AOAWrapper::Init(AOADeviceLife* life,
                      const AOAWrapper::AOAUsbInfo& aoa_usb_info) {
  LOG4CXX_TRACE(logger_, "AOA: init with usb info");
  usb_info_t usb_info;
  PrepareUsbInfo(aoa_usb_info, &usb_info);
  return Init(life, NULL, &usb_info);
}

bool AOAWrapper::HandleDevice(AOADeviceLife* life,
                              const AOAWrapper::AOAUsbInfo& aoa_usb_info) {
  LOG4CXX_AUTO_TRACE(logger_);
  usb_info_t usb_info;
  life_keeper_.AddLife(life);
  PrepareUsbInfo(aoa_usb_info, &usb_info);
  const int ret = aoa_usbinfo_add(&usb_info, AOA_FLAG_UNIQUE_DEVICE);
  if (IsError(ret)) {
    PrintError(ret);
    return false;
  }
  return true;
}

void AOAWrapper::Disconnect(bool forced) {
  Shutdown();
  if (connection_observer_) {
    connection_observer_->OnDisconnected(forced);
  }
}

bool AOAWrapper::Init(AOADeviceLife* life, const char* config_path,
                      usb_info_s* usb_info) {

  life_keeper_.AddLife(life);

  uint32_t flags = 0;
  if (usb_info) {
    flags = AOA_FLAG_UNIQUE_DEVICE;
  } else {
    flags = AOA_FLAG_NO_USB_STACK;
  }

  int ret = aoa_init(config_path, usb_info, &OnConnectedDevice, &life_keeper_, flags);
  if (IsError(ret)) {
    PrintError(ret);
    return false;
  }
  return true;
}

bool AOAWrapper::SetCallback(AOAEndpoint endpoint) const {
  return SetCallback(hdl_, &connection_observer_, timeout_, endpoint);
}

bool AOAWrapper::SetCallback(aoa_hdl_t* hdl, const void* udata, uint32_t timeout, AOAEndpoint endpoint) {
  LOG4CXX_TRACE(logger_,
                "AOA: set callback " << hdl << ", endpoint " << endpoint);
  int ret;
  switch (endpoint) {
    case AOA_Ept_Accessory_BulkIn:
      ret = aoa_bulk_arx(hdl,
                         &OnReceivedData,
                         udata,
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
  if (IsError(ret)) {
    PrintError(ret);
    return false;
  }
  return true;
}

bool AOAWrapper::Subscribe(AOAConnectionObserver *observer) {
  LOG4CXX_TRACE(logger_, "AOA: subscribe on receive data " << hdl_);
  connection_observer_ = observer;
  return SetCallback(AOA_Ept_Accessory_BulkIn);
}

bool AOAWrapper::UnsetCallback(AOAEndpoint endpoint) const {
  return SetCallback(hdl_, NULL, timeout_, endpoint);
}

bool AOAWrapper::Unsubscribe() {
  LOG4CXX_TRACE(logger_, "AOA: unsubscribe on receive data" << hdl_);
  if (!UnsetCallback(AOA_Ept_Accessory_BulkIn)) {
    return false;
  }
  LOG4CXX_TRACE(logger_, "AOA: unsubscribe on transmit data" << hdl_);
  if (!UnsetCallback(AOA_Ept_Accessory_BulkOut)) {
    return false;
  }
  connection_observer_ = 0;
  return true;
}

bool AOAWrapper::Shutdown() {
  LOG4CXX_TRACE(logger_, "AOA: shutdown");
  int ret = aoa_shutdown();
  if (IsError(ret)) {
    PrintError(ret);
    return false;
  }
  active_handle = NULL;
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

uint32_t AOAWrapper::BitEndpoint(AOAEndpoint endpoint) {
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
    OnDied(hdl_);
    connection_observer_->OnDisconnected();
    return false;
  }

  uint8_t *data = NULL;
  if (AOA_EOK != aoa_buffer_alloc(hdl_, &data, message->data_size())) {
    LOG4CXX_ERROR(logger_, "AOA: unable to allocate buffer.");
    return false;
  }

  size_t length = message->data_size();
  ::memcpy(data, message->data(), length);
  int ret = aoa_bulk_tx(hdl_, AOA_EPT_ACCESSORY_BULKOUT, timeout_, data,
                        &length, AOA_FLAG_MANAGED_BUF);

  if (AOA_EOK != aoa_buffer_free(hdl_, data)) {
    LOG4CXX_ERROR(logger_, "AOA: unable to deallocate buffer.");
  }

  if (IsError(ret)) {
    PrintError(ret);
    return false;
  }

  return true;
}

bool AOAWrapper::SendControlMessage(uint16_t request, uint16_t value,
                                    uint16_t index,
                                    ::protocol_handler::RawMessagePtr message) const {
  LOG4CXX_TRACE(logger_, "AOA: send to control endpoint");
  DCHECK(message);

  if (!IsHandleValid()) {
    OnDied(hdl_);
    connection_observer_->OnDisconnected();
    return false;
  }

  uint8_t *data = message->data();
  size_t length = message->data_size();
  int ret = aoa_control_tx(hdl_, AOA_EPT_ACCESSORY_CONTROL, timeout_, request,
                           value, index, data, &length, AOA_FLAG_MANAGED_BUF);
  if (IsError(ret)) {
    PrintError(ret);
    return false;
  }
  return true;
}

::protocol_handler::RawMessagePtr AOAWrapper::ReceiveMessage() const {
  LOG4CXX_AUTO_TRACE(logger_);

  if (!IsHandleValid()) {
    OnDied(hdl_);
    connection_observer_->OnDisconnected();
    return ::protocol_handler::RawMessagePtr();
  }

  uint8_t* data;
  uint32_t size;
  LOG4CXX_TRACE(logger_, "AOA: receiving bulk data");
  int ret = aoa_bulk_rx(hdl_, AOA_EPT_ACCESSORY_BULKIN, timeout_, &data, &size, 0);
  if (IsError(ret)) {
    PrintError(ret);
  } else {
      LOG4CXX_DEBUG(logger_, "AOA: bulk data received");
      if (data) {
        return ::protocol_handler::RawMessagePtr(new ::protocol_handler::RawMessage(0, 0, data, size));
      }
  }
  return ::protocol_handler::RawMessagePtr();
}

::protocol_handler::RawMessagePtr AOAWrapper::ReceiveControlMessage(uint16_t request,
                                                uint16_t value,
                                                uint16_t index) const {
  LOG4CXX_TRACE(logger_, "AOA: receive from control endpoint");

  if (!IsHandleValid()) {
    OnDied(hdl_);
    connection_observer_->OnDisconnected();
    return ::protocol_handler::RawMessagePtr();
  }

  uint8_t *data;
  uint32_t size;
  int ret = aoa_control_rx(hdl_, AOA_EPT_ACCESSORY_CONTROL, timeout_, request,
                           value, index, &data, &size, AOA_FLAG_MANAGED_BUF);
  if (IsError(ret)) {
    PrintError(ret);
  } else if (data) {
    return ::protocol_handler::RawMessagePtr(new ::protocol_handler::RawMessage(0, 0, data, size));
  }
  return ::protocol_handler::RawMessagePtr();
}

bool AOAWrapper::IsError(int ret) {
  return ret != AOA_EOK;
}

void AOAWrapper::PrintError(int ret) {
  LOG4CXX_ERROR(logger_, "AOA: error " << ret << " - " << aoa_err2str(ret));
}

bool AOAWrapper::IsHandleValid(AOAWrapper::AOAHandle hdl) {
  LOG4CXX_TRACE(logger_, "AOA: check handle " << hdl);
  bool valid;
  int ret = aoa_get_valid(hdl, &valid);
  if (IsError(ret)) {
    PrintError(ret);
    return false;
  }
  return valid;
}

void AOAWrapper::OnDied(AOAWrapper::AOAHandle hdl) {
  AOADeviceLife* life = life_keeper_.ReleaseLife(hdl);

  if (NULL != life) {
    life->OnDied(hdl);
  }
}

void AOAWrapper::PrepareUsbInfo(const AOAUsbInfo& aoa_usb_info,
                                usb_info_s* usb_info) {
  usb_info->path = aoa_usb_info.path.c_str();
  usb_info->busno = aoa_usb_info.busno;
  usb_info->devno = aoa_usb_info.devno;
  usb_info->iface = aoa_usb_info.iface;
}

void LifeKeeper::AddLife(AOADeviceLife *life) {
  if (NULL == life) {
    LOG4CXX_DEBUG(logger_,"Life object is NULL shouldn't be added to the pool");
    return;
  }

  free_life_pool.push(life);

}

AOADeviceLife* LifeKeeper::BindHandle2Life(aoa_hdl_t *hdl) {
  if (free_life_pool.empty()) { return NULL; }

  AOADeviceLife* life = free_life_pool.front();
  live_devices.insert(std::make_pair(hdl, life));
  free_life_pool.pop();
  return life;
}

AOADeviceLife* LifeKeeper::GetLife(aoa_hdl_t *hdl){
  if (!LifeExists(hdl)) { return NULL; }

  return live_devices[hdl];
}

AOADeviceLife* LifeKeeper::ReleaseLife(aoa_hdl_t *hdl) {
  AOADeviceLife* life = GetLife(hdl);
  if (NULL != life) {
    live_devices.erase(hdl);
  }

  return life;
}

bool LifeKeeper::LifeExists(aoa_hdl_t* hdl) {
  return live_devices.find(hdl) != live_devices.end();
}

}  // namespace transport_adapter
}  // namespace transport_manager

