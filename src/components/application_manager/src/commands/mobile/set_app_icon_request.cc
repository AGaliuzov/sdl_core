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

#include <algorithm>
#include "application_manager/commands/mobile/set_app_icon_request.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/application_impl.h"
#include "config_profile/profile.h"
#include "interfaces/MOBILE_API.h"
#include "interfaces/HMI_API.h"
#include "utils/file_system.h"
#include "utils/helpers.h"

namespace application_manager {

namespace commands {

SetAppIconRequest::SetAppIconRequest(const MessageSharedPtr& message)
    : CommandRequestImpl(message) {
}

SetAppIconRequest::~SetAppIconRequest() {
}

void SetAppIconRequest::Run() {
  LOG4CXX_AUTO_TRACE(logger_);

  ApplicationSharedPtr app =
      ApplicationManagerImpl::instance()->application(connection_key());

  if (!app) {
    LOG4CXX_ERROR(logger_, "Application is not registered");
    SendResponse(false, mobile_apis::Result::APPLICATION_NOT_REGISTERED);
    return;
  }

  const std::string& sync_file_name =
      (*message_)[strings::msg_params][strings::sync_file_name].asString();

  if (!IsSyncFileNameValid(sync_file_name)) {
    const std::string err_msg = "Sync file name contains forbidden symbols.";
    LOG4CXX_ERROR(logger_, err_msg);
    SendResponse(false, mobile_apis::Result::REJECTED,
                 err_msg.c_str());
    return;
  }

  std::string full_file_path =
#ifndef CUSTOMER_PASA
      file_system::CurrentWorkingDirectory() + "/" +
#endif // CUSTOMER_PASA
      profile::Profile::instance()->app_storage_folder() + "/";
  full_file_path += app->folder_name();
  full_file_path += "/";
  full_file_path += sync_file_name;

  if (!file_system::FileExists(full_file_path)) {
    LOG4CXX_ERROR(logger_, "No such file " << full_file_path);
    SendResponse(false, mobile_apis::Result::INVALID_DATA);
    return;
  }

  smart_objects::SmartObject msg_params = smart_objects::SmartObject(
      smart_objects::SmartType_Map);

  msg_params[strings::app_id] = app->app_id();
  msg_params[strings::sync_file_name] = smart_objects::SmartObject(
      smart_objects::SmartType_Map);

// Panasonic requres unchanged path value without encoded special characters
#ifdef CUSTOMER_PASA
  const std::string full_file_path_for_hmi = full_file_path;
#else
  const std::string full_file_path_for_hmi = file_system::ConvertPathForURL(
      full_file_path);
#endif

  msg_params[strings::sync_file_name][strings::value] = full_file_path_for_hmi;

  // TODO(VS): research why is image_type hardcoded
  msg_params[strings::sync_file_name][strings::image_type] =
      static_cast<int32_t> (SetAppIconRequest::ImageType::DYNAMIC);

  // for further use in on_event function
  (*message_)[strings::msg_params][strings::sync_file_name] =
      msg_params[strings::sync_file_name];

  SendHMIRequest(hmi_apis::FunctionID::UI_SetAppIcon, &msg_params, true);
}

void SetAppIconRequest::on_event(const event_engine::Event& event) {
  LOG4CXX_AUTO_TRACE(logger_);
  using namespace helpers;
  const smart_objects::SmartObject& message = event.smart_object();

  switch (event.id()) {
    case hmi_apis::FunctionID::UI_SetAppIcon: {
      mobile_apis::Result::eType result_code =
          static_cast<mobile_apis::Result::eType>(
              message[strings::params][hmi_response::code].asInt());

     const bool result =
         Compare<mobile_api::Result::eType, EQ, ONE>(
           result_code,
           mobile_api::Result::SUCCESS,
           mobile_api::Result::WARNINGS);

      if (result) {
        ApplicationSharedPtr app =
            ApplicationManagerImpl::instance()->application(connection_key());

        if (!message_.valid() || !app.valid()) {
           LOG4CXX_ERROR(logger_, "NULL pointer.");
           return;
        }

        const std::string& path = (*message_)[strings::msg_params]
                                              [strings::sync_file_name]
                                               [strings::value].asString();
        app->set_app_icon_path(path);

        LOG4CXX_INFO(logger_,
                     "Icon path was set to '" << app->app_icon_path() << "'");
      }

      SendResponse(result, result_code, NULL, &(message[strings::msg_params]));
      break;
    }
    default: {
      LOG4CXX_ERROR(logger_, "Received unknown event" << event.id());
      return;
    }
  }
}

bool SetAppIconRequest::IsSyncFileNameValid(const std::string& sync_file_name) {
  LOG4CXX_AUTO_TRACE(logger_);
  return sync_file_name.end() == std::find(sync_file_name.begin(),
                                           sync_file_name.end(),
                                           '/');
}

}  // namespace commands

}  // namespace application_manager
