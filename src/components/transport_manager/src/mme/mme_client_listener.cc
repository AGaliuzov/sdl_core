/*
 *
 * Copyright (c) 2013, Ford Motor Company
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
#include "transport_manager/mme/mme_client_listener.h"

#include <string.h>
#include <string>

#include "utils/logger.h"
#include "utils/make_shared.h"

#include "config_profile/profile.h"

#include "transport_manager/mme/iap_device.h"
#include "transport_manager/mme/iap2_device.h"
#include "transport_manager/transport_adapter/transport_adapter_impl.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

MmeClientListener::MmeClientListener(TransportAdapterController* controller)
  : controller_(controller),
    qdb_hdl_(NULL),
    notify_thread_(NULL),
    notify_thread_delegate_(NULL) {
}

TransportAdapter::Error MmeClientListener::Init() {
  TransportAdapter::Error error = TransportAdapter::OK;

  const std::string& mme_db_name = profile::Profile::instance()->mme_db_name();
  LOG4CXX_TRACE(logger_, "Connecting to " << mme_db_name);
  qdb_hdl_ = qdb_connect(mme_db_name.c_str(), 0);
  if (qdb_hdl_ != 0) {
    LOG4CXX_DEBUG(logger_, "Connected to " << mme_db_name);
  } else {
    LOG4CXX_ERROR(logger_, "Could not connect to " << mme_db_name);
    error = TransportAdapter::FAIL;
  }

  const std::string& event_mq_name = profile::Profile::instance()->event_mq_name();
  const std::string& ack_mq_name = profile::Profile::instance()->ack_mq_name();

#define CREATE_MME_MQ 0
  LOG4CXX_TRACE(logger_, "Opening " << event_mq_name);
#if CREATE_MME_MQ
  int flags = O_RDONLY | O_CREAT;
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  struct mq_attr attributes;
  attributes.mq_maxmsg = MSGQ_MAX_MESSAGES;
  attributes.mq_msgsize = MAX_QUEUE_MSG_SIZE;
  attributes.mq_flags = 0;
  event_mqd_ = mq_open(event_mq_name.c_str(), flags, mode, &attributes);
#else
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  int flags = O_RDWR;
  event_mqd_ = mq_open(event_mq_name.c_str(), flags, mode);
#endif
  if (event_mqd_ != -1) {
    LOG4CXX_DEBUG(logger_, "Opened " << event_mq_name);
  } else {
    LOG4CXX_ERROR(logger_,
                  "Could not open " << event_mq_name << ", errno = " << errno);
    error = TransportAdapter::FAIL;
  }

  LOG4CXX_TRACE(logger_, "Opening " << ack_mq_name);
#if CREATE_MME_MQ
  flags = O_WRONLY | O_CREAT;
  ack_mqd_ = mq_open(ack_mq_name.c_str(), flags, mode, &attributes);
#else
  flags = O_WRONLY;
  ack_mqd_ = mq_open(ack_mq_name.c_str(), flags);
#endif
  if (ack_mqd_ != -1) {
    LOG4CXX_DEBUG(logger_, "Opened " << ack_mq_name);
  } else {
    LOG4CXX_ERROR(logger_,
                  "Could not open " << ack_mq_name << ", errno = " << errno);
    error = TransportAdapter::FAIL;
  }

  initialised_ = true;
  return error;
}

void MmeClientListener::Terminate() {
  initialised_ = false;

  if (TransportAdapter::OK != StopListening()) {
    LOG4CXX_DEBUG(logger_, "Stop client listening failed. "
                           "Continue terminating.");
  }

  const std::string& event_mq_name =
      profile::Profile::instance()->event_mq_name();
  LOG4CXX_TRACE(logger_, "Closing " << event_mq_name);
  if (mq_close(event_mqd_) != -1) {
    LOG4CXX_DEBUG(logger_, "Closed " << event_mq_name);
  } else {
    LOG4CXX_WARN(logger_, "Could not close " << event_mq_name);
  }
  const std::string& ack_mq_name = profile::Profile::instance()->ack_mq_name();
  LOG4CXX_TRACE(logger_, "Closing " << ack_mq_name);
  if (mq_close(ack_mqd_) != -1) {
    LOG4CXX_DEBUG(logger_, "Closed " << ack_mq_name);
  } else {
    LOG4CXX_WARN(logger_, "Could not close " << ack_mq_name);
  }

  const std::string& mme_db_name = profile::Profile::instance()->mme_db_name();
  LOG4CXX_TRACE(logger_, "Disconnecting from " << mme_db_name);
  if (qdb_disconnect(qdb_hdl_) != -1) {
    LOG4CXX_DEBUG(logger_, "Disconnected from " << mme_db_name);
  } else {
    LOG4CXX_WARN(logger_, "Could not disconnect from " << mme_db_name);
  }
}

bool MmeClientListener::IsInitialised() const {
  return initialised_;
}

TransportAdapter::Error MmeClientListener::StartListening() {
  LOG4CXX_TRACE(logger_, "enter");
  if ((event_mqd_ != -1) && (ack_mqd_ != -1)) {
    notify_thread_delegate_ = new NotifyThreadDelegate(event_mqd_, ack_mqd_, this);
    notify_thread_ = threads::CreateThread("MME MQ notifier", notify_thread_delegate_);
    notify_thread_->start();
    LOG4CXX_TRACE(logger_, "exit");
    return TransportAdapter::OK;
  } else {
    LOG4CXX_ERROR(logger_, "Cannot start MME MQ notifier");
    LOG4CXX_TRACE(logger_, "exit");
    return TransportAdapter::FAIL;
  }
}

TransportAdapter::Error MmeClientListener::StopListening() {
  if (notify_thread_) {
    notify_thread_->join();
    delete notify_thread_->delegate();
    threads::DeleteThread(notify_thread_);
    notify_thread_ = NULL;
    notify_thread_delegate_ = NULL;
  }
  return TransportAdapter::OK;
}

void MmeClientListener::OnDeviceArrived(const MmeDeviceInfo* mme_device_info) {
  if (mme_device_info == NULL) {
    LOG4CXX_ERROR(logger_, "Can't parse device info.");
    return;
  }

  const MmeDevice::Protocol protocol = get_protocol_type(mme_device_info);
  const std::string device_name(mme_device_info->device_name);
  const std::string unique_device_id(mme_device_info->serial);
  const std::string mount_point(mme_device_info->mount_path);
  MmeDevicePtr mme_device;
  switch (protocol) {
    case MmeDevice::IAP: {
      mme_device = utils::MakeShared<IAPDevice>(
            mount_point, device_name, unique_device_id, controller_);
      break;
    }
    case MmeDevice::IAP2: {
      mme_device = utils::MakeShared<IAP2Device>(
            mount_point, device_name, unique_device_id, controller_);
      break;
    }
    case MmeDevice::UnknownProtocol:
    default:
      LOG4CXX_WARN(logger_,
                   "Unsupported protocol for device " << device_name);
      return;
  }
  controller_->AddDevice(mme_device);
// new device must be initialized
// after ON_SEARCH_DEVICE_DONE notification
// because ON_APPLICATION_LIST_UPDATED event can occur immediately
// which doesn't make sense until device list is updated
  mme_device->Init();
}

void MmeClientListener::OnDeviceLeft(const MmeDeviceInfo* mme_device_info) {
  if (mme_device_info == NULL) {
    LOG4CXX_ERROR(logger_, "Can't parse device info.");
    return;
  }

  const std::string unique_device_id(mme_device_info->serial);
  controller_->DeviceDisconnected(unique_device_id, DisconnectDeviceError());
}

MmeDevice::Protocol MmeClientListener::get_protocol_type(
    const MmeDeviceInfo* mme_device_info) const {
  if (mme_device_info == NULL) {
    LOG4CXX_ERROR(logger_, "Can't parse device info.");
    return MmeDevice::UnknownProtocol;
  }

  const std::string fs_type(mme_device_info->fs_type);
  if (0 == fs_type.compare("ipod")) {
    LOG4CXX_DEBUG(logger_, "Protocol iAP");
    return MmeDevice::IAP;
  } else if (0 == fs_type.compare("iap2")) {
    LOG4CXX_DEBUG(logger_, "Protocol iAP2");
    return MmeDevice::IAP2;
  } else {
    LOG4CXX_WARN(logger_, "Unsupported protocol " << fs_type);
    return MmeDevice::UnknownProtocol;
  }
}

MmeClientListener::NotifyThreadDelegate::NotifyThreadDelegate(
    mqd_t event_mqd, mqd_t ack_mqd, MmeClientListener* parent)
    : parent_(parent),
      event_mqd_(event_mqd),
      ack_mqd_(ack_mqd),
      run_(true) {
}

void MmeClientListener::NotifyThreadDelegate::threadMain() {
  const std::string& event_mq_name =
      profile::Profile::instance()->event_mq_name();
  const std::string& ack_mq_name = profile::Profile::instance()->ack_mq_name();
  while (run_) {
    LOG4CXX_TRACE(logger_, "Waiting for message from " << event_mq_name);
    ssize_t size = mq_receive(event_mqd_, buffer_, kBufferSize, 0);
    if (size != -1) {
      LOG4CXX_DEBUG(logger_,
                    "Received " << size << " bytes from " << event_mq_name);
      char code = buffer_[0];
      LOG4CXX_DEBUG(logger_, "code = " << (int) code);
      switch (code) {
        case SDL_MSG_IPOD_DEVICE_CONNECT: {
          MmeDeviceInfo* mme_device_info = (MmeDeviceInfo*) (&buffer_[1]);
          const char* name = mme_device_info->device_name;
          LOG4CXX_DEBUG(
              logger_,
              "SDL_MSG_IPOD_DEVICE_CONNECT: " << " name = " << name );
          parent_->OnDeviceArrived(mme_device_info);
          LOG4CXX_DEBUG(logger_, "Sending SDL_MSG_IPOD_DEVICE_CONNECT_ACK");
          ack_buffer_[0] = SDL_MSG_IPOD_DEVICE_CONNECT_ACK;
          LOG4CXX_TRACE(logger_, "Sending message to " << ack_mq_name);
          if (mq_send(ack_mqd_, ack_buffer_, kAckBufferSize, 0) != -1) {
            LOG4CXX_DEBUG(logger_, "Message sent to " << ack_mq_name);
          } else {
            LOG4CXX_WARN(
                logger_,
                "Error occurred while sending message to " << ack_mq_name << ", errno = " << errno);
          }
          break;
        }
        case SDL_MSG_IPOD_DEVICE_DISCONNECT: {
          MmeDeviceInfo* mme_device_info = (MmeDeviceInfo*) (&buffer_[1]);
          const char* name = mme_device_info->device_name;
          LOG4CXX_DEBUG(
              logger_,
              "SDL_MSG_IPOD_DEVICE_DISCONNECT: " << " name = " << name);
          parent_->OnDeviceLeft(mme_device_info);
          LOG4CXX_DEBUG(logger_, "Sending SDL_MSG_IPOD_DEVICE_DISCONNECT_ACK");
          ack_buffer_[0] = SDL_MSG_IPOD_DEVICE_DISCONNECT_ACK;
          LOG4CXX_TRACE(logger_, "Sending message to " << ack_mq_name);
          if (mq_send(ack_mqd_, ack_buffer_, kAckBufferSize, 0) != -1) {
            LOG4CXX_DEBUG(logger_, "Message sent to " << ack_mq_name);
          } else {
            LOG4CXX_WARN(
                logger_,
                "Error occurred while sending message to " << ack_mq_name << ", errno = " << errno);
          }
          break;
        }
        case SDL_MSG_SDL_STOP: {
            run_ = false;
            LOG4CXX_DEBUG(logger_,"SDL_MSG_SDL_STOP");
          break;
        }
      }
    } else {
      LOG4CXX_WARN(
          logger_,
          "Error occurred while receiving message from " << event_mq_name << ", errno = " << errno);
    }
  }
}
//ToDo: Fix me see datail APPLINK-12215
void MmeClientListener::NotifyThreadDelegate::exitThreadMain() {
    LOG4CXX_AUTO_TRACE(logger_);
    const std::string& ack_mq_name = profile::Profile::instance()->ack_mq_name();
    ack_buffer_[0] = SDL_MSG_SDL_STOP;
    if (mq_send(event_mqd_, ack_buffer_, kAckBufferSize, 0) != -1) {
        LOG4CXX_DEBUG(logger_,"Send 'SDL_MSG_SDL_STOP' to " << ack_mq_name);
    } else {
        LOG4CXX_WARN(logger_,"Error occurred while sending message," <<  " errorno = " << strerror(errno));
    }
}

}  // namespace transport_adapter
}  // namespace transport_manager

