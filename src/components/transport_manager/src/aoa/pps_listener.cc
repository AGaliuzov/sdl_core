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
#include "transport_manager/aoa/pps_listener.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>

#include <aoa/aoa.h>
#include <sys/pps.h>
#include <sys/iomsg.h>
#include <sys/neutrino.h>
#include <applink_types.h>

#include <algorithm>

#include "utils/logger.h"

#include "transport_manager/transport_adapter/transport_adapter_controller.h"
#include "transport_manager/aoa/aoa_dynamic_device.h"

#define REQUEST_MASK    (AOA_MODE_ACCESSORY | AOA_MODE_AUDIO)

#define MAX_MESSAGES 128

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

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

  ATTR_COUNT
};

/* The keys required by usblauncher to send to the device */
static const char *kKeys[] = { "manufacturer", "model", "version",

NULL };

/* The values used to switch the phone into AOA mode */
static const char *kValues[] = { "Ford", "HMI", "1.0",

NULL };

/*
 * Product list of known devices (currently sequential so we
 * could get away with an integer range instead).
 */
static const uint32_t kProductList[] = { 0x2d00, 0x2d01, 0x2d02, 0x2d03, 0x2d04,
    0x2d05 };

const std::string PPSListener::kUSBStackPath = "/dev/otg/io-usb-otg";
const std::string PPSListener::kPpsPathRoot = "/pps/qnx/device/";
const std::string PPSListener::kPpsPathAll = ".all";
const std::string PPSListener::kPpsPathCtrl = "usb_ctrl";

