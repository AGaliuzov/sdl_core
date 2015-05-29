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

#include "policy/cache_manager.h"

#include <algorithm>
#include <functional>
#include <ctime>
#include <cmath>

#include "utils/file_system.h"
#include "json/reader.h"
#include "json/features.h"
#include "json/writer.h"
#include "utils/logger.h"
#include "utils/gen_hash.h"

#include "policy/sql_pt_ext_representation.h"

namespace policy_table = rpc::policy_table_interface_base;

namespace policy {

CREATE_LOGGERPTR_GLOBAL(logger_, "CacheManager")

#define CACHE_MANAGER_CHECK(return_value) {\
  if (!pt_) {\
    LOG4CXX_WARN(logger_, "The cache manager is not initialized");\
    return return_value;\
  }\
}

#define CACHE_MANAGER_CHECK_VOID() {\
  if (!pt_) {\
    LOG4CXX_WARN(logger_, "The cache manager is not initialized");\
    return;\
  }\
}


CacheManager::CacheManager()
  : CacheManagerInterface(),
    pt_(new policy_table::Table),
    backup_( new SQLPTExtRepresentation()),
    update_required(false) {

  LOG4CXX_AUTO_TRACE(logger_);
  backuper_ = new BackgroundBackuper(this);
  backup_thread_ = threads::CreateThread("Backup thread", backuper_);
  backup_thread_->start();
}

CacheManager::~CacheManager() {
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock lock(backuper_locker_);
  backup_thread_->join();
  delete backup_thread_->delegate();
  threads::DeleteThread(backup_thread_);
}

bool CacheManager::CanAppKeepContext(const std::string& app_id) {
  CACHE_MANAGER_CHECK(false);
  bool result = false;
  if (kDeviceId == app_id) {
    result = pt_->policy_table.app_policies_section.device.keep_context;
  } else if (AppExists(app_id)) {
    result = pt_->policy_table.app_policies_section.apps[app_id].keep_context;
  }
  return result;
}

uint16_t CacheManager::HeartBeatTimeout(const std::string& app_id) const {
  CACHE_MANAGER_CHECK(0);
  uint16_t result = 0;
  if (AppExists(app_id)) {
    if (pt_->policy_table.app_policies_section.apps[app_id].heart_beat_timeout_ms
        .is_initialized()) {
      result = *(pt_->policy_table.app_policies_section.apps[app_id].heart_beat_timeout_ms);
    }
  }
  return result;
}

bool CacheManager::CanAppStealFocus(const std::string& app_id) {
  CACHE_MANAGER_CHECK(false);
  bool result = false;
  if (kDeviceId == app_id) {
    result = pt_->policy_table.app_policies_section.device.steal_focus;
  } else if (AppExists(app_id)) {
    result = pt_->policy_table.app_policies_section.apps[app_id].steal_focus;
  }
  return result;
}

bool CacheManager::GetDefaultHMI(const std::string& app_id,
                                 std::string& default_hmi) {
  CACHE_MANAGER_CHECK(false);
  bool result = false;
  default_hmi.clear();
  if (kDeviceId == app_id) {
    default_hmi = EnumToJsonString(pt_->policy_table.
                         app_policies_section.device.default_hmi);
  } else if (AppExists(app_id)) {
      default_hmi = EnumToJsonString(pt_->policy_table.
                         app_policies_section.apps[app_id].default_hmi);
  }
  result = !default_hmi.empty();

  return result;
}

bool CacheManager::ResetUserConsent() {
  CACHE_MANAGER_CHECK(false);
  sync_primitives::AutoLock lock (cache_lock_);
  policy_table::DeviceData::iterator iter =
      pt_->policy_table.device_data->begin();
  policy_table::DeviceData::iterator iter_end =
      pt_->policy_table.device_data->end();

  for (; iter != iter_end; ++iter) {
    iter->second.user_consent_records->clear();
  }
  return true;
}


bool CacheManager::GetUserPermissionsForDevice(const std::string& device_id,
                                               StringArray& consented_groups,
                                               StringArray& disallowed_groups) {

  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK(false);
  policy_table::DeviceData& device_data = *pt_->policy_table.device_data;
  if (device_data.end() == device_data.find(device_id)) {
    return true;
  }
  const policy_table::DeviceParams& params = device_data[device_id];
  const policy_table::UserConsentRecords& ucr = *(params.user_consent_records);
  policy_table::UserConsentRecords::const_iterator iter = ucr.begin();
  policy_table::UserConsentRecords::const_iterator iter_end = ucr.end();

  for (; iter != iter_end; ++iter) {
    policy_table::ConsentGroups::const_iterator con_iter;
    policy_table::ConsentGroups::const_iterator con_iter_end;

    con_iter = (*iter).second.consent_groups->begin();
    con_iter_end = (*iter).second.consent_groups->end();
    for (; con_iter != con_iter_end; ++con_iter) {
      if(true == (*con_iter).second) {
        consented_groups.push_back((*con_iter).first);
      } else {
        disallowed_groups.push_back((*con_iter).first);
      }
    }
  }
  return true;
}

void CacheManager::GetAllAppGroups(const std::string& app_id,
                                   FunctionalGroupIDs& all_group_ids) {

  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK_VOID();
  if (kDeviceId == app_id) {
    policy_table::DevicePolicy& device =
        pt_->policy_table.app_policies_section.device;

    policy_table::Strings::const_iterator iter = device.groups.begin();
    policy_table::Strings::const_iterator iter_end = device.groups.end();

    for (; iter != iter_end; ++iter) {
      const uint32_t group_id =
          static_cast<uint32_t> ((utils::Djb2HashFromString(*iter)));
      all_group_ids.push_back(group_id);
    }

    return;
  }

  policy_table::ApplicationPolicies::const_iterator app_params_iter =
      pt_->policy_table.app_policies_section.apps.find(app_id);

  if (pt_->policy_table.app_policies_section.apps.end() != app_params_iter) {
    policy_table::Strings::const_iterator iter =
        (*app_params_iter).second.groups.begin();
    policy_table::Strings::const_iterator iter_end =
        (*app_params_iter).second.groups.end();

    for (; iter != iter_end; ++iter) {
      const uint32_t group_id =
          static_cast<uint32_t> ((utils::Djb2HashFromString(*iter)));
      all_group_ids.push_back(group_id);
    }
  }
}

void CacheManager::GetPreConsentedGroups(const std::string& app_id,
                                         FunctionalGroupIDs& preconsented_groups) {

  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK_VOID();
  if (kDeviceId == app_id) {
    policy_table::DevicePolicy& device =
        pt_->policy_table.app_policies_section.device;

    policy_table::Strings::const_iterator iter =
        device.preconsented_groups->begin();
    policy_table::Strings::const_iterator iter_end =
        device.preconsented_groups->end();

    for (; iter != iter_end; ++iter) {
      const uint32_t group_id =
          static_cast<uint32_t> ((utils::Djb2HashFromString(*iter)));
      preconsented_groups.push_back(group_id);
    }

    return;
  }

  policy_table::ApplicationPolicies::const_iterator app_param_iter =
      pt_->policy_table.app_policies_section.apps.find(app_id);
  if (pt_->policy_table.app_policies_section.apps.end() != app_param_iter) {

    policy_table::Strings::const_iterator iter =
        (*app_param_iter).second.preconsented_groups->begin();
    policy_table::Strings::const_iterator iter_end =
        (*app_param_iter).second.preconsented_groups->end();
    for (; iter != iter_end; ++iter) {
      const int32_t group_id = utils::Djb2HashFromString(*iter);

      preconsented_groups.push_back(group_id);
    }
  }
}

void CacheManager::GetConsentedGroups(const std::string& device_id,
                                      const std::string& app_id,
                                      FunctionalGroupIDs& allowed_groups,
                                      FunctionalGroupIDs& disallowed_groups) {

  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK_VOID();
  policy_table::DeviceData::iterator dev_params_iter =
      pt_->policy_table.device_data->find(device_id);

  if (pt_->policy_table.device_data->end() != dev_params_iter) {
    const policy_table::DeviceParams& dev_par = (*dev_params_iter).second;

    policy_table::UserConsentRecords::const_iterator iter =
        dev_par.user_consent_records->find(app_id);

    if (dev_par.user_consent_records->end() != iter) {
      policy_table::ConsentGroups::const_iterator consent_iter =
          (*iter).second.consent_groups->begin();
      policy_table::ConsentGroups::const_iterator consent_iter_end =
          (*iter).second.consent_groups->end();

      for (; consent_iter != consent_iter_end; ++consent_iter) {
        const int32_t group_id = utils::Djb2HashFromString((*consent_iter).first);

        if (true == (*consent_iter).second) {
          allowed_groups.push_back(group_id);
        } else {
          disallowed_groups.push_back(group_id);
        }
      }
    }
  }
}

