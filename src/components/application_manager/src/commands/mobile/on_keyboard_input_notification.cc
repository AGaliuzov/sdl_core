/*

 Copyright (c) 2013, Ford Motor Company
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following
 disclaimer in the documentation and/or other materials provided with the
 distribution.

 Neither the name of the Ford Motor Company nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "application_manager/commands/mobile/on_keyboard_input_notification.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/application_impl.h"
#include "interfaces/MOBILE_API.h"

namespace application_manager {

namespace commands {

namespace mobile {

OnKeyBoardInputNotification::OnKeyBoardInputNotification(
    const MessageSharedPtr& message)
    : CommandNotificationImpl(message) {}

OnKeyBoardInputNotification::~OnKeyBoardInputNotification() {}

void OnKeyBoardInputNotification::Run() {
  LOG4CXX_AUTO_TRACE(logger_);

  ApplicationSharedPtr app_to_notify;

  ApplicationManagerImpl::ApplicationListAccessor accessor;
  ApplicationSetIt it = accessor.begin();
  for (; accessor.end() != it; ++it) {
    // if there is app with active perform interaction use it for notification
    if ((*it)->is_perform_interaction_active() &&
        (*it)->perform_interaction_layout() ==
            mobile_apis::LayoutMode::KEYBOARD) {
      LOG4CXX_DEBUG(logger_,
                   "There is application with active PerformInteraction and keyboard layout");
      app_to_notify = *it;
      break;
    }

    if (mobile_apis::HMILevel::eType::HMI_FULL == (*it)->hmi_level()) {
      LOG4CXX_DEBUG(logger_, "There is application in HMI_FULL level");
      app_to_notify = *it;
    }
  }

  if (app_to_notify.valid()) {
    (*message_)[strings::params][strings::connection_key] =
        app_to_notify->app_id();
    SendNotification();
  }
}

}  // namespace mobile

}  // namespace commands

}  // namespace application_manager
