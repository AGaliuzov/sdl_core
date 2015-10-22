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


namespace application_manager {
namespace test {

namespace HmiLanguage = hmi_apis::Common_Language;
namespace HmiResults = hmi_apis::Common_Result;
namespace MobileResults = mobile_apis::Result;
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
