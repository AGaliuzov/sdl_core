/* Copyright (c) 2014, Ford Motor Company
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
#include <stdio.h>
#include <sys/stat.h>

#include "gtest/gtest.h"
#include "driver_dbms.h"
#include "policy/sql_pt_representation.h"
#include "policy/policy_types.h"
#include "json/writer.h"
#include "config_profile/profile.h"
#include "utils/file_system.h"
#include "utils/system.h"

using policy::SQLPTRepresentation;
using policy::CheckPermissionResult;
using policy::EndpointUrls;

namespace test {
namespace components {
namespace policy {

class SQLPTRepresentationTest : public ::testing::Test {
 protected:
  static DBMS* dbms;
  static SQLPTRepresentation* reps;
  static const std::string kDatabaseName;

  static void SetUpTestCase() {
    reps = new SQLPTRepresentation;
    dbms = new DBMS(kDatabaseName);
    EXPECT_EQ(::policy::SUCCESS, reps->Init());
    EXPECT_TRUE(dbms->Open());
  }

  static void TearDownTestCase() {
    EXPECT_TRUE(reps->Drop());
    EXPECT_TRUE(reps->Close());
    delete reps;
    dbms->Close();
  }

  ::testing::AssertionResult IsValid(const policy_table::Table& table) {
    if (table.is_valid()) {
      return ::testing::AssertionSuccess();
    } else {
      ::rpc::ValidationReport report(" - table");
      table.ReportErrors(&report);
      return ::testing::AssertionFailure() << ::rpc::PrettyFormat(report);
    }
  }
};

DBMS* SQLPTRepresentationTest::dbms = 0;
SQLPTRepresentation* SQLPTRepresentationTest::reps = 0;
#ifdef __QNX__
const std::string SQLPTRepresentationTest::kDatabaseName = "policy";
#else  // __QNX__
const std::string SQLPTRepresentationTest::kDatabaseName = "policy.sqlite";
#endif  // __QNX__

class SQLPTRepresentationTest2 : public ::testing::Test {
 protected:
  SQLPTRepresentation* reps;

  virtual void SetUp() {
    const char kDirectory[] = "storage123";
    file_system::CreateDirectory(kDirectory);
    chmod(kDirectory, 00000);
    profile::Profile::instance()->config_file_name("smartDeviceLink3.ini");
    reps = new SQLPTRepresentation;
  }

  virtual void TearDown() {
   profile::Profile::instance()->config_file_name("smartDeviceLink.ini");
    delete reps;
  }
};

TEST_F(SQLPTRepresentationTest2, CheckActualAttemptsToOpenDB_ExpectCorrectNumber) {
  EXPECT_EQ(::policy::FAIL, reps->Init());
  // Check  Actual attempts number made to try to open DB
  EXPECT_EQ(profile::Profile::instance()->attempts_to_open_policy_db(), reps->open_counter());
  // Check timeot value correctly read from config file.
  EXPECT_EQ(profile::Profile::instance()->open_attempt_timeout_ms(), 700);
}

TEST_F(SQLPTRepresentationTest, CheckPermissionsAllowed_SetValuesInAppGroupRpcFunctionalGroup_GetEqualParamsInCheckPermissionResult) {
  // Arrange
  const char* query = "INSERT OR REPLACE INTO `application` (`id`, `memory_kb`,"
      " `heart_beat_timeout_ms`) VALUES ('12345', 5, 10); "
      "INSERT OR REPLACE INTO functional_group (`id`, `name`)"
      "  VALUES (1, 'Base-4'); "
      "INSERT OR REPLACE INTO `app_group` (`application_id`,"
      " `functional_group_id`) VALUES ('12345', 1); "
      "INSERT OR REPLACE INTO `rpc` (`name`, `parameter`, `hmi_level_value`,"
      " `functional_group_id`) VALUES ('Update', 'gps', 'FULL', 1); "
      "INSERT OR REPLACE INTO `rpc` (`name`, `parameter`, `hmi_level_value`,"
      " `functional_group_id`) VALUES ('Update', 'speed', 'FULL', 1);";

  // Assert
  ASSERT_TRUE(dbms->Exec(query));

  // Act
  CheckPermissionResult ret;
  reps->CheckPermissions("12345", "FULL", "Update", ret);

  // Assert
  EXPECT_TRUE(ret.hmi_level_permitted == ::policy::kRpcAllowed);
  ASSERT_EQ(2u, ret.list_of_allowed_params.size());
  EXPECT_EQ("gps", ret.list_of_allowed_params[0]);
  EXPECT_EQ("speed", ret.list_of_allowed_params[1]);
}

TEST_F(SQLPTRepresentationTest, CheckPermissionsAllowedWithoutParameters_SetLimitedPermissions_ExpectEmptyListOfAllowedParams) {
  // Arrange
  const char* query = "INSERT OR REPLACE INTO `application` (`id`, `memory_kb`,"
      " `heart_beat_timeout_ms`) VALUES ('12345', 5, 10); "
      "INSERT OR REPLACE INTO functional_group (`id`, `name`)"
      "  VALUES (1, 'Base-4'); "
      "INSERT OR REPLACE INTO `app_group` (`application_id`,"
      " `functional_group_id`) VALUES ('12345', 1); "
      "DELETE FROM `rpc`; "
      "INSERT OR REPLACE INTO `rpc` (`name`, `hmi_level_value`,"
      " `functional_group_id`) VALUES ('Update', 'LIMITED', 1);";

  // Assert
  ASSERT_TRUE(dbms->Exec(query));

  // Act
  CheckPermissionResult ret;
  reps->CheckPermissions("12345", "LIMITED", "Update", ret);

  // Assert
  EXPECT_TRUE(ret.hmi_level_permitted == ::policy::kRpcAllowed);
  EXPECT_TRUE(ret.list_of_allowed_params.empty());
}

TEST_F(SQLPTRepresentationTest, CheckPermissionsDisallowedWithoutParameters_DeletedAppGroupAndSetFULLLevel_ExpectHmiLevelIsDissalowed) {
  // Arrange
  const char* query = "DELETE FROM `app_group`";

  // Assert
  ASSERT_TRUE(dbms->Exec(query));

  // Act
  CheckPermissionResult ret;
  reps->CheckPermissions("12345", "FULL", "Update", ret);

  // Assert
  EXPECT_EQ(::policy::kRpcDisallowed, ret.hmi_level_permitted);
  EXPECT_TRUE(ret.list_of_allowed_params.empty());
}

TEST_F(SQLPTRepresentationTest, PTPReloaded_UpdateModuleConfig_ReturnIsPTPreloadedTRUE) {
  // Arrange
  const char* query = "UPDATE `module_config` SET `preloaded_pt` = 1";

  // Assert
  ASSERT_TRUE(dbms->Exec(query));
  EXPECT_TRUE(reps->IsPTPreloaded());
}

TEST_F(SQLPTRepresentationTest, GetUpdateUrls_DeleteAndInsertEndpoints_ExpectUpdateUrls) {
  // Arrange
  const char* query_delete = "DELETE FROM `endpoint`; ";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_delete));

  // Act
  EndpointUrls ret = reps->GetUpdateUrls(7);

  // Assert
  EXPECT_TRUE(ret.empty());

  // Act
  const char* query_insert =
      "INSERT INTO `endpoint` (`application_id`, `url`, `service`) "
          "  VALUES ('12345', 'http://ford.com/cloud/1', 7);"
          "INSERT INTO `endpoint` (`application_id`, `url`, `service`) "
          "  VALUES ('12345', 'http://ford.com/cloud/2', 7);";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_insert));
  // Act
  ret = reps->GetUpdateUrls(7);

  // Assert
  ASSERT_EQ(2u, ret.size());
  EXPECT_EQ("http://ford.com/cloud/1", ret[0].url[0]);
  EXPECT_EQ("http://ford.com/cloud/2", ret[1].url[0]);

  // Act
  ret = reps->GetUpdateUrls(0);

  // Assert
  EXPECT_TRUE(ret.empty());
}

TEST_F(SQLPTRepresentationTest, IgnitionCyclesBeforeExchange_WithParametersOfQueryEqualZero) {
  // Arrange
  const char* query_zeros = "UPDATE `module_meta` SET "
      "  `ignition_cycles_since_last_exchange` = 0; "
      "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = 0";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_zeros));
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());

  // Act
  reps->IncrementIgnitionCycles();

  // Assert
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());

}

TEST_F(SQLPTRepresentationTest, IgnitionCyclesBeforeExchange_WithParametersOfQueryAreLessLimit) {
  // Arrange
  const char* query_less_limit = "UPDATE `module_meta` SET "
      "  `ignition_cycles_since_last_exchange` = 5; "
      "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = 10";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_less_limit));
  EXPECT_EQ(5, reps->IgnitionCyclesBeforeExchange());

  // Act
  reps->IncrementIgnitionCycles();

  // Assert
  EXPECT_EQ(4, reps->IgnitionCyclesBeforeExchange());

}

TEST_F(SQLPTRepresentationTest, IgnitionCyclesBeforeExchange_WithLimitCountOfParametersOfQuery) {
  // Arrange
  const char* query_limit = "UPDATE `module_meta` SET "
      "  `ignition_cycles_since_last_exchange` = 9; "
      "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = 10";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_limit));
  EXPECT_EQ(1, reps->IgnitionCyclesBeforeExchange());

  // Act
  reps->IncrementIgnitionCycles();

  // Assert
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());

}

TEST_F(SQLPTRepresentationTest, IgnitionCyclesBeforeExchange_WithMoreLimitCountOfParametersOfQuery) {
  // Arrange
  const char* query_more_limit = "UPDATE `module_meta` SET "
      "  `ignition_cycles_since_last_exchange` = 12; "
      "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = 10";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_more_limit));
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());

}

TEST_F(SQLPTRepresentationTest, IgnitionCyclesBeforeExchange_WithNegativeLimitOfParametersOfQuery) {
  // Arrange
  const char* query_negative_limit = "UPDATE `module_meta` SET "
      "  `ignition_cycles_since_last_exchange` = 3; "
      "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = -1";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_negative_limit));
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());
}

TEST_F(SQLPTRepresentationTest, IgnitionCyclesBeforeExchange_WithNegativeLimitOfCurrentParameterOfQuery) {

  // Arrange
  const char* query_negative_current = "UPDATE `module_meta` SET "
      "  `ignition_cycles_since_last_exchange` = -1; "
      "  UPDATE `module_config` SET `exchange_after_x_ignition_cycles` = 2";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_negative_current));
  EXPECT_EQ(0, reps->IgnitionCyclesBeforeExchange());
}

TEST_F(SQLPTRepresentationTest, KilometersBeforeExchange_WithParametersOfQueryEqualZero) {
  // Arrange
  const char* query_zeros = "UPDATE `module_meta` SET "
      "  `pt_exchanged_at_odometer_x` = 0; "
      "  UPDATE `module_config` SET `exchange_after_x_kilometers` = 0";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_zeros));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(0));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(-10));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(10));
}

TEST_F(SQLPTRepresentationTest, KilometersBeforeExchange_QueryWithNegativeLimit) {
  // Arrange
  const char* query_negative_limit = "UPDATE `module_meta` SET "
      "  `pt_exchanged_at_odometer_x` = 10; "
      "  UPDATE `module_config` SET `exchange_after_x_kilometers` = -10";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_negative_limit));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(0));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(10));
}

TEST_F(SQLPTRepresentationTest, KilometersBeforeExchange_QueryWithNegativeCurrentLimit) {
  // Arrange
  const char* query_negative_last = "UPDATE `module_meta` SET "
      "  `pt_exchanged_at_odometer_x` = -10; "
      "  UPDATE `module_config` SET `exchange_after_x_kilometers` = 20";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_negative_last));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(0));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(10));
}

TEST_F(SQLPTRepresentationTest, KilometersBeforeExchange_QueryWithLimitParameters) {
  // Arrange
  const char* query_limit = "UPDATE `module_meta` SET "
      "  `pt_exchanged_at_odometer_x` = 10; "
      "  UPDATE `module_config` SET `exchange_after_x_kilometers` = 100";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_limit));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(120));
  EXPECT_EQ(60, reps->KilometersBeforeExchange(50));
  EXPECT_EQ(0, reps->KilometersBeforeExchange(5));
}

TEST_F(SQLPTRepresentationTest, DaysBeforeExchange_WithParametersOfQueryEqualZero) {
  // Arrange
  const char* query_zeros = "UPDATE `module_meta` SET "
      "  `pt_exchanged_x_days_after_epoch` = 0; "
      "  UPDATE `module_config` SET `exchange_after_x_days` = 0";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_zeros));
  EXPECT_EQ(0, reps->DaysBeforeExchange(0));
  EXPECT_EQ(0, reps->DaysBeforeExchange(-10));
  EXPECT_EQ(0, reps->DaysBeforeExchange(10));
}

TEST_F(SQLPTRepresentationTest, DaysBeforeExchange_QueryWithNegativeLimit) {
  // Arrange
  const char* query_negative_limit = "UPDATE `module_meta` SET "
      "  `pt_exchanged_x_days_after_epoch` = 10; "
      "  UPDATE `module_config` SET `exchange_after_x_days` = -10";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_negative_limit));
  EXPECT_EQ(0, reps->DaysBeforeExchange(0));
  EXPECT_EQ(0, reps->DaysBeforeExchange(10));
}

TEST_F(SQLPTRepresentationTest, DaysBeforeExchange_QueryWithNegativeCurrentLimit) {
  // Arrange
  const char* query_negative_last = "UPDATE `module_meta` SET "
      "  `pt_exchanged_x_days_after_epoch` = -10; "
      "  UPDATE `module_config` SET `exchange_after_x_days` = 20";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_negative_last));
  EXPECT_EQ(0, reps->DaysBeforeExchange(0));
  EXPECT_EQ(0, reps->DaysBeforeExchange(10));
}

TEST_F(SQLPTRepresentationTest, DaysBeforeExchange_QueryWithLimitParameters) {
  // Arrange
  const char* query_limit = "UPDATE `module_meta` SET "
      "  `pt_exchanged_x_days_after_epoch` = 10; "
      "  UPDATE `module_config` SET `exchange_after_x_days` = 100";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_limit));
  EXPECT_EQ(0, reps->DaysBeforeExchange(120));
  EXPECT_EQ(60, reps->DaysBeforeExchange(50));
  EXPECT_EQ(0, reps->DaysBeforeExchange(5));
}

TEST_F(SQLPTRepresentationTest, SecondsBetweenRetries_DeletedAndInsertedSecondsBetweenRetry_ExpectCountOfSecondsEqualInserted) {
  // Arrange
  std::vector<int> seconds;
  const char* query_delete = "DELETE FROM `seconds_between_retry`; ";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_delete));
  ASSERT_TRUE(reps->SecondsBetweenRetries(&seconds));
  EXPECT_EQ(0u, seconds.size());

  // Arrange
  const char* query_insert =
      "INSERT INTO `seconds_between_retry` (`index`, `value`) "
          "  VALUES (0, 10); "
          "INSERT INTO `seconds_between_retry` (`index`, `value`) "
          "  VALUES (1, 20); ";

  // Assert
  ASSERT_TRUE(dbms->Exec(query_insert));
  ASSERT_TRUE(reps->SecondsBetweenRetries(&seconds));
  ASSERT_EQ(2u, seconds.size());
  EXPECT_EQ(10, seconds[0]);
  EXPECT_EQ(20, seconds[1]);
}

TEST_F(SQLPTRepresentationTest, TimeoutResponse_Set60Seconds_GetEqualTimeout) {
  // Arrange
  const char* query =
      "UPDATE `module_config` SET `timeout_after_x_seconds` = 60";

  // Assert
  ASSERT_TRUE(dbms->Exec(query));
  EXPECT_EQ(60, reps->TimeoutResponse());
}
TEST_F(SQLPTRepresentationTest, DISABLED_GenerateSnapshot_SetPolicyTable_SnapshotIsPresent) {
  // Arrange
  Json::Value table(Json::objectValue);
  table["policy_table"] = Json::Value(Json::objectValue);

  Json::Value& policy_table = table["policy_table"];
  policy_table["module_config"] = Json::Value(Json::objectValue);
  policy_table["functional_groupings"] = Json::Value(Json::objectValue);
  policy_table["consumer_friendly_messages"] = Json::Value(Json::objectValue);
  policy_table["app_policies"] = Json::Value(Json::objectValue);

  Json::Value& module_config = policy_table["module_config"];
  module_config["preloaded_pt"] = Json::Value(true);
  module_config["preloaded_date"] = Json::Value("");
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
  module_config["endpoints"]["0x00"]["default"][0] = Json::Value(
      "http://ford.com/cloud/default");
  module_config["notifications_per_minute_by_priority"] = Json::Value(
      Json::objectValue);
  module_config["notifications_per_minute_by_priority"]["emergency"] =
      Json::Value(1);
  module_config["notifications_per_minute_by_priority"]["navigation"] =
      Json::Value(2);
  module_config["notifications_per_minute_by_priority"]["VOICECOMM"] =
      Json::Value(3);
  module_config["notifications_per_minute_by_priority"]["communication"] =
      Json::Value(4);
  module_config["notifications_per_minute_by_priority"]["normal"] = Json::Value(
      5);
  module_config["notifications_per_minute_by_priority"]["none"] = Json::Value(
      6);
  module_config["vehicle_make"] = Json::Value("MakeT");
  module_config["vehicle_model"] = Json::Value("ModelT");
  module_config["vehicle_year"] = Json::Value("2014");

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
  consumer_friendly_messages["messages"]["MSG1"] = Json::Value(
      Json::objectValue);
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
  app_policies["default"]["priority"] = Json::Value("EMERGENCY");
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

  consumer_friendly_messages.removeMember("messages");
  policy_table["device_data"] = Json::Value(Json::objectValue);
  policy_table["module_meta"] = Json::Value(Json::objectValue);

  policy_table::Table expected(&table);

  Json::StyledWriter writer;
  std::cout << "\nExpected : " << writer.write(expected.ToJsonValue()) << std::endl;
  std::cout << "\nActual   : " << writer.write(snapshot->ToJsonValue()) << std::endl;
  EXPECT_EQ(writer.write(expected.ToJsonValue()), writer.write(snapshot->ToJsonValue()));
  // Assert
  EXPECT_EQ(expected.ToJsonValue().toStyledString(),
            snapshot->ToJsonValue().toStyledString());
}

}  // namespace policy
}  // namespace components
}  // namespace test
