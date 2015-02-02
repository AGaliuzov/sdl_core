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
#include "utils/lock.h"
#include "threads/thread.h"

namespace test {
namespace components {
namespace utils {

using sync_primitives::Lock;
using namespace ::threads;

const uint32_t MAX_SIZE = 20;

// ThreadDelegate successor
class TestThread : public threads::ThreadDelegate {
 public:
  void threadMain() {
    sleep(2);
  }
};

TEST(PosixThreadTest, CreateThread_ExpectThreadCreated) {
  // Create thread
  threads::Thread *thread = NULL;
  threads::ThreadDelegate *threadDelegate = new TestThread();
  thread = CreateThread("test thread", threadDelegate);
  EXPECT_EQ(thread, threadDelegate->thread());
  EXPECT_TRUE(thread);
  DeleteThread(thread);
  delete threadDelegate;
}

TEST(PosixThreadTest, CheckCreatedThreadName_ExpectCorrectName) {
  // Create thread
  threads::Thread *thread = NULL;
  threads::ThreadDelegate *threadDelegate = new TestThread();
  thread = CreateThread("test thread", threadDelegate);
  // Check thread was created with correct name
  EXPECT_EQ(std::string("test thread"), thread->name());
  DeleteThread(thread);
  delete threadDelegate;
}

TEST(PosixThreadTest, CheckCreatedThreadNameChange_ExpectThreadNameChanged) {
  // Create thread
  threads::Thread *thread = NULL;
  threads::ThreadDelegate *threadDelegate = new TestThread();
  thread = CreateThread("test thread", threadDelegate);
  thread->start(threads::ThreadOptions(threads::Thread::kMinStackSize));
  // Check name for created thread
  thread->SetNameForId(thread->thread_handle(),
                       std::string("new thread with changed name"));
  char name[MAX_SIZE];
  int result = pthread_getname_np(thread->thread_handle(), name, sizeof(name));
  if (!result)
    EXPECT_EQ(std::string("new thread with"), std::string(name));
  DeleteThread(thread);
  delete threadDelegate;
}

TEST(PosixThreadTest, StartThread_ExpectThreadStarted) {
  // Create thread
  threads::Thread *thread = NULL;
  TestThread *threadDelegate = new TestThread();
  thread = CreateThread("test thread", threadDelegate);
  // Start created thread
  EXPECT_TRUE(
      thread->start(threads::ThreadOptions(threads::Thread::kMinStackSize)));
  DeleteThread(thread);
  delete threadDelegate;
}

}  // namespace utils
}  // namespace components
}  // namespace test
