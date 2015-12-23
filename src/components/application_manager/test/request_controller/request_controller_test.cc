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

#include "gtest/gtest.h"
#include "application_manager/request_controller.h"
#include "config_profile/profile.h"
#include "mobile/register_app_interface_request.h"
#include "mobile/unregister_app_interface_request.h"
#include "smart_objects/smart_object.h"
#include "application_manager/commands/command_request_impl.h"
#include "application_manager/message_helper.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/application_impl.h"
#include "utils/make_shared.h"

namespace test {
namespace components {
namespace request_controller_test {

using namespace application_manager;
using application_manager::request_controller::RequestController;
using NsSmartDeviceLink::NsSmartObjects::SmartObjectSPtr;
using NsSmartDeviceLink::NsSmartObjects::SmartObject;
using NsSmartDeviceLink::NsSmartObjects::SmartType_Map;
using application_manager::MessageHelper;
using application_manager::commands::RegisterAppInterfaceRequest;
using application_manager::commands::Command;

typedef utils::SharedPtr<Command> RequestPtr;

RequestPtr RegisterApplication(unsigned int id = 1) {
  SmartObjectSPtr resultsmart = new SmartObject(SmartType_Map);
  SmartObject& test_message = *resultsmart;
  uint32_t connection_key = 0;

  test_message[strings::params][strings::message_type] =
      static_cast<int>(kRequest);
  test_message[strings::params][strings::function_id] =
      static_cast<int>(id);
  test_message[strings::params][strings::correlation_id] =
      ApplicationManagerImpl::instance()->GetNextHMICorrelationID();

  test_message[strings::params][strings::connection_key] = connection_key;
  test_message[strings::msg_params][strings::language_desired] = 0;
  test_message[strings::msg_params][strings::hmi_display_language_desired] = 0;
  RequestPtr testregCommand =
      utils::MakeShared<RegisterAppInterfaceRequest>(resultsmart);
  return testregCommand;
}

RequestPtr UnregisterApplication() {
  SmartObjectSPtr resultsmart = MessageHelper::CreateModuleInfoSO(2);
  RequestPtr testregCommand =
      utils::MakeShared<commands::UnregisterAppInterfaceRequest>(resultsmart);
  return testregCommand;
}

class RequestControllerTestClass : public ::testing::Test {
public:
  RequestControllerTestClass():reg(RegisterApplication()) {}

  RequestController::TResult AddHMIRequest(const bool RegisterRequest = false) {
    if (RegisterRequest)
        result = request_ctrl.addHMIRequest(reg);
    else
        result = request_ctrl.addHMIRequest(regEmpty);
    return result;
  }

  RequestController::TResult AddMobileRequest(
      const mobile_apis::HMILevel::eType& hmi_level,
          const bool RegisterRequest = false) {
    if (RegisterRequest)
        result = request_ctrl.addMobileRequest(reg, hmi_level);
    else
        result = request_ctrl.addMobileRequest(regEmpty, hmi_level);
    return result;
  }

  static void SetUpTestCase()
  {
    ::profile::Profile::instance()->
        config_file_name("smartDeviceLink_test3.ini");
  }

  void UnregisterApp(const mobile_apis::HMILevel::eType& hmi_level)
  {
    RequestPtr unreg = UnregisterApplication();
    request_ctrl.addMobileRequest(unreg, hmi_level);
  }

  RequestController request_ctrl;
  RequestPtr reg, regEmpty;
  RequestController::TResult result;
};

// TODO {OHerasym}: APPLINK-20220
TEST_F(RequestControllerTestClass, DISABLED_CheckPosibilitytoAdd_HMI_FULL_SUCCESS) {
  EXPECT_EQ(RequestController::TResult::SUCCESS,
      AddMobileRequest(mobile_apis::HMILevel::HMI_FULL, true));
  ApplicationManagerImpl::instance()->destroy();
  UnregisterApp(mobile_apis::HMILevel::HMI_FULL);
}

TEST_F(RequestControllerTestClass, DISABLED_CheckPosibilitytoAdd_HMI_NONE_SUCCESS) {
  ApplicationManagerImpl::instance();
  EXPECT_EQ(RequestController::TResult::SUCCESS,
      AddMobileRequest(mobile_apis::HMILevel::HMI_NONE, true));
  ApplicationManagerImpl::instance()->destroy();
  UnregisterApp(mobile_apis::HMILevel::HMI_NONE);
}

TEST_F(RequestControllerTestClass, IsLowVoltage_SetOnLowVoltage_TRUE) {
  request_ctrl.OnLowVoltage();
  EXPECT_EQ(true, request_ctrl.IsLowVoltage());
}

TEST_F(RequestControllerTestClass, IsLowVoltage_SetOnWakeUp_FALSE) {
  request_ctrl.OnWakeUp();
  const bool result = false;
  EXPECT_EQ(result, request_ctrl.IsLowVoltage());
}

TEST_F(RequestControllerTestClass,
  AddMobileRequest_SetInvalidData_INVALID_DATA) {
  EXPECT_EQ(RequestController::INVALID_DATA,
      AddMobileRequest(mobile_apis::HMILevel::HMI_NONE));
}

TEST_F(RequestControllerTestClass, addHMIRequest_AddRequest_SUCCESS) {
  EXPECT_EQ(RequestController::SUCCESS, AddHMIRequest(true));
}

TEST_F(RequestControllerTestClass, addHMIRequest_AddInvalidData_INVALID_DATA) {
  EXPECT_EQ(RequestController::INVALID_DATA, AddHMIRequest());
}

}  // namespace request_controller
}  // namespace components
}  // namespace test
