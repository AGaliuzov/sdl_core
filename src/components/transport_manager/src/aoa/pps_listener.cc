/*
 * Copyright (c) 2015, Ford Motor Company
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
#include "transport_manager/aoa/pps_listener.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <utility>
#include <algorithm>

#include <aoa/aoa.h>
#include <sys/pps.h>
#include <sys/iomsg.h>
#include <sys/neutrino.h>
#include <applink_types.h>

#include "utils/logger.h"
#include "utils/file_system.h"
#include "utils/make_shared.h"

#include "transport_manager/aoa/aoa_transport_adapter.h"
#include "transport_manager/aoa/aoa_dynamic_device.h"
#include "transport_manager/error.h"

#define REQUEST_MASK    (AOA_MODE_ACCESSORY | AOA_MODE_AUDIO)

#define MAX_MESSAGES 128

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

namespace {

enum {
  ATTR_AOA = 0,
  ATTR_BUSNO,
  ATTR_DEVNO,
  ATTR_MANUFACTURER,
  ATTR_VENDOR_ID,
  ATTR_PRODUCT,
  ATTR_PRODUCT_ID,
  ATTR_SERIAL_NUMBER,
  ATTR_STACKNO,
  ATTR_UPSTREAM_PORT,
  ATTR_UPSTREAM_DEVICE_ADDRESS,
  ATTR_UPSTREAM_HOST_CONTROLLER,

  ATTR_COUNT
};

// TODO(EZamakhov): prepare Key/Value string one time

/* The keys required by usblauncher to send to the device */
static const char* kKeys[] = {
  "manufacturer", "model", "version",
  NULL
};

/* The values used to switch the phone into AOA mode */
static const char* kValues[] = {
  "SDL", "Core", "1.0",
  NULL
};

/* Name of AOA atributes for reading from PPS */
static const char* pps_obejct_attrs_name[] = {
  "aoa", "busno", "devno", "manufacturer",
  "vendor_id", "product", "product_id", "serial_number", "stackno",
  "upstream_port", "upstream_device_address", "upstream_host_controller",
  NULL
};

/*
 * Product list of known devices (currently sequential so we
 * could get away with an integer range instead).
 */
static const uint32_t kProductList[] = { 0x2d00, 0x2d01, 0x2d02, 0x2d03, 0x2d04,
                                         0x2d05
                                       };

static const uint32_t kVendorIdGoogle = 0x18d1;

static const char kPpsPrefixRemovedDevice = '-';

const std::string kUSBStackPath = "/dev/otg/io-usb-otg";
const std::string kPpsPathRoot = "/pps/qnx/device/";
const std::string kPpsPathAll = ".all";
const std::string kPpsPathCtrl = "usb_ctrl";

void release_thread(threads::Thread** thread) {
  if (thread && *thread) {
    (*thread)->join();
    delete (*thread)->delegate();
    threads::DeleteThread(*thread);
    *thread = NULL;
  }
}

inline std::string CharPtrToString(const char* const value) {
  return value ? std::string(value) : std::string();
}
inline unsigned long int CharPtrToLUInt(const char* const value) {
  return value ? strtoul(value, NULL, 0) : 0u;
}

// trim string from end
static inline std::string rtrim(const std::string &inpute_string) {
    std::string s = inpute_string;
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// Functor for searching in DeviceCollection by object name
struct ObjectNameComparator {
    explicit ObjectNameComparator(const DeviceUID& device_uid) :
        object_name_(device_uid) {}
    const DeviceUID& object_name_;

    bool operator()(const DeviceCollectionPair& device_pair) const {
        const AOAWrapper::AOAUsbInfo& aoa_usb_info = device_pair.first;
        return object_name_ == aoa_usb_info.object_name;
    }
};

}  // namespace


PPSListener::MQHandler::MQHandler(const std::string& name, int flags)
  : handle_(-1) {
  init_mq(name, flags);
}

void PPSListener::MQHandler::Write(const std::vector<char>& message) const {
  LOG4CXX_AUTO_TRACE(logger_);
  if (-1 == handle_) {
    LOG4CXX_ERROR(logger_, "Mq is not opened");
    return;
  }

  if (- 1 == mq_send(handle_, &message[0], MAX_QUEUE_MSG_SIZE, NULL)) {
    LOG4CXX_ERROR(logger_, "Unable to send over mq " <<
                            " : " << strerror(errno));
  }
}

