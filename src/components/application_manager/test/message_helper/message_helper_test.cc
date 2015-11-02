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

namespace application_manager {
namespace test {

namespace HmiLanguage = hmi_apis::Common_Language;
namespace HmiResults = hmi_apis::Common_Result;
namespace MobileResults = mobile_apis::Result;

typedef ::test::components::resumption_test::ApplicationMock AppMock;
typedef utils::SharedPtr<AppMock> ApplicationMockSharedPtr;
using testing::Return;

class MessageHelperTest : public ::testing::Test {
  public:
    MessageHelperTest(){}
  protected:

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
    const policy::StringArray function_id_strings = {
          "RESERVED", "RegisterAppInterface", "UnregisterAppInterface",
          "SetGlobalProperties", "ResetGlobalProperties", "AddCommand",
          "DeleteCommand", "AddSubMenu", "DeleteSubMenu",
          "CreateInteractionChoiceSet", "PerformInteraction", "DeleteInteractionChoiceSet",
          "Alert", "Show", "Speak",
          "SetMediaClockTimer", "PerformAudioPassThru", "EndAudioPassThru",
          "SubscribeButton", "UnsubscribeButton", "SubscribeVehicleData",
          "UnsubscribeVehicleData", "GetVehicleData", "ReadDID",
          "GetDTCs", "ScrollableMessage", "Slider",
          "ShowConstantTBT", "AlertManeuver", "UpdateTurnList",
          "ChangeRegistration", "GenericResponse", "PutFile",
          "DeleteFile", "ListFiles", "SetAppIcon",
          "SetDisplayLayout", "DiagnosticMessage", "SystemRequest",
          "SendLocation", "DialNumber"
        };
    const u_int32_t delta_from_functions_id = 32768;
    const policy::StringArray events_id_strings = {
      "OnHMIStatus", "OnAppInterfaceUnregistered", "OnButtonEvent",
      "OnButtonPress", "OnVehicleData", "OnCommand",
      "OnTBTClientState", "OnDriverDistraction", "OnPermissionsChange",
      "OnAudioPassThru", "OnLanguageChange", "OnKeyboardInput",
      "OnTouchEvent", "OnSystemRequest", "OnHashChange"
    };
    const policy::StringArray hmi_level_strings = {
      "FULL", "LIMITED",
      "BACKGROUND", "NONE"
    };

