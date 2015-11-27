/* Copyright (c) 2015, Ford Motor Company
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

#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include <string>
#include <iterator>
#include <iostream>
#include "gtest/gtest.h"
#include "policy/sql_pt_ext_representation.h"
#include "utils/gen_hash.h"
#include "utils/file_system.h"
#include "sqlite_wrapper/sql_query.h"
#include "rpc_base/rpc_base.h"
#include "policy_table/table_struct_ext/types.h"

using namespace ::policy;
namespace policy_table = rpc::policy_table_interface_base;
using std::string;
using std::map;
using std::pair;
using std::vector;

namespace test {
namespace components {
namespace policy {

class SQLPTExtRepresentationTest : public ::testing::Test {
 public:
  // Collection of pairs of group alias and corresponding group name
  typedef vector<pair<string, string> > GroupsAliasNameCollection;

  SQLPTExtRepresentationTest() : reps(0) {}

 protected:
  SQLPTExtRepresentation* reps;
  static const string kDatabaseName;
  PermissionConsent perm_consent;
  FunctionalGroupPermission group1_perm;
  FunctionalGroupPermission group2_perm;
  utils::dbms::SQLQuery* query_wrapper_;
  static const bool in_memory_;

  void SetUp() OVERRIDE {
    file_system::DeleteFile(kDatabaseName);
    reps = new SQLPTExtRepresentation(in_memory_);
    ASSERT_TRUE (reps != NULL);
    ASSERT_EQ(SUCCESS, reps->Init());
    query_wrapper_ = new utils::dbms::SQLQuery(reps->db());
    ASSERT_TRUE (query_wrapper_ != NULL);
  }

  void TearDown() OVERRIDE {
    delete query_wrapper_;
    EXPECT_TRUE(reps->Drop());
    EXPECT_TRUE(reps->Close());
    delete reps;
  }

  void FillGroupPermission(
      vector<FunctionalGroupPermission>& groups_permissions,
      FunctionalGroupPermission group,
      const GroupsAliasNameCollection& groups_names, GroupConsent state) {
    GroupsAliasNameCollection::const_iterator groups_names_it =
        groups_names.begin();
    while (groups_names_it != groups_names.end()) {
      group.group_alias = groups_names_it->first;
      group.group_name = groups_names_it->second;
      group.group_id = ::utils::Djb2HashFromString(groups_names_it->second);
      group.state = state;
      groups_permissions.push_back(group);
      ++groups_names_it;
    }
  }

  void FillPermissionStruct(
      const string& dev_id, const string& app_id, const string& consent_source,
      const GroupsAliasNameCollection& allowed_groups_names,
      const GroupsAliasNameCollection& disallowed_groups_names) {
    // Arrange
    vector<FunctionalGroupPermission> groups_permissions;
    perm_consent.device_id = dev_id;
    perm_consent.policy_app_id = app_id;
    perm_consent.consent_source = consent_source;
    // Fill groups
    FillGroupPermission(groups_permissions, group1_perm, allowed_groups_names,
                        GroupConsent::kGroupAllowed);
    FillGroupPermission(groups_permissions, group2_perm,
                        disallowed_groups_names,
                        GroupConsent::kGroupDisallowed);
    perm_consent.group_permissions = groups_permissions;
  }

  bool Check(const GroupsAliasNameCollection& groups_names,
             const vector<int32_t>& group_Ids) {
    vector<int32_t>::const_iterator group_Ids_it = group_Ids.begin();
    GroupsAliasNameCollection::const_iterator group_alias_name_it =
        groups_names.begin();

    EXPECT_EQ(groups_names.size(), group_Ids.size());
    while (group_Ids_it != group_Ids.end()) {
      while (group_alias_name_it != groups_names.end()) {
        if (group_Ids.end() != std::find(group_Ids.begin(), group_Ids.end(),
                                         ::utils::Djb2HashFromString(
                                             group_alias_name_it->second))) {
          return true;
        }
        ++group_alias_name_it;
      }
      ++group_Ids_it;
    }
    return false;
  }

  bool CheckGroupTypesExist(
      const FunctionalIdType& group_types,
      const GroupsAliasNameCollection& allowed_groups_names,
      const GroupsAliasNameCollection& disallowed_groups_names) {
    bool result = true;
    map<GroupType, FunctionalGroupIDs>::const_iterator functional_id_type_it1 =
        group_types.find(GroupType::kTypeAllowed);
    map<GroupType, FunctionalGroupIDs>::const_iterator functional_id_type_it2 =
        group_types.find(GroupType::kTypeDisallowed);

    if (functional_id_type_it1 == group_types.end() ||
        functional_id_type_it2 == group_types.end()) {
      return false;
    }

    const std::vector<int32_t>& allowedGroupIDs =
        functional_id_type_it1->second;
    const std::vector<int32_t>& disallowedGroupIDs =
        functional_id_type_it2->second;
    if (0u == allowedGroupIDs.size() && 0u == disallowedGroupIDs.size()) {
      result = false;
    } else if (0u != allowedGroupIDs.size() &&
               0u == disallowedGroupIDs.size()) {
      result = Check(allowed_groups_names, allowedGroupIDs);
    } else if (0u != disallowedGroupIDs.size() &&
               0u == allowedGroupIDs.size()) {
      result = Check(disallowed_groups_names, disallowedGroupIDs);
    } else if (0u != allowedGroupIDs.size() &&
               0u != disallowedGroupIDs.size()) {
      result = Check(allowed_groups_names, allowedGroupIDs) &&
               Check(disallowed_groups_names, disallowedGroupIDs);
    } else {
      result = false;
    }
    return result;
  }
};

const string SQLPTExtRepresentationTest::kDatabaseName = ":memory:";
const bool SQLPTExtRepresentationTest::in_memory_ = true;

::testing::AssertionResult IsValid(const policy_table::Table& table) {
  if (table.is_valid()) {
    return ::testing::AssertionSuccess();
  } else {
    ::rpc::ValidationReport report(" - table");
    table.ReportErrors(&report);
    return ::testing::AssertionFailure() << ::rpc::PrettyFormat(report);
  }
}

TEST_F(SQLPTExtRepresentationTest,
       GenerateSnapshot_SetPolicyTable_SnapshotIsPresent) {
  // Arrange
  Json::Value table(Json::objectValue);
  table["policy_table"] = Json::Value(Json::objectValue);

  Json::Value& policy_table = table["policy_table"];
  policy_table["module_config"] = Json::Value(Json::objectValue);
  policy_table["functional_groupings"] = Json::Value(Json::objectValue);
  policy_table["consumer_friendly_messages"] = Json::Value(Json::objectValue);
  policy_table["app_policies"] = Json::Value(Json::objectValue);

  Json::Value& module_config = policy_table["module_config"];
  module_config["preloaded_date"] = Json::Value("");
  module_config["preloaded_pt"] = Json::Value(true);
  module_config["exchange_after_x_ignition_cycles"] = Json::Value(10);
  module_config["exchange_after_x_kilometers"] = Json::Value(100);
  module_config["exchange_after_x_days"] = Json::Value(5);
  module_config["timeout_after_x_seconds"] = Json::Value(500);
  module_config["seconds_between_retries"] = Json::Value(Json::arrayValue);
  module_config["seconds_between_retries"][0] = Json::Value(10);
  module_config["seconds_between_retries"][1] = Json::Value(20);
  module_config["seconds_between_retries"][2] = Json::Value(30);
  module_config["endpoints"] = Json::Value(Json::objectValue);
  module_config["endpoints"]["0x00"] = Json::Value(Json::objectValue);
  module_config["endpoints"]["0x00"]["default"] = Json::Value(Json::arrayValue);
  module_config["endpoints"]["0x00"]["default"][0] =
      Json::Value("http://ford.com/cloud/default");
  module_config["notifications_per_minute_by_priority"] =
      Json::Value(Json::objectValue);
  module_config["notifications_per_minute_by_priority"]["emergency"] =
      Json::Value(1);
  module_config["notifications_per_minute_by_priority"]["navigation"] =
      Json::Value(2);
  module_config["notifications_per_minute_by_priority"]["VOICECOMM"] =
      Json::Value(3);
  module_config["notifications_per_minute_by_priority"]["communication"] =
      Json::Value(4);
  module_config["notifications_per_minute_by_priority"]["normal"] =
      Json::Value(5);
  module_config["notifications_per_minute_by_priority"]["none"] =
      Json::Value(6);
  module_config["vehicle_make"] = Json::Value("MakeT");
  module_config["vehicle_model"] = Json::Value("ModelT");
  module_config["vehicle_year"] = Json::Value("2014");
  module_config["certificate"] = Json::Value("my_cert");

  Json::Value& functional_groupings = policy_table["functional_groupings"];
  functional_groupings["default"] = Json::Value(Json::objectValue);
  Json::Value& default_group = functional_groupings["default"];
  default_group["rpcs"] = Json::Value(Json::objectValue);
  default_group["rpcs"]["Update"] = Json::Value(Json::objectValue);
  default_group["rpcs"]["Update"]["hmi_levels"] = Json::Value(Json::arrayValue);
  default_group["rpcs"]["Update"]["hmi_levels"][0] = Json::Value("FULL");
  default_group["rpcs"]["Update"]["parameters"] = Json::Value(Json::arrayValue);
  default_group["rpcs"]["Update"]["parameters"][0] = Json::Value("speed");

  Json::Value& consumer_friendly_messages =
      policy_table["consumer_friendly_messages"];
  consumer_friendly_messages["version"] = Json::Value("1.2");
  consumer_friendly_messages["messages"] = Json::Value(Json::objectValue);
  consumer_friendly_messages["messages"]["MSG1"] =
      Json::Value(Json::objectValue);
  Json::Value& msg1 = consumer_friendly_messages["messages"]["MSG1"];
  msg1["languages"] = Json::Value(Json::objectValue);
  msg1["languages"]["en-us"] = Json::Value(Json::objectValue);
  msg1["languages"]["en-us"]["tts"] = Json::Value("TTS message");
  msg1["languages"]["en-us"]["label"] = Json::Value("LABEL message");
  msg1["languages"]["en-us"]["line1"] = Json::Value("LINE1 message");
  msg1["languages"]["en-us"]["line2"] = Json::Value("LINE2 message");
  msg1["languages"]["en-us"]["textBody"] = Json::Value("TEXTBODY message");

  Json::Value& app_policies = policy_table["app_policies"];
  app_policies["default"] = Json::Value(Json::objectValue);
  app_policies["default"]["memory_kb"] = Json::Value(50);
  app_policies["default"]["heart_beat_timeout_ms"] = Json::Value(100);
  app_policies["default"]["groups"] = Json::Value(Json::arrayValue);
  app_policies["default"]["groups"][0] = Json::Value("default");
  app_policies["default"]["priority"] = Json::Value("EMERGENCY");
  app_policies["default"]["default_hmi"] = Json::Value("FULL");
  app_policies["default"]["keep_context"] = Json::Value(true);
  app_policies["default"]["steal_focus"] = Json::Value(true);
  app_policies["pre_DataConsent"] = Json::Value(Json::objectValue);
  app_policies["pre_DataConsent"]["memory_kb"] = Json::Value(50);
  app_policies["pre_DataConsent"]["heart_beat_timeout_ms"] = Json::Value(100);
  app_policies["pre_DataConsent"]["groups"] = Json::Value(Json::arrayValue);
  app_policies["pre_DataConsent"]["groups"][0] = Json::Value("default");
  app_policies["pre_DataConsent"]["priority"] = Json::Value("EMERGENCY");
  app_policies["pre_DataConsent"]["default_hmi"] = Json::Value("FULL");
  app_policies["pre_DataConsent"]["keep_context"] = Json::Value(true);
  app_policies["pre_DataConsent"]["steal_focus"] = Json::Value(true);
  app_policies["1234"] = Json::Value(Json::objectValue);
  app_policies["1234"]["memory_kb"] = Json::Value(50);
  app_policies["1234"]["heart_beat_timeout_ms"] = Json::Value(100);
  app_policies["1234"]["groups"] = Json::Value(Json::arrayValue);
  app_policies["1234"]["groups"][0] = Json::Value("default");
  app_policies["1234"]["priority"] = Json::Value("EMERGENCY");
  app_policies["1234"]["default_hmi"] = Json::Value("FULL");
  app_policies["1234"]["keep_context"] = Json::Value(true);
  app_policies["1234"]["steal_focus"] = Json::Value(true);
  app_policies["device"] = Json::Value(Json::objectValue);
  app_policies["device"]["groups"] = Json::Value(Json::arrayValue);
  app_policies["device"]["groups"][0] = Json::Value("default");
  app_policies["device"]["priority"] = Json::Value("EMERGENCY");
  app_policies["device"]["default_hmi"] = Json::Value("FULL");
  app_policies["device"]["keep_context"] = Json::Value(true);
  app_policies["device"]["steal_focus"] = Json::Value(true);

  policy_table::Table update(&table);
  update.SetPolicyTableType(rpc::policy_table_interface_base::PT_UPDATE);

  // Assert
  ASSERT_TRUE(IsValid(update));
  ASSERT_TRUE(reps->Save(update));

  // Act
  utils::SharedPtr<policy_table::Table> snapshot = reps->GenerateSnapshot();
  snapshot->SetPolicyTableType(rpc::policy_table_interface_base::PT_SNAPSHOT);

  policy_table["module_meta"] = Json::Value(Json::objectValue);
  policy_table["usage_and_error_counts"] = Json::Value(Json::objectValue);
  policy_table["device_data"] = Json::Value(Json::objectValue);

  Json::Value& module_meta = policy_table["module_meta"];
  module_meta["ccpu_version"] = Json::Value("");
  module_meta["language"] = Json::Value("");
  module_meta["wers_country_code"] = Json::Value("");
  module_meta["pt_exchanged_at_odometer_x"] = Json::Value(0);
  module_meta["pt_exchanged_x_days_after_epoch"] = Json::Value(0);
  module_meta["ignition_cycles_since_last_exchange"] = Json::Value(0);
  module_meta["vin"] = Json::Value("");

  Json::Value& usage_and_error_counts = policy_table["usage_and_error_counts"];
  usage_and_error_counts["count_of_iap_buffer_full"] = Json::Value(0);
  usage_and_error_counts["count_sync_out_of_memory"] = Json::Value(0);
  usage_and_error_counts["count_of_sync_reboots"] = Json::Value(0);

  policy_table::Table expected(&table);

  // Assert
  EXPECT_EQ(expected.ToJsonValue().toStyledString(),
            snapshot->ToJsonValue().toStyledString());
}

TEST_F(
    SQLPTExtRepresentationTest,
    CanAppKeepContext_InsertKeepContext_ExpectValuesThatSetInKeepContextParams) {
  // Arrange
  const std::string query_delete = "DELETE FROM `application`; ";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_delete));
  // Act
  const std::string query_insert_12345 =
      "INSERT INTO `application` (`id`, `memory_kb`,"
      " `heart_beat_timeout_ms`, `keep_context`) VALUES ('12345', 5, 10, 1)";

  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_12345));
  EXPECT_FALSE(reps->CanAppKeepContext("0"));
  EXPECT_TRUE(reps->CanAppKeepContext("12345"));
  // Act
  const std::string query_insert_123 =
      "INSERT INTO `application` (`id`, `memory_kb`,"
      " `heart_beat_timeout_ms`, `keep_context`) VALUES ('123', 10, 7, 0)";

  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_123));
  EXPECT_FALSE(reps->CanAppKeepContext("123"));
}

TEST_F(SQLPTExtRepresentationTest,
       CanAppStealFocus_SetStealFocus_ExpectValuesThatSetInStealFocusParam) {
  // Arrange
  const std::string query_delete = "DELETE FROM `application`; ";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_delete));
  // Act
  const std::string query_insert_12345 =
      "INSERT INTO `application` (`id`, `memory_kb`,"
      " `heart_beat_timeout_ms`, `steal_focus`) VALUES ('12345', 5, 10, 1)";

  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_12345));
  EXPECT_TRUE(reps->CanAppStealFocus("12345"));
  EXPECT_FALSE(reps->CanAppStealFocus("0"));
  // Act
  const std::string query_insert_123 =
      "INSERT INTO `application` (`id`, `memory_kb`,"
      " `heart_beat_timeout_ms`, `steal_focus`) VALUES ('123', 10, 7, 0)";

  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_123));
  EXPECT_FALSE(reps->CanAppStealFocus("123"));
}

TEST_F(SQLPTExtRepresentationTest,
       GetDefaultHMI_SetHMI_ExpectValuesThatSetInHMIParam) {
  // Arrange
  const std::string query_delete = "DELETE FROM `application`; ";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_delete));
  // Act
  const std::string query_insert_12345 =
      "INSERT INTO `application` (`id`, `memory_kb`,"
      " `heart_beat_timeout_ms`, `default_hmi`) VALUES ('12345', 5, 10, "
      "'NONE')";
  std::string result;
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_12345));
  EXPECT_TRUE(reps->GetDefaultHMI("12345", &result));
  EXPECT_EQ("NONE", result);
  const std::string query_insert_123 =
      "INSERT INTO `application` (`id`, `memory_kb`,"
      " `heart_beat_timeout_ms`, `default_hmi`) VALUES ('123', 5, 10, "
      "'LIMITED')";

  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_123));
  EXPECT_TRUE(reps->GetDefaultHMI("123", &result));
  EXPECT_EQ("LIMITED", result);
}

TEST_F(SQLPTExtRepresentationTest,
       GetUserPermissionsForDevice_SetPermissions_ExpectValuesThatSetInParams) {
  // Arrange
  StringArray allowed_groups;
  StringArray disallowed_groups;
  EXPECT_TRUE(reps->GetUserPermissionsForDevice("XXX12345ZZZ", &allowed_groups,
                                                &disallowed_groups));
  EXPECT_EQ(0u, allowed_groups.size());
  EXPECT_EQ(0u, disallowed_groups.size());

  const std::string query_delete = "DELETE FROM `device_consent_group`; ";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_delete));
  // Act
  const std::string query_insert_DataConsent =
      "INSERT INTO `device_consent_group` (`device_id`, "
      "`functional_group_id`,'is_consented', `input`, `time_stamp`) VALUES "
      "('XXX12345ZZZ', 'DataConsent-2', 1,'GUI', '2014-01-01T00:00:52Z')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_DataConsent));
  const std::string query_insert_Navigation =
      "INSERT INTO `device_consent_group` (`device_id`, "
      "`functional_group_id`,'is_consented', `input`, `time_stamp`) VALUES "
      "('XXX12345ZZZ', 'Navigation-1', 0,'GUI', '2015-01-01T00:00:52Z')";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_Navigation));
  EXPECT_TRUE(reps->GetUserPermissionsForDevice("XXX12345ZZZ", &allowed_groups,
                                                &disallowed_groups));
  EXPECT_EQ(1u, allowed_groups.size());
  EXPECT_EQ(1u, disallowed_groups.size());
}

TEST_F(SQLPTExtRepresentationTest,
       SetUserPermissionsForDevice_SetPermissions_ExpectValuesThatSetInParams) {
  // Arrange
  StringArray allowed_groups;
  StringArray disallowed_groups;
  EXPECT_TRUE(reps->GetUserPermissionsForDevice("XXX12345ZZZ", &allowed_groups,
                                                &disallowed_groups));
  EXPECT_EQ(0u, allowed_groups.size());
  EXPECT_EQ(0u, disallowed_groups.size());
  allowed_groups.push_back("DataConsent-2");
  disallowed_groups.push_back("Navigation-1");
  EXPECT_TRUE(reps->SetUserPermissionsForDevice("XXX12345ZZZ", allowed_groups,
                                                disallowed_groups));

  allowed_groups.clear();
  disallowed_groups.clear();
  // Act
  EXPECT_TRUE(reps->GetUserPermissionsForDevice("XXX12345ZZZ", &allowed_groups,
                                                &disallowed_groups));
  // Checks
  EXPECT_EQ(1u, allowed_groups.size());
  EXPECT_EQ(1u, disallowed_groups.size());
  EXPECT_TRUE(std::find(allowed_groups.begin(), allowed_groups.end(),
                        "DataConsent-2") != allowed_groups.end());
  EXPECT_TRUE(std::find(disallowed_groups.begin(), disallowed_groups.end(),
                        "Navigation-1") != disallowed_groups.end());
}

TEST_F(SQLPTExtRepresentationTest,
       ResetDeviceConsents_SetConsentThenReset_ExpectValuesReset) {
  // Arrange
  StringArray allowed_groups;
  StringArray disallowed_groups;
  EXPECT_TRUE(reps->GetUserPermissionsForDevice("XXX12345ZZZ", &allowed_groups,
                                                &disallowed_groups));
  EXPECT_EQ(0u, allowed_groups.size());
  EXPECT_EQ(0u, disallowed_groups.size());
  allowed_groups.push_back("DataConsent-2");
  disallowed_groups.push_back("Navigation-1");
  EXPECT_TRUE(reps->SetUserPermissionsForDevice("XXX12345ZZZ", allowed_groups,
                                                disallowed_groups));

  allowed_groups.clear();
  disallowed_groups.clear();
  // Act
  EXPECT_TRUE(reps->GetUserPermissionsForDevice("XXX12345ZZZ", &allowed_groups,
                                                &disallowed_groups));
  // Checks
  EXPECT_EQ(1u, allowed_groups.size());
  EXPECT_EQ(1u, disallowed_groups.size());
  EXPECT_TRUE(std::find(allowed_groups.begin(), allowed_groups.end(),
                        "DataConsent-2") != allowed_groups.end());
  EXPECT_TRUE(std::find(disallowed_groups.begin(), disallowed_groups.end(),
                        "Navigation-1") != disallowed_groups.end());
  allowed_groups.clear();
  disallowed_groups.clear();
  // Act
  reps->ResetDeviceConsents();
  EXPECT_TRUE(reps->GetUserPermissionsForDevice("XXX12345ZZZ", &allowed_groups,
                                                &disallowed_groups));
  // Checks
  EXPECT_EQ(0u, allowed_groups.size());
  EXPECT_EQ(0u, disallowed_groups.size());
}

TEST_F(SQLPTExtRepresentationTest,
       GetPermissionsForApp_SetPermissions_ExpectValuesThatSetInParams) {
  // Arrange
  const std::string query_insert_input_0 =
      "INSERT INTO `consent_group` (`device_id`, 'application_id' , "
      "`functional_group_id`, 'is_consented', `input`, `time_stamp`) VALUES "
      "('XXX12345ZZZ', '12345', 414812216, 1,'GUI', "
      "'2014-01-01T00:00:52Z')";

  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_input_0));
  const std::string query_insert_input_1 =
      "INSERT INTO `consent_group` (`device_id`, 'application_id' , "
      "`functional_group_id`, 'is_consented', `input`, `time_stamp`) VALUES "
      "('XXX12345ZZZ', '12345', 686787169, 0,'GUI', "
      "'2014-01-01T00:00:52Z')";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_input_1));
  GroupsAliasNameCollection allowed_groups;
  allowed_groups.push_back(std::make_pair("DataConsent", "DataConsent-2"));

  GroupsAliasNameCollection disallowed_groups;
  disallowed_groups.push_back(std::make_pair("", "Base-4"));
  FillPermissionStruct("", "", "", allowed_groups, disallowed_groups);
  FunctionalIdType group_types;
  ASSERT_TRUE(reps->GetPermissionsForApp("XXX12345ZZZ", "12345", &group_types));
  EXPECT_TRUE(
      CheckGroupTypesExist(group_types, allowed_groups, disallowed_groups));
}

TEST_F(SQLPTExtRepresentationTest,
       SetUserPermissionsForApp_SetPermissions_ExpectValuesThatSetInParams) {
  // Arrange
  GroupsAliasNameCollection allowed_groups;
  allowed_groups.push_back(std::make_pair("Notifications", "Notifications"));

  GroupsAliasNameCollection disallowed_groups;
  disallowed_groups.push_back(std::make_pair("DataConsent", "DataConsent-2"));
  FillPermissionStruct("XXX12345ZZZ", "12345", "VR", allowed_groups,
                       disallowed_groups);
  EXPECT_TRUE(reps->SetUserPermissionsForApp(perm_consent));

  FunctionalIdType group_types;
  ASSERT_TRUE(reps->GetPermissionsForApp("XXX12345ZZZ", "12345", &group_types));
  EXPECT_TRUE(
      CheckGroupTypesExist(group_types, allowed_groups, disallowed_groups));
}

TEST_F(SQLPTExtRepresentationTest,
       ResetAppConsents_SetPermissionsThenReset_ExpectValuesReset) {
  // Arrange
  GroupsAliasNameCollection allowed_groups;
  allowed_groups.push_back(std::make_pair("Notifications", "Notifications"));

  GroupsAliasNameCollection disallowed_groups;
  disallowed_groups.push_back(std::make_pair("DataConsent", "DataConsent-2"));
  FillPermissionStruct("XXX12345ZZZ", "12345", "VR", allowed_groups,
                       disallowed_groups);
  EXPECT_TRUE(reps->SetUserPermissionsForApp(perm_consent));

  FunctionalIdType group_types;
  ASSERT_TRUE(reps->GetPermissionsForApp("XXX12345ZZZ", "12345", &group_types));
  EXPECT_TRUE(
      CheckGroupTypesExist(group_types, allowed_groups, disallowed_groups));

  reps->ResetAppConsents();
  ASSERT_TRUE(reps->GetPermissionsForApp("XXX12345ZZZ", "12345", &group_types));
  EXPECT_FALSE(
      CheckGroupTypesExist(group_types, allowed_groups, disallowed_groups));
}

TEST_F(SQLPTExtRepresentationTest,
       ResetUserConsent_SetConsentThenReset_ExpectValuesReset) {
  // Arrange
  GroupsAliasNameCollection perm_allowed_groups;
  perm_allowed_groups.push_back(
      std::make_pair("Notifications", "Notifications"));

  GroupsAliasNameCollection perm_disallowed_groups;
  perm_disallowed_groups.push_back(
      std::make_pair("DataConsent", "DataConsent-2"));
  FillPermissionStruct("XXX12345ZZZ", "12345", "VR", perm_allowed_groups,
                       perm_disallowed_groups);
  // Set permissions for app
  EXPECT_TRUE(reps->SetUserPermissionsForApp(perm_consent));

  FunctionalIdType group_types;
  ASSERT_TRUE(reps->GetPermissionsForApp("XXX12345ZZZ", "12345", &group_types));
  EXPECT_TRUE(CheckGroupTypesExist(group_types, perm_allowed_groups,
                                   perm_disallowed_groups));

  StringArray allowed_groups;
  StringArray disallowed_groups;
  allowed_groups.push_back("DataConsent-2");
  disallowed_groups.push_back("Navigation-1");
  // Set permissions for device
  EXPECT_TRUE(reps->SetUserPermissionsForDevice("XXX12345ZZZ", allowed_groups,
                                                disallowed_groups));

  allowed_groups.clear();
  disallowed_groups.clear();
  // Act
  reps->ResetUserConsent();
  EXPECT_TRUE(reps->GetUserPermissionsForDevice("XXX12345ZZZ", &allowed_groups,
                                                &disallowed_groups));
  // Checks
  EXPECT_EQ(0u, allowed_groups.size());
  EXPECT_EQ(0u, disallowed_groups.size());
  EXPECT_TRUE(reps->GetPermissionsForApp("XXX12345ZZZ", "12345", &group_types));
  EXPECT_FALSE(CheckGroupTypesExist(group_types, perm_allowed_groups,
                                    perm_disallowed_groups));
}

TEST_F(SQLPTExtRepresentationTest,
       GetDeviceGroupsFromPolicies_SetGroups_ExpectValuesThatSetInParams) {
  // Arrange
  const std::string query_insert_app_group_1 =
      "INSERT INTO `app_group` (`application_id`, 'functional_group_id') "
      "VALUES ('device', '414812216')";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_app_group_1));

  const std::string query_insert_DataConsent =
      "INSERT INTO `functional_group` (`id`, 'user_consent_prompt', 'name') "
      "VALUES ('414812216', 'DataConsent', 'DataConsent-2')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_DataConsent));

  const std::string query_insert_app_group_2 =
      "INSERT INTO `app_group` (`application_id`, 'functional_group_id') "
      "VALUES ('device', 1809526495)";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_app_group_2));

  const std::string query_insert_Notifications =
      "INSERT INTO `functional_group` (`id`, 'user_consent_prompt', 'name') "
      "VALUES (1809526495, 'Notifications', 'Notifications')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_Notifications));

  const std::string query_insert_preconsented_group =
      "INSERT INTO `preconsented_group` (`application_id`, "
      "`functional_group_id`) "
      "VALUES ('device', 686787169)";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_preconsented_group));

  const std::string query_insert_Base =
      "INSERT INTO `functional_group` (`id`, 'user_consent_prompt', 'name') "
      "VALUES (686787169, '', 'Base-4')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_Base));

  policy_table::Strings groups;
  policy_table::Strings preconsented_groups;

  EXPECT_TRUE(reps->GetDeviceGroupsFromPolicies(&groups, &preconsented_groups));
  EXPECT_EQ(2u, groups.size());
  EXPECT_EQ(1u, preconsented_groups.size());

  const uint32_t min_size = 1u;
  const uint32_t max_size = 255u;

  rpc::String<min_size, max_size> temp1("DataConsent-2");
  rpc::String<min_size, max_size> temp2("Notifications");
  rpc::String<min_size, max_size> temp3("Base-4");

  EXPECT_NE(groups.end(), std::find(groups.begin(), groups.end(), temp1));
  EXPECT_NE(groups.end(), std::find(groups.begin(), groups.end(), temp2));
  EXPECT_NE(
      preconsented_groups.end(),
      std::find(preconsented_groups.begin(), preconsented_groups.end(), temp3));
}

TEST_F(SQLPTExtRepresentationTest,
       SetDeviceData_SetDeviceData_ExpectValuesThatSetInParams) {
  // Arrange
  utils::dbms::SQLQuery query(reps->db());
  reps->SetDeviceData("08-00-27-CE-76-FE", "hardware IPX", "v.8.0.1", "Android",
                      "4.4.2", "Life", 2, "Bluetooth");
  const std::string query_select_hardware =
      "SELECT `hardware` FROM `device` WHERE `id` = '08-00-27-CE-76-FE'";
  const std::string query_select_firmware_rev =
      "SELECT `firmware_rev` FROM `device` WHERE `id` = '08-00-27-CE-76-FE'";
  const std::string query_select_os =
      "SELECT `os` FROM `device` WHERE `id` = '08-00-27-CE-76-FE'";
  const std::string query_select_os_version =
      "SELECT `os_version` FROM `device` WHERE `id` = '08-00-27-CE-76-FE'";
  const std::string query_select_carrier =
      "SELECT `carrier` FROM `device` WHERE `id` = '08-00-27-CE-76-FE'";
  const std::string query_select_max_rfports_number =
      "SELECT `max_number_rfcom_ports` FROM `device` WHERE `id` = "
      "'08-00-27-CE-76-FE'";
  const std::string query_select_connection_type =
      "SELECT `connection_type` FROM `device` WHERE `id` = '08-00-27-CE-76-FE'";

  // Checks
  query.Prepare(query_select_hardware);
  query.Next();
  EXPECT_EQ(string("hardware IPX"), query.GetString(0));

  query.Prepare(query_select_firmware_rev);
  query.Next();
  EXPECT_EQ("v.8.0.1", query.GetString(0));

  query.Prepare(query_select_os);
  query.Next();
  EXPECT_EQ("Android", query.GetString(0));

  query.Prepare(query_select_os_version);
  query.Next();
  EXPECT_EQ("4.4.2", query.GetString(0));

  query.Prepare(query_select_carrier);
  query.Next();
  EXPECT_EQ("Life", query.GetString(0));

  query.Prepare(query_select_max_rfports_number);
  query.Next();
  EXPECT_EQ(2, query.GetInteger(0));

  query.Prepare(query_select_connection_type);
  query.Next();
  EXPECT_EQ("Bluetooth", query.GetString(0));
}

TEST_F(
    SQLPTExtRepresentationTest,
    ReactOnUserDevConsentForApp_SetDeviceAllowedAppHasPreDataConsent_ExpectAppHasDefaultPolicies) {
  // Arrange
  const std::string query_insert_device =
      "INSERT INTO `app_group` (`application_id`, `functional_group_id`) "
      "VALUES ('device', '414812216')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_device));

  const std::string query_insert_default =
      "INSERT INTO `app_group` (`application_id`, `functional_group_id`) "
      "VALUES ('default', '686787169')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_default));

  const std::string query_insert_pre_DataConsent =
      "INSERT INTO `app_group` (`application_id`, `functional_group_id`) "
      "VALUES ('pre_DataConsent', '129372391')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_pre_DataConsent));

  const std::string query_insert_1234 =
      "INSERT INTO `app_group` (`application_id`, `functional_group_id`) "
      "VALUES ('1234', '129372391')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_1234));

  const std::string query_insert_preconsented_group =
      "INSERT INTO `preconsented_group` (`application_id`, "
      "`functional_group_id`) "
      "VALUES ('1234', '129372391')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_preconsented_group));

  const std::string query_insert_functional_group =
      "INSERT INTO `functional_group` (`id`, `user_consent_prompt`, `name`) "
      "VALUES (129372391, '', 'pre_DataConsent')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_functional_group));

  const std::string query_insert_application =
      "INSERT INTO `application` (`id`, `memory_kb`,"
      " `heart_beat_timeout_ms`, `is_predata`, `keep_context`) VALUES ('1234', "
      "5, 10, 1, 0)";

  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_application));

  EXPECT_TRUE(reps->IsPredataPolicy("1234"));
  reps->ReactOnUserDevConsentForApp("1234", true);
  EXPECT_TRUE(reps->IsDefaultPolicy("1234"));
}

TEST_F(
    SQLPTExtRepresentationTest,
    ReactOnUserDevConsentForApp_SetDeviceAllowedAppHasSpecificPoliciesThenSetPredata_ExpectAppGroupsRestored) {
  // Arrange
  const std::string query_insert_device_consent_group =
      "INSERT INTO `device_consent_group` (`device_id`, "
      "`functional_group_id`,'is_consented', `input`, `time_stamp`) VALUES "
      "('XXX12345ZZZ', 'Location-1', 1,'GUI', '2015-01-01T00:00:52Z')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_device_consent_group));

  const std::string query_insert_consent_group_1 =
      "INSERT INTO `consent_group` (`device_id`, 'application_id' , "
      "`functional_group_id`, `is_consented`, `input`, `time_stamp`) VALUES "
      "('XXX12345ZZZ', '1234', 156072572, 1,'GUI', "
      "'2014-01-01T00:00:52Z')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_consent_group_1));

  const std::string query_insert_consent_group_2 =
      "INSERT INTO `consent_group` (`device_id`, 'application_id' , "
      "`functional_group_id`, `is_consented`, `input`, `time_stamp`) VALUES "
      "('XXX12345ZZZ', '1234', 1809526495, 1,'GUI', "
      "'2014-01-01T00:00:52Z')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_consent_group_2));

  const std::string query_insert_pre_DataConsent =
      "INSERT INTO `functional_group` (`id`, `user_consent_prompt`, `name`) "
      "VALUES (129372391, '', 'pre_DataConsent')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_pre_DataConsent));

  const std::string query_insert_Notifications =
      "INSERT INTO `functional_group` (`id`, `user_consent_prompt`, `name`) "
      "VALUES (1809526495, 'Notifications', 'Notifications')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_Notifications));

  const std::string query_insert_Location =
      "INSERT INTO `functional_group` (`id`, `user_consent_prompt`, `name`) "
      "VALUES (156072572, 'Location', 'Location-1')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_Location));

  // Add to app_group
  const std::string query_insert_app_group_1 =
      "INSERT INTO `app_group` (`application_id`, `functional_group_id`) "
      "VALUES ('1234', '1809526495')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_app_group_1));

  const std::string query_insert_app_group_2 =
      "INSERT INTO `app_group` (`application_id`, `functional_group_id`) "
      "VALUES ('1234', '156072572')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_app_group_2));
  // Add to pre_consented groups
  const std::string query_insert_preconsented_group =
      "INSERT INTO `preconsented_group` (`application_id`, "
      "`functional_group_id`) "
      "VALUES ('1234', '129372391')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_preconsented_group));

  const std::string query_insert_application =
      "INSERT INTO `application` (`id`, `memory_kb`,"
      " `heart_beat_timeout_ms`, `is_predata`, `keep_context`) VALUES ('1234', "
      "5, 10, 0, 0)";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_application));
  FunctionalIdType group_types;
  ASSERT_TRUE(reps->GetPermissionsForApp("XXX12345ZZZ", "1234", &group_types));
  EXPECT_FALSE(reps->IsPredataPolicy("1234"));
  EXPECT_FALSE(reps->IsDefaultPolicy("1234"));
  std::map<GroupType, FunctionalGroupIDs>::iterator it1 =
      group_types.find(GroupType::kTypeAllowed);
  EXPECT_TRUE(group_types.end() != it1);
  EXPECT_EQ(2u, it1->second.size());
  std::vector<int32_t>::iterator it2 =
      std::find(it1->second.begin(), it1->second.end(), 156072572);
  ASSERT_TRUE(it2 != it1->second.end());
  it2 = std::find(it1->second.begin(), it1->second.end(), 1809526495);
  ASSERT_TRUE(it2 != it1->second.end());
  reps->SetIsPredata("1234", true);
  EXPECT_TRUE(reps->IsPredataPolicy("1234"));
  reps->ReactOnUserDevConsentForApp("1234", true);
  group_types.clear();
  ASSERT_TRUE(reps->GetPermissionsForApp("XXX12345ZZZ", "1234", &group_types));
  EXPECT_FALSE(reps->IsPredataPolicy("1234"));
  EXPECT_FALSE(reps->IsDefaultPolicy("1234"));
  it1 = group_types.find(GroupType::kTypeAllowed);
  EXPECT_TRUE(group_types.end() != it1);
  EXPECT_EQ(2u, it1->second.size());
  it2 = std::find(it1->second.begin(), it1->second.end(), 156072572);
  ASSERT_TRUE(it2 != it1->second.end());
  it2 = std::find(it1->second.begin(), it1->second.end(), 1809526495);
  ASSERT_TRUE(it2 != it1->second.end());
}

TEST_F(SQLPTExtRepresentationTest,
       GetUserFriendlyMsg_SetMsg_ExpectReceivedMsgSetInParams) {
  // Arrange
  const std::string query_insert_message =
      "INSERT INTO `message` (`tts`, `label`,`line1`, `line2`, `textBody`, "
      "`language_code`, `message_type_name`) VALUES ('test tts message', "
      "'GPS and speed', 'test', 'test1', 'test3', 'en-en', 'AppPermissions')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_message));
  const std::string query_insert_message_type =
      "INSERT INTO `message_type` (`name`) VALUES ('AppPermissions')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_message_type));
  std::vector<string> msg_code;
  msg_code.push_back("AppPermissions");
  // Act
  std::vector<UserFriendlyMessage> result =
      reps->GetUserFriendlyMsg(msg_code, string("en-en"));
  // Checks
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ(result[0].message_code, "AppPermissions");
  EXPECT_EQ(result[0].tts, "test tts message");
  EXPECT_EQ(result[0].label, "GPS and speed");
  EXPECT_EQ(result[0].line1, "test");
  EXPECT_EQ(result[0].line2, "test1");
  EXPECT_EQ(result[0].text_body, "test3");
}

TEST_F(SQLPTExtRepresentationTest,
       IncrementGlobalCounter_IncrementThreeTimes_ExpectCountEqual3) {
  // Arrange
  utils::dbms::SQLQuery query(reps->db());
  const std::string query_update =
      "UPDATE `usage_and_error_count` SET"
      " `count_of_sync_reboots` = 0";

  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_update));

  // Act
  reps->Increment("count_of_sync_reboots");
  reps->Increment("count_of_sync_reboots");
  reps->Increment("count_of_sync_reboots");

  const std::string query_select =
      "SELECT `count_of_sync_reboots` FROM `usage_and_error_count`";
  // Assert
  query.Prepare(query_select);
  query.Next();
  EXPECT_EQ(3, query.GetInteger(0));
}

TEST_F(
    SQLPTExtRepresentationTest,
    IncrementAppCounter_IncrementCountOfUserSelections3Times_ExpectCountEqual3) {
  // Arrange
  utils::dbms::SQLQuery query(reps->db());
  const std::string query_delete =
      "DELETE FROM `app_level` WHERE `application_id` = '12345'";

  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_delete));

  // Act
  reps->Increment("12345", "count_of_user_selections");
  reps->Increment("12345", "count_of_user_selections");
  reps->Increment("12345", "count_of_user_selections");

  const std::string query_select =
      "SELECT `count_of_user_selections` FROM `app_level`"
      "  WHERE `application_id` = '12345'";

  // Assert
  query.Prepare(query_select);
  query.Next();
  EXPECT_EQ(3, query.GetInteger(0));
}

TEST_F(SQLPTExtRepresentationTest,
       AppInfo_SetLanguageRuInGUIAndEnInVUI_ExpectRuInGUIAndEnInVUI) {
  // Arrange
  utils::dbms::SQLQuery query(reps->db());
  const std::string query_delete =
      "DELETE FROM `app_level` WHERE `application_id` = '12345'";

  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_delete));
  // Act
  reps->Set("12345", "app_registration_language_gui", "ru-ru");
  reps->Set("12345", "app_registration_language_vui", "en-en");

  const std::string query_select_gui =
      "SELECT `app_registration_language_gui`"
      " FROM `app_level` WHERE `application_id` = '12345'";

  const std::string query_select_vui =
      "SELECT `app_registration_language_vui`"
      " FROM `app_level` WHERE `application_id` = '12345'";

  // Assert
  query.Prepare(query_select_gui);
  query.Next();
  EXPECT_EQ("ru-ru", query.GetString(0));

  query.Prepare(query_select_vui);
  query.Next();
  EXPECT_EQ("en-en", query.GetString(0));
}

TEST_F(SQLPTExtRepresentationTest,
       AddAppStopwatch_Set10And60MinutesForStopwatch_Expect70Minutes) {
  // Arrange
  utils::dbms::SQLQuery query(reps->db());
  const std::string query_delete =
      "DELETE FROM `app_level` WHERE `application_id` = '12345'";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_delete));
  // Act
  reps->Add("12345", "minutes_in_hmi_full", 10);
  reps->Add("12345", "minutes_in_hmi_full", 60);

  const std::string query_select =
      "SELECT `minutes_in_hmi_full` FROM `app_level`"
      "  WHERE `application_id` = '12345'";

  // Assert
  query.Prepare(query_select);
  query.Next();
  EXPECT_EQ(70, query.GetInteger(0));
}

TEST_F(
    SQLPTExtRepresentationTest,
    SetUnpairedDevice_SetUnpairedDeviceId12345_ExpectUnpairedDeviceIdEquals12345) {
  // Arrange
  utils::dbms::SQLQuery query(reps->db());
  const std::string query_delete = "DELETE FROM `device`";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_delete));
  // Act
  const std::string query_insert = "INSERT INTO `device` (`id`) VALUES('12345')";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert));
  ASSERT_TRUE(reps->SetUnpairedDevice("12345", true));
  // Act
  const std::string query_select = "SELECT `id` FROM `device` WHERE `unpaired` = 1";
  // Assert
  query.Prepare(query_select);
  query.Next();
  EXPECT_EQ("12345", query.GetString(0));
}

TEST_F(
    SQLPTExtRepresentationTest,
    UnpairedDevicesList_SetUnpairedDevicesWithId12345AndId54321_Expect2UnpairedDevices) {
  // Arrange
  const std::string query_delete = "DELETE FROM `device`";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_delete));
  // Act
  const std::string query_insert_12345 =
      "INSERT INTO `device` (`id`, `unpaired`)"
      " VALUES('12345', 1)";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_12345));
  // Act
  const std::string query_insert_54321 =
      "INSERT INTO `device` (`id`, `unpaired`) VALUES('54321', 1)";
  // Assert
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_54321));
  // Act
  std::vector<std::string> output;
  // Assert
  ASSERT_TRUE(reps->UnpairedDevicesList(&output));
  ASSERT_EQ(2u, output.size());
  EXPECT_NE(output.end(), std::find(output.begin(), output.end(), "12345"));
  EXPECT_NE(output.end(), std::find(output.begin(), output.end(), "54321"));
}

TEST_F(SQLPTExtRepresentationTest,
       SetMetaInfo_SetMetaInfo_ExpectValuesSetInParams) {
  // Arrange
  ASSERT_TRUE(reps->SetMetaInfo("4.1.3.B_EB355B", "WAEGB", "ru-ru"));
  utils::dbms::SQLQuery query(reps->db());
  const std::string query_select_ccpu = "SELECT `ccpu_version` FROM `module_meta`";
  const std::string query_select_wers_country_code =
      "SELECT `wers_country_code` FROM `module_meta`";
  const std::string query_select_language = "SELECT `language` FROM `module_meta`";

  // Assert
  query.Prepare(query_select_ccpu);
  query.Next();
  EXPECT_EQ("4.1.3.B_EB355B", query.GetString(0));
  query.Prepare(query_select_wers_country_code);
  query.Next();
  EXPECT_EQ("WAEGB", query.GetString(0));
  query.Prepare(query_select_language);
  query.Next();
  EXPECT_EQ("ru-ru", query.GetString(0));
}

TEST_F(SQLPTExtRepresentationTest,
       IsMetaInfoPresent_SetMetaInfo_ExpectMetaInfoPresent) {
  // Arrange
  const std::string query_insert_meta_info =
      "UPDATE `module_meta` SET `ccpu_version` = '4.1.3.B_EB355B', "
      "`wers_country_code` = 'WAEGB', `language` = 'ru-ru' ";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_meta_info));
  EXPECT_TRUE(reps->IsMetaInfoPresent());
}

TEST_F(SQLPTExtRepresentationTest,
       SetSystemLanguage_SetSystemLanguage_ExpectValueSetInParams) {
  // Arrange
  utils::dbms::SQLQuery query(reps->db());
  ASSERT_TRUE(reps->SetSystemLanguage("ru-ru"));
  const std::string query_select_language = "SELECT `language` FROM `module_meta`";
  // Assert
  query.Prepare(query_select_language);
  query.Next();
  EXPECT_EQ("ru-ru", query.GetString(0));
}

TEST_F(
    SQLPTExtRepresentationTest,
    GetFunctionalGroupNames_SetGroupsManuallyThenGetGroupNames_ExpectAllGroupsReceived) {
  // Arrange
  const std::string query_insert_pre_DataConsent =
      "INSERT INTO `functional_group` (`id`, `user_consent_prompt`, `name`) "
      "VALUES (129372391, '', 'pre_DataConsent')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_pre_DataConsent));

  const std::string query_insert_Notifications =
      "INSERT INTO `functional_group` (`id`, `user_consent_prompt`, `name`) "
      "VALUES (1809526495, 'Notifications', 'Notifications')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_Notifications));

  const std::string query_insert_Location =
      "INSERT INTO `functional_group` (`id`, `user_consent_prompt`, `name`) "
      "VALUES (156072572, 'Location', 'Location-1')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_Location));

  std::map<uint32_t, std::pair<std::string, std::string>> FunctionalGroupNames;
  std::map<uint32_t, std::pair<std::string, std::string>>::iterator it;
  reps->GetFunctionalGroupNames(FunctionalGroupNames);
  EXPECT_EQ(3u, FunctionalGroupNames.size());
  ASSERT_TRUE(FunctionalGroupNames.end() !=
              (it = FunctionalGroupNames.find(129372391)));
  EXPECT_EQ("", it->second.first);
  EXPECT_EQ("pre_DataConsent", it->second.second);

  ASSERT_TRUE(FunctionalGroupNames.end() !=
              (it = FunctionalGroupNames.find(156072572)));
  EXPECT_EQ("Location", it->second.first);
  EXPECT_EQ("Location-1", it->second.second);

  ASSERT_TRUE(FunctionalGroupNames.end() !=
              (it = FunctionalGroupNames.find(1809526495)));
  EXPECT_EQ("Notifications", it->second.first);
  EXPECT_EQ("Notifications", it->second.second);
}

TEST_F(
    SQLPTExtRepresentationTest,
    RemoveAppConsentForGroup_SetAppConsentThenRemove_ExpectAppConsentForGroupRemoved) {
  // Arrange
  const std::string query_insert_pre_DataConsent =
      "INSERT INTO `functional_group` (`id`, `user_consent_prompt`, `name`) "
      "VALUES (129372391, '', 'pre_DataConsent')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_pre_DataConsent));

  const std::string query_insert_Notifications =
      "INSERT INTO `functional_group` (`id`, `user_consent_prompt`, `name`) "
      "VALUES (1809526495, 'Notifications', 'Notifications')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_Notifications));

  const std::string query_insert_Location =
      "INSERT INTO `functional_group` (`id`, `user_consent_prompt`, `name`) "
      "VALUES (156072572, 'Location', 'Location-1')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_Location));

  const std::string query_insert_consent_group_1 =
      "INSERT INTO `consent_group` (`device_id`, 'application_id' , "
      "`functional_group_id`, `is_consented`, `input`, `time_stamp`) VALUES "
      "('XXX12345ZZZ', '1234', 1809526495, 1,'GUI', "
      "'2014-01-01T00:00:52Z')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_consent_group_1));

  const std::string query_insert_consent_group_2 =
      "INSERT INTO `consent_group` (`device_id`, 'application_id' , "
      "`functional_group_id`, `is_consented`, `input`, `time_stamp`) VALUES "
      "('XXX12345ZZZ', '1234', 156072572, 1,'GUI', "
      "'2015-01-01T00:00:52Z')";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_consent_group_2));

  GroupsAliasNameCollection allowed_groups;
  allowed_groups.push_back(std::make_pair("Location", "Location-1"));
  allowed_groups.push_back(std::make_pair("Notifications", "Notifications"));

  GroupsAliasNameCollection disallowed_groups;

  FillPermissionStruct("XXX12345ZZZ", "1234", "VR", allowed_groups,
                       disallowed_groups);
  FunctionalIdType group_types;
  ASSERT_TRUE(reps->GetPermissionsForApp("XXX12345ZZZ", "1234", &group_types));
  EXPECT_TRUE(
      CheckGroupTypesExist(group_types, allowed_groups, disallowed_groups));
  group_types.clear();
  reps->RemoveAppConsentForGroup("1234", "Notifications");
  ASSERT_TRUE(reps->GetPermissionsForApp("XXX12345ZZZ", "1234", &group_types));
  allowed_groups.pop_back();
  EXPECT_TRUE(
      CheckGroupTypesExist(group_types, allowed_groups, disallowed_groups));
}

TEST_F(SQLPTExtRepresentationTest,
       CleanUnpaireDevices_SetDevicesThenCleanup_ExpectDevicesDeleted) {
  // Arrange
  utils::dbms::SQLQuery query(reps->db());
  reps->SetDeviceData("XXX12345ZZZ", "hardware IPX", "v.8.0.1", "Android",
                      "4.4.2", "Life", 2, "Bluetooth");

  StringArray allowed_groups;
  StringArray disallowed_groups;
  EXPECT_TRUE(reps->GetUserPermissionsForDevice("XXX12345ZZZ", &allowed_groups,
                                                &disallowed_groups));
  EXPECT_EQ(0u, allowed_groups.size());
  EXPECT_EQ(0u, disallowed_groups.size());
  allowed_groups.push_back("DataConsent-2");
  disallowed_groups.push_back("Navigation-1");
  EXPECT_TRUE(reps->SetUserPermissionsForDevice("XXX12345ZZZ", allowed_groups,
                                                disallowed_groups));

  GroupsAliasNameCollection perm_allowed_groups;
  perm_allowed_groups.push_back(
      std::make_pair("Notifications", "Notifications"));

  GroupsAliasNameCollection perm_disallowed_groups;
  perm_disallowed_groups.push_back(std::make_pair("Location", "Location-1"));
  FillPermissionStruct("XXX12345ZZZ", "12345", "VR", perm_allowed_groups,
                       perm_disallowed_groups);

  EXPECT_TRUE(reps->SetUserPermissionsForApp(perm_consent));

  const std::string query_select_device =
      "SELECT COUNT(*) FROM `device` WHERE `id` = 'XXX12345ZZZ'";
  const std::string query_select_device_consent =
      "SELECT COUNT(*) FROM `device_consent_group` WHERE `device_id` = "
      "'XXX12345ZZZ'";
  const std::string query_select_consent_group =
      "SELECT COUNT(*) FROM `consent_group` WHERE `device_id` = 'XXX12345ZZZ'";

  query.Prepare(query_select_device);
  query.Next();
  EXPECT_EQ(1, query.GetInteger(0));

  query.Prepare(query_select_device_consent);
  query.Next();
  EXPECT_EQ(2, query.GetInteger(0));

  query.Prepare(query_select_consent_group);
  query.Next();
  EXPECT_EQ(2, query.GetInteger(0));

  EXPECT_TRUE(reps->SetUnpairedDevice("XXX12345ZZZ", true));

  std::vector<std::string> DeviceIds;
  DeviceIds.push_back("XXX12345ZZZ");
  EXPECT_TRUE(reps->CleanupUnpairedDevices(DeviceIds));

  // Assert
  query.Prepare(query_select_device);
  query.Next();
  EXPECT_EQ(0, query.GetInteger(0));

  query.Prepare(query_select_device_consent);
  query.Next();
  EXPECT_EQ(0, query.GetInteger(0));

  query.Prepare(query_select_consent_group);
  query.Next();
  EXPECT_EQ(0, query.GetInteger(0));
}

TEST_F(
    SQLPTExtRepresentationTest,
    SetDefaultPolicy_SetPredataThenChangeToDefaultPolicy_ExpectDefaultPolicySet) {
  // Arrange
  const std::string query_insert_app =
      "INSERT OR IGNORE INTO `application`(`id`, `keep_context`, "
      "`steal_focus`, "
      " `default_hmi`, `priority_value`, `is_revoked`, `is_default`, "
      "`is_predata`, `memory_kb`, "
      " `heart_beat_timeout_ms`) VALUES( '12345', 0, 0, 'NONE', 'NONE', 0, 0, "
      "0, 64, 10) ";

  ASSERT_TRUE(query_wrapper_->Exec(query_insert_app));
  GroupsAliasNameCollection allowed_groups;
  allowed_groups.push_back(std::make_pair("Notifications", "Notifications"));

  GroupsAliasNameCollection disallowed_groups;
  disallowed_groups.push_back(std::make_pair("DataConsent", "DataConsent-2"));
  FillPermissionStruct("XXX12345ZZZ", "12345", "VR", allowed_groups,
                       disallowed_groups);
  EXPECT_TRUE(reps->SetUserPermissionsForApp(perm_consent));
  // Act
  ASSERT_TRUE(reps->SetIsPredata("12345", true));
  // Check
  EXPECT_TRUE(reps->IsPredataPolicy("12345"));
  // Act
  EXPECT_TRUE(reps->SetDefaultPolicy("12345"));
  // Check
  EXPECT_TRUE(reps->IsDefaultPolicy("12345"));
}

TEST_F(SQLPTExtRepresentationTest,
       SetIsPreData_SetSpecificPoliciesThenSetIsPredata_ExpectPreDataSet) {
  // Arrange
  const std::string query_insert_app =
      "INSERT OR IGNORE INTO `application`(`id`, `keep_context`, "
      "`steal_focus`, "
      " `default_hmi`, `priority_value`, `is_revoked`, `is_default`, "
      "`is_predata`, `memory_kb`, "
      " `heart_beat_timeout_ms`) VALUES( '12345', 0, 0, 'NONE', 'NONE', 0, 0, "
      "0, 64, 10) ";

  ASSERT_TRUE(query_wrapper_->Exec(query_insert_app));
  GroupsAliasNameCollection allowed_groups;
  allowed_groups.push_back(std::make_pair("Notifications", "Notifications"));

  GroupsAliasNameCollection disallowed_groups;
  disallowed_groups.push_back(std::make_pair("DataConsent", "DataConsent-2"));
  FillPermissionStruct("XXX12345ZZZ", "12345", "VR", allowed_groups,
                       disallowed_groups);
  EXPECT_TRUE(reps->SetUserPermissionsForApp(perm_consent));
  EXPECT_FALSE(reps->IsPredataPolicy("12345"));
  // Act
  ASSERT_TRUE(reps->SetIsPredata("12345", false));
  // Check
  EXPECT_FALSE(reps->IsPredataPolicy("12345"));
  // Act
  ASSERT_TRUE(reps->SetIsPredata("12345", true));
  // Check
  EXPECT_TRUE(reps->IsPredataPolicy("12345"));
}

TEST_F(
    SQLPTExtRepresentationTest,
    SetPreDataPolicy_SetSpecificPoliciesThenSetPredataPolicy_ExpectPreDataPolicySet) {
  // Arrange
  const std::string query_insert_app =
      "INSERT OR IGNORE INTO `application`(`id`, `keep_context`, "
      "`steal_focus`, "
      " `default_hmi`, `priority_value`, `is_revoked`, `is_default`, "
      "`is_predata`, `memory_kb`, "
      " `heart_beat_timeout_ms`) VALUES( '12345', 0, 0, 'NONE', 'NONE', 0, 0, "
      "0, 64, 10) ";

  ASSERT_TRUE(query_wrapper_->Exec(query_insert_app));
  GroupsAliasNameCollection allowed_groups;
  allowed_groups.push_back(std::make_pair("Notifications", "Notifications"));

  GroupsAliasNameCollection disallowed_groups;
  disallowed_groups.push_back(std::make_pair("DataConsent", "DataConsent-2"));
  FillPermissionStruct("XXX12345ZZZ", "12345", "VR", allowed_groups,
                       disallowed_groups);
  EXPECT_TRUE(reps->SetUserPermissionsForApp(perm_consent));
  EXPECT_FALSE(reps->IsPredataPolicy("12345"));
  // Act
  ASSERT_TRUE(reps->SetPredataPolicy("12345"));
  // Check
  EXPECT_TRUE(reps->IsPredataPolicy("12345"));
}

TEST_F(SQLPTExtRepresentationTest,
       IsPreDataPolicy_SetPredataPolicy_ExpectPreDataPolicySet) {
  // Arrange
  const std::string query_insert_app =
      "INSERT OR IGNORE INTO `application`(`id`, `keep_context`, "
      "`steal_focus`, "
      " `default_hmi`, `priority_value`, `is_revoked`, `is_default`, "
      "`is_predata`, `memory_kb`, "
      " `heart_beat_timeout_ms`) VALUES( '12345', 0, 0, 'NONE', 'NONE', 0, 0, "
      "1, 64, 10) ";
  ASSERT_TRUE(query_wrapper_->Exec(query_insert_app));
  // Check
  EXPECT_TRUE(reps->IsPredataPolicy("12345"));
}

}  // namespace policy
}  // namespace components
}  // namespace test
