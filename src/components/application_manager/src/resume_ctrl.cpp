/*
 Copyright (c) 2015, Ford Motor Company
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
#include "application_manager/resume_ctrl.h"

#include <fstream>
#include <algorithm>

#include "config_profile/profile.h"
#include "utils/file_system.h"
#include "connection_handler/connection_handler_impl.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/application.h"
#include "application_manager/message_helper.h"
#include "smart_objects/smart_object.h"
#include "connection_handler/connection.h"
#include "formatters/CFormatterJsonBase.hpp"
#include "application_manager/commands/command_impl.h"
#include "resumption/last_state.h"
#include "policy/policy_manager_impl.h"
#include "application_manager/policies/policy_handler.h"
#include "application_manager/state_controller.h"

namespace application_manager {

CREATE_LOGGERPTR_GLOBAL(logger_, "ResumeCtrl")

namespace Formatters = NsSmartDeviceLink::NsJSONHandler::Formatters;

ResumeCtrl::ResumeCtrl(ApplicationManagerImpl* app_mngr)
  : resumtion_lock_(true),
    app_mngr_(app_mngr),
    save_persistent_data_timer_("RsmCtrlPercist",
                                this, &ResumeCtrl::SaveDataOnTimer, true),
    restore_hmi_level_timer_("RsmCtrlRstore",
                             this, &ResumeCtrl::ApplicationResumptiOnTimer),
    is_resumption_active_(false),
    is_data_saved(true),
    launch_time_(time(NULL)) {
  LoadResumeData();
  save_persistent_data_timer_.start(profile::Profile::instance()->app_resumption_save_persistent_data_timeout());
}

void ResumeCtrl::SaveAllApplications() {
  LOG4CXX_AUTO_TRACE(logger_);
  std::set<ApplicationSharedPtr> apps(retrieve_application());
  std::set<ApplicationSharedPtr>::iterator it_begin = apps.begin();
  std::set<ApplicationSharedPtr>::iterator it_end = apps.end();

  resumtion_lock_.Acquire();
  for (; it_begin != it_end; ++it_begin) {
    if (StateApplicationData::kNotSavedDataForResumption ==
        ((*it_begin)->is_application_data_changed())) {
      resumption_storage->SaveApplication();
      (*it_begin)->set_is_application_data_changed(
          StateApplicationData::kSavedDataForResumption);
    }
  }
  resumtion_lock_.Release();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ResumeCtrl::SaveApplication(ApplicationConstSharedPtr application) {
  DCHECK(application.get());

  if (!application) {
    LOG4CXX_FATAL(logger_, "Application object is NULL.");
    return;
  }

  const std::string& m_app_id = application->mobile_app_id();
  LOG4CXX_TRACE(logger_, "ENTER app_id : " << application->app_id()
                << " mobile app_id : " << m_app_id);

  const std::string hash = application->curHash(); // let's make a copy not to depend on application
  const uint32_t grammar_id = application->get_grammar_id();
  const uint32_t time_stamp = (uint32_t)time(NULL);

  const mobile_apis::HMILevel::eType hmi_level = application->hmi_level();

  resumtion_lock_.Acquire();
  Json::Value& json_app = GetFromSavedOrAppend(m_app_id);

  json_app[strings::device_id] =
    MessageHelper::GetDeviceMacAddressForHandle(application->device());
  json_app[strings::app_id] = m_app_id;
  json_app[strings::grammar_id] = grammar_id;
  json_app[strings::connection_key] = application->app_id();
  json_app[strings::hmi_app_id] = application->hmi_app_id();
  json_app[strings::is_media_application] = application->IsAudioApplication();
  json_app[strings::hmi_level] = static_cast<int32_t> (hmi_level);
  json_app[strings::ign_off_count] = 0;
  json_app[strings::suspend_count] = 0;
  json_app[strings::hash_id] = hash;
  json_app[strings::application_commands] =
    GetApplicationCommands(application);
  json_app[strings::application_submenus] =
    GetApplicationSubMenus(application);
  json_app[strings::application_choice_sets] =
    GetApplicationInteractionChoiseSets(application);
  json_app[strings::application_global_properties] =
    GetApplicationGlobalProperties(application);
  json_app[strings::application_subscribtions] =
    GetApplicationSubscriptions(application);
  json_app[strings::application_files] = GetApplicationFiles(application);
  json_app[strings::time_stamp] = time_stamp;
  LOG4CXX_DEBUG(logger_, "SaveApplication : " << json_app.toStyledString());

  resumtion_lock_.Release();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ResumeCtrl::on_event(const event_engine::Event& event) {
  LOG4CXX_TRACE(logger_, "Response from HMI command");
}


bool ResumeCtrl::RestoreAppHMIState(ApplicationSharedPtr application) {
  LOG4CXX_AUTO_TRACE(logger_);

  using namespace mobile_apis;
  if (!application) {
    LOG4CXX_ERROR(logger_, " RestoreApplicationHMILevel() application pointer in invalid");
    return false;
  }
  LOG4CXX_DEBUG(logger_, "ENTER app_id : " << application->app_id());

  const HMILevel::eType saved_hmi_level = static_cast<mobile_apis::HMILevel::eType>(
      resumption_storage->GetStoredHMILevel(application->mobile_app_id(),
       MessageHelper::GetDeviceMacAddressForHandle(application->device())));

  if (mobile_apis::HMILevel::INVALID_ENUM != saved_hmi_level) {
    LOG4CXX_DEBUG(logger_, "Saved HMI Level is : " << saved_hmi_level);
    return SetAppHMIState(application, saved_hmi_level);
  }
  LOG4CXX_INFO(logger_, "Failed to restore application HMILevel");
  return false;
}

bool ResumeCtrl::SetupDefaultHMILevel(ApplicationSharedPtr application) {
  DCHECK_OR_RETURN(application, false);
  LOG4CXX_AUTO_TRACE(logger_);
  mobile_apis::HMILevel::eType default_hmi =
      ApplicationManagerImpl::instance()-> GetDefaultHmiLevel(application);
  bool result = SetAppHMIState(application, default_hmi, false);
  return result;
}

bool ResumeCtrl::SetAppHMIState(ApplicationSharedPtr application,
                               const mobile_apis::HMILevel::eType hmi_level,
                               bool check_policy) {
  using namespace mobile_apis;
  LOG4CXX_AUTO_TRACE(logger_);
  if (false == application.valid()) {
    LOG4CXX_ERROR(logger_, "Application pointer in invalid");
    return false;
  }
  LOG4CXX_TRACE(logger_, " app_id : ( " << application->app_id()
                << ", hmi_level : " << hmi_level
                << ", check_policy : " << check_policy << " )");
  const std::string device_id =
      MessageHelper::GetDeviceMacAddressForHandle(application->device());

  if (check_policy &&
      policy::PolicyHandler::instance()->GetUserConsentForDevice(device_id)
      != policy::DeviceConsent::kDeviceAllowed) {
    LOG4CXX_ERROR(logger_, "Resumption abort. Data consent wasn't allowed");
    SetupDefaultHMILevel(application);
    return false;
  }
  HMILevel::eType restored_hmi_level = hmi_level;

  if ((hmi_level == application->hmi_level()) &&
      (hmi_level != mobile_apis::HMILevel::HMI_NONE)) {
    LOG4CXX_DEBUG(logger_, "Hmi level " << hmi_level << " should not be set to "
                 << application->mobile_app_id()
                 <<" current hmi_level is " << application->hmi_level());
    return false;
  }

  if (HMILevel::HMI_FULL == hmi_level) {
    restored_hmi_level = app_mngr_->IsHmiLevelFullAllowed(application);
  } else if (HMILevel::HMI_LIMITED == hmi_level) {
    bool allowed_limited = true;
    ApplicationManagerImpl::ApplicationListAccessor accessor;
    ApplicationManagerImpl::ApplictionSetConstIt it = accessor.begin();
    for (; accessor.end() != it && allowed_limited; ++it) {
      const ApplicationSharedPtr curr_app = *it;
      if (curr_app->is_media_application()) {
        if (curr_app->hmi_level() == HMILevel::HMI_FULL ||
            curr_app->hmi_level() == HMILevel::HMI_LIMITED) {
          allowed_limited = false;
        }
      }
    }
    if (allowed_limited) {
      restored_hmi_level = HMILevel::HMI_LIMITED;
    } else {
      restored_hmi_level =
          ApplicationManagerImpl::instance()->GetDefaultHmiLevel(application);
    }
  }
  if (HMILevel::HMI_LIMITED == restored_hmi_level) {
    MessageHelper::SendOnResumeAudioSourceToHMI(application->app_id());
  }

  const AudioStreamingState::eType restored_audio_state =
      HMILevel::HMI_FULL == restored_hmi_level ||
      HMILevel::HMI_LIMITED == restored_hmi_level ? AudioStreamingState::AUDIBLE:
                                                    AudioStreamingState::NOT_AUDIBLE;

  if (restored_hmi_level == HMILevel::HMI_FULL) {
    ApplicationManagerImpl::instance()->SetState<true>(application->app_id(),
                                                       restored_hmi_level,
                                                       restored_audio_state);
  } else {
    ApplicationManagerImpl::instance()->SetState<false>(application->app_id(),
                                                        restored_hmi_level,
                                                        restored_audio_state);
  }
  LOG4CXX_INFO(logger_, "Set up application "
               << application->mobile_app_id()
               << " to HMILevel " << hmi_level);
  return true;
}


bool ResumeCtrl::RestoreApplicationData(ApplicationSharedPtr application) {
  LOG4CXX_AUTO_TRACE(logger_);

  sync_primitives::AutoLock lock(resumtion_lock_);
  const bool result = resumption_storage->RestoreApplicationData(application);
  if (result) {
    ProcessHMIRequests(MessageHelper::CreateAddSubMenuRequestToHMI(application));
    ProcessHMIRequests(MessageHelper::CreateAddCommandRequestToHMI(application));
    ProcessHMIRequests(
        MessageHelper::CreateAddVRCommandRequestFromChoiceToHMI(application));
    MessageHelper::SendGlobalPropertiesToHMI(application);
    ProcessHMIRequests(MessageHelper::GetIVISubscriptionRequests(application));
  }
  return result;
}

bool ResumeCtrl::IsHMIApplicationIdExist(uint32_t hmi_app_id) {
  LOG4CXX_TRACE(logger_, "ENTER hmi_app_id :"  << hmi_app_id);

  sync_primitives::AutoLock lock(resumtion_lock_);
  if (resumption_storage->IsHMIApplicationIdExist(hmi_app_id)) {
    return true;
  }

  ApplicationManagerImpl::ApplicationListAccessor accessor;
  ApplicationManagerImpl::ApplictionSet apps(accessor.applications());
  ApplicationManagerImpl::ApplictionSetIt it = apps.begin();
  ApplicationManagerImpl::ApplictionSetIt it_end = apps.end();

  for (;it != it_end; ++it) {
    if (hmi_app_id == (*it)->hmi_app_id()) {
      LOG4CXX_TRACE(logger_, "EXIT result = true");
      return true;
    }
  }
  LOG4CXX_TRACE(logger_, "EXIT result = false");
  return false;
}

bool ResumeCtrl::IsApplicationSaved(const std::string& mobile_app_id,
                                    const std::string& device_id) {
  LOG4CXX_TRACE(logger_, "ENTER mobile_app_id :"  << mobile_app_id);

  sync_primitives::AutoLock lock(resumtion_lock_);
  return resumption_storage->CheckSavedApplication(mobile_app_id, device_id);
}

uint32_t ResumeCtrl::GetHMIApplicationID(const std::string& mobile_app_id,
                                         const std::string& device_id) {
  LOG4CXX_AUTO_TRACE(logger_);

  sync_primitives::AutoLock lock(resumtion_lock_);
  return resumption_storage->GetHMIApplicationID(mobile_app_id, device_id);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ResumeCtrl::RemoveApplicationFromSaved(const std::string& mobile_app_id) {
  LOG4CXX_TRACE(logger_, "Remove mobile_app_id " << mobile_app_id);
  sync_primitives::AutoLock lock(resumtion_lock_);
  bool result = false;
  std::vector<Json::Value> temp;
  for (Json::Value::iterator it = GetSavedApplications().begin();
      it != GetSavedApplications().end(); ++it) {
    if ((*it).isMember(strings::app_id)) {
      const std::string& saved_m_app_id = (*it)[strings::app_id].asString();

      if (saved_m_app_id != mobile_app_id) {
        temp.push_back((*it));
      } else {
        result = true;
      }
    }
  }

  if (false == result) {
    LOG4CXX_TRACE(logger_, "EXIT result: " << (result ? "true" : "false"));
    return result;
  }

  GetSavedApplications().clear();
  for (std::vector<Json::Value>::iterator it = temp.begin();
      it != temp.end(); ++it) {
    GetSavedApplications().append((*it));
  }
  LOG4CXX_TRACE(logger_, "EXIT result: " << (result ? "true" : "false"));
  return result;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ResumeCtrl::Suspend() {
  LOG4CXX_AUTO_TRACE(logger_);

  StopSavePersistentDataTimer();
  SaveAllApplications();
  sync_primitives::AutoLock lock(resumtion_lock_);
  resumption_storage->Suspend();
}

void ResumeCtrl::OnAwake() {
  LOG4CXX_AUTO_TRACE(logger_);

  sync_primitives::AutoLock lock(resumtion_lock_);
  resumption_storage->OnAwake();
  ResetLaunchTime();
  StartSavePersistentDataTimer();
}

void ResumeCtrl::OnAppActivated(ApplicationSharedPtr application) {
  if (is_resumption_active_) {
    RemoveFromResumption(application->app_id());
  }
}

void ResumeCtrl::StartSavePersistentDataTimer() {
  LOG4CXX_AUTO_TRACE(logger_);
  if (!save_persistent_data_timer_.isRunning()) {
    save_persistent_data_timer_.start(
        profile::Profile::instance()->app_resumption_save_persistent_data_timeout());
  }
}

void ResumeCtrl::StopSavePersistentDataTimer() {
  LOG4CXX_AUTO_TRACE(logger_);
  if (save_persistent_data_timer_.isRunning()) {
    save_persistent_data_timer_.stop();
  }
}


bool ResumeCtrl::StartResumption(ApplicationSharedPtr application,
                                 const std::string& hash) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (!application) {
    LOG4CXX_WARN(logger_, "Application not exist");
    return false;
  }

  SetupDefaultHMILevel(application);

  LOG4CXX_DEBUG(logger_, " Resume app_id = " << application->app_id()
                        << " hmi_app_id = " << application->hmi_app_id()
                        << " mobile_id = " << application->mobile_app_id()
                        << "received hash = " << hash);

  sync_primitives::AutoLock lock(resumtion_lock_);
  std::string saved_hash;
  bool result = resumption_storage->GetHashId(
      application->mobile_app_id(),
      MessageHelper::GetDeviceMacAddressForHandle(application->device()),
      saved_hash);

  if (!result) {
    return false;
  }

  if (saved_hash == hash) {
    RestoreApplicationData(application);
  }
  application->UpdateHash();

  sync_primitives::AutoUnlock unlock(lock);
  AddToResumption(application->app_id());

  queue_lock_.Acquire();
  waiting_for_timer_.push_back(application->app_id());
  queue_lock_.Release();
  if (!is_resumption_active_) {
    is_resumption_active_ = true;
    restore_hmi_level_timer_.start(
        profile::Profile::instance()->app_resuming_timeout());
  }
  return true;
}

void ResumeCtrl::StartAppHmiStateResumption(ApplicationSharedPtr application) {
  LOG4CXX_AUTO_TRACE(logger_);
  using namespace profile;
  using namespace date_time;
  DCHECK_OR_RETURN_VOID(application);
  smart_objects::SmartObject saved_app(smart_objects::SmartType_Map);
  sync_primitives::AutoLock lock(resumtion_lock_);
  bool result = resumption_storage->GetSavedApplication(
      application->mobile_app_id(),
      MessageHelper::GetDeviceMacAddressForHandle(application->device()),
      saved_app);
  DCHECK_OR_RETURN_VOID(result);

  if (!saved_app.keyExists(strings::ign_off_count)) {
    LOG4CXX_INFO(logger_, "Do not need to resume application "
                 << application->app_id());
    SetupDefaultHMILevel(application);
    return;
  }

  // check if is resumption during one IGN cycle
  const uint32_t ign_off_count = saved_app[strings::ign_off_count].asUInt();
  const std::string m_app_id = application->mobile_app_id();
  const std::string device_id =
      MessageHelper::GetDeviceMacAddressForHandle(application->device());

  if (0 == ign_off_count) {
    if (CheckAppRestrictions(application, saved_app)) {
      LOG4CXX_INFO(logger_, "Resume application after short IGN cycle");
      RestoreAppHMIState(application);
      resumption_storage->RemoveApplicationFromSaved(m_app_id, device_id);
    } else {
      LOG4CXX_INFO(logger_, "Do not need to resume application "
                   << application->app_id());
      }
  } else {
    if (CheckIgnCycleRestrictions(saved_app) &&
        CheckAppRestrictions(application, saved_app)) {
      LOG4CXX_INFO(logger_, "Resume application after IGN cycle");
      RestoreAppHMIState(application);
      resumption_storage->RemoveApplicationFromSaved(m_app_id, device_id);
    } else {
      LOG4CXX_INFO(logger_, "Do not need to resume application "
                   << application->app_id());
    }
  }
}

std::set<ApplicationSharedPtr> ResumeCtrl::retrieve_application() {
  ApplicationManagerImpl::ApplicationListAccessor accessor;
  return std::set<ApplicationSharedPtr>(accessor.begin(), accessor.end());
}

bool ResumeCtrl::StartResumptionOnlyHMILevel(ApplicationSharedPtr application) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (!application.valid()) {
    LOG4CXX_WARN(logger_, "Application do not exists");
    return false;
  }

  SetupDefaultHMILevel(application);

  LOG4CXX_DEBUG(logger_, "ENTER app_id = " << application->app_id()
                        << "mobile_id = "
                        << application->mobile_app_id());

  sync_primitives::AutoLock lock(resumtion_lock_);
  if (-1 == (resumption_storage->IsApplicationSaved(
      application->mobile_app_id,
      MessageHelper::GetDeviceMacAddressForHandle(application->device())))) {
    LOG4CXX_WARN(logger_, "Application not saved");
    return false;
  }

  sync_primitives::AutoUnlock unlock(lock);
  AddToResumption(application->app_id());

  return true;
}

bool ResumeCtrl::CheckPersistenceFilesForResumption(ApplicationSharedPtr application) {
  LOG4CXX_AUTO_TRACE(logger_);

  if (!application.valid()) {
    LOG4CXX_WARN(logger_, "Application do not exists");
    return false;
  }
  LOG4CXX_DEBUG(logger_, "Process app_id = " << application->app_id());

  smart_objects::SmartObject saved_app(smart_objects::SmartType_Map);
  sync_primitives::AutoLock lock(resumtion_lock_);
  bool result = resumption_storage->GetSavedApplication(
      application->mobile_app_id(),
      MessageHelper::GetDeviceMacAddressForHandle(application->device()),
      saved_app);

  if (!result) {
    LOG4CXX_WARN(logger_, "Application not saved");
    return false;
  }

  if (!saved_app.keyExists(strings::application_commands) ||
      !saved_app.keyExists(strings::application_choice_sets)) {
    LOG4CXX_WARN(logger_, "application_commands or "
                 "application_choice_sets are not exists");
    return false;
  }

  if (!CheckIcons(application, saved_app[strings::application_commands])) {
    return false;
  }
  if (!CheckIcons(application, saved_app[strings::application_choice_sets])) {
    return false;
  }
  LOG4CXX_DEBUG(logger_, " result = true");
  return true;
}

bool ResumeCtrl::CheckApplicationHash(ApplicationSharedPtr application,
                                      const std::string& hash) {
  if (!application) {
    LOG4CXX_ERROR(logger_, "Application pointer is invalid");
    return false;
  }

  LOG4CXX_DEBUG(logger_, "ENTER app_id : " << application->app_id()
                << " hash : " << hash);

  sync_primitives::AutoLock lock(resumtion_lock_);
  std::string saved_hash;
  bool result = resumption_storage->GetHashId(
      application->mobile_app_id(),
      MessageHelper::GetDeviceMacAddressForHandle(application->device()),
      saved_hash);

  const int idx = GetObjectIndex(application->mobile_app_id());
  if (!result) {
    return false;
  }

  const Json::Value& json_app = GetSavedApplications()[idx];
  LOG4CXX_INFO(logger_, "received hash = " << hash);
  LOG4CXX_INFO(logger_, "saved hash = " << saved_hash);
  if (hash == saved_hash) {
    return true;
  }
  return false;
}

void ResumeCtrl::SaveDataOnTimer() {
  LOG4CXX_AUTO_TRACE(logger_);
  if (is_resumption_active_) {
    LOG4CXX_WARN(logger_, "Resumption timer is active skip saving");
    return;
  }

  if (false == is_data_saved) {
    SaveAllApplications();
    is_data_saved = true;
    resumption::LastState::instance()->SaveToFileSystem();
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Json::Value&ResumeCtrl::GetResumptionData() {
  LOG4CXX_AUTO_TRACE(logger_);
  Json::Value& last_state = resumption::LastState::instance()->dictionary;
  if (!last_state.isMember(strings::resumption)) {
    last_state[strings::resumption] = Json::Value(Json::objectValue);
    LOG4CXX_WARN(logger_, "resumption section is missed");
  }
  Json::Value& resumption =  last_state[strings::resumption];
  if (!resumption.isObject()) {
    LOG4CXX_ERROR(logger_, "resumption type INVALID rewrite");
    resumption =  Json::Value(Json::objectValue);
  }
  return resumption;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Json::Value& ResumeCtrl::GetSavedApplications() {
  LOG4CXX_AUTO_TRACE(logger_);
  Json::Value& resumption = GetResumptionData();
  if (!resumption.isMember(strings::resume_app_list)) {
    resumption[strings::resume_app_list] = Json::Value(Json::arrayValue);
    LOG4CXX_WARN(logger_, "app_list section is missed");
  }
  Json::Value& resume_app_list = resumption[strings::resume_app_list];
  if (!resume_app_list.isArray()) {
    LOG4CXX_ERROR(logger_, "resume_app_list type INVALID rewrite");
    resume_app_list =  Json::Value(Json::arrayValue);
  }
  return resume_app_list;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
time_t ResumeCtrl::GetIgnOffTime() {
  LOG4CXX_AUTO_TRACE(logger_);
  Json::Value& resumption = GetResumptionData();
  if (!resumption.isMember(strings::last_ign_off_time)) {
    resumption[strings::last_ign_off_time] = 0;
    LOG4CXX_WARN(logger_, "last_save_time section is missed");
  }
  time_t last_ign_off = static_cast<time_t>(
                           resumption[strings::last_ign_off_time].asUInt());
  return last_ign_off;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ResumeCtrl::SetLastIgnOffTime(time_t ign_off_time) {
  LOG4CXX_AUTO_TRACE(logger_);
  LOG4CXX_WARN(logger_, "ign_off_time = " << ign_off_time);
  Json::Value& resumption = GetResumptionData();
  resumption[strings::last_ign_off_time] = static_cast<uint32_t>(ign_off_time);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ResumeCtrl::SetSavedApplication(Json::Value& apps_json) {
  Json::Value& app_list = GetSavedApplications();
  app_list = apps_json;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Json::Value ResumeCtrl::GetApplicationCommands(
    ApplicationConstSharedPtr application) {
  LOG4CXX_AUTO_TRACE(logger_);
  Json::Value result;
  DCHECK(application.get());
  if (!application) {
    LOG4CXX_ERROR(logger_, "NULL Pointer App");
    return result;
  }
  const DataAccessor<CommandsMap> accessor = application->commands_map();
  const CommandsMap& commands = accessor.GetData();
  CommandsMap::const_iterator it = commands.begin();
  for (;it != commands.end(); ++it) {
    smart_objects::SmartObject* so = it->second;
    Json::Value curr;
    Formatters::CFormatterJsonBase::objToJsonValue(*so, curr);
    result.append(curr);
  }
  return result;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Json::Value ResumeCtrl::GetApplicationSubMenus(
    ApplicationConstSharedPtr application) {
  LOG4CXX_AUTO_TRACE(logger_);
  Json::Value result;
  DCHECK(application.get());
  if (!application) {
    LOG4CXX_ERROR(logger_, "NULL Pointer App");
    return result;
  }
  const DataAccessor<SubMenuMap> accessor = application->sub_menu_map();
  const SubMenuMap& sub_menus = accessor.GetData();
  SubMenuMap::const_iterator it = sub_menus.begin();
  for (;it != sub_menus.end(); ++it) {
    smart_objects::SmartObject* so = it->second;
    Json::Value curr;
    Formatters::CFormatterJsonBase::objToJsonValue(*so, curr);
    result.append(curr);
  }
  return result;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Json::Value ResumeCtrl::GetApplicationInteractionChoiseSets(
    ApplicationConstSharedPtr application) {
  DCHECK(application.get());
  LOG4CXX_TRACE(logger_, "ENTER app_id:"
               << application->app_id());

  Json::Value result;
  const DataAccessor<ChoiceSetMap> accessor = application->choice_set_map();
  const ChoiceSetMap& choices = accessor.GetData();
  ChoiceSetMap::const_iterator it = choices.begin();
  for ( ;it != choices.end(); ++it) {
    smart_objects::SmartObject* so = it->second;
    Json::Value curr;
    Formatters::CFormatterJsonBase::objToJsonValue(*so, curr);
    result.append(curr);
  }
  return result;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Json::Value ResumeCtrl::GetApplicationGlobalProperties(
    ApplicationConstSharedPtr application) {
  LOG4CXX_AUTO_TRACE(logger_);
  Json::Value sgp;
  DCHECK(application.get());
  if (!application) {
    LOG4CXX_ERROR(logger_, "NULL Pointer App");
    return sgp;
  }

  const smart_objects::SmartObject* help_promt = application->help_prompt();
  const smart_objects::SmartObject* timeout_prompt = application->timeout_prompt();
  const smart_objects::SmartObject* vr_help = application->vr_help();
  const smart_objects::SmartObject* vr_help_title = application->vr_help_title();
  const smart_objects::SmartObject* vr_synonyms = application->vr_synonyms();
  const smart_objects::SmartObject* keyboard_props = application->keyboard_props();
  const smart_objects::SmartObject* menu_title = application->menu_title();
  const smart_objects::SmartObject* menu_icon = application->menu_icon();

  sgp[strings::help_prompt] = JsonFromSO(help_promt);
  sgp[strings::timeout_prompt] = JsonFromSO(timeout_prompt);
  sgp[strings::vr_help] = JsonFromSO(vr_help);
  sgp[strings::vr_help_title] = JsonFromSO(vr_help_title);
  sgp[strings::vr_synonyms] = JsonFromSO(vr_synonyms);
  sgp[strings::keyboard_properties] = JsonFromSO(keyboard_props);
  sgp[strings::menu_title] = JsonFromSO(menu_title);
  sgp[strings::menu_icon] = JsonFromSO(menu_icon);
  return sgp;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Json::Value ResumeCtrl::GetApplicationSubscriptions(
    ApplicationConstSharedPtr application) {
  LOG4CXX_AUTO_TRACE(logger_);
  Json::Value result;
  DCHECK(application.get());
  if (!application) {
    LOG4CXX_ERROR(logger_, "NULL Pointer App");
    return result;
  }
  LOG4CXX_DEBUG(logger_, "app_id:" << application->app_id());
  LOG4CXX_DEBUG(logger_, "SubscribedButtons:" << application->SubscribedButtons().size());
  Append(application->SubscribedButtons().begin(),
         application->SubscribedButtons().end(),
         strings::application_buttons, result);
  LOG4CXX_DEBUG(logger_, "SubscribesIVI:" << application->SubscribesIVI().size());
  Append(application->SubscribesIVI().begin(),
         application->SubscribesIVI().end(),
         strings::application_vehicle_info, result);
  return result;
}

Json::Value ResumeCtrl::GetApplicationFiles(
    ApplicationConstSharedPtr application) {
  DCHECK(application.get());
  LOG4CXX_TRACE(logger_, "ENTER app_id:"
               << application->app_id());

  Json::Value result;
  const AppFilesMap& app_files = application->getAppFiles();
  for(AppFilesMap::const_iterator file_it = app_files.begin();
      file_it != app_files.end(); file_it++) {
    const AppFile& file = file_it->second;
    if (file.is_persistent) {
      Json::Value file_data;
      file_data[strings::persistent_file] = file.is_persistent;
      file_data[strings::is_download_complete] = file.is_download_complete;
      file_data[strings::sync_file_name] = file.file_name;
      file_data[strings::file_type] = file.file_type;
      result.append(file_data);
    }
  }
  return result;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ResumeCtrl::ProcessHMIRequest(smart_objects::SmartObjectSPtr request,
                                   bool use_events) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (use_events) {
    const hmi_apis::FunctionID::eType function_id =
        static_cast<hmi_apis::FunctionID::eType>(
            (*request)[strings::function_id].asInt());

    const int32_t hmi_correlation_id =
        (*request)[strings::correlation_id].asInt();
    subscribe_on_event(function_id, hmi_correlation_id);
  }
  if (!ApplicationManagerImpl::instance()->ManageHMICommand(request)) {
    LOG4CXX_ERROR(logger_, "Unable to send request");
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ResumeCtrl::AddFiles(ApplicationSharedPtr application, const Json::Value& saved_app) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (saved_app.isMember(strings::application_files)) {
    const Json::Value& application_files = saved_app[strings::application_files];
    for (Json::Value::iterator json_it = application_files.begin();
        json_it != application_files.end(); ++json_it)  {
      const Json::Value& file_data = *json_it;

      const bool is_persistent = file_data.isMember(strings::persistent_file) &&
          file_data[strings::persistent_file].asBool();
      if (is_persistent) {
        AppFile file;
        file.is_persistent = is_persistent;
        file.is_download_complete = file_data[strings::is_download_complete].asBool();
        file.file_name = file_data[strings::sync_file_name].asString();
        file.file_type = static_cast<mobile_apis::FileType::eType> (
                           file_data[strings::file_type].asInt());
        application->AddFile(file);
      }
    }
  } else {
    LOG4CXX_FATAL(logger_, "application_files section is not exists");
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ResumeCtrl::AddSubmenues(ApplicationSharedPtr application, const Json::Value& saved_app) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (saved_app.isMember(strings::application_submenus)) {
    const Json::Value& app_submenus = saved_app[strings::application_submenus];
    for (Json::Value::iterator json_it = app_submenus.begin();
        json_it != app_submenus.end(); ++json_it) {
      const Json::Value& json_submenu = *json_it;
      smart_objects::SmartObject message(smart_objects::SmartType::SmartType_Map);
      Formatters::CFormatterJsonBase::jsonValueToObj(json_submenu, message);
      application->AddSubMenu(message[strings::menu_id].asUInt(), message);
    }

    ProcessHMIRequests(MessageHelper::CreateAddSubMenuRequestToHMI(application));
  } else {
    LOG4CXX_FATAL(logger_, "application_submenus section is not exists");
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ResumeCtrl::AddCommands(ApplicationSharedPtr application, const Json::Value& saved_app) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (saved_app.isMember(strings::application_commands)) {
    const Json::Value& app_commands = saved_app[strings::application_commands];
    for (Json::Value::iterator json_it = app_commands.begin();
        json_it != app_commands.end(); ++json_it)  {
      const Json::Value& json_command = *json_it;
      smart_objects::SmartObject message(smart_objects::SmartType::SmartType_Map);
      Formatters::CFormatterJsonBase::jsonValueToObj(json_command, message);
      application->AddCommand(message[strings::cmd_id].asUInt(), message);
    }

    ProcessHMIRequests(MessageHelper::CreateAddCommandRequestToHMI(application));
  } else {
     LOG4CXX_FATAL(logger_, "application_commands section is not exists");
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ResumeCtrl::AddChoicesets(ApplicationSharedPtr application, const Json::Value& saved_app) {
  if(saved_app.isMember(strings::application_choice_sets)) {
    const Json::Value& app_choise_sets = saved_app[strings::application_choice_sets];
    for (Json::Value::iterator json_it = app_choise_sets.begin();
        json_it != app_choise_sets.end(); ++json_it)  {
      const Json::Value& json_choiset = *json_it;
      smart_objects::SmartObject msg_param(smart_objects::SmartType::SmartType_Map);
      Formatters::CFormatterJsonBase::jsonValueToObj(json_choiset , msg_param);
      const int32_t choice_set_id = msg_param
          [strings::interaction_choice_set_id].asInt();
      uint32_t choice_grammar_id = msg_param[strings::grammar_id].asUInt();
      application->AddChoiceSet(choice_set_id, msg_param);

      const size_t size = msg_param[strings::choice_set].length();
      for (size_t j = 0; j < size; ++j) {
        smart_objects::SmartObject choise_params(smart_objects::SmartType_Map);
        choise_params[strings::app_id] = application->app_id();
        choise_params[strings::cmd_id] =
            msg_param[strings::choice_set][j][strings::choice_id];
        choise_params[strings::vr_commands] = smart_objects::SmartObject(
                                             smart_objects::SmartType_Array);
        choise_params[strings::vr_commands] =
            msg_param[strings::choice_set][j][strings::vr_commands];

        choise_params[strings::type] = hmi_apis::Common_VRCommandType::Choice;
        choise_params[strings::grammar_id] =  choice_grammar_id;
        SendHMIRequest(hmi_apis::FunctionID::VR_AddCommand, &choise_params);
      }
    }
  } else {
    LOG4CXX_FATAL(logger_, "There is no any choicesets");
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ResumeCtrl::SetGlobalProperties(ApplicationSharedPtr application, const Json::Value& saved_app) {
  const Json::Value& global_properties = saved_app[strings::application_global_properties];
  if (!global_properties.isNull()) {
    smart_objects::SmartObject properties_so(smart_objects::SmartType::SmartType_Map);
    Formatters::CFormatterJsonBase::jsonValueToObj(global_properties , properties_so);
    application->load_global_properties(properties_so);
    MessageHelper::SendGlobalPropertiesToHMI(application);
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ResumeCtrl::AddSubscriptions(ApplicationSharedPtr application, const Json::Value& saved_app) {
  if (saved_app.isMember(strings::application_subscribtions)) {
    const Json::Value& subscribtions = saved_app[strings::application_subscribtions];

    if (subscribtions.isMember(strings::application_buttons)) {
      const Json::Value& subscribtions_buttons = subscribtions[strings::application_buttons];
      mobile_apis::ButtonName::eType btn;
      for (Json::Value::iterator json_it = subscribtions_buttons.begin();
           json_it != subscribtions_buttons.end(); ++json_it) {
        btn = static_cast<mobile_apis::ButtonName::eType>((*json_it).asInt());
        application->SubscribeToButton(btn);
      }
    }
    if (subscribtions.isMember(strings::application_vehicle_info)) {
      const Json::Value& subscribtions_ivi= subscribtions[strings::application_vehicle_info];
      VehicleDataType ivi;
      for (Json::Value::iterator json_it = subscribtions_ivi.begin();
           json_it != subscribtions_ivi.end(); ++json_it) {
        ivi = static_cast<VehicleDataType>((*json_it).asInt());
        application->SubscribeToIVI(ivi);
      }
    }

    ProcessHMIRequests(MessageHelper::GetIVISubscriptionRequests(application));
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ResumeCtrl::ProcessHMIRequests(const smart_objects::SmartObjectList& requests) {
  for (smart_objects::SmartObjectList::const_iterator it = requests.begin(),
       total = requests.end();
       it != total; ++it) {
    ProcessHMIRequest(*it, true);
  }
}

bool ResumeCtrl::CheckIcons(ApplicationSharedPtr application,
                            const smart_objects::SmartObject& smart_obj) {
  LOG4CXX_AUTO_TRACE(logger_);
  bool result = true;
  if (smart_objects::SmartType_Null != smart_obj.getType()) {
    size_t length_obj = smart_obj.length();
    for (size_t i = 0; i < length_obj && result; ++i) {
      const smart_objects::SmartObject& so_command = smart_obj[i];
      if (smart_objects::SmartType_Null != so_command.getType()) {
        const mobile_apis::Result::eType verify_images =
            MessageHelper::VerifyImageFiles(message, application);
        result = (mobile_apis::Result::INVALID_DATA != verify_images);
      } else {
        LOG4CXX_WARN(logger_, "Invalid smart object");
      }
    }
  } else {
        LOG4CXX_WARN(logger_, "Passed smart object is null");
  }
  LOG4CXX_DEBUG(logger_, "CheckIcons result " << result);
  return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Json::Value& ResumeCtrl::GetFromSavedOrAppend(const std::string& mobile_app_id) {
  LOG4CXX_AUTO_TRACE(logger_);
  for (Json::Value::iterator it = GetSavedApplications().begin();
      it != GetSavedApplications().end(); ++it) {
    if (mobile_app_id == (*it)[strings::app_id].asString()) {
      return *it;
    }
  }

  return GetSavedApplications().append(Json::Value());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ResumeCtrl::CheckIgnCycleRestrictions(
    const smart_objects::SmartObject& saved_app) {
  LOG4CXX_AUTO_TRACE(logger_);
  bool result = true;

  if (!CheckDelayAfterIgnOn()) {
    LOG4CXX_INFO(logger_, "Application was connected long after ign on");
    result = false;
  }

  if (!DisconnectedJustBeforeIgnOff(saved_app)) {
    LOG4CXX_INFO(logger_, "Application was dissconnected long before ign off");
    result = false;
  }
  return result;
}

bool ResumeCtrl::DisconnectedJustBeforeIgnOff(const smart_objects::SmartObject& saved_app) {
  using namespace date_time;
  using namespace profile;
  LOG4CXX_AUTO_TRACE(logger_);
  DCHECK_OR_RETURN(saved_app.keyExists(strings::time_stamp), false);

  const time_t time_stamp =
      static_cast<time_t>(saved_app[strings::time_stamp].asUInt());
  time_t ign_off_time =
      static_cast<time_t>(resumption_storage->GetIgnOffTime());
  const uint32_t sec_spent_before_ign = labs(ign_off_time - time_stamp);
  LOG4CXX_DEBUG(logger_,"ign_off_time " << ign_off_time
                << "; app_disconnect_time " << time_stamp
                << "; sec_spent_before_ign " << sec_spent_before_ign
                << "; resumption_delay_before_ign " <<
                Profile::instance()->resumption_delay_before_ign());
  return sec_spent_before_ign <=
      Profile::instance()->resumption_delay_before_ign();
}

bool ResumeCtrl::CheckDelayAfterIgnOn() {
  using namespace date_time;
  using namespace profile;
  LOG4CXX_AUTO_TRACE(logger_);
  time_t curr_time = time(NULL);
  time_t sdl_launch_time = launch_time();
  const uint32_t seconds_from_sdl_start = labs(curr_time - sdl_launch_time);
  const uint32_t wait_time =
      Profile::instance()->resumption_delay_after_ign();
  LOG4CXX_DEBUG(logger_, "curr_time " << curr_time
               << "; sdl_launch_time " << sdl_launch_time
               << "; seconds_from_sdl_start " << seconds_from_sdl_start
               << "; wait_time " << wait_time);
  return seconds_from_sdl_start <= wait_time;
}

bool ResumeCtrl::CheckAppRestrictions(ApplicationSharedPtr application,
                                      const smart_objects::SmartObject& saved_app) {
  using namespace mobile_apis;
  LOG4CXX_AUTO_TRACE(logger_);
  DCHECK_OR_RETURN(saved_app.keyExists(strings::hmi_level), false);

  const bool is_media_app = application->is_media_application();
  const HMILevel::eType hmi_level =
      static_cast<HMILevel::eType>(saved_app[strings::hmi_level].asInt());
  LOG4CXX_DEBUG(logger_, "is_media_app " << is_media_app
               << "; hmi_level " << hmi_level);

  if (hmi_level == HMILevel::HMI_FULL ||
      hmi_level == HMILevel::HMI_LIMITED) {
    return true;
  }

  return false;
}
/////////////////////////////////////////////////////////////////////////////////////////////
int ResumeCtrl::GetObjectIndex(const std::string& mobile_app_id) {
  LOG4CXX_AUTO_TRACE(logger_);

  sync_primitives::AutoLock lock(resumtion_lock_);
  const Json::Value& apps = GetSavedApplications();
  const Json::ArrayIndex size = apps.size();
  Json::ArrayIndex idx = 0;
  for (; idx != size; ++idx) {
    const std::string& saved_app_id = apps[idx][strings::app_id].asString();
    if (mobile_app_id == saved_app_id) {
      LOG4CXX_DEBUG(logger_, "Found " << idx);
      return idx;
    }
  }
  return -1;
}
/////////////////////////////////////////////////////////////////////////////////////////////
time_t ResumeCtrl::launch_time() const {
  return launch_time_;
}

void ResumeCtrl::ResetLaunchTime() {
  launch_time_ = time(NULL);
}

void ResumeCtrl::ApplicationResumptiOnTimer() {
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock auto_lock(queue_lock_);
  std::vector<uint32_t>::iterator it = waiting_for_timer_.begin();

  for (; it != waiting_for_timer_.end(); ++it) {
    ApplicationSharedPtr app =
        ApplicationManagerImpl::instance()->application(*it);
    if (!app.get()) {
      LOG4CXX_ERROR(logger_, "Invalid app_id = " << *it);
      continue;
    }

    StartAppHmiStateResumption(app);
   }

  is_resumption_active_ = false;
  waiting_for_timer_.clear();
}

void ResumeCtrl::LoadResumeData() {
  LOG4CXX_AUTO_TRACE(logger_);

  sync_primitives::AutoLock lock(resumtion_lock_);

  smart_objects::SmartObject so_applications_data;
  resumption_storage->GetDataForLoadResumeData(so_applications_data);
  size_t length = so_applications_data.length();
  smart_objects::SmartObject* full_app = NULL;
  smart_objects::SmartObject* limited_app = NULL;

  time_t time_stamp_full = 0;
  time_t time_stamp_limited = 0;

  // only apps with first IGN should be resumed
  const int32_t first_ign = 1;

  for (size_t i = 0; i < length; ++i) {

    if (first_ign == so_applications_data[i][strings::ign_off_count].asInt()) {

      const mobile_apis::HMILevel::eType saved_hmi_level =
          static_cast<mobile_apis::HMILevel::eType>(
              so_application_data[i][strings::hmi_level].asInt());

      const time_t saved_time_stamp = static_cast<time_t>(
          so_application_data[i][strings::time_stamp].asUInt());

      if (mobile_apis::HMILevel::HMI_FULL == saved_hmi_level) {
        if (time_stamp_full < saved_time_stamp) {
          time_stamp_full = saved_time_stamp;
          full_app = &(so_application_data[i]);
        }
      }

      if (mobile_apis::HMILevel::HMI_LIMITED == saved_hmi_level) {
        if (time_stamp_limited < saved_time_stamp) {
          time_stamp_limited = saved_time_stamp;
          limited_app = &(so_application_data[i]);
        }
      }
    }
    // set invalid HMI level for all
    resumption_storage->SetHMILevelForSavedApplication(
        so_application_data[i][strings::app_id].asString(),
        so_application_data[i][strings::device_id].asString(),
        static_cast<int32_t>(mobile_apis::HMILevel::INVALID_ENUM));

  }

  if (full_app != NULL) {
    resumption_storage->SetHMILevelForSavedApplication(
        (*full_app)[strings::app_id].asString(),
        (*full_app)[strings::device_id].asString(),
        static_cast<int32_t>(mobile_apis::HMILevel::HMI_FULL));
  }

  if (limited_app != NULL) {
    resumption_storage->SetHMILevelForSavedApplication(
        (*limited_app)[strings::app_id].asString(),
        (*limited_app)[strings::device_id].asString(),
        static_cast<int32_t>(mobile_apis::HMILevel::HMI_LIMITED));
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ResumeCtrl::IsResumptionDataValid(uint32_t index) {
  const Json::Value& json_app = GetSavedApplications()[index];
  if (!json_app.isMember(strings::app_id) ||
      !json_app.isMember(strings::ign_off_count) ||
      !json_app.isMember(strings::hmi_level) ||
      !json_app.isMember(strings::hmi_app_id) ||
      !json_app.isMember(strings::time_stamp)) {
    LOG4CXX_ERROR(logger_, "Wrong resumption data");
    return false;
  }

  if (json_app.isMember(strings::hmi_app_id) &&
      0 >= json_app[strings::hmi_app_id].asUInt()) {
    LOG4CXX_ERROR(logger_, "Wrong resumption hmi app ID");
    return false;
  }

  return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ResumeCtrl::AddToResumption(uint32_t app_id) {
  queue_lock_.Acquire();
  waiting_for_timer_.push_back(app_id);
  queue_lock_.Release();
  if (!is_resumption_active_) {
    is_resumption_active_ = true;
    restore_hmi_level_timer_.start(
        profile::Profile::instance()->app_resuming_timeout());
  }
}

void ResumeCtrl::RemoveFromResumption(uint32_t app_id) {
  queue_lock_.Acquire();
  std::vector<uint32_t>::iterator pos =
    std::find(waiting_for_timer_.begin(), waiting_for_timer_.end(), app_id);
  if (pos != waiting_for_timer_.end()) {
    waiting_for_timer_.erase(pos);
  }
  queue_lock_.Release();
}

void ResumeCtrl::SendHMIRequest(
    const hmi_apis::FunctionID::eType& function_id,
    const smart_objects::SmartObject* msg_params, bool use_events) {
  LOG4CXX_AUTO_TRACE(logger_);
  smart_objects::SmartObjectSPtr result =
      MessageHelper::CreateModuleInfoSO(function_id);
  int32_t hmi_correlation_id =
      (*result)[strings::params][strings::correlation_id].asInt();
  if (use_events) {
    subscribe_on_event(function_id, hmi_correlation_id);
  }

  if (msg_params) {
    (*result)[strings::msg_params] = *msg_params;
  }

  if (!ApplicationManagerImpl::instance()->ManageHMICommand(result)) {
    LOG4CXX_ERROR(logger_, "Unable to send request");
  }
}

}  // namespace application_manager
