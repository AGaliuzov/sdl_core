#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/commands/command_request_impl.h"
#include "application_manager/message_helper.h"

using ::testing::Return;

TEST(ApplicationManagerTest, SingltoneTestCheckWork) {
  application_manager::ApplicationManagerImpl* am = application_manager::ApplicationManagerImpl::instance();
  application_manager::ApplicationManagerImpl* am2 = application_manager::ApplicationManagerImpl::instance();
  ASSERT_TRUE(am == am2);
  EXPECT_CALL((*am), GetNextHMICorrelationID()).WillRepeatedly(Return(1));
  smart_objects::SmartObject* so = application_manager::MessageHelper::CreateModuleInfoSO(0);
  delete so;
  application_manager::ApplicationManagerImpl::destroy();
}

TEST(MobileCommandsTest, CommandImplTimeOut) {
  application_manager::ApplicationManagerImpl* am = application_manager::ApplicationManagerImpl::instance();
  smart_objects::SmartObject* so = application_manager::MessageHelper::CreateModuleInfoSO(0);
  application_manager::commands::CommandRequestImpl request(so);
  EXPECT_CALL((*am), ManageMobileCommand(::testing::_));
  request.onTimeOut();
  application_manager::ApplicationManagerImpl::destroy();
}
