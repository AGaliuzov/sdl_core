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

#include <unistd.h>
#include "gtest/gtest.h"
#include "utils/message_queue.h"
#include <iostream>

namespace test {
namespace components {
namespace utils {

using ::utils::MessageQueue;

namespace {
MessageQueue<std::string> test_queue;
std::string test_val_1("Hello,");
std::string test_val_2("Beautiful ");
std::string test_val_3("World!");
}

TEST(MessageQueueTest, DefaultCtorTest_ExpectEmptyQueueCreated) {
  bool test_value = true;
  // Check if the queue is empty
  ASSERT_EQ(test_value, test_queue.empty());
}

TEST(MessageQueueTest, MessageQueuePushThreeElementsTest_ExpectThreeElementsAdded) {
  // Add 3 elements to the queue
  test_queue.push(test_val_1);
  test_queue.push(test_val_2);
  test_queue.push(test_val_3);
  // check if 3 elements were added successfully
  ASSERT_EQ(3, test_queue.size());
}

TEST(MessageQueueTest, MessageQueuePopOneElementTest_ExpectOneElementRemovedFromQueue) {
  // Remove 1 element from beginning of queue
  // Check if first element was removed successfully
  ASSERT_EQ(test_val_1, test_queue.pop());
  // Check the size of queue after 1 element was removed
  ASSERT_EQ(2, test_queue.size());
}

TEST(MessageQueueTest, MessageQueueResetTest_ExpectEmptyQueue) {
  // Resetting queue
  test_queue.Reset();
  // Check if queue is empty
  ASSERT_TRUE(test_queue.empty());
  // Check the size of queue after reset
  ASSERT_EQ(0, test_queue.size());
}

} // namespace utils
} // namespace components
} // namespace test
