/* Copyright (c) 2013, Ford Motor Company
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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mock_policy_listener.h"
#include "mock_pt_representation.h"
#include "mock_pt_ext_representation.h"
#include "mock_cache_manager.h"
#include "mock_update_status_manager.h"
#include "policy/policy_manager_impl.h"
#include "policy/update_status_manager_interface.h"
#include "policy/cache_manager_interface.h"
#include "json/value.h"
#include "utils/shared_ptr.h"

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgReferee;
using ::testing::NiceMock;

using ::policy::PTRepresentation;
using ::policy::MockPolicyListener;
using ::policy::MockPTRepresentation;
using ::policy::MockPTExtRepresentation;
using ::policy::MockCacheManagerInterface;
using ::policy::MockUpdateStatusManagerInterface;
using ::policy::PolicyManagerImpl;
using ::policy::PolicyTable;
using ::policy::EndpointUrls;
using ::policy::CacheManagerInterfaceSPtr;
using ::policy::UpdateStatusManagerInterfaceSPtr;

namespace policy_table = rpc::policy_table_interface_base;

namespace test {
namespace components {
namespace policy {

class PolicyManagerImplTest : public ::testing::Test {
 protected:
  PolicyManagerImpl* manager;
  MockCacheManagerInterface* cache_manager;
  MockUpdateStatusManagerInterface* update_manager;

  void SetUp() {
    manager = new PolicyManagerImpl();
    cache_manager = new NiceMock<MockCacheManagerInterface>();
    update_manager = new MockUpdateStatusManagerInterface();
    manager->set_cache_manager(cache_manager);
    manager->set_update_status_manager(update_manager);
  }

  void TearDown() {
    delete update_manager;
    delete cache_manager;
  }
};

TEST_F(PolicyManagerImplTest, ExceededIgnitionCycles) {
  EXPECT_CALL(*cache_manager, IgnitionCyclesBeforeExchange()).Times(2).WillOnce(
      Return(5)).WillOnce(Return(0));
  EXPECT_CALL(*cache_manager, IncrementIgnitionCycles()).Times(1);

  EXPECT_FALSE(manager->ExceededIgnitionCycles());
  manager->IncrementIgnitionCycles();
  EXPECT_TRUE(manager->ExceededIgnitionCycles());
}

TEST_F(PolicyManagerImplTest, ExceededDays) {
  EXPECT_CALL(*cache_manager, DaysBeforeExchange(_)).Times(2).WillOnce(
      Return(5)).WillOnce(Return(0));

  EXPECT_FALSE(manager->ExceededDays(5));
  EXPECT_TRUE(manager->ExceededDays(15));
}

TEST_F(PolicyManagerImplTest, ExceededKilometers) {
  EXPECT_CALL(*cache_manager, KilometersBeforeExchange(_)).Times(2).WillOnce(
      Return(50)).WillOnce(Return(0));

  EXPECT_FALSE(manager->ExceededKilometers(50));
  EXPECT_TRUE(manager->ExceededKilometers(150));
}

TEST_F(PolicyManagerImplTest, RefreshRetrySequence) {
  std::vector<int> seconds;
  seconds.push_back(50);
  seconds.push_back(100);
  seconds.push_back(200);

  EXPECT_CALL(*cache_manager, TimeoutResponse()).Times(1).WillOnce(Return(60));
  EXPECT_CALL(*cache_manager, SecondsBetweenRetries(_)).Times(1).WillOnce(
      DoAll(SetArgReferee<0>(seconds), Return(true)));

  manager->RefreshRetrySequence();
  EXPECT_EQ(50, manager->NextRetryTimeout());
  EXPECT_EQ(100, manager->NextRetryTimeout());
  EXPECT_EQ(200, manager->NextRetryTimeout());
  EXPECT_EQ(0, manager->NextRetryTimeout());
}

TEST_F(PolicyManagerImplTest, GetUpdateUrl) {
  EndpointUrls urls_1234, urls_4321;
  urls_1234.push_back(::policy::EndpointData("http://ford.com/cloud/1"));
  urls_1234.push_back(::policy::EndpointData("http://ford.com/cloud/2"));
  urls_1234.push_back(::policy::EndpointData("http://ford.com/cloud/3"));
  urls_4321.push_back(::policy::EndpointData("http://panasonic.com/cloud/1"));
  urls_4321.push_back(::policy::EndpointData("http://panasonic.com/cloud/2"));
  urls_4321.push_back(::policy::EndpointData("http://panasonic.com/cloud/3"));

  EXPECT_CALL(*cache_manager, GetUpdateUrls(7)).Times(4).WillRepeatedly(
      Return(urls_1234));
  EXPECT_CALL(*cache_manager, GetUpdateUrls(4)).Times(2).WillRepeatedly(
      Return(urls_4321));

  EXPECT_EQ("http://ford.com/cloud/1", manager->GetUpdateUrl(7));
  EXPECT_EQ("http://ford.com/cloud/2", manager->GetUpdateUrl(7));
  EXPECT_EQ("http://ford.com/cloud/3", manager->GetUpdateUrl(7));
  EXPECT_EQ("http://panasonic.com/cloud/1", manager->GetUpdateUrl(4));
  EXPECT_EQ("http://ford.com/cloud/2", manager->GetUpdateUrl(7));
  EXPECT_EQ("http://panasonic.com/cloud/3", manager->GetUpdateUrl(4));
}

#ifdef EXTENDED_POLICY
TEST_F(PolicyManagerImplTest, IncrementGlobalCounter) {
  EXPECT_CALL(*cache_manager, Increment(usage_statistics::SYNC_REBOOTS))
      .Times(1);

  manager->Increment(usage_statistics::SYNC_REBOOTS);
}

TEST_F(PolicyManagerImplTest, IncrementAppCounter) {
  EXPECT_CALL(*cache_manager, Increment("12345",
                                        usage_statistics::USER_SELECTIONS))
      .Times(1);

  manager->Increment("12345", usage_statistics::USER_SELECTIONS);
}

TEST_F(PolicyManagerImplTest, SetAppInfo) {
  EXPECT_CALL(*cache_manager, Set("12345", usage_statistics::LANGUAGE_GUI,
                                  "de-de")).Times(1);

  manager->Set("12345", usage_statistics::LANGUAGE_GUI, "de-de");
}

TEST_F(PolicyManagerImplTest, AddAppStopwatch) {
  EXPECT_CALL(*cache_manager, Add("12345", usage_statistics::SECONDS_HMI_FULL,
                                  30)).Times(1);

  manager->Add("12345", usage_statistics::SECONDS_HMI_FULL, 30);
}
#endif  // EXTENDED_POLICY

TEST_F(PolicyManagerImplTest, ResetPT) {
  EXPECT_CALL(*cache_manager, ResetPT("filename")).WillOnce(Return(true))
      .WillOnce(Return(false));

  EXPECT_TRUE(manager->ResetPT("filename"));
  EXPECT_FALSE(manager->ResetPT("filename"));
}

#ifdef EXTENDED_POLICY
TEST_F(PolicyManagerImplTest, CheckPermissions) {
  NiceMock<MockPolicyListener> mock_listener;
  manager->set_listener(&mock_listener);

  EXPECT_CALL(mock_listener, OnCurrentDeviceIdUpdateRequired("12345678"))
      .Times(1);

  ::policy::CheckPermissionResult output;
  manager->CheckPermissions("12345678", "FULL", "Alert", output);
  EXPECT_EQ(::policy::kRpcAllowed, output.hmi_level_permitted);
  ASSERT_TRUE(!output.list_of_allowed_params.empty());
  ASSERT_EQ(2u, output.list_of_allowed_params.size());
  EXPECT_EQ("speed", output.list_of_allowed_params[0]);
  EXPECT_EQ("gps", output.list_of_allowed_params[1]);
}
#else  // EXTENDED_POLICY
TEST_F(PolicyManagerImplTest, CheckPermissions) {
  ::policy::CheckPermissionResult expected;
  expected.hmi_level_permitted = ::policy::kRpcAllowed;
  expected.list_of_allowed_params.push_back("speed");
  expected.list_of_allowed_params.push_back("gps");

  EXPECT_CALL(*cache_manager, GetFunctionalGroupings(_)).Times(1);

  ::policy::CheckPermissionResult output;
  manager->CheckPermissions("12345678", "FULL", "Alert", output);
  EXPECT_EQ(::policy::kRpcAllowed, output.hmi_level_permitted);
  ASSERT_TRUE(!output.list_of_allowed_params.empty());
  ASSERT_EQ(2u, output.list_of_allowed_params.size());
  EXPECT_EQ("speed", output.list_of_allowed_params[0]);
  EXPECT_EQ("gps", output.list_of_allowed_params[1]);
}
#endif  // EXTENDED_POLICY

TEST_F(PolicyManagerImplTest, DISABLED_LoadPT) {
  // TODO(KKolodiy): PolicyManagerImpl is hard for testing
  MockPolicyListener mock_listener;

  Json::Value table(Json::objectValue);
  table["policy_table"] = Json::Value(Json::objectValue);

  Json::Value& policy_table = table["policy_table"];
  policy_table["module_meta"] = Json::Value(Json::objectValue);
  policy_table["module_config"] = Json::Value(Json::objectValue);
  policy_table["usage_and_error_counts"] = Json::Value(Json::objectValue);
  policy_table["device_data"] = Json::Value(Json::objectValue);
  policy_table["functional_groupings"] = Json::Value(Json::objectValue);
  policy_table["consumer_friendly_messages"] = Json::Value(Json::objectValue);
  policy_table["app_policies"] = Json::Value(Json::objectValue);
  policy_table["app_policies"]["1234"] = Json::Value(Json::objectValue);

  Json::Value& module_meta = policy_table["module_meta"];
  module_meta["ccpu_version"] = Json::Value("ccpu version");
  module_meta["language"] = Json::Value("ru");
  module_meta["wers_country_code"] = Json::Value("ru");
  module_meta["pt_exchanged_at_odometer_x"] = Json::Value(0);
  module_meta["pt_exchanged_x_days_after_epoch"] = Json::Value(0);
  module_meta["ignition_cycles_since_last_exchange"] = Json::Value(0);
  module_meta["vin"] = Json::Value("vin");

  Json::Value& module_config = policy_table["module_config"];
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
  module_config["vehicle_year"] = Json::Value(2014);

  Json::Value& usage_and_error_counts = policy_table["usage_and_error_counts"];
  usage_and_error_counts["count_of_iap_buffer_full"] = Json::Value(0);
  usage_and_error_counts["count_sync_out_of_memory"] = Json::Value(0);
  usage_and_error_counts["count_of_sync_reboots"] = Json::Value(0);

  Json::Value& device_data = policy_table["device_data"];
  device_data["DEVICEHASH"] = Json::Value(Json::objectValue);
  device_data["DEVICEHASH"]["hardware"] = Json::Value("hardware");
  device_data["DEVICEHASH"]["firmware_rev"] = Json::Value("firmware_rev");
  device_data["DEVICEHASH"]["os"] = Json::Value("os");
  device_data["DEVICEHASH"]["os_version"] = Json::Value("os_version");
  device_data["DEVICEHASH"]["carrier"] = Json::Value("carrier");
  device_data["DEVICEHASH"]["max_number_rfcom_ports"] = Json::Value(10);

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

  Json::Value& app_policies = policy_table["app_policies"];
  app_policies["default"] = Json::Value(Json::objectValue);
  app_policies["default"]["memory_kb"] = Json::Value(50);
  app_policies["default"]["watchdog_timer_ms"] = Json::Value(100);
  app_policies["default"]["groups"] = Json::Value(Json::arrayValue);
  app_policies["default"]["groups"][0] = Json::Value("default");
  app_policies["default"]["priority"] = Json::Value("EMERGENCY");
  app_policies["default"]["default_hmi"] = Json::Value("FULL");
  app_policies["default"]["keep_context"] = Json::Value(true);
  app_policies["default"]["steal_focus"] = Json::Value(true);
  app_policies["default"]["certificate"] = Json::Value("sign");

  std::string json = table.toStyledString();
  ::policy::BinaryMessage msg(json.begin(), json.end());

  EXPECT_CALL(*cache_manager, Save(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(mock_listener,
                                         OnUpdateStatusChanged(_))
.Times(1);

  manager->set_listener(&mock_listener);

  EXPECT_TRUE(manager->LoadPT("file_pt_update.json", msg));
}

TEST_F(PolicyManagerImplTest, RequestPTUpdate) {
  ::utils::SharedPtr< ::policy_table::Table> p_table =
      new ::policy_table::Table();
  std::string json = p_table->ToJsonValue().toStyledString();
  ::policy::BinaryMessageSptr expect = new ::policy::BinaryMessage(json.begin(),
      json.end());

  EXPECT_CALL(*cache_manager, GenerateSnapshot()).WillOnce(Return(p_table));

  ::policy::BinaryMessageSptr output = manager->RequestPTUpdate();
  EXPECT_EQ(*expect, *output);
}

#ifdef EXTENDED_POLICY
TEST_F(PolicyManagerImplTest, ResetUserConsent) {
  EXPECT_CALL(*cache_manager, ResetUserConsent()).WillOnce(Return(true)).WillOnce(
      Return(false));

  EXPECT_TRUE(manager->ResetUserConsent());
  EXPECT_FALSE(manager->ResetUserConsent());
}
#endif  // EXTENDED_POLICY

TEST_F(PolicyManagerImplTest, AddApplication) {
  // TODO(AOleynik): Implementation of method should be changed to avoid
  // using of snapshot
  //manager->AddApplication("12345678");
}

TEST_F(PolicyManagerImplTest, GetPolicyTableStatus) {
  // TODO(AOleynik): Test is not finished, to be continued
  //manager->GetPolicyTableStatus();
}

#ifdef EXTENDED_POLICY
TEST_F(PolicyManagerImplTest, MarkUnpairedDevice) {
  EXPECT_CALL(*cache_manager, SetUnpairedDevice("12345")).WillOnce(Return(true));

  manager->MarkUnpairedDevice("12345");
}
#endif  // EXTENDED_POLICY

}  // namespace policy
}  // namespace components
}  // namespace test
