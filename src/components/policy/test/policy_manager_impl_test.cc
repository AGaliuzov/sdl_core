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
#include "policy/update_status_manager.h"
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
using ::policy::MockUpdateStatusManager;
using ::policy::PolicyManagerImpl;
using ::policy::PolicyTable;
using ::policy::EndpointUrls;
using ::policy::CacheManagerInterfaceSPtr;

namespace policy_table = rpc::policy_table_interface_base;

namespace test {
namespace components {
namespace policy {

class PolicyManagerImplTest : public ::testing::Test {
 protected:
  PolicyManagerImpl* manager;
  MockCacheManagerInterface* cache_manager;
  MockUpdateStatusManager update_manager;
  MockPolicyListener* listener;

  void SetUp() {
    manager = new PolicyManagerImpl();

    cache_manager = new MockCacheManagerInterface();
    manager->set_cache_manager(cache_manager);

    listener = new MockPolicyListener();
    manager->set_listener(listener);
  }

  void TearDown() {
    delete manager;
    delete listener;
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

TEST_F(PolicyManagerImplTest, DISABLED_RefreshRetrySequence) {
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
  EXPECT_CALL(*cache_manager, TimeoutResponse()).Times(1);
  EXPECT_CALL(*cache_manager, SecondsBetweenRetries(_)).Times(1);

  EXPECT_TRUE(manager->ResetPT("filename"));
  EXPECT_FALSE(manager->ResetPT("filename"));
}

#ifdef EXTENDED_POLICY
TEST_F(PolicyManagerImplTest, DISABLED_CheckPermissions) {
  policy_table::RpcParameters rpc_parameters;
  rpc_parameters.hmi_levels.push_back(policy_table::HL_FULL);
  rpc_parameters.parameters->push_back(policy_table::P_SPEED);
  rpc_parameters.parameters->push_back(policy_table::P_GPS);
  rpc_parameters.parameters->push_back(policy_table::P_ODOMETER);

  policy_table::Rpc rpc;
  rpc["Alert"] = rpc_parameters;

  policy_table::Rpcs rpcs;
  rpcs.rpcs = rpc;

  policy_table::FunctionalGroupings groups;
  groups["AlertGroup"] = rpcs;

  ::policy::FunctionalIdType group_types;
  group_types[::policy::kTypeAllowed].push_back(1);

  ::policy::FunctionalGroupNames group_names;
  group_names[1] = std::make_pair<std::string, std::string>("", "AlertGroup");

  EXPECT_CALL(*listener, OnCurrentDeviceIdUpdateRequired("12345678"))
      .Times(1);
  EXPECT_CALL(*cache_manager, GetFunctionalGroupings(_)).Times(1)
      .WillOnce(DoAll(SetArgReferee<0>(groups), Return(true)));
  EXPECT_CALL(*cache_manager, IsDefaultPolicy("12345678")).Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(*cache_manager, IsPredataPolicy("12345678")).Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(*cache_manager, GetPermissionsForApp(_, "12345678", _)).Times(1)
      .WillOnce(DoAll(SetArgReferee<2>(group_types),Return(true)));
  EXPECT_CALL(*cache_manager, GetFunctionalGroupNames(_)).Times(1)
      .WillOnce(DoAll(SetArgReferee<0>(group_names), Return(true)));

  ::policy::RPCParams input_params;
  ::policy::CheckPermissionResult output;
  manager->CheckPermissions("12345678", "FULL", "Alert", input_params, output);
  EXPECT_EQ(::policy::kRpcAllowed, output.hmi_level_permitted);
  ASSERT_TRUE(!output.list_of_allowed_params.empty());
  ASSERT_EQ(3u, output.list_of_allowed_params.size());
  EXPECT_EQ("gps", output.list_of_allowed_params[0]);
  EXPECT_EQ("odometer", output.list_of_allowed_params[1]);
  EXPECT_EQ("speed", output.list_of_allowed_params[2]);
}
#else  // EXTENDED_POLICY
TEST_F(PolicyManagerImplTest, CheckPermissions) {
  ::policy::CheckPermissionResult expected;
  expected.hmi_level_permitted = ::policy::kRpcAllowed;
  expected.list_of_allowed_params.push_back("speed");
  expected.list_of_allowed_params.push_back("gps");

  EXPECT_CALL(*cache_manager, CheckPermissions("12345678", "FULL", "Alert", _))
      .Times(1).WillOnce(SetArgReferee<3>(expected));

  ::policy::RPCParams input_params;
  ::policy::CheckPermissionResult output;
  manager->CheckPermissions("12345678", "FULL", "Alert", input_params, output);

  EXPECT_EQ(::policy::kRpcAllowed, output.hmi_level_permitted);
  ASSERT_TRUE(!output.list_of_allowed_params.empty());
  ASSERT_EQ(2u, output.list_of_allowed_params.size());
  EXPECT_EQ("speed", output.list_of_allowed_params[0]);
  EXPECT_EQ("gps", output.list_of_allowed_params[1]);
}
#endif  // EXTENDED_POLICY

TEST_F(PolicyManagerImplTest, LoadPT) {
  Json::Value table(Json::objectValue);
  table["policy_table"] = Json::Value(Json::objectValue);

  Json::Value& policy_table = table["policy_table"];
  policy_table["module_config"] = Json::Value(Json::objectValue);
  policy_table["functional_groupings"] = Json::Value(Json::objectValue);
  policy_table["consumer_friendly_messages"] = Json::Value(Json::objectValue);
  policy_table["app_policies"] = Json::Value(Json::objectValue);

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
  app_policies["default"]["certificate"] = Json::Value("sign");
  app_policies["1234"] = Json::Value(Json::objectValue);
  app_policies["1234"]["memory_kb"] = Json::Value(50);
  app_policies["1234"]["heart_beat_timeout_ms"] = Json::Value(100);
  app_policies["1234"]["groups"] = Json::Value(Json::arrayValue);
  app_policies["1234"]["groups"][0] = Json::Value("default");
  app_policies["1234"]["priority"] = Json::Value("EMERGENCY");
  app_policies["1234"]["default_hmi"] = Json::Value("FULL");
  app_policies["1234"]["keep_context"] = Json::Value(true);
  app_policies["1234"]["steal_focus"] = Json::Value(true);
  app_policies["1234"]["certificate"] = Json::Value("sign");

  policy_table::Table update(&table);
  update.SetPolicyTableType(rpc::policy_table_interface_base::PT_UPDATE);
  ASSERT_TRUE(IsValid(update));

  std::string json = table.toStyledString();
  ::policy::BinaryMessage msg(json.begin(), json.end());

  utils::SharedPtr<policy_table::Table> snapshot =
      new policy_table::Table(update.policy_table);

  EXPECT_CALL(*cache_manager, GenerateSnapshot()).Times(1).WillOnce(Return(snapshot));
  EXPECT_CALL(*cache_manager, ApplyUpdate(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(*listener, GetAppName("1234")).Times(1).WillOnce(Return(""));
  EXPECT_CALL(*listener, OnUpdateStatusChanged(_)).Times(1);
  EXPECT_CALL(*cache_manager, SaveUpdateRequired(false)).Times(1);
  EXPECT_CALL(*cache_manager, TimeoutResponse()).Times(1);
  EXPECT_CALL(*cache_manager, SecondsBetweenRetries(_)).Times(1);


  EXPECT_TRUE(manager->LoadPT("file_pt_update.json", msg));
}

TEST_F(PolicyManagerImplTest, RequestPTUpdate) {
  ::utils::SharedPtr< ::policy_table::Table> p_table =
      new ::policy_table::Table();

  EXPECT_CALL(*cache_manager, GenerateSnapshot()).WillOnce(Return(p_table));
  manager->RequestPTUpdate();
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
  EXPECT_CALL(*cache_manager, SetUnpairedDevice("12345", true)).WillOnce(Return(true));
  EXPECT_CALL(*cache_manager, GetDeviceGroupsFromPolicies(_, _)).Times(1);
  manager->MarkUnpairedDevice("12345");
}
#endif  // EXTENDED_POLICY

}  // namespace policy
}  // namespace components
}  // namespace test