void
PPSListener::MQHandler::Read(std::vector<char>& data) const {
  LOG4CXX_AUTO_TRACE(logger_);
  data.resize(MAX_QUEUE_MSG_SIZE);
  if (-1 ==  handle_) {
    LOG4CXX_ERROR(logger_, "The queue with handle " << handle_ << " not opened");
    data.resize(0);
    return;
  }
  ssize_t data_size = mq_receive(handle_, &data[0], MAX_QUEUE_MSG_SIZE, NULL);
  if (-1 == data_size) {
    LOG4CXX_ERROR(logger_, "Unable to receive data from mq with handle: " << handle_
                           << " error is " << strerror(errno));
    data_size = 0;
  }
  data.resize(data_size);
}

void PPSListener::MQHandler::deinit_mq() {
  LOG4CXX_AUTO_TRACE(logger_);
  if (-1 == mq_close(handle_)) {
    LOG4CXX_ERROR(logger_, "Unable to close mq: " << strerror(errno));
  }
}

void PPSListener::MQHandler::init_mq(const std::string& name, int flags) {
  LOG4CXX_AUTO_TRACE(logger_);
  struct mq_attr attributes;
  attributes.mq_maxmsg = MAX_MESSAGES;
  attributes.mq_msgsize = MAX_QUEUE_MSG_SIZE;
  attributes.mq_flags = 0;

  handle_ = mq_open(name.c_str(), flags, 0666, &attributes);

  if (-1 == handle_) {
    LOG4CXX_ERROR(logger_, "Unable to open mq " << name.c_str() <<
                           " : " << strerror(errno));
  }
}

PPSListener::MQHandler::~MQHandler() {
  LOG4CXX_AUTO_TRACE(logger_);
  deinit_mq();
}

PPSListener::PPSListener(AOATransportAdapter* controller)
  : initialised_(false),
    controller_(controller),
    fd_(-1),
    pps_thread_(NULL),
    mq_thread_(NULL),
    is_aoa_available_(false),
    mq_from_applink_handler_(std::string("/dev/mqueue/aoa"), O_CREAT | O_RDONLY),
    mq_to_applink_handler_(PREFIX_STR_FROMSDL_QUEUE, O_CREAT | O_WRONLY) {
  LOG4CXX_AUTO_TRACE(logger_);
}

PPSListener::~PPSListener() {
  LOG4CXX_AUTO_TRACE(logger_);
  if (initialised_) {
    Terminate();
  }
  release_thread(&pps_thread_);
  release_thread(&mq_thread_);
}

TransportAdapter::Error PPSListener::Init() {
  LOG4CXX_AUTO_TRACE(logger_);
  initialised_ = OpenPps();
  return (initialised_) ? TransportAdapter::OK : TransportAdapter::FAIL;
}

void PPSListener::Terminate() {
  LOG4CXX_AUTO_TRACE(logger_);
  initialised_ = false;
  ClosePps();
}

bool PPSListener::IsInitialised() const {
  return initialised_;
}

TransportAdapter::Error PPSListener::StartListening() {
  LOG4CXX_AUTO_TRACE(logger_);
  pps_thread_ = threads::CreateThread("PPS listener",    new PpsThreadDelegate(this));
  mq_thread_  = threads::CreateThread("PPS MQ Listener", new PpsMQListener(this));
  pps_thread_->start();
  mq_thread_->start();
  return TransportAdapter::OK;
}

TransportAdapter::Error PPSListener::StopListening() {
  LOG4CXX_AUTO_TRACE(logger_);
  release_thread(&mq_thread_);
  release_thread(&pps_thread_);
  return TransportAdapter::OK;
}

void PPSListener::ReceiveMQMessage(std::vector<char>& message) {
  LOG4CXX_AUTO_TRACE(logger_);
  mq_from_applink_handler_.Read(message);
}

void PPSListener::SendMQMessage(const std::vector<char>& message) {
  LOG4CXX_AUTO_TRACE(logger_);
  mq_to_applink_handler_.Write(message);
}


bool PPSListener::OpenPps() {
  LOG4CXX_AUTO_TRACE(logger_);
  const std::string kPath = kPpsPathRoot + kPpsPathAll;
  if ((fd_ = open(kPath.c_str(), O_RDONLY)) == -1) {
    LOG4CXX_ERROR(
      logger_,
      "AOA: error opening file '" << kPath << "': (" << strerror(errno) << ")");
    return false;
  }
  return true;
}

