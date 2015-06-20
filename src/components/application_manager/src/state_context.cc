/*
 * Copyright (c) 2015, Ford Motor Company
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
#include "application_manager/state_context.h"
#include "application_manager/application_manager_impl.h"
#include "config_profile/profile.h"

namespace application_manager {

StateContext::StateContext(ApplicationManager* app_mngr)
    : app_mngr_(app_mngr) {}

StateContext::~StateContext() { set_app_mngr(NULL); }

bool StateContext::is_navi_app(const uint32_t app_id) const {
  DCHECK_OR_RETURN(app_mngr_, false);
  ApplicationSharedPtr app = app_mngr_->application(app_id);
  DCHECK_OR_RETURN(app, false);
  return app ? app->is_navi() : false;
}

bool StateContext::is_media_app(const uint32_t app_id) const {
  DCHECK_OR_RETURN(app_mngr_, false);
  ApplicationSharedPtr app = app_mngr_->application(app_id);
  return app ? app->is_media_application() : false;
}

bool StateContext::is_voice_communication_app(const uint32_t app_id) const {
  DCHECK_OR_RETURN(app_mngr_, false);
  ApplicationSharedPtr app = app_mngr_->application(app_id);
  return app ? app->is_voice_communication_supported() : false;
}

bool StateContext::is_attenuated_supported() const {
  DCHECK_OR_RETURN(app_mngr_, false);
  const HMICapabilities& hmi_capabilities = app_mngr_->hmi_capabilities();
  return hmi_capabilities.attenuated_supported() &&
         profile::Profile::instance()->is_mixing_audio_supported();
}

void StateContext::set_app_mngr(ApplicationManager* app_mngr) {
  app_mngr_ = app_mngr;
}

void StateContext::OnHMILevelChanged(uint32_t app_id,
                                     mobile_apis::HMILevel::eType from,
                                     mobile_apis::HMILevel::eType to) const {
  DCHECK_OR_RETURN_VOID(app_mngr_);
  app_mngr_->OnHMILevelChanged(app_id, from, to);
}

void StateContext::SendHMIStatusNotification(
    const utils::SharedPtr<Application> app) const {
  DCHECK_OR_RETURN_VOID(app_mngr_);
  app_mngr_->SendHMIStatusNotification(app);
}

const uint32_t StateContext::application_id(
    const int32_t correlation_id) const {
  DCHECK_OR_RETURN(app_mngr_, 0);
  return app_mngr_->application_id(correlation_id);
}

mobile_apis::HMILevel::eType StateContext::GetDefaultHmiLevel(
    ApplicationConstSharedPtr application) const {
  return app_mngr_->GetDefaultHmiLevel(application);
}
ApplicationSharedPtr StateContext::application_by_hmi_app(
    const uint32_t hmi_app_id) const {
  DCHECK_OR_RETURN(app_mngr_, ApplicationSharedPtr());
  return app_mngr_->application_by_hmi_app(hmi_app_id);
}
}  // namespace application_manager
