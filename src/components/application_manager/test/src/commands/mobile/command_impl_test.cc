#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/commands/command_request_impl.h"
//#include "application_manager/message_helper.h"
//using ::testing::AtLeast;


TEST(AMTest, OK) {
  application_manager::ApplicationManagerImpl* am = application_manager::ApplicationManagerImpl::instance();
  application_manager::ApplicationManagerImpl* am2 = application_manager::ApplicationManagerImpl::instance();
  EXPECT_CALL((*am), Init());
  ASSERT_TRUE(am == am2);
  am->Init();
  application_manager::ApplicationManagerImpl::destroy();
}

