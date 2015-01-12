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

#include "gtest/gtest.h"
#include "utils/conditional_variable.h"

namespace test {
namespace components {
namespace utils {

// Global variables
namespace {
std::string test_value("initialized");
sync_primitives::ConditionalVariable cond_var;
sync_primitives::Lock test_mutex;
sync_primitives::Lock test_mutex2;
sync_primitives::AutoLock test_lock(test_mutex2);
unsigned counter = 0;
}

// Defines threads behaviour which depends on counter value
void* check_counter(void *arg) {
  unsigned cnt = *(unsigned *) arg;
  test_mutex.Acquire();
  if (cnt <= 1) {
    counter++;
    cond_var.Wait(test_mutex);  // Mutex unlock & Thread sleeps until Notification
    test_mutex.Release();
    return NULL;
  } else if (cnt == 2) {
    test_mutex.Release();
    cond_var.Broadcast();  // Notify All threads waiting on conditional variable
    return NULL;
  }
  return NULL;
}

// Tasks for threads to begin with
void* task_one(void *arg) {
  test_mutex.Acquire();  // Mutex lock
  test_value = "changed by thread 1";
  cond_var.NotifyOne();  // Notify At least one thread waiting on conditional variable
  test_value = "changed again by thread 1";
  test_mutex.Release();  // Mutex release
  return NULL;
}

TEST(ConditionalVariableTest, CheckNotifyOne_OneThreadNotified_ExpectSuccessful)
{
  pthread_t thread1;
  int check_value = 0;
  test_mutex.Acquire();
  test_value = "changed by main thread";
  check_value = pthread_create(&thread1, NULL, task_one, NULL);
  if (check_value) {
    std::cout << "thread1 is not created! " << std::endl;
    exit(1);
  }
  test_value = "changed twice by main thread";
  cond_var.Wait(test_mutex);
  test_mutex.Release();
  std::string last_value("changed again by thread 1");
  EXPECT_EQ(last_value, test_value);
}

TEST(ConditionalVariableTest, CheckBroadcast_AllThreadsNotified_ExpectSuccessful)
{
  pthread_t thread1;
  pthread_t thread2;
  int check_value = 0;
  check_value = pthread_create(&thread1, NULL, check_counter, &counter);
  if (check_value) {
    std::cout << "thread1 is not created! " << std::endl;
    exit(1);
  }
  check_value = pthread_create(&thread2, NULL, check_counter, &counter);
  if (check_value) {
    std::cout << "thread2 is not created! " << std::endl;
    exit(1);
  }
  check_counter(&counter);
  EXPECT_EQ(2, counter);
}

TEST(ConditionalVariableTest, CheckWaitForWithTimeout2secs_ThreadBlockedForTimeout_ExpectSuccessfulWakeUp)
{
  sync_primitives::ConditionalVariable::WaitStatus wait_st = cond_var.WaitFor(test_lock, 2000);
  std::cout<<std::endl<<"\t\t Thread has been woken up after 2 secs"<<std::endl;
  EXPECT_EQ(1, wait_st);
}

} // namespace utils
} // namespace components
} // namespace test

