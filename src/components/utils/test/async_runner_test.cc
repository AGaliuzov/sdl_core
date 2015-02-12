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

#include "lock.h"
#include "threads/async_runner.h"
#include "utils/conditional_variable.h"

#include "gtest/gtest.h"

namespace test {
namespace components {
namespace utils {

using namespace sync_primitives;
using namespace threads;

namespace {
uint32_t check_value = 0;
}

class TestThreadDelegate;

class AsyncRunnerTest : public ::testing::Test, public AsyncRunner {
 public:
  AsyncRunnerTest()
      : AsyncRunner("test"),
        delegate_(NULL) {
  }

 protected:
  Lock test_lock_;
  enum { kDelegatesNum_ = 5 };
  ConditionalVariable cond_var_;
  TestThreadDelegate *delegate_;
  TestThreadDelegate *delegates_[kDelegatesNum_];
};

// ThreadDelegate successor
class TestThreadDelegate : public ThreadDelegate {
 public:
  void threadMain() {
    ++check_value;
  }
};

TEST_F(AsyncRunnerTest, CheckASyncRunOneDelegate_ExpectSuccessfulDelegateRun) {
  AutoLock lock(test_lock_);
  // Clear global value before test
  check_value = 0;
  // Create delegate
  delegate_ = new TestThreadDelegate;
  // Run cretaed delegate
  AsyncRun(delegate_);
  // Give 2 secs to delegate to run
  cond_var_.WaitFor(lock, 2000);
  // Expect delegate successfully run
  EXPECT_EQ(1u, check_value);
}

TEST_F(AsyncRunnerTest, ASyncRunManyDelegates_ExpectSuccessfulAllDelegatesRun) {
  AutoLock lock(test_lock_);
  // Clear global value before test
  check_value = 0;
  // Create Delegates and run
  for (int i = 0; i < kDelegatesNum_; ++i) {
    delegates_[i] = new TestThreadDelegate;
    AsyncRun(delegates_[i]);
  }
  // Wait for 2 secs. Give this time to delegates to be run
  cond_var_.WaitFor(lock, 2000);
  // Expect all delegates run successfully
  EXPECT_EQ(5u, check_value);
}

TEST_F(AsyncRunnerTest, RunManyDelegatesAndStop_ExpectSuccessfulDelegatesStop) {
  AutoLock lock(test_lock_);
  // Clear global value before test
  check_value = 0;
  // Create Delegates
  for (int i = 0; i < kDelegatesNum_; ++i) {
    delegates_[i] = new TestThreadDelegate;
  }
  // Wait for 2 secs
  cond_var_.WaitFor(lock, 2000);
  // Run created delegates
  for (int i = 0; i < kDelegatesNum_; ++i) {
    if (i == 3) {
      Stop();
    }
    AsyncRun(delegates_[i]);
  }
  // Expect 3 delegates run successlully. The other stopped.
  EXPECT_EQ(3u, check_value);
}

}  // namespace utils
}  // namespace components
}  // namespace test

