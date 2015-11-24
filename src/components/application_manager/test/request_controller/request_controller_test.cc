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

namespace test {
namespace components {
namespace request_controller {

using namespace application_manager;
using application_manager::request_controller::RequestController;
using NsSmartDeviceLink::NsSmartObjects::SmartObjectSPtr;
using NsSmartDeviceLink::NsSmartObjects::SmartObject;
using NsSmartDeviceLink::NsSmartObjects::SmartType_Map;
using application_manager::MessageHelper;
using application_manager::commands::RegisterAppInterfaceRequest;
using application_manager::commands::Command;

typedef utils::SharedPtr<Command> RequestPtr;

Command* RegisterApplication(unsigned int id = 1) {
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
  commands::Command* testregCommand =
      new RegisterAppInterfaceRequest(resultsmart);
  return testregCommand;
}

Command* UnregisterApplication() {
  SmartObjectSPtr resultsmart = MessageHelper::CreateModuleInfoSO(2);
  commands::Command *testregCommand =
      new commands::UnregisterAppInterfaceRequest(resultsmart);
  return testregCommand;
}

TEST(RequestControllerTest, CheckPosibilitytoAdd_HMI_FULL_Expect_SUCCESS) {
  ::profile::Profile::instance()->config_file_name("smartDeviceLink_test3.ini");

  RequestController::TResult result;
  RequestController request_ctrl;

  commands::Command * reg = RegisterApplication();

  result = request_ctrl.addMobileRequest(reg, mobile_apis::HMILevel::HMI_FULL);
  ApplicationManagerImpl::instance()->destroy();

  EXPECT_EQ(RequestController::TResult::SUCCESS, result);

  commands::Command* unreg = UnregisterApplication();
  request_ctrl.addMobileRequest(unreg, mobile_apis::HMILevel::HMI_FULL);
}

TEST(RequestControllerTest, CheckPosibilitytoAdd_HMI_NONE_Expect_SUCCESS) {
  ::profile::Profile::instance()->config_file_name("smartDeviceLink_test3.ini");

  RequestController::TResult result;
  RequestController request_ctrl;
  ApplicationManagerImpl::instance();
  commands::Command * reg = RegisterApplication();

  result = request_ctrl.addMobileRequest(reg, mobile_apis::HMILevel::HMI_NONE);
  ApplicationManagerImpl::instance()->destroy();

  EXPECT_EQ(RequestController::TResult::SUCCESS, result);

  commands::Command* unreg = UnregisterApplication();
  request_ctrl.addMobileRequest(unreg, mobile_apis::HMILevel::HMI_NONE);
}

TEST(RequestControllerTest, IsLowVoltage_Expect_TRUE) {
  RequestController request_ctrl;
  request_ctrl.OnLowVoltage();
  EXPECT_EQ(true, request_ctrl.IsLowVoltage());
}

TEST(RequestControllerTest, IsLowVoltage_Expect_FALSE) {
  RequestController request_ctrl;
  request_ctrl.OnWakeUp();
  bool result = false;
  EXPECT_EQ(result, request_ctrl.IsLowVoltage());
}

TEST(RequestControllerTest, AddMobileRequest_Expect_INVALID_DATA) {
  RequestController::TResult result;
  RequestController request_ctrl;
  RequestPtr reg;

  result = request_ctrl.addMobileRequest(reg, mobile_apis::HMILevel::HMI_NONE);

  EXPECT_EQ(RequestController::INVALID_DATA, result);
}

TEST(RequestControllerTest, addHMIRequest_Expect_SUCCESS) {
  RequestController::TResult result;
  RequestController request_ctrl;
  commands::Command * reg = RegisterApplication();

  result = request_ctrl.addHMIRequest(reg);

  EXPECT_EQ(RequestController::SUCCESS, result);
}

TEST(RequestControllerTest, addHMIRequest_Expect_INVALID_DATA) {
  RequestController::TResult result;
  RequestController request_ctrl;
  RequestPtr reg;

  result = request_ctrl.addHMIRequest(reg);

  EXPECT_EQ(RequestController::INVALID_DATA, result);
}

}  // namespace request_controller
}  // namespace components
}  // namespace test