bool PPSListener::ArmEvent(const struct sigevent* event) {
  LOG4CXX_AUTO_TRACE(logger_);
  uint8_t buf[2048];
  while (ionotify(fd_, _NOTIFY_ACTION_POLLARM, _NOTIFY_COND_INPUT, event)
         & _NOTIFY_COND_INPUT) {
    const int size = read(fd_, buf, sizeof(buf));
    if (size > 0) {
      buf[size] = '\0';
      // In case AOA is not available don't need to process any messages from PPS.
      ProcessPpsMessage(buf, size);
    }
  }
  return true;
}

void PPSListener::ProcessPpsMessage(uint8_t* message, const size_t size) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (!message) {
    LOG4CXX_ERROR(logger_, "Empty PPS message");
    return;
  }
  if (size < 2) {
    LOG4CXX_ERROR(logger_, "Size of PPS message: " <<
                  size << ", required bigger than 1");
    return;
  }
  char* ppsdata = reinterpret_cast<char*>(message);
  LOG4CXX_DEBUG(logger_, "pps data: " << ppsdata);

  const bool device_was_removed = (ppsdata[0] == kPpsPrefixRemovedDevice);
  if (device_was_removed) {
    LOG4CXX_TRACE(logger_, "PPS remove notification found");
    // TODO(EZamakhov): APPLINK-19940 replace trim with a getting info from ppsparse function
    const std::string& object_name = rtrim(std::string(ppsdata + 1, ppsdata + size));
    DeviceCollection::iterator it = std::find_if(
                devices_.begin(), devices_.end(),
                ObjectNameComparator(object_name));
    if (it == devices_.end()) {
      LOG4CXX_DEBUG(logger_, "Removed unknown for SDL SSP device with object_name: \""
                    << object_name << '"');
      return;
    }
    const DeviceUID device_name_copy = it->second;
    LOG4CXX_INFO(logger_, "Removing AOA device with object_name: \""
                 << object_name << "\", device_name: \""
                 << device_name_copy << '"');
    devices_lock_.Acquire();
    devices_.erase(it);
    devices_lock_.Release();
    controller_->RemoveDevice(device_name_copy);
    if (is_aoa_available_) {
      std::vector<char> buf(1);
      buf[0] = AOA_RELEASED;
      LOG4CXX_DEBUG(logger_, "Send " << static_cast<int32_t>(buf[0]) << " signal to AppLink");
      mq_to_applink_handler_.Write(buf);
    }
    return;
  }

  AOAWrapper::AOAUsbInfo aoa_usb_info;
  const char* vals[ATTR_COUNT] = { 0 };
  aoa_usb_info.object_name = ParsePpsData(ppsdata, vals);
  if (!IsAOADevice(vals)) {
    LOG4CXX_DEBUG(logger_, "Handled pps data not for a AOA device");
    return;
  }
  LOG4CXX_INFO(logger_, "AOA device found");

  FillUsbInfo(vals, &aoa_usb_info);
  const DeviceCollectionPair device_pair =
    std::make_pair(aoa_usb_info, std::string());
  devices_lock_.Acquire();
  devices_.insert(device_pair);
  const size_t devices_size = devices_.size();
  devices_lock_.Release();
  LOG4CXX_DEBUG(logger_, "Devices queue size: " << devices_size);

  if (!is_aoa_available_) {
    LOG4CXX_WARN(logger_, "AOA is not available yet or taken by system");
    return;
  }

  ProcessAOADevice(device_pair);
}

std::string PPSListener::ParsePpsData(char* ppsdata, const char** vals) {
  LOG4CXX_AUTO_TRACE(logger_);

  std::string object_name;
  pps_attrib_t info = { 0 };
  /* Loop through the PPS objects looking for aoa, busno, devno and etc. */
  do {
    const int rc = ppsparse(&ppsdata, NULL, pps_obejct_attrs_name, &info, 0);
    if (rc == PPS_END) {
      LOG4CXX_TRACE(logger_, "Nothing parsed");
      break;
    }
    if (rc == PPS_OBJECT) {
      if (info.obj_name) {
        object_name = info.obj_name;
      }
      LOG4CXX_DEBUG(logger_, "AOA: object name " << object_name);
    } else if (rc == PPS_ATTRIBUTE && info.attr_index != -1) {
      LOG4CXX_DEBUG(
        logger_,
        "AOA: index " << info.attr_index << ", value " << info.value);
      /* If our value matches our attribute index, keep a pointer to it */
      vals[info.attr_index] = info.value;
    }
  } while (true);

  return object_name;
}

