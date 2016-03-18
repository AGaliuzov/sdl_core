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
#include "application_manager/application_manager_impl.h"

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

  if (ApplicationManagerImpl::instance()->
      IsStateActive(HmiState::STATE_ID_DEACTIVATE_HMI)) {
    LOG4CXX_DEBUG(logger_, "DeactivateHmi state is active. "
                           "Sends response with result code REJECTED");
    SendErrorResponse(correlation_id(),
                      static_cast<eType>(function_id()),
                      hmi_apis::Common_Result::REJECTED);
  } else {
    const uint32_t application_id = app_id();
    application_manager::ApplicationManagerImpl::instance()->GetPolicyHandler().OnActivateApp(application_id,
                                                     correlation_id());
  }
}

uint32_t SDLActivateAppRequest::app_id() const {

  if ((*message_).keyExists(strings::msg_params)) {
    if ((*message_)[strings::msg_params].keyExists(strings::app_id)){
        return (*message_)[strings::msg_params][strings::app_id].asUInt();
    }
  }
  LOG4CXX_DEBUG(logger_, "app_id section is absent in the message.");
  return 0;
}

}  // namespace commands
}  // namespace application_manager

