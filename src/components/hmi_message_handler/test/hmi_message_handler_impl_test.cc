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

#include "gmock/gmock.h"
#include "application_manager/message.h"
#include "config_profile/profile.h"
#include "hmi_message_handler/hmi_message_handler_impl.h"
#include "hmi_message_handler/messagebroker_adapter.h"
#include "hmi_message_handler/mock_hmi_message_observer.h"

namespace test {
namespace components {
namespace hmi_message_handler_test {

class HMIMessageHandlerImplTest : public ::testing::Test {
  protected:
    static hmi_message_handler::MessageBrokerAdapter* mb_adapter_;
    static hmi_message_handler::HMIMessageHandlerImpl* hmi_handler_;
    static hmi_message_handler::MockHMIMessageObserver*
      mock_hmi_message_observer_;

    static void SetUpTestCase() {
      hmi_handler_ = hmi_message_handler::HMIMessageHandlerImpl::instance();
      ASSERT_TRUE(NULL != hmi_handler_);
      mb_adapter_ = new hmi_message_handler::MessageBrokerAdapter(
          hmi_handler_, "localhost", 22);
      ASSERT_TRUE(NULL != mb_adapter_);
      mock_hmi_message_observer_ =
          hmi_message_handler::MockHMIMessageObserver::instance();
      ASSERT_TRUE(NULL != mock_hmi_message_observer_);
      hmi_handler_->set_message_observer(mock_hmi_message_observer_);
      EXPECT_TRUE(NULL != hmi_handler_->observer());
    }

    static void TearDownTestCase() {
      hmi_handler_->set_message_observer(NULL);
      hmi_message_handler::MockHMIMessageObserver::destroy();
      hmi_message_handler::HMIMessageHandlerImpl::destroy();
      delete mb_adapter_;
    }
};

hmi_message_handler::HMIMessageHandlerImpl*
    HMIMessageHandlerImplTest::hmi_handler_ = NULL;
hmi_message_handler::MessageBrokerAdapter*
    HMIMessageHandlerImplTest::mb_adapter_ = NULL;
hmi_message_handler::MockHMIMessageObserver*
    HMIMessageHandlerImplTest::mock_hmi_message_observer_ = NULL;

TEST_F(HMIMessageHandlerImplTest, OnErrorSending_ExpectCallProceeded) {
  // Arrange
  hmi_message_handler::MessageSharedPointer message;
  EXPECT_CALL(*mock_hmi_message_observer_, OnErrorSending(message));
  // Act
  hmi_handler_->OnErrorSending(message);
}

TEST_F(HMIMessageHandlerImplTest, AddHMIMessageAdapter_ExpectAdded) {
  // Check before action
  EXPECT_EQ(0u, hmi_handler_->message_adapters().size());
  // Act
  hmi_handler_->AddHMIMessageAdapter(mb_adapter_);
  // Check after action
  EXPECT_EQ(1u, hmi_handler_->message_adapters().size());
  hmi_handler_->RemoveHMIMessageAdapter(mb_adapter_);
  EXPECT_EQ(0u, hmi_handler_->message_adapters().size());
}

TEST_F(HMIMessageHandlerImplTest, RemoveHMIMessageAdapter_ExpectRemoved) {
  // Arrange
  hmi_handler_->AddHMIMessageAdapter(mb_adapter_);
  // Check before action
  EXPECT_EQ(1u, hmi_handler_->message_adapters().size());
  // Act
  hmi_handler_->RemoveHMIMessageAdapter(mb_adapter_);
  // Check after action
  EXPECT_EQ(0u, hmi_handler_->message_adapters().size());
}

}  // namespace hmi_message_handler_test
}  // namespace components
}  // namespace test
