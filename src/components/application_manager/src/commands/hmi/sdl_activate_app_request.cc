/*
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

#include "application_manager/commands/hmi/sdl_activate_app_request.h"
#include "application_manager/policies/policy_handler.h"
#include "application_manager/message_helper.h"

namespace application_manager {

namespace commands {

SDLActivateAppRequest::SDLActivateAppRequest(const MessageSharedPtr& message)
    : RequestFromHMI(message) {
}

SDLActivateAppRequest::~SDLActivateAppRequest() {
}

void SDLActivateAppRequest::Run() {
  LOG4CXX_AUTO_TRACE(logger_);
  using namespace hmi_apis::FunctionID;

  const uint32_t app_id = connection_key();
  ApplicationConstSharedPtr app =
      ApplicationManagerImpl::instance()->application(app_id);

  if (app && !app->IsRegistered()) {
    MessageHelper::SendLaunchApp(app_id, app->SchemaUrl(), app->PackageName());
    subscribe_on_event(BasicCommunication_OnAppRegistered);
  } else {
    policy::PolicyHandler::instance()->OnActivateApp(app_id, correlation_id());
  }
}

void SDLActivateAppRequest::onTimeOut() {
  using namespace hmi_apis::FunctionID;
  using namespace hmi_apis::Common_Result;
  using namespace application_manager;
  unsubscribe_from_event(BasicCommunication_OnAppRegistered);
  const bool is_success = false;
  SendResponse(is_success, correlation_id(),
               BasicCommunication_ActivateApp, APPLICATION_NOT_REGISTERED);
}

void SDLActivateAppRequest::on_event(const event_engine::Event& event) {
  using namespace hmi_apis::FunctionID;
  if (event.id() != BasicCommunication_OnAppRegistered) {
    return;
  }
  unsubscribe_from_event(BasicCommunication_OnAppRegistered);
  policy::PolicyHandler::instance()->OnActivateApp(connection_key(), correlation_id());
}

}  // namespace commands
}  // namespace application_manager