void CacheManager::GetUnconsentedGroups(const std::string& device_id,
                                        const std::string& policy_app_id,
                                        FunctionalGroupIDs& unconsented_groups) {
  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK_VOID();
  policy_table::Strings::iterator iter_groups;
  policy_table::Strings::iterator iter_groups_end = iter_groups;
  if (kDeviceId == policy_app_id) {
    iter_groups =
        pt_->policy_table.app_policies_section.device.groups.begin();
    iter_groups_end =
        pt_->policy_table.app_policies_section.device.groups.end();
  } else if (!AppExists(policy_app_id)) {
    return;
  } else {
    iter_groups = pt_->policy_table.app_policies_section.
                  apps[policy_app_id].groups.begin();
    iter_groups_end = pt_->policy_table.app_policies_section.
                      apps[policy_app_id].groups.end();
  }

  for (;iter_groups != iter_groups_end; ++iter_groups) {
    policy_table::FunctionalGroupings::const_iterator func_groups =
        pt_->policy_table.functional_groupings.find(*iter_groups);
    // Try to find app-specific group in common groups list;
    if (pt_->policy_table.functional_groupings.end() != func_groups) {
      // Check if groups have user consents field.
      if (func_groups->second.user_consent_prompt.is_initialized()) {
        // Try to find certain group among already consented groups.
        policy_table::DeviceData::const_iterator device_iter =
            pt_->policy_table.device_data->find(device_id);
        if (pt_->policy_table.device_data->end() != device_iter) {
          policy_table::UserConsentRecords::const_iterator ucr_iter =
            device_iter->second.user_consent_records->find(policy_app_id);
          if (device_iter->second.user_consent_records->end() != ucr_iter) {
            if ((*ucr_iter).second.consent_groups->end() ==
                 (*ucr_iter).second.consent_groups->find(*iter_groups)) {
                unconsented_groups.push_back(utils::Djb2HashFromString(*iter_groups));
            }
          } else {
            unconsented_groups.push_back(utils::Djb2HashFromString(*iter_groups));
          }
        }
      }
    }
  }
}

void CacheManager::RemoveAppConsentForGroup(const std::string& app_id,
                                            const std::string& group_name) {
  CACHE_MANAGER_CHECK_VOID();
  policy_table::DeviceData::iterator device_iter =
      pt_->policy_table.device_data->begin();
  policy_table::DeviceData::iterator device_iter_end =
      pt_->policy_table.device_data->end();

  policy_table::UserConsentRecords::iterator ucr_iter;
  for (; device_iter != device_iter_end; ++device_iter) {
    ucr_iter = device_iter->second.user_consent_records->find(app_id);
    if (device_iter->second.user_consent_records->end() != ucr_iter) {
      ucr_iter->second.consent_groups->erase(group_name);
    }
  }
}

bool CacheManager::ApplyUpdate(const policy_table::Table& update_pt) {
  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK(false);
  sync_primitives::AutoLock auto_lock(cache_lock_);
  pt_->policy_table.functional_groupings =
      update_pt.policy_table.functional_groupings;

  policy_table::ApplicationPolicies::const_iterator iter =
      update_pt.policy_table.app_policies_section.apps.begin();
  policy_table::ApplicationPolicies::const_iterator iter_end =
      update_pt.policy_table.app_policies_section.apps.end();

  for (;iter != iter_end; ++iter) {
    if (iter->second.is_null()) {
      pt_->policy_table.app_policies_section.apps[iter->first].set_to_null();
      pt_->policy_table.app_policies_section.apps[iter->first].set_to_string("");
    } else if (policy::kDefaultId == (iter->second).get_string()) {
        policy_table::ApplicationPolicies::const_iterator iter_default =
            update_pt.policy_table.app_policies_section.apps.find(kDefaultId);
        if (update_pt.policy_table.app_policies_section.apps.end() == iter_default) {
            LOG4CXX_ERROR(logger_, "The default section was not found in PTU");
            continue;
          }
      pt_->policy_table.app_policies_section.apps[iter->first] = iter_default->second;
    } else {
      pt_->policy_table.app_policies_section.apps[iter->first] = iter->second;
    }
  }

  pt_->policy_table.module_config.SafeCopyFrom(update_pt.policy_table.module_config);

  pt_->policy_table.consumer_friendly_messages.assign_if_valid(
        update_pt.policy_table.consumer_friendly_messages);

  ResetCalculatedPermissions();
  Backup();
  return true;
}

void CacheManager::GetHMIAppTypeAfterUpdate(std::map<std::string, StringArray>& app_hmi_types) {
  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK_VOID();
  policy_table::ApplicationPolicies::const_iterator policy_iter_begin =
      pt_->policy_table.app_policies_section.apps.begin();
  policy_table::ApplicationPolicies::const_iterator policy_iter_end =
      pt_->policy_table.app_policies_section.apps.end();
  std::vector<std::string> transform_app_hmi_types;
  for(; policy_iter_begin != policy_iter_end; ++policy_iter_begin) {
    const policy_table::ApplicationParams& app_params = (*policy_iter_begin).second;
    if(app_params.AppHMIType.is_initialized()) {
      if(!(transform_app_hmi_types.empty())) {
        transform_app_hmi_types.clear();
      }
      std::transform(app_params.AppHMIType->begin(), app_params.AppHMIType->end(),
                     std::back_inserter(transform_app_hmi_types), AppHMITypeToString());
      app_hmi_types[(*policy_iter_begin).first] = transform_app_hmi_types;
    }
  }
}

void CacheManager::Backup() {
  sync_primitives::AutoLock lock(backuper_locker_);
  DCHECK(backuper_);
  backuper_->DoBackup();
}

std::string CacheManager::currentDateTime() {
  time_t     now = time(0);
  struct tm  tstruct;
  char       buf[80];
  tstruct = *localtime(&now);
  // ISO_8601 format is expected, e.g. “2000-01-01T12:18:53Z”
  strftime(buf, sizeof(buf), "%Y-%m-%dT%XZ", &tstruct);
  return buf;
}

bool CacheManager::GetPermissionsForApp(const std::string& device_id,
                                        const std::string& app_id,
                                        FunctionalIdType& group_types) {

  LOG4CXX_AUTO_TRACE(logger_);
  GetAllAppGroups(app_id, group_types[kTypeGeneral]);
  GetAllAppGroups(kDefaultId, group_types[kTypeDefault]);
  GetAllAppGroups(kPreDataConsentId, group_types[kTypePreDataConsented]);
  GetPreConsentedGroups(app_id, group_types[kTypePreconsented]);

  GetConsentedGroups(device_id, app_id,
                     group_types[kTypeAllowed], group_types[kTypeDisallowed]);

  GetUnconsentedGroups(device_id, app_id, group_types[kTypeUnconsented]);

  GetAllAppGroups(kDeviceId, group_types[kTypeDevice]);
  return true;
}

bool CacheManager::GetDeviceGroupsFromPolicies(
  policy_table::Strings& groups,
  policy_table::Strings& preconsented_groups) {
  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK(false);
  groups = pt_->policy_table.app_policies_section.device.groups;
  preconsented_groups =
      *(pt_->policy_table.app_policies_section.device).preconsented_groups;
  return true;
}

bool CacheManager::AddDevice(const std::string &device_id,
                             const std::string &connection_type) {
  LOG4CXX_AUTO_TRACE(logger_);

  sync_primitives::AutoLock auto_lock(cache_lock_);
  CACHE_MANAGER_CHECK(false);
  policy_table::DeviceParams& params =
      (*(pt_->policy_table.device_data))[device_id];
  *params.connection_type = connection_type;

  // We have to set preloaded flag as false in policy table on adding new
  // information (SDLAQ-CRS-2365). It can happens only after device addition.
  *pt_->policy_table.module_config.preloaded_pt = false;

  Backup();
  return true;
}

