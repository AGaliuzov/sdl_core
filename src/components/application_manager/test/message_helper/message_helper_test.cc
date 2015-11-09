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

#include <fstream>
#include <string>

#include "gmock/gmock.h"
#include "utils/macro.h"
#include "utils/make_shared.h"
#include "application_manager/policies/policy_handler.h"
#include "config_profile/profile.h"
#include "policy/policy_manager_impl.h"
#include "connection_handler/connection_handler_impl.h"
#include "encryption/hashing.h"
#include "application_manager/test/resumption/include/application_mock.h"
#include "application_manager/application.h"

namespace application_manager {
namespace test {

namespace HmiLanguage = hmi_apis::Common_Language;
namespace HmiResults = hmi_apis::Common_Result;
namespace MobileResults = mobile_apis::Result;

typedef ::test::components::resumption_test::ApplicationMock AppMock;
typedef ::application_manager::Application App;

typedef utils::SharedPtr<AppMock> ApplicationMockSharedPtr;
typedef utils::SharedPtr<App> ApplicationSharedPtr;

using testing::Return;
using testing::AtLeast;
using testing::ReturnRefOfCopy;
using testing::ReturnRef;

TEST(MessageHelperTestCreate,
   CreateHashUpdateNotification_FunctionId_Equal) {
  uint32_t app_id = 0;
  smart_objects::SmartObjectSPtr ptr =
      MessageHelper::CreateHashUpdateNotification(app_id);

  EXPECT_TRUE(ptr);

  smart_objects::SmartObject& obj = *ptr;
  
  int function_id =
      static_cast<int>(mobile_apis::FunctionID::OnHashChangeID);
  int notification = static_cast<int>(kNotification);

  EXPECT_EQ(function_id, obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(app_id, obj[strings::params][strings::connection_key].asInt());
  EXPECT_EQ(notification,
      obj[strings::params][strings::message_type].asInt());
}

TEST(MessageHelperTestCreate,
   CreateBlockedByPoliciesResponse_SmartObject_Equal) {
  mobile_apis::FunctionID::eType function_id =
      mobile_apis::FunctionID::eType::AddCommandID;
  mobile_apis::Result::eType result = mobile_apis::Result::eType::ABORTED;
  uint32_t correlation_id = 0;
  uint32_t connection_key = 0;
  bool success = false;
  
  smart_objects::SmartObjectSPtr ptr =
      MessageHelper::CreateBlockedByPoliciesResponse(
              function_id, result, correlation_id, connection_key);

  EXPECT_TRUE(ptr);

  smart_objects::SmartObject& obj = *ptr;
  
  EXPECT_EQ(function_id,
      obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(kResponse,
      obj[strings::params][strings::message_type].asInt());
  EXPECT_EQ(success,
      obj[strings::msg_params][strings::success].asBool());
  EXPECT_EQ(result,
      obj[strings::msg_params][strings::result_code].asInt());
  EXPECT_EQ(correlation_id,
      obj[strings::params][strings::correlation_id].asInt());
  EXPECT_EQ(connection_key,
      obj[strings::params][strings::connection_key].asInt());
  EXPECT_EQ(kV2,
      obj[strings::params][strings::protocol_version].asInt());
}

TEST(MessageHelperTestCreate,
   CreateSetAppIcon_SendNullPathImagetype_Equal) {
  std::string path_to_icon = "";
  uint32_t app_id = 0;
  smart_objects::SmartObjectSPtr ptr =
      MessageHelper::CreateSetAppIcon(path_to_icon, app_id);

  EXPECT_TRUE(ptr);

  smart_objects::SmartObject& obj = *ptr;
  
  int image_type = static_cast<int>(mobile_api::ImageType::DYNAMIC);
  
  EXPECT_EQ(path_to_icon,
      obj[strings::sync_file_name][strings::value].asString());
  EXPECT_EQ(image_type,
      obj[strings::sync_file_name][strings::image_type].asInt());
  EXPECT_EQ(app_id, obj[strings::app_id].asInt());
}

TEST(MessageHelperTestCreate,
   CreateSetAppIcon_SendPathImagetype_Equal) {
  std::string path_to_icon = "/qwe/qwe/";
  uint32_t app_id = 10;
  smart_objects::SmartObjectSPtr ptr =
      MessageHelper::CreateSetAppIcon(path_to_icon, app_id);

  EXPECT_TRUE(ptr);

  smart_objects::SmartObject& obj = *ptr;

  int image_type = static_cast<int>(mobile_api::ImageType::DYNAMIC);

  EXPECT_EQ(path_to_icon,
      obj[strings::sync_file_name][strings::value].asString());
  EXPECT_EQ(image_type,
      obj[strings::sync_file_name][strings::image_type].asInt());
  EXPECT_EQ(app_id, obj[strings::app_id].asInt());
}

TEST(MessageHelperTestCreate,
   CreateGlobalPropertiesRequestsToHMI_SmartObject_EmptyList) {
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  
  EXPECT_CALL(*appSharedMock, vr_help_title()).Times(AtLeast(1));
  EXPECT_CALL(*appSharedMock, vr_help()).Times(AtLeast(1));
  EXPECT_CALL(*appSharedMock, help_prompt()).Times(AtLeast(1));
  EXPECT_CALL(*appSharedMock, timeout_prompt()).Times(AtLeast(1));
  
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateGlobalPropertiesRequestsToHMI(appSharedMock);
  
  EXPECT_TRUE(ptr.empty());
}

TEST(MessageHelperTestCreate,
   CreateGlobalPropertiesRequestsToHMI_SmartObject_NotEmpty) {
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  smart_objects::SmartObjectSPtr objPtr =
      MakeShared<smart_objects::SmartObject>();

  (*objPtr)[0][strings::vr_help_title] = "111";
  (*objPtr)[1][strings::vr_help] = "222";
  (*objPtr)[2][strings::keyboard_properties] = "333";
  (*objPtr)[3][strings::menu_title] = "444";
  (*objPtr)[4][strings::menu_icon] = "555";
  (*objPtr)[5][strings::help_prompt] = "666";
  (*objPtr)[6][strings::timeout_prompt] = "777";
  
  EXPECT_CALL(*appSharedMock,
      vr_help_title()).Times(AtLeast(3)).WillRepeatedly(Return(&(*objPtr)[0]));
  EXPECT_CALL(*appSharedMock,
      vr_help()).Times(AtLeast(2)).WillRepeatedly(Return(&(*objPtr)[1]));
  EXPECT_CALL(*appSharedMock,
      help_prompt()).Times(AtLeast(3)).WillRepeatedly(Return(&(*objPtr)[5]));
  EXPECT_CALL(*appSharedMock,
      timeout_prompt()).Times(AtLeast(2)).WillRepeatedly(Return(&(*objPtr)[6]));
  EXPECT_CALL(*appSharedMock,
      keyboard_props()).Times(AtLeast(2)).WillRepeatedly(Return(&(*objPtr)[2]));
  EXPECT_CALL(*appSharedMock,
      menu_title()).Times(AtLeast(2)).WillRepeatedly(Return(&(*objPtr)[3]));
  EXPECT_CALL(*appSharedMock,
      menu_icon()).Times(AtLeast(2)).WillRepeatedly(Return(&(*objPtr)[4]));
  EXPECT_CALL(*appSharedMock, app_id()).WillRepeatedly(Return(0));
  
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateGlobalPropertiesRequestsToHMI(appSharedMock);

  EXPECT_FALSE(ptr.empty());

  smart_objects::SmartObject& first = *ptr[0];
  smart_objects::SmartObject& second = *ptr[1];
  
  EXPECT_EQ((*objPtr)[0], first[strings::msg_params][strings::vr_help_title]);
  EXPECT_EQ((*objPtr)[1], first[strings::msg_params][strings::vr_help]);
  EXPECT_EQ((*objPtr)[2], first[strings::msg_params][strings::keyboard_properties]);
  EXPECT_EQ((*objPtr)[3], first[strings::msg_params][strings::menu_title]);
  EXPECT_EQ((*objPtr)[4], first[strings::msg_params][strings::menu_icon]);
  EXPECT_EQ((*objPtr)[5], second[strings::msg_params][strings::help_prompt]);
  EXPECT_EQ((*objPtr)[6], second[strings::msg_params][strings::timeout_prompt]);
}

TEST(MessageHelperTestCreate, CreateAppVrHelp_AppName_Equal) {
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  
  application_manager::CommandsMap vis;
  DataAccessor< ::application_manager::CommandsMap>
      data_accessor(vis, true);
  
  const smart_objects::SmartObject* objPtr = NULL;
  const std::string app_name = "213";
  EXPECT_CALL(*appSharedMock, name() ).WillOnce(ReturnRefOfCopy(app_name));
  EXPECT_CALL(*appSharedMock,
      vr_synonyms() ).Times(AtLeast(1)).WillRepeatedly(Return(objPtr));
  EXPECT_CALL(*appSharedMock,
      commands_map() ).WillOnce(Return(data_accessor));
  
  smart_objects::SmartObjectSPtr ptr =
      MessageHelper::CreateAppVrHelp(appSharedMock);

  EXPECT_TRUE(ptr);

  smart_objects::SmartObject& obj = *ptr;
  
  EXPECT_EQ(app_name, obj[strings::vr_help_title].asString());
}

TEST(MessageHelperTestCreate,
   CreateShowRequestToHMI_SendSmartObject_Equal) {
  ApplicationMockSharedPtr appSharedMock =
          utils::MakeShared<AppMock>();
  
  smart_objects::SmartObjectSPtr smartObjectPtr =
      utils::MakeShared<smart_objects::SmartObject>();

  const smart_objects::SmartObject& object = *smartObjectPtr;
  
  EXPECT_CALL(*appSharedMock,
      show_command()).Times(AtLeast(2)).WillRepeatedly(Return(&object));
  
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateShowRequestToHMI(appSharedMock);

  EXPECT_FALSE(ptr.empty());

  smart_objects::SmartObject& obj = *ptr[0];
  
  int function_id = static_cast<int>(hmi_apis::FunctionID::UI_Show);
  
  EXPECT_EQ(function_id, obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(*smartObjectPtr, obj[strings::msg_params]);
}

TEST(MessageHelperTestCreate,
   CreateAddCommandRequestToHMI_SendSmartObject_Empty) {
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  CommandsMap vis;
  DataAccessor< CommandsMap> data_accessor(vis, true);
  
  EXPECT_CALL(*appSharedMock,
      commands_map()).WillOnce(Return(data_accessor));
  
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddCommandRequestToHMI(appSharedMock);
  
  EXPECT_TRUE(ptr.empty());
}

TEST(MessageHelperTestCreate,
    CreateAddCommandRequestToHMI_SendSmartObject_Equal) {
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  CommandsMap vis;
  DataAccessor< CommandsMap> data_accessor(vis, true);
  smart_objects::SmartObjectSPtr smartObjectPtr =
      utils::MakeShared<smart_objects::SmartObject>();

  smart_objects::SmartObject& object = *smartObjectPtr;

  object[strings::menu_params] = 1;
  object[strings::cmd_icon] = 1;
  object[strings::cmd_icon][strings::value] = "10";

  vis.insert(std::pair<uint32_t,
      smart_objects::SmartObject*>(5, &object));

  EXPECT_CALL(*appSharedMock,
      commands_map()).WillOnce(Return(data_accessor));
  EXPECT_CALL(*appSharedMock,
      app_id()).WillOnce(Return(1u));  

  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddCommandRequestToHMI(appSharedMock);

  EXPECT_FALSE(ptr.empty());

  smart_objects::SmartObject& obj = *ptr[0];  

  int function_id = static_cast<int>(hmi_apis::FunctionID::UI_AddCommand);  

  EXPECT_EQ(function_id, obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(1, obj[strings::msg_params][strings::app_id].asInt());
  EXPECT_EQ(5, obj[strings::msg_params][strings::cmd_id].asInt());
  EXPECT_EQ(object[strings::menu_params],
      obj[strings::msg_params][strings::menu_params]);
  EXPECT_EQ(object[strings::cmd_icon],
      obj[strings::msg_params][strings::cmd_icon]);
  EXPECT_EQ("10", obj[strings::msg_params]
      [strings::cmd_icon][strings::value].asString());
}

TEST(MessageHelperTestCreate,
   CreateAddVRCommandRequestFromChoiceToHMI_SendEmptyData_EmptyList) {
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  application_manager::ChoiceSetMap vis;
  DataAccessor< ::application_manager::ChoiceSetMap> data_accessor(vis, true);
  
  EXPECT_CALL(*appSharedMock,
      choice_set_map()).WillOnce(Return(data_accessor));
  
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddVRCommandRequestFromChoiceToHMI(appSharedMock);
  
  EXPECT_TRUE(ptr.empty());
}

TEST(MessageHelperTestCreate,
  CreateAddVRCommandRequestFromChoiceToHMI_SendObject_EqualList) {
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  application_manager::ChoiceSetMap vis;
  DataAccessor< ::application_manager::ChoiceSetMap> data_accessor(vis, true);
  smart_objects::SmartObjectSPtr smartObjectPtr =
      utils::MakeShared<smart_objects::SmartObject>();

  smart_objects::SmartObject& object = *smartObjectPtr;
  
  object[strings::choice_set] = "10";
  object[strings::grammar_id] = 111;
  object[strings::choice_set][0][strings::choice_id] = 1;
  object[strings::choice_set][0][strings::vr_commands] = 2;

  vis.insert(std::pair<uint32_t,
      smart_objects::SmartObject*>(5, &object));
  vis.insert(std::pair<uint32_t,
      smart_objects::SmartObject*>(6, &object));
  vis.insert(std::pair<uint32_t,
      smart_objects::SmartObject*>(7, &object));
  vis.insert(std::pair<uint32_t,
      smart_objects::SmartObject*>(8, &object));
  vis.insert(std::pair<uint32_t,
      smart_objects::SmartObject*>(9, &object));

  EXPECT_CALL(*appSharedMock,
      choice_set_map()).WillOnce(Return(data_accessor));
  EXPECT_CALL(*appSharedMock,
      app_id()).Times(AtLeast(5)).WillRepeatedly(Return(1u));
  
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddVRCommandRequestFromChoiceToHMI(appSharedMock);

  EXPECT_FALSE(ptr.empty());

  int function_id = static_cast<int>(hmi_apis::FunctionID::VR_AddCommand);
  int type = static_cast<int>(hmi_apis::Common_VRCommandType::Choice);

  smart_objects::SmartObject& obj = *ptr[0];

  EXPECT_EQ(function_id, obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(1, obj[strings::msg_params][strings::app_id].asUInt());
  EXPECT_EQ(111, obj[strings::msg_params][strings::grammar_id].asUInt());
  EXPECT_EQ(object[strings::choice_set][0][strings::choice_id],
      obj[strings::msg_params][strings::cmd_id]);
  EXPECT_EQ(object[strings::choice_set][0][strings::vr_commands],
      obj[strings::msg_params][strings::vr_commands]);
  EXPECT_EQ(type, obj[strings::msg_params][strings::type].asInt());
}

TEST(MessageHelperTestCreate, CreateAddSubMenuRequestToHMI_SendObject_Equal) {
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  application_manager::SubMenuMap vis;
  DataAccessor< ::application_manager::SubMenuMap> data_accessor(vis, true);
  smart_objects::SmartObjectSPtr smartObjectPtr =
      utils::MakeShared<smart_objects::SmartObject>();

  smart_objects::SmartObject& object = *smartObjectPtr;

  object[strings::position] = 1;
  object[strings::menu_name] = 1;

  vis.insert(std::pair<uint32_t,
      smart_objects::SmartObject*>(5, &object));

  EXPECT_CALL(*appSharedMock,
      sub_menu_map() ).WillOnce(Return(data_accessor));
  EXPECT_CALL(*appSharedMock,
      app_id()).Times(AtLeast(1)).WillOnce(Return(1u));

  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddSubMenuRequestToHMI(appSharedMock);

  EXPECT_FALSE(ptr.empty());

  smart_objects::SmartObject& obj = *ptr[0];

  int function_id = static_cast<int>(hmi_apis::FunctionID::UI_AddSubMenu);

  EXPECT_EQ(function_id,
      obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(5,
      obj[strings::msg_params][strings::menu_id].asInt());
  EXPECT_EQ(1,
      obj[strings::msg_params]
          [strings::menu_params][strings::position].asInt());
  EXPECT_EQ(1, obj[strings::msg_params]
      [strings::menu_params][strings::menu_name].asInt());
  EXPECT_EQ(1,
      obj[strings::msg_params][strings::app_id].asUInt());
}

TEST(MessageHelperTestCreate,
  CreateAddSubMenuRequestToHMI_SendEmptyMap_EmptySmartObjectList) {
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  application_manager::SubMenuMap vis;
  DataAccessor< ::application_manager::SubMenuMap> data_accessor(vis, true);

  EXPECT_CALL(*appSharedMock,
      sub_menu_map() ).WillOnce(Return(data_accessor));

  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddSubMenuRequestToHMI(appSharedMock);

  EXPECT_TRUE(ptr.empty());
}

TEST(MessageHelperTestCreate, CreateNegativeResponse_SendSmartObject_Equal) {
  uint32_t connection_key = 111;
  int32_t function_id = 222;
  uint32_t correlation_id = 333;
  int32_t result_code = 0;

  smart_objects::SmartObjectSPtr ptr =
      MessageHelper::CreateNegativeResponse(connection_key, function_id,
          correlation_id, result_code);

  EXPECT_TRUE(ptr);

  smart_objects::SmartObject& obj = *ptr;

  int objFunction_id =
      obj[strings::params][strings::function_id].asInt();
  int objCorrelation_id =
      obj[strings::params][strings::correlation_id].asInt();
  int objResult_code =
      obj[strings::msg_params][strings::result_code].asInt();
  int objConnection_key =
     obj[strings::params][strings::connection_key].asInt();

  int message_type =
      static_cast<int>(mobile_apis::messageType::response);
  int protocol_type =
      static_cast<int>(commands::CommandImpl::mobile_protocol_type_);
  int protocol_version =
      static_cast<int>(commands::CommandImpl::protocol_version_);
  bool success = false;

  EXPECT_EQ(function_id, objFunction_id);
  EXPECT_EQ(message_type,
      obj[strings::params][strings::message_type].asInt());
  EXPECT_EQ(protocol_type,
      obj[strings::params][strings::protocol_type].asInt());
  EXPECT_EQ(protocol_version,
      obj[strings::params][strings::protocol_version].asInt());
  EXPECT_EQ(correlation_id, objCorrelation_id);
  EXPECT_EQ(result_code, objResult_code);
  EXPECT_EQ(success,
      obj[strings::msg_params][strings::success].asBool());
  EXPECT_EQ(connection_key, objConnection_key);
}

class MessageHelperTest : public ::testing::Test {
  public:
    MessageHelperTest(){}
  protected:
    policy::PolicyHandler* handler_;

    const policy::StringArray language_strings = {
      "EN-US", "ES-MX", "FR-CA", "DE-DE", "ES-ES", "EN-GB", "RU-RU", "TR-TR",
      "PL-PL", "FR-FR", "IT-IT", "SV-SE", "PT-PT", "NL-NL", "EN-AU", "ZH-CN",
      "ZH-TW", "JA-JP", "AR-SA", "KO-KR", "PT-BR", "CS-CZ", "DA-DK", "NO-NO",
      "NL-BE", "EL-GR", "HU-HU", "FI-FI", "SK-SK"
    };

    const policy::StringArray hmi_result_strings = {
      "SUCCESS", "UNSUPPORTED_REQUEST", "UNSUPPORTED_RESOURCE",
      "DISALLOWED", "REJECTED", "ABORTED",
      "IGNORED", "RETRY", "IN_USE",
      "DATA_NOT_AVAILABLE", "TIMED_OUT", "INVALID_DATA",
      "CHAR_LIMIT_EXCEEDED", "INVALID_ID", "DUPLICATE_NAME",
      "APPLICATION_NOT_REGISTERED", "WRONG_LANGUAGE", "OUT_OF_MEMORY",
      "TOO_MANY_PENDING_REQUESTS", "NO_APPS_REGISTERED", "NO_DEVICES_CONNECTED",
      "WARNINGS", "GENERIC_ERROR", "USER_DISALLOWED",
      "TRUNCATED_DATA"
    };

    const policy::StringArray mobile_result_strings = {
      "SUCCESS", "UNSUPPORTED_REQUEST", "UNSUPPORTED_RESOURCE",
      "DISALLOWED", "REJECTED", "ABORTED",
      "IGNORED", "RETRY", "IN_USE",
      "VEHICLE_DATA_NOT_AVAILABLE", "TIMED_OUT", "INVALID_DATA",
      "CHAR_LIMIT_EXCEEDED", "INVALID_ID", "DUPLICATE_NAME",
      "APPLICATION_NOT_REGISTERED", "WRONG_LANGUAGE", "OUT_OF_MEMORY",
      "TOO_MANY_PENDING_REQUESTS", "TOO_MANY_APPLICATIONS",
      "APPLICATION_REGISTERED_ALREADY", "WARNINGS", "GENERIC_ERROR",
      "USER_DISALLOWED", "UNSUPPORTED_VERSION", "VEHICLE_DATA_NOT_ALLOWED",
      "FILE_NOT_FOUND", "CANCEL_ROUTE", "TRUNCATED_DATA",
      "SAVED", "INVALID_CERT", "EXPIRED_CERT", "RESUME_FAILED"
    };

    virtual void SetUp() OVERRIDE {

    }
    virtual void TearDown() OVERRIDE {}

    void EnablePolicy() {
      handler_ = policy::PolicyHandler::instance();
      ASSERT_TRUE(NULL != handler_);
      // Change default ini file to test ini file with policy enabled value
      profile::Profile::instance()->
          config_file_name("smartDeviceLink_test2.ini");
      ASSERT_TRUE(handler_->PolicyEnabled());
    }

    void FillPolicytable() {
      utils::SharedPtr<policy::PolicyManager> shared_pm =
          utils::MakeShared<policy::PolicyManagerImpl>();
      (*shared_pm).set_listener(handler_);

      const std::string file_name = "sdl_pt_update.json";
      LoadPolicyTableToPolicyManager(file_name, *shared_pm);

      handler_->SetPolicyManager(shared_pm);
    }

    void LoadPolicyTableToPolicyManager(const std::string& file_name,
        policy::PolicyManager& pm){
      // Get PTU
      std::ifstream ifile(file_name);
      Json::Reader reader;
      std::string json;
      Json::Value root(Json::objectValue);
      if (ifile != NULL && reader.parse(ifile, root, true)) {
        json = root.toStyledString();
      }
      ifile.close();

      ::policy::BinaryMessage msg(json.begin(), json.end());
      // Load Json to cache
      EXPECT_TRUE(pm.LoadPT("file_pt_update.json", msg));
    }

    void AddDeviceToConnectionHandler(const std::string& mac,
        const u_int32_t handle = 1,
        const std::string& name = "Device1",
        const std::string& connection_type = "ConnType") {
      transport_manager::DeviceInfo
          fake_device1( handle, mac, name, connection_type );
      connection_handler::ConnectionHandlerImpl::instance()->
          OnDeviceAdded(fake_device1);
    }
};

TEST_F(MessageHelperTest, CheckWithPolicy) {
  // Enabling and filling policy table
  EnablePolicy();
  FillPolicytable();

  // Always true
  EXPECT_TRUE(MessageHelper::CheckWithPolicy(
      mobile_apis::SystemAction::DEFAULT_ACTION,"AnyAppId"));
  // Always false
  EXPECT_FALSE(MessageHelper::CheckWithPolicy(
      mobile_apis::SystemAction::INVALID_ENUM,"AnyAppId"));

  EXPECT_TRUE(MessageHelper::CheckWithPolicy(
      mobile_apis::SystemAction::KEEP_CONTEXT,"1766825573"));
  EXPECT_TRUE(MessageHelper::CheckWithPolicy(
      mobile_apis::SystemAction::STEAL_FOCUS,"1766825573"));
}

TEST_F(MessageHelperTest, CommonLanguageFromString) {
  for(u_int32_t array_index = 0;
      array_index < language_strings.size();
      ++array_index) {
    EXPECT_EQ(static_cast<HmiLanguage::eType>(array_index),
        MessageHelper::CommonLanguageFromString(
            language_strings[array_index]));
  }
  EXPECT_EQ(HmiLanguage::INVALID_ENUM,
      MessageHelper::CommonLanguageFromString(""));
}

TEST_F(MessageHelperTest, CommonLanguageToString) {
  for(u_int32_t array_index = 0;
      array_index < language_strings.size();
      ++array_index) {
    EXPECT_EQ(language_strings[array_index],
        MessageHelper::CommonLanguageToString(
            static_cast<HmiLanguage::eType>(array_index)));
  }
  EXPECT_EQ("", MessageHelper::CommonLanguageToString(
      HmiLanguage::INVALID_ENUM));
}

TEST_F(MessageHelperTest,ConvertEnumAPINoCheck) {
  hmi_apis::Common_AppHMIType::eType converted =
      MessageHelper::ConvertEnumAPINoCheck <hmi_apis::Common_LayoutMode::eType,
          hmi_apis::Common_AppHMIType::eType>(
              hmi_apis::Common_LayoutMode::ICON_ONLY);
  EXPECT_EQ(hmi_apis::Common_AppHMIType::DEFAULT, converted);
}

TEST_F( MessageHelperTest, GetAppCommandLimit ) {
  // Enabling and filling policy table
  EnablePolicy();
  FillPolicytable();

  const std::string app_id = "1766825573";
  EXPECT_EQ(60u, MessageHelper::GetAppCommandLimit(app_id));
  EXPECT_EQ(0u, MessageHelper::GetAppCommandLimit("default"));
}

TEST_F( MessageHelperTest, GetConnectedDevicesMAC ) {
  // Adding some device to connectionHandler
  const std::string device_mac = "50:46:5d:4d:96:c1";
  AddDeviceToConnectionHandler(device_mac);

  // Device macs check
  std::vector<std::string> device_macs;
  MessageHelper::GetConnectedDevicesMAC(device_macs);

  EXPECT_EQ(encryption::MakeHash(device_mac),device_macs[0]);
}

TEST_F( MessageHelperTest, GetDeviceHandleForMac) {
  // Adding some device to connectionHandler
  const std::string device_mac = "50:46:5d:4d:96:c1";
  AddDeviceToConnectionHandler(device_mac);

  // Check handle
  const u_int32_t received_handle =
      MessageHelper::GetDeviceHandleForMac(encryption::MakeHash(device_mac));
  EXPECT_EQ(1u,received_handle);
}

TEST_F( MessageHelperTest, GetDeviceInfoForHandle) {
  // Adding some device to connectionHandler
  const u_int32_t device_handle = 1;
  const std::string device_mac = "50:46:5d:4d:96:c1";
  const std::string device_name = "Device1";
  const std::string connection_type = "ConnType";
  AddDeviceToConnectionHandler( device_mac, device_handle,
      device_name, connection_type);

  // Check received data
  policy::DeviceParams device_info;
  MessageHelper::GetDeviceInfoForHandle(device_handle, &device_info);

  EXPECT_EQ(encryption::MakeHash(device_mac), device_info.device_mac_address);
  EXPECT_EQ(device_name, device_info.device_name);
  EXPECT_EQ(connection_type, device_info.device_connection_type);
}

TEST_F( MessageHelperTest, GetDeviceMacAddressForHandle) {
  // Adding some device to connectionHandler
  const u_int32_t device_handle =  1;
  const std::string device_mac = "50:46:5d:4d:96:c1";
  AddDeviceToConnectionHandler(device_mac,device_handle);

  // Check handle
  const std::string received_mac =
      MessageHelper::GetDeviceMacAddressForHandle(device_handle);
  EXPECT_EQ(encryption::MakeHash(device_mac),received_mac);
}

TEST_F( MessageHelperTest, HMIResultFromString) {
  for(u_int32_t array_index = 0;
      array_index < hmi_result_strings.size();
      ++array_index) {
    EXPECT_EQ(static_cast<HmiResults::eType>(array_index),
        MessageHelper::HMIResultFromString( hmi_result_strings[array_index] ));
  }
  EXPECT_EQ(HmiResults::INVALID_ENUM,
      MessageHelper::HMIResultFromString(""));
}

TEST_F( MessageHelperTest, HMIResultToString) {
  for(u_int32_t array_index = 0;
      array_index < hmi_result_strings.size();
      ++array_index) {
    EXPECT_EQ(hmi_result_strings[array_index],
        MessageHelper::HMIResultToString(
            static_cast<HmiResults::eType>(array_index)));
  }
  EXPECT_EQ("", MessageHelper::HMIResultToString(
      HmiResults::INVALID_ENUM));
}

TEST_F( MessageHelperTest, HMIToMobileResult) {
  for(u_int32_t enum_index = 0;
      enum_index < hmi_result_strings.size();
      ++enum_index) {
    EXPECT_EQ(
        MessageHelper::MobileResultFromString(hmi_result_strings[enum_index]),
        MessageHelper::HMIToMobileResult(
            static_cast<HmiResults::eType>(enum_index)));
  }
  EXPECT_EQ(MobileResults::INVALID_ENUM, MessageHelper::HMIToMobileResult(
      HmiResults::INVALID_ENUM));
}

TEST_F( MessageHelperTest, MobileResultFromString) {
  for(u_int32_t array_index = 0;
      array_index < mobile_result_strings.size();
      ++array_index) {
    EXPECT_EQ(static_cast<MobileResults::eType>(array_index),
        MessageHelper::MobileResultFromString(
            mobile_result_strings[array_index] ));
  }
  EXPECT_EQ(MobileResults::INVALID_ENUM,
      MessageHelper::MobileResultFromString(""));
}

TEST_F( MessageHelperTest, MobileResultToString) {
  for(u_int32_t array_index = 0;
      array_index < mobile_result_strings.size();
      ++array_index) {
    EXPECT_EQ(mobile_result_strings[array_index],
        MessageHelper::MobileResultToString(
            static_cast<MobileResults::eType>(array_index)));
  }
  EXPECT_EQ("", MessageHelper::MobileResultToString(
      MobileResults::INVALID_ENUM));
}

TEST_F( MessageHelperTest, MobileToHMIResult) {
  for(u_int32_t enum_index = 0;
      enum_index < mobile_result_strings.size();
      ++enum_index) {
    EXPECT_EQ(
        MessageHelper::HMIResultFromString(mobile_result_strings[enum_index]),
        MessageHelper::MobileToHMIResult(
            static_cast<MobileResults::eType>(enum_index)));
  }
  EXPECT_EQ(HmiResults::INVALID_ENUM, MessageHelper::MobileToHMIResult(
      MobileResults::INVALID_ENUM));
}

TEST_F( MessageHelperTest, VerifySoftButtonString_WrongStrings_ExpectFalse) {
  const policy::StringArray wrong_strings = {
    "soft_button1\t\ntext",
    "soft_button1\\ntext",
    "soft_button1\\ttext",
    " ",
    "soft_button1\t\n",
    "soft_button1\\n",
    "soft_button1\\t"
  };
  for(u_int32_t i = 0; i < wrong_strings.size(); ++i) {
    EXPECT_FALSE( MessageHelper::VerifySoftButtonString( wrong_strings[i] ) );
  }
}

TEST_F( MessageHelperTest, VerifySoftButtonString_CorrectStrings_ExpectTrue) {
  const policy::StringArray wrong_strings = {
    "soft_button1.text",
    "soft_button1?text",
    " asd asdasd    ",
    "soft_button1??....asd",
    "soft_button12313fcvzxc./.,"
  };
  for(u_int32_t i = 0; i < wrong_strings.size(); ++i) {
    EXPECT_TRUE( MessageHelper::VerifySoftButtonString( wrong_strings[i] ) );
  }
}

} // namespace test
} // namespace application_manager
