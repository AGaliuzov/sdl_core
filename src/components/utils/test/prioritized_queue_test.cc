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
#include "utils/prioritized_queue.h"

namespace test {
namespace components {
namespace utils {

using ::utils::PrioritizedQueue;

class TestMessage {
 public:
  TestMessage()
      : priority(0) {
  }
  TestMessage(std::string message, size_t msg_priority)
      : msg_(message),
        priority(msg_priority) {
  }
  size_t PriorityOrder() const;
  std::string msg() const;
  friend bool operator==(const TestMessage &msg1, const TestMessage &msg2);
 private:
  std::string msg_;
  size_t priority;
};

size_t TestMessage::PriorityOrder() const {
  return priority;
}

std::string TestMessage::msg() const {
  return msg_;
}

bool operator==(const TestMessage &msg1, const TestMessage &msg2) {
  return (msg1.msg() == msg2.msg()
      && msg1.PriorityOrder() == msg2.PriorityOrder());
}

class PrioritizedQueueTest : public testing::Test {
 protected:
  PrioritizedQueue<TestMessage> test_queue;
};

TEST_F(PrioritizedQueueTest, DefaultCtorTest_ExpectEmptyQueueCreated) {
  EXPECT_TRUE(test_queue.empty());
}

TEST_F(PrioritizedQueueTest, PushFourElementsTest_ExpectFourElementsAdded) {
  // Creating 4 messages
  TestMessage message1("Ford", 2);
  TestMessage message2("Hello", 1);
  TestMessage message3("Luxoft", 4);
  TestMessage message4("from", 3);
  // Adding created messages to Prioritized queue
  test_queue.push(message4);
  test_queue.push(message3);
  test_queue.push(message1);
  test_queue.push(message2);
  // Expect 4 messages were added successfully
  EXPECT_EQ(4, test_queue.size());
}

TEST_F(PrioritizedQueueTest, CheckMessageOrderTest_ExpectMessageWithHighestPriorityAddedFirst) {
  // Creating 4 messages with different priorities
  TestMessage message1("Ford", 111);
  TestMessage message2("Hello", 21);
  TestMessage message3("Luxoft", 14);
  TestMessage message4("from", 4);
  // Adding created messages to Prioritized queue. Expect queue ordered according priority
  test_queue.push(message4);
  test_queue.push(message3);
  test_queue.push(message1);
  test_queue.push(message2);
  // Check the first message is the message with highest priority
  EXPECT_EQ(message1, test_queue.front());
}

TEST_F(PrioritizedQueueTest, NotEmptyPrioritizedQueuePopElement_ExpectQueueDecreasedOneElement) {
  // Creating 4 prioritized messages
  TestMessage message1("Alice", 111);
  TestMessage message2("walked", 21);
  TestMessage message3("in", 14);
  TestMessage message4("Wonderland", 4);
  // Adding created messages to Prioritized queue
  test_queue.push(message4);
  test_queue.push(message3);
  test_queue.push(message1);
  test_queue.push(message2);
  // Extracting first element from the queue
  test_queue.pop();
  // Check that one message was extracted
  EXPECT_EQ(3, test_queue.size());
  // Checking if extracted message was the message with highest priority
  // therefore now first message in queue has highest priority
  EXPECT_EQ(message2, test_queue.front());
  // Extracting first element from the queue
  test_queue.pop();
  // Checking if extracted message was the message with highest priority
  // therefore now first message in queue has highest priority
  EXPECT_EQ(message3, test_queue.front());
}

}  // namespace utils
}  // namespace components
}  // namespace test