void PPSListener::ProcessAOADevice(const DeviceCollectionPair device) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (IsAOAMode(device.first)) {
    AddDevice(device.first);
  } else {
    SwitchMode(device.first);
  }
}

bool PPSListener::IsAOAMode(const AOAWrapper::AOAUsbInfo& aoa_usb_info) {
  LOG4CXX_AUTO_TRACE(logger_);
  /*
   * Check a device if it's already in AOA mode (product and vendor ID's
   * will match a known list).
   */
  if (aoa_usb_info.vendor_id == kVendorIdGoogle) {
    const uint32_t* begin = kProductList;
    const uint32_t* end   = kProductList
                            + sizeof(kProductList) / sizeof(uint32_t);
    const uint32_t* p = std::find(begin, end, aoa_usb_info.product_id);
    if (p != end) {
      LOG4CXX_DEBUG(
        logger_,
        "AOA: mode of device " << aoa_usb_info.serial_number << " is AOA");
      return true;
    }
  }
  LOG4CXX_DEBUG(logger_, "AOA: mode of device "
                << aoa_usb_info.serial_number << " is not AOA");
  return false;
}

bool PPSListener::IsAOADevice(const char** attrs) {
  LOG4CXX_AUTO_TRACE(logger_);
  /* Make sure we support AOA and have all fields */
  for (int i = 0; i < ATTR_COUNT; i++) {
    if (!attrs[i]) {
      LOG4CXX_DEBUG(logger_, "AOA: device doesn't support AOA mode");
      return false;
    }
  }
  return true;
}

void PPSListener::FillUsbInfo(const char** attrs,
                              AOAWrapper::AOAUsbInfo* info) {
  LOG4CXX_AUTO_TRACE(logger_);
  info->iface = 0;
  info->path = kUSBStackPath;
  info->aoa_version   = CharPtrToLUInt(attrs[ATTR_AOA]);
  info->busno         = CharPtrToLUInt(attrs[ATTR_BUSNO]);
  info->devno         = CharPtrToLUInt(attrs[ATTR_DEVNO]);
  info->product_id    = CharPtrToLUInt(attrs[ATTR_PRODUCT_ID]);
  info->vendor_id     = CharPtrToLUInt(attrs[ATTR_VENDOR_ID]);
  info->manufacturer  = CharPtrToString(attrs[ATTR_MANUFACTURER]);
  info->product       = CharPtrToString(attrs[ATTR_PRODUCT]);
  info->serial_number = CharPtrToString(attrs[ATTR_SERIAL_NUMBER]);
  info->stackno       = CharPtrToString(attrs[ATTR_STACKNO]);
  info->upstream_port            = CharPtrToString(attrs[ATTR_UPSTREAM_PORT]);
  info->upstream_device_address  = CharPtrToString(attrs[ATTR_UPSTREAM_DEVICE_ADDRESS]);
  info->upstream_host_controller = CharPtrToString(attrs[ATTR_UPSTREAM_HOST_CONTROLLER]);
}

void PPSListener::SwitchMode(const AOAWrapper::AOAUsbInfo& usb_info) {
  LOG4CXX_AUTO_TRACE(logger_);

  char cmd[PATH_MAX];

  /* Create the PPS request string
   * (start_aoa::busno,devno,request_mask,key=value,key=value,...) */
  size_t sz = snprintf(cmd, sizeof(cmd), "start_aoa::busno=%u,devno=%u,modes=%u",
                       usb_info.busno, usb_info.devno, REQUEST_MASK);

  int i = 0;
  while (kKeys[i]) {
    /* Add all the key/value pairs to the request */
    sz += snprintf(cmd + sz, sizeof(cmd) - sz, ",%s=%s", kKeys[i], kValues[i]);
    i++;
  }

  /* Open the object we want to write to */
  const std::string ctrl = kPpsPathRoot + kPpsPathCtrl + usb_info.stackno;
  std::vector<char> data(cmd, cmd + sz);
  LOG4CXX_DEBUG(logger_, "Message to write: " << &data[0]);
  file_system::Write(ctrl, data);
}

void PPSListener::ClosePps() {
  LOG4CXX_AUTO_TRACE(logger_);
  if (fd_ != -1) {
    close(fd_);
    fd_ = -1;
  }
}