bool CacheManager::SetDeviceData(const std::string &device_id,
                                 const std::string &hardware,
                                 const std::string &firmware,
                                 const std::string &os,
                                 const std::string &os_version,
                                 const std::string &carrier,
                                 const uint32_t number_of_ports,
                                 const std::string& connection_type) {
  LOG4CXX_AUTO_TRACE(logger_);

  sync_primitives::AutoLock auto_lock(cache_lock_);
  CACHE_MANAGER_CHECK(false);

  if (pt_->policy_table.device_data->end() ==
      pt_->policy_table.device_data->find(device_id)) {
    LOG4CXX_ERROR(logger_, "Unable to find mobile device: "
                    << device_id);
    return false;
  }

  policy_table::DeviceParams& params =
      (*(pt_->policy_table.device_data))[device_id];
  *params.hardware = hardware;
  *params.firmware_rev = firmware;
  *params.os = os;
  *params.os_version = os_version;
  *params.carrier = carrier;
  *params.max_number_rfcom_ports = number_of_ports;
  *params.connection_type = connection_type;

  Backup();
  return true;
}

bool CacheManager::SetUserPermissionsForDevice(
    const std::string& device_id,
    const StringArray& consented_groups,
    const StringArray& disallowed_groups) {
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock auto_lock(cache_lock_);
  CACHE_MANAGER_CHECK(false);
  policy_table::DeviceParams& params = (*pt_->policy_table.device_data)[device_id];
  policy_table::UserConsentRecords& ucr = *(params.user_consent_records);

  StringArray::const_iterator consent_iter_end = consented_groups.end();
  StringArray::const_iterator consent_iter = consented_groups.begin();
  StringArray::const_iterator un_consent_iter_end = disallowed_groups.end();
  StringArray::const_iterator un_consent_iter = disallowed_groups.begin();

  for (; consent_iter != consent_iter_end; ++consent_iter) {
    (*ucr[kDeviceId].consent_groups)[*consent_iter] = true;
  }

  for (; un_consent_iter != un_consent_iter_end; ++un_consent_iter) {
    (*ucr[kDeviceId].consent_groups)[*un_consent_iter] = false;
  }

  policy_table::UserConsentRecords::iterator ucr_iter = ucr.begin();
  policy_table::UserConsentRecords::iterator ucr_iter_end = ucr.end();
  // TODO(AGaliuzov): Get this info from external data
  for (;ucr_iter != ucr_iter_end;++ucr_iter) {
    *ucr_iter->second.input = policy_table::Input::I_GUI;
    *ucr_iter->second.time_stamp = currentDateTime();
  }
  Backup();
  return true;
}

bool CacheManager::ReactOnUserDevConsentForApp(const std::string& app_id,
                                               bool is_device_allowed) {
  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK(false);
  bool result = true;
  cache_lock_.Acquire();
  if (is_device_allowed) {
    // If app has pre_DataConsented groups it should be 'promoted' to default
    if (IsPredataPolicy(app_id)) {
      result = SetDefaultPolicy(app_id);
    }
  }
  cache_lock_.Release();
  Backup();
  return result;
}

void CacheManager::GetGroupNameByHashID(const int32_t group_id,
                                        std::string& group_name) {

  CACHE_MANAGER_CHECK_VOID();
  policy_table::FunctionalGroupings::const_iterator fg_iter =
      pt_->policy_table.functional_groupings.begin();
  policy_table::FunctionalGroupings::const_iterator fg_iter_end =
      pt_->policy_table.functional_groupings.end();

  for (; fg_iter != fg_iter_end; ++fg_iter) {
    const int32_t id = utils::Djb2HashFromString((*fg_iter).first);
    if (group_id == id) {
      group_name = (*fg_iter).first;
    }
  }
}

bool CacheManager::SetUserPermissionsForApp(
    const PermissionConsent& permissions) {
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock auto_lock(cache_lock_);
  CACHE_MANAGER_CHECK(false);
  std::vector<FunctionalGroupPermission>::const_iterator iter =
      permissions.group_permissions.begin();
  std::vector<FunctionalGroupPermission>::const_iterator iter_end =
      permissions.group_permissions.end();

  std::string group_name;
  for (;iter != iter_end; ++iter) {
    if (policy::kGroupUndefined != (*iter).state) {
      policy_table::DeviceParams& params = (*pt_->policy_table.device_data)[permissions.device_id];
      rpc::policy_table_interface_base::ConsentRecords& ucr =
          (*params.user_consent_records)[permissions.policy_app_id];

      GetGroupNameByHashID((*iter).group_id, group_name);

      (*ucr.consent_groups)[group_name] = ((*iter).state == policy::kGroupAllowed);
      *ucr.input = policy_table::Input::I_GUI;
      *ucr.time_stamp = currentDateTime();
    }
  }
  Backup();
  return true;
}

bool CacheManager::UpdateRequired() const {
  return update_required;
}

void CacheManager::SaveUpdateRequired(bool status) {
  update_required = status;
  Backup();
}

bool CacheManager::IsApplicationRevoked(const std::string& app_id) const {
  CACHE_MANAGER_CHECK(false);
  bool is_revoked = false;
  if (pt_->policy_table.app_policies_section.apps.end() !=
      pt_->policy_table.app_policies_section.apps.find(app_id)) {
    is_revoked = pt_->policy_table.app_policies_section.apps[app_id].is_null();
  }

  return is_revoked;
}

void CacheManager::CheckPermissions(const PTString& app_id,
                                    const PTString& hmi_level,
                                    const PTString& rpc,
                                    CheckPermissionResult& result) {
  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK_VOID();

  if (pt_->policy_table.app_policies_section.apps.end() ==
      pt_->policy_table.app_policies_section.apps.find(app_id)) {
    LOG4CXX_ERROR(logger_, "Application id " << app_id
                  << " was not found in policy DB.");
    return;
  }

  policy_table::Strings::const_iterator app_groups_iter =
      pt_->policy_table.app_policies_section.apps[app_id].groups.begin();

  policy_table::Strings::const_iterator app_groups_iter_end =
      pt_->policy_table.app_policies_section.apps[app_id].groups.end();

  policy_table::FunctionalGroupings::const_iterator concrete_group;

  for (; app_groups_iter != app_groups_iter_end; ++app_groups_iter) {
    concrete_group = pt_->policy_table.functional_groupings.find(*app_groups_iter);
    if (pt_->policy_table.functional_groupings.end() != concrete_group) {
      const policy_table::Rpcs& rpcs = concrete_group->second;

      policy_table::Rpc::const_iterator rpc_iter = rpcs.rpcs.find(rpc);
      if (rpcs.rpcs.end() != rpc_iter) {
        policy_table::RpcParameters rpc_param = rpc_iter->second;

        policy_table::HmiLevel hmi_level_e;
        policy_table::EnumFromJsonString(hmi_level, &hmi_level_e);

        policy_table::HmiLevels::const_iterator hmi_iter =
            std::find(rpc_param.hmi_levels.begin(), rpc_param.hmi_levels.end(),
                  hmi_level_e);

        if (rpc_param.hmi_levels.end() != hmi_iter) {
          result.hmi_level_permitted = PermitResult::kRpcAllowed;

          policy_table::Parameters::const_iterator params_iter =
              rpc_param.parameters->begin();
          policy_table::Parameters::const_iterator params_iter_end =
              rpc_param.parameters->end();

          for (;params_iter != params_iter_end; ++params_iter) {
            result.list_of_allowed_params.push_back(
                  policy_table::EnumToJsonString(*params_iter));
          }
        }
      }
    }
  }
}

bool CacheManager::IsPTPreloaded() {
  CACHE_MANAGER_CHECK(false);
  return *pt_->policy_table.module_config.preloaded_pt;
}

int CacheManager::IgnitionCyclesBeforeExchange() {
  CACHE_MANAGER_CHECK(0);
  const uint8_t limit = std::max(
        static_cast<int>(
          pt_->policy_table.module_config.exchange_after_x_ignition_cycles), 0);
  LOG4CXX_DEBUG(logger_, "IgnitionCyclesBeforeExchange limit:" << limit);
  uint8_t current = 0;

  const int last_exch = static_cast<int>
      (*pt_->policy_table.module_meta->ignition_cycles_since_last_exchange);
  current = std::max(last_exch, 0);
  LOG4CXX_DEBUG(logger_, "IgnitionCyclesBeforeExchange current:" << current);

  return std::max(limit - current, 0);
}

int CacheManager::KilometersBeforeExchange(int current) {
  CACHE_MANAGER_CHECK(0);
  const int limit = std::max(
        static_cast<int>(
          pt_->policy_table.module_config.exchange_after_x_kilometers), 0);
  LOG4CXX_DEBUG(logger_, "KilometersBeforeExchange limit:" << limit);
  int last = 0;

  const int odo_val = static_cast<int>
      (*pt_->policy_table.module_meta->pt_exchanged_at_odometer_x);
  last = std::max(odo_val, 0);
  LOG4CXX_DEBUG(logger_, "KilometersBeforeExchange last:" << last);

  const int actual = std::max((current - last), 0);
  LOG4CXX_DEBUG(logger_, "KilometersBeforeExchange actual:" << actual);
  return std::max(limit - actual, 0);
}

