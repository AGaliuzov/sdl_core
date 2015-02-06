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
#include "utils/timer_thread.h"

namespace test {
namespace components {
namespace utils {

using namespace timer;
using namespace sync_primitives;

class TimerThreadTest : public ::testing::Test {
 public:
  TimerThreadTest()
      : check_val(0) {
  }

  void function() {
    AutoLock alock(lock_);
    ++check_val;
    condvar_.NotifyOne();
  }

 protected:
  uint32_t check_val;
  Lock lock_;
  ConditionalVariable condvar_;
};

TEST_F(TimerThreadTest, StartTimerThreadWithTimeoutOneSec_ExpectSuccessfullInvokeCallbackFuncOnTimeout) {
  // Create Timer with TimerDeleagate
  TimerThread<TimerThreadTest> timer("Test", this, &TimerThreadTest::function, false);
  AutoLock alock(lock_);
  EXPECT_EQ(0, this->check_val);
  // Start timer with 1 second timeout
  timer.start(1);
  condvar_.WaitFor(alock, 3000);
  EXPECT_EQ(1, this->check_val);
}

TEST_F(TimerThreadTest, StartTimerThreadWithTimeoutOneSecInLoop_ExpectSuccessfullInvokeCallbackFuncOnEveryTimeout) {
  // Create Timer with TimerLooperDeleagate
  TimerThread<TimerThreadTest> timer("Test", this, &TimerThreadTest::function, true);
  AutoLock alock(lock_);
  EXPECT_EQ(0, this->check_val);
  // Start timer with 1 second timeout
  timer.start(1);
  while (this->check_val < 4) {
    condvar_.WaitFor(alock, 3000);
  }
  // Check callback function was called 4 times
  EXPECT_EQ(4, this->check_val);
}

TEST_F(TimerThreadTest, StopStartedTimerThreadWithTimeoutOneSecInLoop_ExpectSuccessfullStop) {
  // Create Timer with TimerLooperDeleagate
  TimerThread<TimerThreadTest> timer("Test", this, &TimerThreadTest::function, true);
  AutoLock alock(lock_);
  EXPECT_EQ(0, this->check_val);
  // Start timer with 1 second timeout
  timer.start(1);
  // Stop timer on 3rd second
  while (this->check_val < 4) {
    if (this->check_val == 3) {
      timer.stop();
      break;
    }
    condvar_.WaitFor(alock, 3000);
  }
  EXPECT_EQ(3, this->check_val);
}

TEST_F(TimerThreadTest, ChangeTimeoutForStartedTimerThreadWithTimeoutOneSecInLoop_ExpectSuccessfullStop) {
  // Create Timer with TimerLooperDeleagate
  TimerThread<TimerThreadTest> timer("Test", this, &TimerThreadTest::function, true);
  AutoLock alock(lock_);
  EXPECT_EQ(0, this->check_val);
  // Start timer with 1 second timeout
  timer.start(1);
  // Change timer timeout on 3rd second
  while (this->check_val < 4) {
    if (this->check_val == 3) {
      timer.updateTimeOut(2);
    }
    condvar_.WaitFor(alock, 3000);
  }
  EXPECT_EQ(4, this->check_val);
}

TEST_F(TimerThreadTest, PauseStartedTimerThreadWithTimeoutOneSecInLoop_ExpectSuccessfullStop) {
  // Create Timer with TimerLooperDeleagate
  TimerThread<TimerThreadTest> timer("Test", this, &TimerThreadTest::function, true);
  AutoLock alock(lock_);
  EXPECT_EQ(0, this->check_val);
  // Start timer with 1 second timeout
  timer.start(1);
  // Change timer timeout on 3rd second
  while (this->check_val < 4) {
    if (this->check_val == 3) {
      timer.updateTimeOut(2);
    }
    condvar_.WaitFor(alock, 3000);
  }
  EXPECT_EQ(4, this->check_val);
}

}  // namespace utils
}  // namespace components
}  // namespace test

