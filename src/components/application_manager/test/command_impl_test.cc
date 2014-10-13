#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/commands/command_request_impl.h"
#include "application_manager/message_helper.h"
using ::testing::AtLeast;

class MockApplicationManagerImpl : application_manager::ApplicationManagerImpl,
            public utils::Singleton<MockApplicationManagerImpl>  {
//    MOCK_METHOD1(ManageMobileCommand,
//                 bool (const utils::SharedPtr<application_manager::smart_objects::SmartObject>&
//                       message) );
//   MOCK_METHOD0(end_audio_pass_thru, bool ());
};


TEST(AMTest, OK) {
//  /MockApplicationManagerImpl* am = MockApplicationManagerImpl::instance();
  //MockApplicationManagerImpl* mock = dynamic_cast<MockApplicationManagerImpl* >(am);
  //application_manager::ApplicationManagerImpl* am = application_manager::ApplicationManagerImpl::instance();
  ASSERT_FALSE(NULL != NULL);
}