bool CacheManager::SetCountersPassedForSuccessfulUpdate(policy::Counters counter,
                                                        int value) {
  CACHE_MANAGER_CHECK(false);
  switch (counter) {
  case KILOMETERS:
    *pt_->policy_table.module_meta->pt_exchanged_at_odometer_x = value;
    LOG4CXX_DEBUG(logger_, "SetCountersPassedForSuccessfulUpdate km:" << value);
    break;
  case DAYS_AFTER_EPOCH:
    *pt_->policy_table.module_meta->pt_exchanged_x_days_after_epoch = value;
    LOG4CXX_DEBUG(logger_,
                  "SetCountersPassedForSuccessfulUpdate days after epoch:"
                  << value);
    break;
  default:
    LOG4CXX_ERROR(logger_, "Unknown counter was requested to set: "
                  << counter);
    return false;
  }

  Backup();
  return true;
}

int CacheManager::DaysBeforeExchange(int current) {
  CACHE_MANAGER_CHECK(0);
  const uint8_t limit = std::max(
        static_cast<int>(
          pt_->policy_table.module_config.exchange_after_x_days), 0);
  LOG4CXX_DEBUG(logger_, "DaysBeforeExchange limit:" << limit);
  uint8_t last = 0;

  const int odo_val = static_cast<int>
      (*pt_->policy_table.module_meta->pt_exchanged_x_days_after_epoch);
  last = std::max(odo_val, 0);
  LOG4CXX_DEBUG(logger_, "DaysBeforeExchange last:" << last);

  const uint8_t actaul = std::max((current - last), 0);
  LOG4CXX_DEBUG(logger_, "DaysBeforeExchange actual:" << actaul);
  return std::max(limit - actaul, 0);
}

void CacheManager::IncrementIgnitionCycles() {
  CACHE_MANAGER_CHECK_VOID();
  const int ign_val = static_cast<int>
      (*pt_->policy_table.module_meta->ignition_cycles_since_last_exchange);
  (*pt_->policy_table.module_meta->ignition_cycles_since_last_exchange) = ign_val + 1;
  LOG4CXX_DEBUG(logger_, "IncrementIgnitionCycles ignitions:" << ign_val);
  Backup();
}

void CacheManager::ResetIgnitionCycles() {
  CACHE_MANAGER_CHECK_VOID();
  (*pt_->policy_table.module_meta->ignition_cycles_since_last_exchange) = 0;
  Backup();
}

int CacheManager::TimeoutResponse() {
  CACHE_MANAGER_CHECK(0);
  return pt_->policy_table.module_config.timeout_after_x_seconds;
}

bool CacheManager::SecondsBetweenRetries(std::vector<int>& seconds) {
  CACHE_MANAGER_CHECK(false);
  rpc::policy_table_interface_base::SecondsBetweenRetries::iterator iter =
    pt_->policy_table.module_config.seconds_between_retries.begin();
  rpc::policy_table_interface_base::SecondsBetweenRetries::iterator iter_end =
    pt_->policy_table.module_config.seconds_between_retries.end();

  const std::size_t size =
      pt_->policy_table.module_config.seconds_between_retries.size();
  seconds.reserve(size);
  for (;iter != iter_end; ++iter) {
    seconds.push_back(*iter);
  }
  return true;
}

const policy::VehicleInfo CacheManager::GetVehicleInfo() const {
  CACHE_MANAGER_CHECK(VehicleInfo());
  policy_table::ModuleConfig& module_config =
      pt_->policy_table.module_config;
  VehicleInfo vehicle_info;
  vehicle_info.vehicle_make = *module_config.vehicle_make;
  vehicle_info.vehicle_model = *module_config.vehicle_model;
  vehicle_info.vehicle_year = *module_config.vehicle_year;
  LOG4CXX_DEBUG(logger_, "Vehicle info (make, model, year):"
                << vehicle_info.vehicle_make << ","
                << vehicle_info.vehicle_model << ","
                << vehicle_info.vehicle_year );
  return vehicle_info;
}

std::vector<UserFriendlyMessage> CacheManager::GetUserFriendlyMsg(
    const std::vector<std::string>& msg_codes, const std::string& language) {

  LOG4CXX_AUTO_TRACE(logger_);
  std::vector<UserFriendlyMessage> result;
  CACHE_MANAGER_CHECK(result);

  const std::string fallback_language = "en-us";
  std::vector<std::string>::const_iterator it = msg_codes.begin();
  std::vector<std::string>::const_iterator it_end = msg_codes.end();
  for (; it != it_end; ++it) {

    std::string msg_language = language;

    rpc::policy_table_interface_base::MessageLanguages msg_languages =
        (*pt_->policy_table.consumer_friendly_messages->messages)[*it];

    // If message has no records with required language, fallback language
    // should be used instead.
    if (msg_languages.languages.end() ==
        msg_languages.languages.find(msg_language)) {
      msg_language = fallback_language;
    }

    rpc::policy_table_interface_base::MessageString message_string =
        msg_languages.languages[msg_language];

    UserFriendlyMessage msg;
    msg.message_code = *it;
    msg.tts = *message_string.tts;
    msg.label = *message_string.label;
    msg.line1 = *message_string.line1;
    msg.line2 = *message_string.line2;
    msg.text_body = *message_string.textBody;

    result.push_back(msg);
  }
  return result;
}

void CacheManager::GetUpdateUrls(int service_type,
                                 EndpointUrls& end_points) {
  LOG4CXX_AUTO_TRACE(logger_);

  CACHE_MANAGER_CHECK_VOID();
  char buff[32];
  sprintf(buff, "%x", service_type);

  std::string serv_type(buff);
  // TODO: remove this workaround
  if (service_type <= 0x9) {
    serv_type.insert(0,"0x0", 3);
  } else {
    serv_type.insert(0,"0x", 2);
  }

  policy_table::ServiceEndpoints::const_iterator iter =
      pt_->policy_table.module_config.endpoints.find(serv_type);

  if (pt_->policy_table.module_config.endpoints.end() != iter) {
    policy_table::URLList::const_iterator url_list_iter = (*iter).second.begin();
    policy_table::URLList::const_iterator url_list_iter_end = (*iter).second.end();
    for (; url_list_iter != url_list_iter_end; ++url_list_iter) {
      EndpointData data;
      data.app_id = (*url_list_iter).first;
      std::copy((*url_list_iter).second.begin(), (*url_list_iter).second.end(),
              std::back_inserter(data.url));

      end_points.push_back(data);
    }
  }
}

int CacheManager::GetNotificationsNumber(const std::string& priority) {
  CACHE_MANAGER_CHECK(0);
  typedef rpc::policy_table_interface_base::NumberOfNotificationsPerMinute NNPM;

  const NNPM& nnpm =
      pt_->policy_table.module_config.notifications_per_minute_by_priority;

  NNPM::const_iterator priority_iter = nnpm.find(priority);

  const int result = (nnpm.end() != priority_iter ? (*priority_iter).second : 0);
  return result;
}

bool CacheManager::GetPriority(const std::string& policy_app_id,
                               std::string& priority) {
  CACHE_MANAGER_CHECK(false);
  if (kDeviceId == policy_app_id) {
    priority = EnumToJsonString(
                 pt_->policy_table.app_policies_section.device.priority);
    return true;
  }

  const policy_table::ApplicationPolicies& policies =
      pt_->policy_table.app_policies_section.apps;

  policy_table::ApplicationPolicies::const_iterator policy_iter =
      policies.find(policy_app_id);
  const bool app_id_exists = policies.end() != policy_iter;
  if (app_id_exists) {
    priority = EnumToJsonString((*policy_iter).second.priority);
  }

  return app_id_exists;
}

