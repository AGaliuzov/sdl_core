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

#include <pthread.h>
#include <iostream>
#include "lock.h"
#include "macro.h"

#include "gtest/gtest.h"
#include "utils/conditional_variable.h"

namespace{
std::string test_value_;
sync_primitives::ConditionalVariable cond_var_;
sync_primitives::Lock test_mutex_;
unsigned counter_ = 0;
}

namespace test {
namespace components {
namespace utils {

//
//class ConditionalVariableTest : public ::testing::Test {
// public:
//  ConditionalVariableTest()
//      : test_value_("initialized"), counter_(0) {
//  }
//  void* check_counter(void *arg);
//  void* task_one(void *arg);
//
////  sync_primitives::ConditionalVariable cond_var() const {
////    return cond_var_;
////  }
////  sync_primitives::Lock test_mutex() const {
////    return test_mutex_;
////  }
//
//  std::string test_value() const {
//    return test_value_;
//  }
//  void counter_increment()
//  {
//    counter_++;
//  }
//  unsigned counter() const
//  {
//    return counter_;
//  }
// protected:
//  std::string test_value_;
//  sync_primitives::ConditionalVariable cond_var_;
//  sync_primitives::Lock test_mutex_;
//  unsigned counter_;
//};

// Defines threads behaviour which depends on counter value
void* check_counter(void *arg) {
  unsigned cnt = *(unsigned *) arg;
  sync_primitives::AutoLock test_lock(test_mutex_);
  if (cnt <= 1) {
    counter_++;
    cond_var_.Wait(test_mutex_);  // Mutex unlock & Thread sleeps until Notification
    return NULL;
  }
  DCHECK(cnt == 2);
  cond_var_.Broadcast();  // Notify All threads waiting on conditional variable
  return NULL;
}

// Tasks for threads to begin with
void* task_one(void *arg) {
  sync_primitives::AutoLock test_lock(test_mutex_);
  test_value_ = "changed by thread 1";
  cond_var_.NotifyOne();  // Notify At least one thread waiting on conditional variable
  test_value_ = "changed again by thread 1";
  return NULL;
}

TEST(ConditionalVariableTest, CheckNotifyOne_OneThreadNotified_ExpectSuccessful)
{
  pthread_t thread1;
  sync_primitives::AutoLock test_lock(test_mutex_);
  test_value_ = "changed by main thread";
  const bool thread_created = pthread_create(&thread1, NULL, &task_one, NULL);
  if (thread_created) {
    std::cout << "thread1 is not created! " << std::endl;
    exit(1);
  }
  test_value_ = "changed twice by main thread";
  cond_var_.Wait(test_mutex_);
  std::string last_value("changed again by thread 1");
  EXPECT_EQ(last_value, test_value_);
}

TEST(ConditionalVariableTest, CheckBroadcast_AllThreadsNotified_ExpectSuccessful)
{
  pthread_t thread1;
  pthread_t thread2;
  bool thread_created = pthread_create(&thread1, NULL, check_counter, &counter_);
  if (thread_created) {
    std::cout << "thread1 is not created! " << std::endl;
    exit(1);
  }
  thread_created = pthread_create(&thread2, NULL, check_counter, &counter_);
  if (thread_created) {
    std::cout << "thread2 is not created! " << std::endl;
    exit(1);
  }
  check_counter(&counter_);
  EXPECT_EQ(2, counter_);
}

TEST(ConditionalVariableTest, CheckWaitForWithTimeout2secs_ThreadBlockedForTimeout_ExpectSuccessfulWakeUp)
{
  sync_primitives::AutoLock test_lock(test_mutex_);
  sync_primitives::ConditionalVariable::WaitStatus test_value = sync_primitives::ConditionalVariable::kTimeout;
  sync_primitives::ConditionalVariable::WaitStatus wait_st = cond_var_.WaitFor(test_lock, 2000);
  EXPECT_EQ(test_value, wait_st);
}

} // namespace utils
} // namespace components
} // namespace test