PPSListener::PPSListener(TransportAdapterController* controller)
    : initialised_(false),
      controller_(controller),
      fd_(-1),
      pps_thread_(NULL),
      mq_thread_(NULL),
      is_aoa_available_(false) {
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

bool PPSListener::ArmEvent(struct sigevent* event) {
  LOG4CXX_AUTO_TRACE(logger_);
  uint8_t buf[2048];
  while (ionotify(fd_, _NOTIFY_ACTION_POLLARM, _NOTIFY_COND_INPUT, event)
      & _NOTIFY_COND_INPUT) {
    int size = read(fd_, buf, sizeof(buf));
    if (size > 0) {
      buf[size] = '\0';
      // In case AOA is not vailable don't need to process any messages from PPS.
      if (is_aoa_available_) {
        Process(buf, size);
      }
    }
  }
  return true;
}

std::string PPSListener::ParsePpsData(char* ppsdata, const char** vals) {
  LOG4CXX_AUTO_TRACE(logger_);
  static const char* attrs[] = { "aoa", "busno", "devno", "manufacturer",
      "vendor_id", "product", "product_id", "serial_number", "stackno", NULL };
  int rc;
  std::string object_name;
  pps_attrib_t info = { 0 };
  /* Loop through the PPS objects looking for aoa, busno, devno and etc. */
  while ((rc = ppsparse(&ppsdata, NULL, attrs, &info, 0))) {
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
  }

  return object_name;
}

void PPSListener::Process(uint8_t* buf, size_t size) {
  LOG4CXX_AUTO_TRACE(logger_);
  DCHECK(buf);

  char* ppsdata = reinterpret_cast<char*>(buf);
  const char* vals[ATTR_COUNT] = { 0 };

  std::string object_name = ParsePpsData(ppsdata, vals);
  if (IsAOADevice(vals)) {
    AOAWrapper::AOAUsbInfo aoa_usb_info;
    FillUsbInfo(object_name, vals, &aoa_usb_info);
    if (IsAOAMode(aoa_usb_info)) {
      AddDevice(aoa_usb_info);
    } else {
      SwitchMode(object_name.c_str(), vals);
    }
  }
}

bool PPSListener::IsAOAMode(const AOAWrapper::AOAUsbInfo& aoa_usb_info) {
  LOG4CXX_AUTO_TRACE(logger_);
  /*
   * Check a device if it's already in AOA mode (product and vendor ID's
   * will match a known list).
   */
  const uint32_t kVendorIdGoogle = 0x18d1;
  if (aoa_usb_info.vendor_id == kVendorIdGoogle) {
    const uint32_t* begin = kProductList;
    const uint32_t* end = kProductList
        + sizeof(kProductList) / sizeof(uint32_t);
    const uint32_t* p = std::find(begin, end, aoa_usb_info.product_id);
    if (p != end) {
      LOG4CXX_DEBUG(
          logger_,
          "AOA: mode of device " << aoa_usb_info.serial_number << " is AOA");
      return true;
    }
  }
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

void PPSListener::FillUsbInfo(const std::string& object_name,
                              const char** attrs,
                              AOAWrapper::AOAUsbInfo* info) {
  LOG4CXX_AUTO_TRACE(logger_);
  info->path = kUSBStackPath;
  info->aoa_version = strtoul(attrs[ATTR_AOA], NULL, 0);
  info->busno = strtoul(attrs[ATTR_BUSNO], NULL, 0);
  info->devno = strtoul(attrs[ATTR_DEVNO], NULL, 0);
  info->manufacturer = attrs[ATTR_MANUFACTURER];
  info->vendor_id = strtoul(attrs[ATTR_VENDOR_ID], NULL, 0);
  info->product = attrs[ATTR_PRODUCT];
  info->product_id = strtoul(attrs[ATTR_PRODUCT_ID], NULL, 0);
  info->serial_number = attrs[ATTR_SERIAL_NUMBER];
  info->stackno = attrs[ATTR_STACKNO];
  info->iface = 0;
}

void PPSListener::SwitchMode(const char* objname, const char** attrs) {
  LOG4CXX_AUTO_TRACE(logger_);
  int i, rc, fd;
  char cmd[PATH_MAX];
  size_t sz;

  /* Do we have an object name? */
  DCHECK(objname);

  /* Create the PPS request string (start_aoa::busno,devno,request_mask,key=value,key=value,...) */
  sz = snprintf(cmd, sizeof(cmd), "start_aoa::busno=%s,devno=%s,modes=%u",
                attrs[ATTR_BUSNO], attrs[ATTR_DEVNO], REQUEST_MASK);

  i = 0;
  while (kKeys[i]) {
    /* Add all the key/value pairs to the request */
    sz += snprintf(cmd + sz, sizeof(cmd) - sz, ",%s=%s", kKeys[i], kValues[i]);
    i++;
  }

  /* Open the object we want to write to */
  std::string ctrl = kPpsPathRoot + kPpsPathCtrl + attrs[ATTR_STACKNO];
  fd = open(ctrl.c_str(), O_WRONLY);
  if (fd == -1) {
    LOG4CXX_ERROR(
        logger_,
        "AOA: Error opening file '" << ctrl << "': " << strerror(errno) << "'");
    return;
  }

  /* Write to the PPS object */
  rc = write(fd, cmd, sz);
  if (rc == -1) {
    LOG4CXX_ERROR(
        logger_,
        "AOA: Error writing to file '" << ctrl << "': " << strerror(errno) << "' " << cmd);
    return;
  }

  close(fd);
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
  AOADynamicDeviceSPtr aoa_device(new AOADynamicDevice(
      aoa_usb_info.product, aoa_usb_info.serial_number, aoa_usb_info,
      controller_));
  controller_->AddDevice(aoa_device);
  if (!aoa_device->StartHandling()) {
    LOG4CXX_WARN(logger_, "Can not initialize device");
  }
}

bool PPSListener::init_aoa() {
  LOG4CXX_AUTO_TRACE(logger_);
  is_aoa_available_ = AOAWrapper::Init(NULL);
  return is_aoa_available_;
}

bool PPSListener::release_aoa() const {
  AOAWrapper::Disconnect(true);
  is_aoa_available_ = false;
  return is_aoa_available_;
}

void PPSListener::release_thread(threads::Thread** thread) {
  if(thread && *thread) {
    (*thread)->join();
    delete (*thread)->delegate();
    threads::DeleteThread(*thread);
    *thread = NULL;
  }
}

TransportAdapter::Error PPSListener::StartListening() {
  LOG4CXX_AUTO_TRACE(logger_);
  pps_thread_ = threads::CreateThread("PPS listener", new PpsThreadDelegate(this));
  mq_thread_ = threads::CreateThread("PPS MQ Listener", new PpsMQListener(this));
  pps_thread_->start();
  mq_thread_->start();
  return TransportAdapter::OK;
}

TransportAdapter::Error PPSListener::StopListening() {
  LOG4CXX_AUTO_TRACE(logger_);
  release_thread(&mq_thread_);
  release_thread(&pps_thread_);
  AOAWrapper::Shutdown();
  return TransportAdapter::OK;
}

PPSListener::PpsMQListener::PpsMQListener(PPSListener* parent)
  : parent_(parent),
    run_(false) {

  init_mq(PREFIX_STR_TOSDL_QUEUE, O_CREAT|O_RDONLY, mq_from_applink_handle_);
  init_mq(PREFIX_STR_FROMSDL_QUEUE, O_CREAT|O_WRONLY, mq_to_applink_handle_);

  run_ = true;
}

void PPSListener::PpsMQListener::threadMain() {
  if (mq_to_applink_handle_ != -1) {
    while (run_) {
      char msg[MAX_QUEUE_MSG_SIZE];
      ssize_t size = mq_receive(mq_from_applink_handle_, msg, MAX_QUEUE_MSG_SIZE, NULL);
      LOG4CXX_DEBUG(logger_, "Receive message from Applink with size: " << size
                    << ". Message signal: " << static_cast<int32_t>(msg[0])
                    << ". TAKE_AOA signal is: " << static_cast<int32_t>(TAKE_AOA)
                    << ". RELEASE_AOA signal is: " << static_cast<int32_t>(RELEASE_AOA));
      if (-1 != size) {
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
}

void PPSListener::PpsMQListener::exitThreadMain() {
  run_ = false;
  deinit_mq(mq_from_applink_handle_);
  deinit_mq(mq_to_applink_handle_);
}

void PPSListener::PpsMQListener::take_aoa() {
  LOG4CXX_AUTO_TRACE(logger_);
  const bool is_inited = parent_->init_aoa();
  if (-1 != mq_from_applink_handle_) {
    char buf[MAX_QUEUE_MSG_SIZE];
    buf[0] = AOA_TAKEN;
    buf[1] = AOA_INACCESSIBLE;
    if (- 1 == mq_send(mq_to_applink_handle_, is_inited ? &buf[0] : &buf[1], MAX_QUEUE_MSG_SIZE, NULL)) {
      LOG4CXX_ERROR(logger_, "Unable to send over mq " <<
                    " : " << strerror(errno));
    }
  }
}

void PPSListener::PpsMQListener::release_aoa() {
  LOG4CXX_AUTO_TRACE(logger_);
  parent_->release_aoa();
  if (-1 != mq_from_applink_handle_) {
    char buf[MAX_QUEUE_MSG_SIZE];
    buf[0] = AOA_RELEASED;
    if (- 1 == mq_send(mq_to_applink_handle_, &buf[0], MAX_QUEUE_MSG_SIZE, NULL)) {
      LOG4CXX_ERROR(logger_, "Unable to send over mq " <<
                    " : " << strerror(errno));
    }
  }
}

void PPSListener::PpsMQListener::deinit_mq(mqd_t descriptor) {
  if(-1 == mq_close(descriptor)) {
    LOG4CXX_ERROR(logger_, "Unable to close mq: " << strerror(errno));
  }
}

void PPSListener::PpsMQListener::init_mq(const std::string& name,
                                         int flags, int& descriptor) {
  struct mq_attr attributes;
  attributes.mq_maxmsg = MAX_MESSAGES;
  attributes.mq_msgsize = MAX_QUEUE_MSG_SIZE;
  attributes.mq_flags = 0;

  descriptor = mq_open(name.c_str(), flags, 0666, &attributes);

  if (-1 == descriptor) {
    LOG4CXX_ERROR(logger_, "Unable to open mq " << name.c_str() <<
                  " : " << strerror(errno));
  }
}

PPSListener::PpsThreadDelegate::PpsThreadDelegate(PPSListener* parent)
    : parent_(parent) {
}

bool PPSListener::PpsThreadDelegate::ArmEvent(struct sigevent* event) {
  return parent_->ArmEvent(event);
}

void PPSListener::PpsThreadDelegate::OnPulse() {
}

}  // namespace transport_adapter
}  // namespace transport_manager