void CacheManager::CheckSnapshotInitialization() {
  CACHE_MANAGER_CHECK_VOID();
  if (!snapshot_) {
    LOG4CXX_ERROR(logger_, "Snapshot pointer is not initialized");
    return;
  }

  *(snapshot_->policy_table.module_config.preloaded_pt) = false;

  snapshot_->policy_table.consumer_friendly_messages->messages =
      rpc::Optional<policy_table::Messages>();

  rpc::Optional<policy_table::ModuleMeta>& module_meta =
      snapshot_->policy_table.module_meta;
  if (!module_meta->pt_exchanged_at_odometer_x->is_initialized()) {
    *(module_meta->pt_exchanged_at_odometer_x) = 0;
  }

  if (!module_meta->pt_exchanged_x_days_after_epoch->is_initialized()) {
    *(module_meta->pt_exchanged_x_days_after_epoch) = 0;
  }

  rpc::Optional<policy_table::UsageAndErrorCounts>& usage_and_error_counts =
      snapshot_->policy_table.usage_and_error_counts;
  if (!usage_and_error_counts->count_of_iap_buffer_full->is_initialized()) {
    *(usage_and_error_counts->count_of_iap_buffer_full) = 0;
  }

  if (!usage_and_error_counts->count_of_sync_reboots->is_initialized()) {
    *(usage_and_error_counts->count_of_sync_reboots) = 0;
  }

  if (!usage_and_error_counts->count_sync_out_of_memory->is_initialized()) {
    *(usage_and_error_counts->count_sync_out_of_memory) = 0;
  }

  if (usage_and_error_counts->app_level->is_initialized()) {
    policy_table::AppLevels::iterator it =
        usage_and_error_counts->app_level->begin();
    policy_table::AppLevels::const_iterator it_end =
        usage_and_error_counts->app_level->end();
    for (;it != it_end; ++it) {
      if (!(*it).second.minutes_in_hmi_full.is_initialized()) {
        (*it).second.minutes_in_hmi_full = 0;
      }

      if (!(*it).second.app_registration_language_gui.is_initialized()) {
        (*it).second.app_registration_language_gui = "unknown";
      }

      if (!(*it).second.app_registration_language_vui.is_initialized()) {
        (*it).second.app_registration_language_vui = "unknown";
      }

      if (!(*it).second.minutes_in_hmi_limited.is_initialized()) {
        (*it).second.minutes_in_hmi_limited = 0;
      }

      if (!(*it).second.minutes_in_hmi_background.is_initialized()) {
        (*it).second.minutes_in_hmi_background = 0;
      }

      if (!(*it).second.minutes_in_hmi_none.is_initialized()) {
        (*it).second.minutes_in_hmi_none = 0;
      }

      if (!(*it).second.count_of_user_selections.is_initialized()) {
        (*it).second.count_of_user_selections = 0;
      }

      if (!(*it).second.count_of_rejections_sync_out_of_memory.is_initialized()) {
        (*it).second.count_of_rejections_sync_out_of_memory = 0;
      }

      if (!(*it).second.count_of_rejections_nickname_mismatch.is_initialized()) {
        (*it).second.count_of_rejections_nickname_mismatch = 0;
      }

      if (!(*it).second.count_of_rejections_duplicate_name.is_initialized()) {
        (*it).second.count_of_rejections_duplicate_name = 0;
      }

      if (!(*it).second.count_of_rejected_rpc_calls.is_initialized()) {
        (*it).second.count_of_rejected_rpc_calls = 0;
      }

      if (!(*it).second.count_of_rpcs_sent_in_hmi_none.is_initialized()) {
        (*it).second.count_of_rpcs_sent_in_hmi_none = 0;
      }

      if (!(*it).second.count_of_removals_for_bad_behavior.is_initialized()) {
        (*it).second.count_of_removals_for_bad_behavior = 0;
      }

      if (!(*it).second.count_of_run_attempts_while_revoked.is_initialized()) {
        (*it).second.count_of_run_attempts_while_revoked = 0;
      }
    }
  }
}

void CacheManager::PersistData() {
  LOG4CXX_AUTO_TRACE(logger_);
  if (backup_.valid()) {
    if (pt_.valid()) {

      cache_lock_.Acquire();
      policy_table::Table copy_pt(*pt_);
      cache_lock_.Release();

      backup_->Save(copy_pt);
      backup_->SaveUpdateRequired(update_required);

      policy_table::ApplicationPolicies::const_iterator app_policy_iter =
          copy_pt.policy_table.app_policies_section.apps.begin();
      policy_table::ApplicationPolicies::const_iterator app_policy_iter_end =
          copy_pt.policy_table.app_policies_section.apps.end();

      bool is_revoked = false;
      bool is_default_policy;
      bool is_predata_policy;

      for (; app_policy_iter != app_policy_iter_end; ++app_policy_iter) {

        const std::string app_id = (*app_policy_iter).first;

        if (copy_pt.policy_table.app_policies_section.apps.end() !=
            copy_pt.policy_table.app_policies_section.apps.find(app_id)) {
          is_revoked = copy_pt.policy_table.app_policies_section.apps[app_id].is_null();
        }

        is_default_policy = copy_pt.policy_table.app_policies_section.apps.end() !=
            copy_pt.policy_table.app_policies_section.apps.find(app_id) &&
            policy::kDefaultId ==
            copy_pt.policy_table.app_policies_section.apps[app_id].get_string();

        // TODO(AOleynik): Remove this field from DB
        is_predata_policy = copy_pt.policy_table.app_policies_section.apps.end() !=
            copy_pt.policy_table.app_policies_section.apps.find(app_id) &&
            policy::kPreDataConsentId ==
            copy_pt.policy_table.app_policies_section.apps[app_id].get_string();

        backup_->SaveApplicationCustomData(app_id,
                                           is_revoked,
                                           is_default_policy,
                                           is_predata_policy);
        is_revoked = false;
      }

  // In case of extended policy the meta info should be backuped as well.
      if (ex_backup_.valid()) {

        ex_backup_->SetMetaInfo(*(*copy_pt.policy_table.module_meta).ccpu_version,
                                *(*copy_pt.policy_table.module_meta).wers_country_code,
                                *(*copy_pt.policy_table.module_meta).language);
        ex_backup_->SetVINValue(*(*copy_pt.policy_table.module_meta).vin);

        //Save unpaired flag for devices
        policy_table::DeviceData::const_iterator it_device =
            copy_pt.policy_table.device_data->begin();
        policy_table::DeviceData::const_iterator it_end_device =
            copy_pt.policy_table.device_data->end();

#ifdef ENABLE_LOG
        policy_table::DeviceData& device_data = *copy_pt.policy_table.device_data;
        LOG4CXX_DEBUG(logger_, "Device_data size is: " << device_data.size());
#endif //ENABLE_LOG
        for (;it_device != it_end_device; ++it_device) {
          if (is_unpaired_.end() != is_unpaired_.find(it_device->first)) {
            ex_backup_->SetUnpairedDevice(it_device->first, true);
          } else {
            ex_backup_->SetUnpairedDevice(it_device->first, false);
          }
        }
        LOG4CXX_DEBUG(logger_, "Device_data size is: " << device_data.size());
      }
      backup_->WriteDb();
    }
  }
}

void CacheManager::ResetCalculatedPermissions() {
  LOG4CXX_TRACE(logger_, "ResetCalculatedPermissions");
  sync_primitives::AutoLock lock(calculated_permissions_lock_);
  calculated_permissions_.clear();
}

void CacheManager::AddCalculatedPermissions(
    const std::string& device_id,
    const std::string& policy_app_id,
    const Permissions& permissions) {
  LOG4CXX_DEBUG(logger_, "AddCalculatedPermissions for device: " << device_id
               << " and app: " << policy_app_id);
  sync_primitives::AutoLock lock(calculated_permissions_lock_);
  calculated_permissions_[device_id][policy_app_id] = permissions;
}

bool CacheManager::IsPermissionsCalculated(
    const std::string& device_id,
    const std::string& policy_app_id,
    Permissions& permission) {
  LOG4CXX_DEBUG(logger_, "IsPermissionsCalculated for device: " << device_id
               << " and app: " << policy_app_id);
  sync_primitives::AutoLock lock(calculated_permissions_lock_);
  CalculatedPermissions::const_iterator it =
      calculated_permissions_.find(device_id);

  if (calculated_permissions_.end() == it) {
    return false;
  }

  AppCalculatedPermissions::const_iterator app_it =
      (*it).second.find(policy_app_id);
  if ((*it).second.end() == app_it) {
    return false;
  } else {
    permission = (*app_it).second;
    return true;
  }
  return false;
}

std::string CacheManager::RemoteAppsUrl() const {
  return "Not implemented";
}

utils::SharedPtr<policy_table::Table>
CacheManager::GenerateSnapshot() {
  CACHE_MANAGER_CHECK(snapshot_);
  snapshot_ = new policy_table::Table();
  snapshot_->policy_table = pt_->policy_table;
  CheckSnapshotInitialization();
  return snapshot_;
}

