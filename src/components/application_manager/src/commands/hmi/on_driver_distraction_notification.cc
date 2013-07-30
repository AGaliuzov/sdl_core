/**
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

#include <set>
#include "application_manager/commands/hmi/on_driver_distraction_notification.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/application_impl.h"
#include "interfaces/MOBILE_API.h"
#include "interfaces/HMI_API.h"

namespace application_manager {

namespace commands {

OnDriverDistractionNotification::OnDriverDistractionNotification(
  const MessageSharedPtr& message): NotificationFromHMI(message) {
}

OnDriverDistractionNotification::~OnDriverDistractionNotification() {
}

void OnDriverDistractionNotification::Run() {
  LOG4CXX_INFO(logger_, "OnDriverDistractionNotification::Run");

  const std::set<Application*>& app_list =
    ApplicationManagerImpl::instance()->applications();

  const hmi_apis::Common_DriverDistractionState::eType state =
    static_cast<hmi_apis::Common_DriverDistractionState::eType>(
      (*message_)[strings::msg_params][hmi_notification::state].asInt());
  ApplicationManagerImpl::instance()->set_driver_distraction(state);

  std::set<Application*>::const_iterator it = app_list.begin();
  for (; app_list.end() != it; ++it) {
    const Application* app = *it;
    if (NULL != app) {
      const mobile_api::HMILevel::eType hmiLevel = app->hmi_level();
      if (mobile_api::HMILevel::HMI_FULL == hmiLevel ||
          mobile_api::HMILevel::HMI_BACKGROUND == hmiLevel) {
        NotifyMobileApp(app);
      }
    }
  }
  return;
}

void OnDriverDistractionNotification::NotifyMobileApp(
  const Application* app) {
  smart_objects::SmartObject* on_driver_distraction =
    new smart_objects::SmartObject();

  if (NULL == on_driver_distraction) {
    LOG4CXX_ERROR_EXT(logger_, "NULL pointer");
    return;
  }

  (*on_driver_distraction)[strings::params][strings::function_id] =
      mobile_api::FunctionID::OnDriverDistractionID;

  (*on_driver_distraction)[strings::params][strings::correlation_id] =
      (*message_)[strings::params][strings::correlation_id];

  (*on_driver_distraction)[strings::params][strings::message_type] =
      MessageType::kNotification;

  (*on_driver_distraction)[strings::msg_params][mobile_notification::state] =
      ApplicationManagerImpl::instance()->driver_distraction();

  (*on_driver_distraction)[strings::msg_params][strings::app_id] =
      app->app_id();

  SendNotificationToMobile(on_driver_distraction);
}

}  // namespace commands

}  // namespace application_manager
