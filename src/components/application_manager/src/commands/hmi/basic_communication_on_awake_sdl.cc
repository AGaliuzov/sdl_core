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
#ifdef CUSTOMER_PASA
#include "application_manager/commands/hmi/basic_communication_on_awake_sdl.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/message_helper.h"

namespace application_manager {

namespace commands {

OnAwakeSDLNotification::OnAwakeSDLNotification(
    const MessageSharedPtr& message) : NotificationFromHMI(message) {
}

OnAwakeSDLNotification::~OnAwakeSDLNotification() {
}

void OnAwakeSDLNotification::Run() {
  LOG4CXX_INFO(logger_, "OnAwakeSDLNotification::Run");

  ApplicationManagerImpl* app_manager = ApplicationManagerImpl::instance();
  if (app_manager->state_suspended()) {
    app_manager->set_state_suspended(false);
    ApplicationManagerImpl::ApplicationListAccessor accessor;
    ApplicationManagerImpl::TAppList local_app_list = accessor.applications();
    ApplicationManagerImpl::TAppListIt it = local_app_list.begin();
    ApplicationManagerImpl::TAppListIt itEnd = local_app_list.end();
    for (; it != itEnd; ++it) {
      if ((*it).valid()) {
        if ((*it)->flag_sending_hash_change_after_awake()) {
          MessageHelper::SendHashUpdateNotification((*it)->app_id());
          (*it)->set_flag_sending_hash_change_after_awake(false);
        }
      }
    }
    (app_manager->resume_controller()).StartSavePersistentDataTimer();
  }
}

}  // namespace commands

}  // namespace application_manager
#endif // CUSTOMER_PASA