void PPSListener::AddDevice(const AOAWrapper::AOAUsbInfo& aoa_usb_info) {
  LOG4CXX_AUTO_TRACE(logger_);
  const AOADynamicDeviceSPtr aoa_device =
    utils::MakeShared<AOADynamicDevice>(
      aoa_usb_info.product, aoa_usb_info.serial_number,
      aoa_usb_info, controller_);

  // Devices list is being locked in init_aoa-->ProcessAOADevice-->AddDevice
  devices_[aoa_usb_info] = aoa_device->unique_device_id();
  controller_->AddDevice(aoa_device);
}

void PPSListener::DisconnectDevice(const DeviceCollectionPair device) {
  ApplicationList app_list = controller_->GetApplicationList(device.second);
  ApplicationList::const_iterator it = app_list.begin();
  ApplicationList::const_iterator end = app_list.end();
  CommunicationError error;
  error.set_error(kResourceBusy);
  while (it != end) {
    controller_->ConnectionAborted(device.second, *it, error);
    ++it;
  }

  // TODO(EZamakhov): APPLINK-19534 - Fix sleep with hadling all session unregistered
  sleep(2);
  controller_->RemoveDevice(device.second);
}

bool PPSListener::init_aoa() {
  LOG4CXX_AUTO_TRACE(logger_);
  is_aoa_available_ = true;

  sync_primitives::AutoLock lock(devices_lock_);
  std::for_each(devices_.begin(), devices_.end(),
                std::bind1st(
                  std::mem_fun(&PPSListener::ProcessAOADevice), this));

  LOG4CXX_INFO(logger_, "AOA: Initialized (taken) by system");
  return is_aoa_available_;
}

void PPSListener::release_aoa() const {
  LOG4CXX_AUTO_TRACE(logger_);
  is_aoa_available_ = false;

  sync_primitives::AutoLock lock(devices_lock_);
  std::for_each(devices_.begin(), devices_.end(),
                std::bind1st(
                  std::mem_fun(&PPSListener::DisconnectDevice),
                  this));
  LOG4CXX_INFO(logger_, "AOA: Deinitialized (release) by system");
}

PPSListener::PpsMQListener::PpsMQListener(PPSListener* parent)
  : parent_(parent),
    run_(false) {
  run_ = true;
}

void PPSListener::PpsMQListener::threadMain() {
    std::vector<char> msg(MAX_QUEUE_MSG_SIZE);
    while (run_) {
      msg.clear();
      parent_->ReceiveMQMessage(msg);

      const size_t size = msg.size();
      LOG4CXX_DEBUG(logger_, "Receive message from Applink with size: " << size
                    << ". Message signal: " << static_cast<int32_t>(size ? msg[0] : -1)
                    << ". TAKE_AOA signal is: " << static_cast<int32_t>(TAKE_AOA)
                    << ". RELEASE_AOA signal is: " << static_cast<int32_t>(RELEASE_AOA)
                    << ". AOA_RELEASED signal is: " << static_cast<int32_t>(AOA_RELEASED)
                    << ". AOA_TAKEN signal is: " << static_cast<int32_t>(AOA_TAKEN)
                    << ". AOA_INACCESSIBLE signal is: " << static_cast<int32_t>(AOA_INACCESSIBLE));
      if (0 != size) {
        switch (msg[0]) {
          case TAKE_AOA:
            take_aoa();
            break;
          case RELEASE_AOA:
            release_aoa();
            break;
          default:
            break;
        }
      }
    }
}

void PPSListener::PpsMQListener::exitThreadMain() {
  run_ = false;
}

void PPSListener::PpsMQListener::take_aoa() {
  LOG4CXX_AUTO_TRACE(logger_);
  const bool is_inited = parent_->init_aoa();
  std::vector<char> buf(1);
  buf[0] = is_inited ? AOA_TAKEN : AOA_INACCESSIBLE;
  LOG4CXX_DEBUG(logger_, "Send " << static_cast<int32_t>(buf[0]) << " signal to AppLink");
  parent_->SendMQMessage(buf);
}

void PPSListener::PpsMQListener::release_aoa() {
  LOG4CXX_AUTO_TRACE(logger_);
  parent_->release_aoa();
  std::vector<char> buf(1);
  buf[0] = AOA_RELEASED;
  LOG4CXX_DEBUG(logger_, "Send " << static_cast<int32_t>(buf[0]) << " signal to AppLink");
  parent_->SendMQMessage(buf);
}


PPSListener::PpsThreadDelegate::PpsThreadDelegate(PPSListener* parent)
  : parent_(parent) {
}

bool PPSListener::PpsThreadDelegate::ArmEvent(struct sigevent* event) {
  return parent_->ArmEvent(event);
}

}  // namespace transport_adapter
}  // namespace transport_manager