bool CacheManager::GetInitialAppData(const std::string& app_id,
                                     StringArray& nicknames,
                                     StringArray& app_hmi_types) {

  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK(false);
  policy_table::ApplicationPolicies::const_iterator policy_iter =
      pt_->policy_table.app_policies_section.apps.find(app_id);

  if (pt_->policy_table.app_policies_section.apps.end() != policy_iter) {
    const policy_table::ApplicationParams& app_params = (*policy_iter).second;

    std::copy(app_params.nicknames->begin(), app_params.nicknames->end(),
              std::back_inserter(nicknames));

    std::transform(app_params.AppHMIType->begin(), app_params.AppHMIType->end(),
                   std::back_inserter(app_hmi_types), AppHMITypeToString());
  }
  return true;
}

bool CacheManager::GetFunctionalGroupings(
    policy_table::FunctionalGroupings& groups) {

  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK(false);
  const policy_table::FunctionalGroupings& f_groupings =
    pt_->policy_table.functional_groupings;

  groups.insert(f_groupings.begin(), f_groupings.end());
  return true;
}

int CacheManager::CountUnconsentedGroups(const std::string& policy_app_id,
                                          const std::string& device_id) {
  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK(false);
  LOG4CXX_DEBUG(logger_, "Application id: " << policy_app_id);
  int result = 0;
  if (kDeviceId != policy_app_id && !AppExists(policy_app_id)) {
    return 0;
  } else if (IsDefaultPolicy(policy_app_id)) {
    return 0;
  } else if (IsPredataPolicy(policy_app_id)) {
    return 0;
  }

  policy_table::FunctionalGroupings::const_iterator groups_iter_end =
      pt_->policy_table.functional_groupings.end();

  policy_table::ApplicationPoliciesSection& app_policies_section =
      pt_->policy_table.app_policies_section;

  policy_table::Strings::iterator app_groups;
  policy_table::Strings::iterator app_groups_end = app_groups;
  policy_table::Strings::iterator app_pre_groups;
  policy_table::Strings::iterator app_pre_groups_end = app_pre_groups;

  if (kDeviceId == policy_app_id) {
    app_groups = app_policies_section.device.groups.begin();

    app_groups_end = app_policies_section.device.groups.end();

    app_pre_groups =
        app_policies_section.device.preconsented_groups->begin();

    app_pre_groups_end =
        app_policies_section.device.preconsented_groups->end();
  } else {
    app_groups = app_policies_section.apps[policy_app_id].groups.begin();

    app_groups_end = app_policies_section.apps[policy_app_id].groups.end();

    app_pre_groups =
        app_policies_section.apps[policy_app_id].preconsented_groups->begin();

    app_pre_groups_end =
        app_policies_section.apps[policy_app_id].preconsented_groups->end();
  }


  policy_table::Strings groups_to_be_consented;
  policy_table::FunctionalGroupings::iterator current_groups_iter;
  for (; app_groups != app_groups_end; ++app_groups) {

    current_groups_iter =
        pt_->policy_table.functional_groupings.find(*app_groups);

    if (groups_iter_end != current_groups_iter) {
      if (current_groups_iter->second.user_consent_prompt.is_initialized()) {
        // Check if groups which requires user consent prompt
        // not included in "preconsented_groups" section
        if (app_pre_groups_end ==
            std::find(app_pre_groups, app_pre_groups_end, *app_groups)) {
          groups_to_be_consented.push_back(*app_groups);
        }
      }
    }
  }

  if (groups_to_be_consented.empty()) {
    return 0;
  }

  // If there is no device record, all groups with consents should be consented
  if (pt_->policy_table.device_data->end() ==
      pt_->policy_table.device_data->find(device_id)) {
    return groups_to_be_consented.size();
  }

  policy_table::DeviceParams& params =
      (*pt_->policy_table.device_data)[device_id];

  policy_table::UserConsentRecords& ucr = *(params.user_consent_records);

  // If there is no application record, all groups with consents should be
  // consented
  if (ucr.end() == ucr.find(policy_app_id)) {
    return groups_to_be_consented.size();
  }

  policy_table::ConsentRecords& cgr = ucr[policy_app_id];

  policy_table::Strings::const_iterator to_consent_it =
      groups_to_be_consented.begin();

  for (;to_consent_it != groups_to_be_consented.end(); ++to_consent_it) {
    policy_table::ConsentGroups::const_iterator already_consented_iter =
        cgr.consent_groups->find(*to_consent_it);
    if (already_consented_iter == cgr.consent_groups->end()) {
      ++result;
    }
  }

  return result;
}

bool CacheManager::SetMetaInfo(const std::string& ccpu_version,
                               const std::string& wers_country_code,
                               const std::string& language) {
  CACHE_MANAGER_CHECK(false);
  *pt_->policy_table.module_meta->ccpu_version = ccpu_version;
  *pt_->policy_table.module_meta->wers_country_code = wers_country_code;
  *pt_->policy_table.module_meta->language = language;

  // We have to set preloaded flag as false in policy table on any response
  // of GetSystemInfo (SDLAQ-CRS-2365)
  *pt_->policy_table.module_config.preloaded_pt = false;

  Backup();
  return true;
}

bool CacheManager::IsMetaInfoPresent() const {
  CACHE_MANAGER_CHECK(false);
  bool result = true;
  result = NULL != pt_->policy_table.module_meta->ccpu_version &&
      NULL != pt_->policy_table.module_meta->wers_country_code &&
      NULL != pt_->policy_table.module_meta->language;
  return result;
}

bool CacheManager::SetSystemLanguage(const std::string& language) {
  CACHE_MANAGER_CHECK(false);
  *pt_->policy_table.module_meta->language = language;
  Backup();
  return true;
}

bool CacheManager::GetFunctionalGroupNames(FunctionalGroupNames& names) {
  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK(false);
  rpc::policy_table_interface_base::FunctionalGroupings::iterator iter =
      pt_->policy_table.functional_groupings.begin();
  rpc::policy_table_interface_base::FunctionalGroupings::iterator iter_end =
      pt_->policy_table.functional_groupings.end();

  for (; iter != iter_end; ++iter) {
    const int32_t id = utils::Djb2HashFromString((*iter).first);
    std::pair<std::string, std::string> value =
        std::make_pair( *(*iter).second.user_consent_prompt, (*iter).first);

    names.insert(std::pair<uint32_t, std::pair<std::string, std::string> >(id, value));
  }
  return true;
}

bool CacheManager::CleanupUnpairedDevices() {
  CACHE_MANAGER_CHECK(false);
  sync_primitives::AutoLock lock(cache_lock_);
  sync_primitives::AutoLock lock_unpaired(unpaired_lock_);
  UnpairedDevices::iterator iter = is_unpaired_.begin();
  UnpairedDevices::const_iterator iter_end = is_unpaired_.end();
  LOG4CXX_DEBUG(logger_, "Is_unpaired size is: " << is_unpaired_.size());
  for (; iter != iter_end; ++iter) {
    // Delete device
    if (!pt_->policy_table.device_data.is_initialized()) {
      LOG4CXX_ERROR(logger_, "Device_data section is not initialized.");
      return false;
    }
    policy_table::DeviceData& device_data = *pt_->policy_table.device_data;
    policy_table::DeviceData::iterator it_device = device_data.find(*iter);
    if (device_data.end() == it_device) {
      LOG4CXX_INFO(logger_, "No device id " << *iter
                   <<  " had been found in device_data section.");
      return false;
    }

    LOG4CXX_DEBUG(logger_, "Device_data size is: " << device_data.size());
    device_data.erase(it_device);
    LOG4CXX_INFO(logger_, "Device id " << *iter
                 <<  " had been deleted from device_data section.");
    LOG4CXX_DEBUG(logger_, "Device_data size is: " << device_data.size());
  }
  is_unpaired_.clear();
  Backup();
  return true;
}

void CacheManager::Increment(usage_statistics::GlobalCounterId type) {
  CACHE_MANAGER_CHECK_VOID();
  switch (type) {
    case usage_statistics::IAP_BUFFER_FULL:
      ++(*pt_->policy_table.usage_and_error_counts->count_of_iap_buffer_full);
      break;
    case usage_statistics::SYNC_OUT_OF_MEMORY:
      ++(*pt_->policy_table.usage_and_error_counts->count_sync_out_of_memory);
      break;
    case usage_statistics::SYNC_REBOOTS:
      ++(*pt_->policy_table.usage_and_error_counts->count_of_sync_reboots);
      break;
    default:
      LOG4CXX_WARN(logger_, "Type global counter is unknown");
      return;
  }
  Backup();
}

