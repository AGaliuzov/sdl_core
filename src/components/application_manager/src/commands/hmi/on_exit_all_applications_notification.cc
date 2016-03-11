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

#include "application_manager/commands/hmi/on_exit_all_applications_notification.h"

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "application_manager/application_manager_impl.h"
#include "interfaces/HMI_API.h"
#ifdef CUSTOMER_PASA
#include <mqueue.h>
#include <applink_types.h>
#endif


namespace application_manager {

namespace commands {

OnExitAllApplicationsNotification::OnExitAllApplicationsNotification(
    const MessageSharedPtr& message) : NotificationFromHMI(message) {
}

OnExitAllApplicationsNotification::~OnExitAllApplicationsNotification() {
}

void OnExitAllApplicationsNotification::Run() {
  LOG4CXX_AUTO_TRACE(logger_);

  const hmi_apis::Common_ApplicationsCloseReason::eType reason =
      static_cast<hmi_apis::Common_ApplicationsCloseReason::eType>(
          (*message_)[strings::msg_params][hmi_request::reason].asInt());
  LOG4CXX_DEBUG(logger_, "Reason " << reason);

  mobile_api::AppInterfaceUnregisteredReason::eType mob_reason =
      mobile_api::AppInterfaceUnregisteredReason::INVALID_ENUM;

  ApplicationManagerImpl* app_manager = ApplicationManagerImpl::instance();

  switch (reason) {
    case hmi_apis::Common_ApplicationsCloseReason::IGNITION_OFF: {
      mob_reason = mobile_api::AppInterfaceUnregisteredReason::IGNITION_OFF;
      break;
    }
    case hmi_apis::Common_ApplicationsCloseReason::MASTER_RESET: {
      mob_reason = mobile_api::AppInterfaceUnregisteredReason::MASTER_RESET;
      break;
    }
    case hmi_apis::Common_ApplicationsCloseReason::FACTORY_DEFAULTS: {
      mob_reason = mobile_api::AppInterfaceUnregisteredReason::FACTORY_DEFAULTS;
      break;
    }
    case hmi_apis::Common_ApplicationsCloseReason::SUSPEND: {
#ifdef CUSTOMER_PASA
      mob_reason = mobile_api::AppInterfaceUnregisteredReason::IGNITION_OFF;
      ApplicationManagerImpl::instance()->resume_controller().OnSuspend();

      /*
       * 'Suspended' flag should be set after all suspend-related flows have
       * been finished. Otherwise resume data may not be saved (for case of
       * unexpected disconnect on Bluetooth).
       */
      app_manager->set_state_suspended(true);
#endif // CUSTOMER_PASA
      SendOnSDLPersistenceComplete();
      return;
    }
    default : {
      LOG4CXX_ERROR(logger_, "Unknown Application close reason" << reason);
      return;
    }
  }

  app_manager->SetUnregisterAllApplicationsReason(mob_reason);

  if (mobile_api::AppInterfaceUnregisteredReason::MASTER_RESET == mob_reason ||
      mobile_api::AppInterfaceUnregisteredReason::FACTORY_DEFAULTS == mob_reason) {
    app_manager->HeadUnitReset(mob_reason);
  }
#ifdef CUSTOMER_PASA
  struct mq_attr attributes;
  attributes.mq_maxmsg = 128;
  attributes.mq_msgsize = 4095;
  attributes.mq_flags = 0;

  mqd_t mq = mq_open(PREFIX_STR_SDL_PROXY_QUEUE,
                O_WRONLY | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
                &attributes);
  if (-1 == mq) {
    LOG4CXX_ERROR(logger_, "Unable to open mqueue " << strerror(errno));
    return;
  }
  LOG4CXX_DEBUG(logger_, "Opened mqueue");
  char buf = SDL_MSG_SDL_STOP;
  if (-1 != mq_send(mq, &buf, 1, 0)) {
    LOG4CXX_INFO(logger_, "SDL_MSG_SDL_STOP sent to SDL controller");
  } else {
    LOG4CXX_FATAL(logger_, "Unable to send SDL_MSG_SDL_STOP " << strerror(errno));
  }
  mq_close(mq);
#else
  kill(getpid(), SIGINT);
#endif
}

void OnExitAllApplicationsNotification::SendOnSDLPersistenceComplete() {
  LOG4CXX_AUTO_TRACE(logger_);

  smart_objects::SmartObjectSPtr message =
      new smart_objects::SmartObject(smart_objects::SmartType_Map);
  (*message)[strings::params][strings::function_id] =
      hmi_apis::FunctionID::BasicCommunication_OnSDLPersistenceComplete;
  (*message)[strings::params][strings::message_type] = MessageType::kNotification;
  (*message)[strings::params][strings::correlation_id] =
      ApplicationManagerImpl::instance()->GetNextHMICorrelationID();

   ApplicationManagerImpl::instance()->ManageHMICommand(message);
}

}  // namespace commands

}  // namespace application_manager

