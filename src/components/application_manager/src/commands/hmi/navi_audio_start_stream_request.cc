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

#include "application_manager/commands/hmi/navi_audio_start_stream_request.h"
#include "application_manager/application_manager_impl.h"
#include "protocol_handler/protocol_handler.h"

namespace application_manager {

namespace commands {

AudioStartStreamRequest::AudioStartStreamRequest(
    const MessageSharedPtr& message)
    : RequestToHMI(message) {
}

AudioStartStreamRequest::~AudioStartStreamRequest() {
}

void AudioStartStreamRequest::Run() {
  LOG4CXX_AUTO_TRACE(logger_);

  subscribe_on_event(hmi_apis::FunctionID::Navigation_StartAudioStream,
                     correlation_id());
  ApplicationManagerImpl* app_mgr = ApplicationManagerImpl::instance();
  DCHECK_OR_RETURN_VOID(app_mgr);
  ApplicationSharedPtr app = app_mgr->application_by_hmi_app(application_id());
  if (app) {
    app->set_audio_streaming_allowed(true);
    SendRequest();
  } else {
    LOG4CXX_ERROR(logger_, "Applcation with hhi_app_id "
                 << application_id() << "does not exist");
  }
}

void AudioStartStreamRequest::on_event(const event_engine::Event& event) {
  using namespace protocol_handler;
  LOG4CXX_AUTO_TRACE(logger_);

  ApplicationManagerImpl* app_mgr = ApplicationManagerImpl::instance();
  DCHECK_OR_RETURN_VOID(app_mgr);

  ApplicationSharedPtr app = app_mgr->application_by_hmi_app(application_id());
  if (!app) {
    LOG4CXX_ERROR_EXT(logger_,
        "StartAudioStreamRequest aborted. Application not found");
    return;
  }

  const smart_objects::SmartObject& message = event.smart_object();
  switch (event.id()) {
    case hmi_apis::FunctionID::Navigation_StartAudioStream: {
      LOG4CXX_DEBUG(logger_, "Received StartStream event");

      const hmi_apis::Common_Result::eType code =
          static_cast<hmi_apis::Common_Result::eType>(
              message[strings::params][hmi_response::code].asInt());

      if (hmi_apis::Common_Result::SUCCESS == code) {
        LOG4CXX_DEBUG(logger_, "StartAudioStreamResponse SUCCESS");
        if (app_mgr->IsStreamingAllowed(app->app_id(), ServiceType::kAudio)) {
          app->set_audio_streaming_started(true);
        } else {
          LOG4CXX_DEBUG(logger_,
                       "StartAudioStreamRequest aborted. Application can not stream");
          ApplicationManagerImpl::instance()->EndNaviServices(app->app_id());
        }
      }
      break;
    }
    default: {
      LOG4CXX_ERROR(logger_,"Received unknown event" << event.id());
      return;
    }
  }
}

}  // namespace commands

}  // namespace application_manager