    virtual void SetUp() OVERRIDE {
    }
    virtual void TearDown() OVERRIDE {}
};
TEST_F(MessageHelperTest,
    CommonLanguageFromString_SendStringValueOfEnum_ExpectCorrectEType) {
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

TEST_F(MessageHelperTest,
    CommonLanguageToString_SendETypeValueOfEnum_ExpectCorrectString) {
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

TEST_F(MessageHelperTest,
    ConvertEnumAPINoCheck_AnyEnumType_ExpectAnotherEnumType) {
  hmi_apis::Common_AppHMIType::eType converted =
      MessageHelper::ConvertEnumAPINoCheck <hmi_apis::Common_LayoutMode::eType,
          hmi_apis::Common_AppHMIType::eType>(
              hmi_apis::Common_LayoutMode::ICON_ONLY);
  EXPECT_EQ(hmi_apis::Common_AppHMIType::DEFAULT, converted);
}

TEST_F( MessageHelperTest,
    HMIResultFromString_SendStringValueOfEnum_ExpectCorrectEType) {
  for(u_int32_t array_index = 0;
      array_index < hmi_result_strings.size();
      ++array_index) {
    EXPECT_EQ(static_cast<HmiResults::eType>(array_index),
        MessageHelper::HMIResultFromString( hmi_result_strings[array_index] ));
  }
  EXPECT_EQ(HmiResults::INVALID_ENUM,
      MessageHelper::HMIResultFromString(""));
}

TEST_F( MessageHelperTest,
    HMIResultToString_SendETypeValueOfEnum_ExpectCorrectString) {
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

TEST_F( MessageHelperTest,
    HMIToMobileResult_SendHmiResultEType_ExpectGetCorrectMobileResultEType) {
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

TEST_F( MessageHelperTest,
    MobileResultFromString_SendStringValueOfEnum_ExpectCorrectEType) {
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

TEST_F( MessageHelperTest,
    MobileResultToString_SendETypeValueOfEnum_ExpectCorrectString) {
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

TEST_F( MessageHelperTest,
    MobileToHMIResult_SendMobileResultEType_ExpectGetCorrectHmiResultEType) {
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
    " asd asdasd    .././/",
    "soft_button1??....asd",
    "soft_button12313fcvzxc./.,"
  };
  for(u_int32_t i = 0; i < wrong_strings.size(); ++i) {
    EXPECT_TRUE( MessageHelper::VerifySoftButtonString( wrong_strings[i] ) );
  }
}

TEST_F( MessageHelperTest,
    GetIVISubscriptionRequests_SendValidApplication_ExpectHmiRequestNotEmpty) {
  // Creating sharedPtr to ApplicationMock
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  // Creating data acessor
  application_manager::VehicleInfoSubscriptions vis;
  DataAccessor<application_manager::VehicleInfoSubscriptions>
      data_accessor( vis, true );
  // Calls for ApplicationManager
  EXPECT_CALL( *appSharedMock, app_id() )
      .WillOnce(Return(1u));
  EXPECT_CALL( *appSharedMock, SubscribedIVI())
      .WillOnce(Return(data_accessor));
  smart_objects::SmartObjectList outList =
      MessageHelper::GetIVISubscriptionRequests(appSharedMock);
  // Expect not empty request
  EXPECT_FALSE(outList.empty());
}

TEST_F( MessageHelperTest,
    ProcessSoftButtons_SendSmartObjectWithoutButtonsKey_ExpectSuccess) {
  // Creating sharedPtr to ApplicationMock
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  // Creating input data for method
  smart_objects::SmartObject object;
  // Method call
  mobile_apis::Result::eType result =
      MessageHelper::ProcessSoftButtons( object, appSharedMock );
  // Expect
  EXPECT_EQ(mobile_apis::Result::SUCCESS, result);
}

TEST_F( MessageHelperTest,
    ProcessSoftButtons_SendIncorectSoftButonValue_ExpectInvalidData) {
  // Creating sharedPtr to ApplicationMock
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  // Creating input data for method
  smart_objects::SmartObject object;
  smart_objects::SmartObject& buttons = object[strings::soft_buttons];
  // Setting invalid image string to button
  buttons[0][strings::image][strings::value] = "invalid\\nvalue";
  // Method call
  mobile_apis::Result::eType result =
      MessageHelper::ProcessSoftButtons( object, appSharedMock );
  // Expect
  EXPECT_EQ(mobile_apis::Result::INVALID_DATA, result);
}

TEST_F( MessageHelperTest,
    VerifyImage_ImageTypeIsStatic_ExpectSuccess) {
  // Creating sharedPtr to ApplicationMock
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  // Creating input data for method
  smart_objects::SmartObject image;
  image[strings::image_type] = mobile_apis::ImageType::STATIC;
  // Method call
  mobile_apis::Result::eType result =
      MessageHelper::VerifyImage( image, appSharedMock );
  //EXPECT
  EXPECT_EQ(mobile_apis::Result::SUCCESS, result);
}

TEST_F( MessageHelperTest,
    VerifyImage_ImageValueNotValid_ExpectInvalidData) {
  // Creating sharedPtr to ApplicationMock
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  // Creating input data for method
  smart_objects::SmartObject image;
  image[strings::image_type] = mobile_apis::ImageType::DYNAMIC;
  // Invalid value
  image[strings::value] = "   ";
  // Method call
  mobile_apis::Result::eType result =
      MessageHelper::VerifyImage( image, appSharedMock );
  //EXPECT
  EXPECT_EQ(mobile_apis::Result::INVALID_DATA, result);
}


TEST_F ( MessageHelperTest,
    VerifyImageFiles_SmartObjectWithValidData_ExpectSuccess) {
  // Creating sharedPtr to ApplicationMock
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  // Creating input data for method
  smart_objects::SmartObject images;
  images[0][strings::image_type] = mobile_apis::ImageType::STATIC;
  images[1][strings::image_type] = mobile_apis::ImageType::STATIC;
  // Method call
  mobile_apis::Result::eType result =
      MessageHelper::VerifyImageFiles( images, appSharedMock );
  //EXPECT
  EXPECT_EQ(mobile_apis::Result::SUCCESS, result);
}

TEST_F ( MessageHelperTest,
    VerifyImageFiles_SmartObjectWithInvalidData_ExpectNotSuccsess) {
  // Creating sharedPtr to ApplicationMock
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  // Creating input data for method
  smart_objects::SmartObject images;
  images[0][strings::image_type] = mobile_apis::ImageType::DYNAMIC;
  images[1][strings::image_type] = mobile_apis::ImageType::DYNAMIC;
  // Invalid values
  images[0][strings::value] = "   ";
  images[1][strings::value] = "image\\n";
  // Method call
  mobile_apis::Result::eType result =
      MessageHelper::VerifyImageFiles( images, appSharedMock );
  //EXPECT
  EXPECT_EQ(mobile_apis::Result::INVALID_DATA, result);
}

TEST_F ( MessageHelperTest,
    VerifyImageVrHelpItems_SmartObjectWithSeveralValidImages_ExpectSuccsess) {
  // Creating sharedPtr to ApplicationMock
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  // Creating input data for method
  smart_objects::SmartObject message;
  message[0][strings::image][strings::image_type] =
      mobile_apis::ImageType::STATIC;
  message[1][strings::image][strings::image_type] =
      mobile_apis::ImageType::STATIC;
  // Method call
  mobile_apis::Result::eType result =
      MessageHelper::VerifyImageVrHelpItems( message, appSharedMock );
  //EXPECT
  EXPECT_EQ(mobile_apis::Result::SUCCESS, result);
}

TEST_F ( MessageHelperTest,
    VerifyImageVrHelpItems_SmartObjectWithSeveralInvalidImages_ExpectNotSuccsess) {
  // Creating sharedPtr to ApplicationMock
  ApplicationMockSharedPtr appSharedMock = utils::MakeShared<AppMock>();
  // Creating input data for method
  smart_objects::SmartObject message;
  message[0][strings::image][strings::image_type] =
      mobile_apis::ImageType::DYNAMIC;
  message[1][strings::image][strings::image_type] =
      mobile_apis::ImageType::DYNAMIC;
  // Invalid values
  message[0][strings::image][strings::value] = "   ";
  message[1][strings::image][strings::value] = "image\\n";
  // Method call
  mobile_apis::Result::eType result =
      MessageHelper::VerifyImageVrHelpItems(message, appSharedMock );
  //EXPECT
  EXPECT_EQ(mobile_apis::Result::INVALID_DATA, result);
}

TEST_F( MessageHelperTest,
    StringifiedFunctionID_FinctionId_ExpectEqualsWithStringsInArray) {
  // Start from 1 because 1 == RESERVED and haven`t ID in last 2 characters
  // if FUNCTION ID == 1 inner DCHECK is false
  for (u_int32_t i = 1; i < function_id_strings.size(); ++i) {
    EXPECT_EQ( function_id_strings[i],
        MessageHelper::StringifiedFunctionID(
            static_cast<mobile_apis::FunctionID::eType>(i) ) );
  }
  // EventIDs emum strarts from delta_from_functions_id = 32768
  for (u_int32_t i = delta_from_functions_id;
      i < events_id_strings.size()+delta_from_functions_id;
      ++i) {
    EXPECT_EQ( events_id_strings[i-delta_from_functions_id],
        MessageHelper::StringifiedFunctionID(
            static_cast<mobile_apis::FunctionID::eType>(i) )
    );
  }
}

TEST_F( MessageHelperTest,
    StringifiedHmiLevel_LevelEnum_ExpectEqualsWithStringsInArray) {
  for (u_int32_t i = 0; i < hmi_level_strings.size(); ++i) {
    EXPECT_EQ (hmi_level_strings[i],
        MessageHelper::StringifiedHMILevel(
            static_cast<mobile_apis::HMILevel::eType>(i)));
  }
}

TEST_F( MessageHelperTest,
    StringToHmiLevel_LevelString_ExpectEqEType) {
  for (u_int32_t i = 0; i < hmi_level_strings.size(); ++i) {
    EXPECT_EQ ( static_cast<mobile_apis::HMILevel::eType>(i),
        MessageHelper::StringToHMILevel( hmi_level_strings[i] ) );
  }
}

TEST_F( MessageHelperTest,
    SubscribeApplicationToSoftButton_ExpectCallFromApp) {
  // Create application mock
  ApplicationMockSharedPtr appSharedPtr = utils::MakeShared<AppMock>();
  // Prepare data for method
  smart_objects::SmartObject message_params;
  u_int32_t function_id = 1;
  //
  EXPECT_CALL(*appSharedPtr , SubscribeToSoftButtons(function_id,SoftButtonID()))
      .Times(1);
  MessageHelper::SubscribeApplicationToSoftButton(
      message_params, appSharedPtr, function_id );
}

} // namespace test
} // namespace application_manager