void CacheManager::Increment(const std::string& app_id,
                             usage_statistics::AppCounterId type) {
  CACHE_MANAGER_CHECK_VOID();
  switch (type) {
    case usage_statistics::USER_SELECTIONS:
      ++(*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          count_of_user_selections;
      break;
    case usage_statistics::REJECTIONS_SYNC_OUT_OF_MEMORY:
      ++(*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          count_of_rejections_sync_out_of_memory;
      break;
    case usage_statistics::REJECTIONS_NICKNAME_MISMATCH:
      ++(*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          count_of_rejections_nickname_mismatch;
      break;
    case usage_statistics::REJECTIONS_DUPLICATE_NAME:
      ++(*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          count_of_rejections_duplicate_name;
      break;
    case usage_statistics::REJECTED_RPC_CALLS:
      ++(*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          count_of_rejected_rpc_calls;
      break;
    case usage_statistics::RPCS_IN_HMI_NONE:
      ++(*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          count_of_rpcs_sent_in_hmi_none;
      break;
    case usage_statistics::REMOVALS_MISBEHAVED:
      ++(*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          count_of_removals_for_bad_behavior;
      break;
    case usage_statistics::RUN_ATTEMPTS_WHILE_REVOKED:
      ++(*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          count_of_run_attempts_while_revoked;
      break;
    default:
      LOG4CXX_WARN(logger_, "Type app counter is unknown");
      return;
  }
  Backup();
}

void CacheManager::Set(const std::string& app_id,
                       usage_statistics::AppInfoId type,
                       const std::string& value) {
  CACHE_MANAGER_CHECK_VOID();
  switch (type) {
    case usage_statistics::LANGUAGE_GUI:
      (*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          app_registration_language_gui = value;
      break;
    case usage_statistics::LANGUAGE_VUI:
      (*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          app_registration_language_vui = value;
      break;
    default:
      LOG4CXX_WARN(logger_, "Type app info is unknown");
      return;
  }
  Backup();
}

void CacheManager::Add(const std::string& app_id,
                       usage_statistics::AppStopwatchId type,
                       int seconds) {
  CACHE_MANAGER_CHECK_VOID();
  const int minutes = ConvertSecondsToMinute(seconds);
  switch (type) {
    case usage_statistics::SECONDS_HMI_FULL:
      (*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          minutes_in_hmi_full += minutes;
      break;
    case usage_statistics::SECONDS_HMI_LIMITED:
      (*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          minutes_in_hmi_limited += minutes;
      break;
    case usage_statistics::SECONDS_HMI_BACKGROUND:
      (*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          minutes_in_hmi_background += minutes;
      break;
    case usage_statistics::SECONDS_HMI_NONE:
      (*pt_->policy_table.usage_and_error_counts->app_level)[app_id].
          minutes_in_hmi_none += minutes;
      break;
    default:
      LOG4CXX_WARN(logger_, "Type app stopwatch is unknown");
      return;
  }
  Backup();
}

long CacheManager::ConvertSecondsToMinute(int seconds) {
  const float seconds_in_minute = 60.0;
  return std::round(seconds / seconds_in_minute);
}

bool CacheManager::SetDefaultPolicy(const std::string& app_id) {
  CACHE_MANAGER_CHECK(false);
  policy_table::ApplicationPolicies::const_iterator iter =
      pt_->policy_table.app_policies_section.apps.find(kDefaultId);
  if (pt_->policy_table.app_policies_section.apps.end() != iter) {
    pt_->policy_table.app_policies_section.apps[app_id] =
        pt_->policy_table.app_policies_section.apps[kDefaultId];

    SetIsDefault(app_id);
  }
  Backup();
  return true;
}

bool CacheManager::IsDefaultPolicy(const std::string& app_id) {
  CACHE_MANAGER_CHECK(false);
  const bool result =
      pt_->policy_table.app_policies_section.apps.end() !=
      pt_->policy_table.app_policies_section.apps.find(app_id) &&
      policy::kDefaultId ==
      pt_->policy_table.app_policies_section.apps[app_id].get_string();

  return result;
}

bool CacheManager::SetIsDefault(const std::string& app_id) {
  CACHE_MANAGER_CHECK(false);
  policy_table::ApplicationPolicies::const_iterator iter =
      pt_->policy_table.app_policies_section.apps.find(app_id);
  if (pt_->policy_table.app_policies_section.apps.end() != iter) {
    pt_->policy_table.app_policies_section.apps[app_id].set_to_string(kDefaultId);
  }
  return true;
}

bool CacheManager::SetPredataPolicy(const std::string& app_id) {
  CACHE_MANAGER_CHECK(false);
  policy_table::ApplicationPolicies::const_iterator iter =
      pt_->policy_table.app_policies_section.apps.find(kPreDataConsentId);

  if (pt_->policy_table.app_policies_section.apps.end() == iter) {
    LOG4CXX_ERROR(logger_, "Could not set " << kPreDataConsentId
                  << " permissions for app " << app_id);
    return false;
  }

  pt_->policy_table.app_policies_section.apps[app_id] =
      pt_->policy_table.app_policies_section.apps[kPreDataConsentId];

  pt_->policy_table.app_policies_section.apps[app_id].set_to_string(kPreDataConsentId);

  Backup();
  return true;
}

bool CacheManager::IsPredataPolicy(const std::string& app_id) {
  // TODO(AOleynik): Maybe change for comparison with pre_DataConsent
  // permissions or check string value from get_string()
  policy_table::ApplicationParams& pre_data_app =
    pt_->policy_table.app_policies_section.apps[kPreDataConsentId];
  policy_table::ApplicationParams& specific_app =
    pt_->policy_table.app_policies_section.apps[app_id];

  policy_table::Strings res;
  std::set_intersection(pre_data_app.groups.begin(),
                        pre_data_app.groups.end(),
                        specific_app.groups.begin(),
                        specific_app.groups.end(),
                        std::back_inserter(res));

  bool is_marked_as_predata =
      kPreDataConsentId ==
      pt_->policy_table.app_policies_section.apps[app_id].get_string();

  return !res.empty() && is_marked_as_predata;
}

bool CacheManager::SetUnpairedDevice(const std::string& device_id,
                bool unpaired) {
  const bool result =
      pt_->policy_table.device_data->end() !=
      pt_->policy_table.device_data->find(device_id);
  if (!result) {
    LOG4CXX_DEBUG(logger_, "Couldn't set unpaired flag for device id "
                  << device_id << " , since it wasn't found.");
    return false;
  }

  sync_primitives::AutoLock lock(unpaired_lock_);
  if (unpaired) {
    is_unpaired_.insert(device_id);
    LOG4CXX_DEBUG(logger_, "Unpaired flag was set for device id " << device_id);
  } else {
    is_unpaired_.erase(device_id);
    LOG4CXX_DEBUG(logger_, "Unpaired flag was removed for device id " << device_id);
  }
  return result;
}

bool CacheManager::SetVINValue(const std::string& value) {
  CACHE_MANAGER_CHECK(false);
   cache_lock_.Acquire();
  *pt_->policy_table.module_meta->vin = value;
   cache_lock_.Release();
  Backup();
  return true;
}

bool CacheManager::IsApplicationRepresented(const std::string& app_id) const {
  CACHE_MANAGER_CHECK(false);
  if (kDeviceId == app_id) {
    return true;
  }
  policy_table::ApplicationPolicies::const_iterator iter =
      pt_->policy_table.app_policies_section.apps.find(app_id);
  return pt_->policy_table.app_policies_section.apps.end() != iter;
}

bool CacheManager::Init(const std::string& file_name) {
  LOG4CXX_AUTO_TRACE(logger_);

  InitResult init_result = backup_->Init();
  ex_backup_ = utils::SharedPtr<PTRepresentation>::
      dynamic_pointer_cast<PTExtRepresentation >(backup_);

  bool result = true;
  switch (init_result) {
    case InitResult::EXISTS: {
      LOG4CXX_INFO(logger_, "Policy Table exists, was loaded correctly.");
      result = LoadFromBackup();
      if (result) {
        MergePreloadPT(file_name);
      }
    } break;
    case InitResult::SUCCESS: {
      LOG4CXX_INFO(logger_, "Policy Table was inited successfully");
      result = LoadFromFile(file_name, *pt_);
      if (result) {
        Backup();
      } else {
        ex_backup_->RemoveDB();
      }
    } break;
    default: {
      result = false;
      LOG4CXX_ERROR(logger_, "Failed to init policy table.");
    } break;
  }

  return result;
}

void CacheManager::FillDeviceSpecificData() {
  DeviceIds unpaired_ids;
  ex_backup_->UnpairedDevicesList(&unpaired_ids);
  sync_primitives::AutoLock lock(unpaired_lock_);
  is_unpaired_.clear();
  for (DeviceIds::const_iterator ids_iter= unpaired_ids.begin();
       ids_iter!= unpaired_ids.end(); ++ids_iter) {
    is_unpaired_.insert(*ids_iter);
  }
}

bool CacheManager::LoadFromBackup() {
  sync_primitives::AutoLock lock(cache_lock_);
  pt_ = backup_->GenerateSnapshot();
  update_required = backup_->UpdateRequired();

  FillDeviceSpecificData();

  return true;
}

bool CacheManager::LoadFromFile(const std::string& file_name,
                                policy_table::Table& table) {
  LOG4CXX_AUTO_TRACE(logger_);
  BinaryMessage json_string;
  if (!file_system::ReadBinaryFile(file_name, json_string)) {
    LOG4CXX_FATAL(logger_, "Failed to read pt file.");
    return false;
  }

  Json::Value value;
  Json::Reader reader(Json::Features::strictMode());
  std::string json(json_string.begin(), json_string.end());
  if (!reader.parse(json.c_str(), value)) {
    LOG4CXX_FATAL(
        logger_,
        "Preloaded PT is corrupted: " << reader.getFormattedErrorMessages());
    return false;
  }

  LOG4CXX_TRACE(logger_, "Start create PT");
  sync_primitives::AutoLock locker(cache_lock_);

  table = policy_table::Table(&value);

  Json::StyledWriter s_writer;
  LOG4CXX_DEBUG(logger_, "PT out:");
  LOG4CXX_DEBUG(logger_, s_writer.write(table.ToJsonValue()));

  if (!table.is_valid()) {
    rpc::ValidationReport report("policy_table");
    table.ReportErrors(&report);
    LOG4CXX_FATAL(logger_,
                  "Parsed table is not valid " << rpc::PrettyFormat(report));
    return false;
  }
  return true;
}

bool CacheManager::ResetPT(const std::string& file_name) {
  bool result = true;
  is_unpaired_.clear();
  backup_->RefreshDB();
  result = LoadFromFile(file_name, *pt_);
  Backup();
  return result;
}

bool CacheManager::AppExists(const std::string& app_id) const {
  CACHE_MANAGER_CHECK(false);
  if (kDeviceId == app_id) {
    return true;
  }
  policy_table::ApplicationPolicies::iterator policy_iter =
      pt_->policy_table.app_policies_section.apps.find(app_id);
  return pt_->policy_table.app_policies_section.apps.end() != policy_iter;
}

void CacheManager::GetAppRequestTypes(
    const std::string& policy_app_id,
    std::vector<std::string>& request_types) const {
  LOG4CXX_AUTO_TRACE(logger_);
  CACHE_MANAGER_CHECK_VOID();
  policy_table::ApplicationPolicies::iterator policy_iter =
      pt_->policy_table.app_policies_section.apps.find(policy_app_id);
  if (pt_->policy_table.app_policies_section.apps.end() == policy_iter) {
    LOG4CXX_DEBUG(logger_, "Can't find request types for app_id "
                  << policy_app_id);
    return;
  }
  if (policy_iter->second.RequestType.is_initialized()) {
    policy_table::RequestTypes::iterator it_request_type =
        policy_iter->second.RequestType->begin();
    for (;it_request_type != policy_iter->second.RequestType->end();
         ++it_request_type) {
      request_types.push_back(EnumToJsonString(*it_request_type));
    }
  }
  return;
}

void CacheManager::MergePreloadPT(const std::string& file_name) {
  LOG4CXX_AUTO_TRACE(logger_);
  policy_table::Table table;
  if (!LoadFromFile(file_name, table)) {
    LOG4CXX_DEBUG(logger_, "Unable to load preloaded PT.");
    return;
  }

  sync_primitives::AutoLock lock(cache_lock_);
  policy_table::PolicyTable& current = pt_->policy_table;
  policy_table::PolicyTable& new_table = table.policy_table;
  const std::string date_current = *current.module_config.preloaded_date;
  const std::string date_new = *new_table.module_config.preloaded_date;
  if (date_current != date_new) {
    MergeMC(new_table, current);
    MergeFG(new_table, current);
    MergeAP(new_table, current);
    MergeCFM(new_table, current);
    Backup();
  }
}

void CacheManager::MergeMC(const policy_table::PolicyTable& new_pt,
                           policy_table::PolicyTable& pt) {
  LOG4CXX_AUTO_TRACE(logger_);
  policy_table::ModuleConfig copy(pt.module_config);

  pt.module_config = new_pt.module_config;
  pt.module_config.vehicle_make = copy.vehicle_make;
  pt.module_config.vehicle_year = copy.vehicle_year;
  pt.module_config.vehicle_model = copy.vehicle_model;
}

void CacheManager::MergeFG(const policy_table::PolicyTable& new_pt,
                           policy_table::PolicyTable& pt) {
  LOG4CXX_AUTO_TRACE(logger_);
  policy_table::FunctionalGroupings::const_iterator it = new_pt.functional_groupings.begin();

  for (; it != new_pt.functional_groupings.end(); ++it) {
    LOG4CXX_DEBUG(logger_, "Merge functional group: " << it->first);
    pt.functional_groupings[it->first] = it->second;
  }
}

void CacheManager::MergeAP(const policy_table::PolicyTable& new_pt,
                           policy_table::PolicyTable& pt) {
  LOG4CXX_AUTO_TRACE(logger_);
  pt.app_policies_section.device = const_cast<policy_table::PolicyTable&>
      (new_pt).app_policies_section.device;

  pt.app_policies_section.apps[kDefaultId] =
      const_cast<policy_table::PolicyTable&>
      (new_pt).app_policies_section.apps[kDefaultId];

  pt.app_policies_section.apps[kPreDataConsentId] =
      const_cast<policy_table::PolicyTable&>
      (new_pt).app_policies_section.apps[kPreDataConsentId];
}

void CacheManager::MergeCFM(const policy_table::PolicyTable& new_pt,
                            policy_table::PolicyTable& pt) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (new_pt.consumer_friendly_messages.is_initialized()) {
    if (!pt.consumer_friendly_messages.is_initialized()) {
      pt.consumer_friendly_messages = new_pt.consumer_friendly_messages;
    } else {
      policy_table::Messages::const_iterator it = new_pt.consumer_friendly_messages->messages->begin();

      pt.consumer_friendly_messages->version =
          new_pt.consumer_friendly_messages->version;
      for (; it != new_pt.consumer_friendly_messages->messages->end(); ++it) {
        LOG4CXX_DEBUG(logger_, "Merge CFM: " << it->first);
        if (!(pt.consumer_friendly_messages->messages.is_initialized())) {
          LOG4CXX_DEBUG(logger_, "CFM not initialized.");
        }
        (*pt.consumer_friendly_messages->messages)[it->first] = it->second;
      }

    }
  }
}

CacheManager::BackgroundBackuper::BackgroundBackuper(CacheManager* cache_manager)
  : cache_manager_(cache_manager),
    stop_flag_(false),
    new_data_available_(false) {
  LOG4CXX_AUTO_TRACE(logger_);
}

CacheManager::BackgroundBackuper::~BackgroundBackuper() {
  LOG4CXX_AUTO_TRACE(logger_);
}

void CacheManager::BackgroundBackuper::InternalBackup() {
  LOG4CXX_AUTO_TRACE(logger_);
  DCHECK(cache_manager_);

  while (new_data_available_) {
    new_data_available_ = false;
    LOG4CXX_DEBUG(logger_, "DoBackup");
    cache_manager_->PersistData();
  }
}

void CacheManager::BackgroundBackuper::threadMain() {
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock lock(need_backup_lock_);
  while (!stop_flag_) {
    need_backup_lock_.Release();
    InternalBackup();
    need_backup_lock_.Acquire();
    if (new_data_available_ || stop_flag_) {
      continue;
    }
    LOG4CXX_DEBUG(logger_, "Wait for a next backup");
    backup_notifier_.Wait(need_backup_lock_);
  }
}

void CacheManager::BackgroundBackuper::exitThreadMain() {
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock auto_lock(need_backup_lock_);
  stop_flag_ = true;
  backup_notifier_.NotifyOne();
}

void CacheManager::BackgroundBackuper::DoBackup() {
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock auto_lock(need_backup_lock_);
  new_data_available_ = true;
  backup_notifier_.NotifyOne();
}

}  // namespace policy
