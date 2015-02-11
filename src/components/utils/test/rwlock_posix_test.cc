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
#include "utils/rwlock.h"

namespace test {
namespace components {
namespace utils {

using sync_primitives::RWLock;

class RWlockTest : public ::testing::Test {
 public:
  void ThreadsDispatcher(pthread_t *thread, void* (*func)(void*)) {
    for (uint8_t i = 0; i < kNum_threads_; ++i) {
      bool thread_created = pthread_create(&thread[i], NULL, func, this);
      ASSERT_FALSE(thread_created)<< "thread is not created!";
    }
    for (uint8_t i = 0; i < kNum_threads_; ++i) {
      pthread_join(thread[i], NULL);
    }
  }

  void ReadLock() {
    EXPECT_TRUE(test_rwlock.AcquireForReading());
    EXPECT_TRUE(test_rwlock.Release());
  }

  void TryReadLock() {
    bool temp = test_rwlock.TryAcquireForReading();
    EXPECT_FALSE(temp);
    if (temp) {
      EXPECT_TRUE(test_rwlock.Release());
    }
  }

  void TryWriteLock() {
    bool temp = test_rwlock.TryAcquireForWriting();
    EXPECT_FALSE(temp);
    if (temp) {
      EXPECT_TRUE(test_rwlock.Release());
    }
  }

  static void* ReadLock_helper(void *context) {
    RWlockTest *temp = reinterpret_cast<RWlockTest *>(context);
    temp->ReadLock();
    return NULL;
  }

  static void* TryReadLock_helper(void *context) {
    RWlockTest *temp = reinterpret_cast<RWlockTest *>(context);
    temp->TryReadLock();
    return NULL;
  }

  static void* TryWriteLock_helper(void *context) {
    RWlockTest *temp = reinterpret_cast<RWlockTest *>(context);
    temp->TryWriteLock();
    return NULL;
  }

 protected:
  RWLock test_rwlock;
  static const uint32_t kNum_threads_;
};

const uint32_t RWlockTest::kNum_threads_ = 5;

TEST_F(RWlockTest, AcquireForReading_ExpectAccessForReading) {
  // Threads IDs
  pthread_t thread[kNum_threads_];
  // Lock rw lock for reading
  EXPECT_TRUE(test_rwlock.AcquireForReading());
  // Try to lock rw lock for reading again
  EXPECT_TRUE(test_rwlock.AcquireForReading());
  // Creating kNumThreads threads, starting them with callback function, waits until all of them finished
  ThreadsDispatcher(thread, &RWlockTest::ReadLock_helper);
  // Releasing RW locks
  EXPECT_TRUE(test_rwlock.Release());
  EXPECT_TRUE(test_rwlock.Release());
}

TEST_F(RWlockTest, AcquireForReading_ExpectNoAccessForWriting) {
  // Threads IDs
  pthread_t thread[kNum_threads_];
  // Lock rw lock for reading
  EXPECT_TRUE(test_rwlock.AcquireForReading());
  // Try to lock rw lock for writing
  EXPECT_FALSE(test_rwlock.TryAcquireForWriting());
  // Creating kNumThreads threads, starting them with callback function, waits until all of them finished
  ThreadsDispatcher(thread, &RWlockTest::TryWriteLock_helper);
  EXPECT_TRUE(test_rwlock.Release());
}

TEST_F(RWlockTest, AcquireForWriting_ExpectNoAccessForReading) {
  // Threads IDs
  pthread_t thread[kNum_threads_];
  // Lock rw lock for writing
  EXPECT_TRUE(test_rwlock.AcquireForWriting());
  // Try to lock rw lock for reading
  EXPECT_FALSE(test_rwlock.TryAcquireForReading());
  // Creating kNumThreads threads, starting them with callback function, waits until all of them finished
  ThreadsDispatcher(thread, &RWlockTest::TryReadLock_helper);
  EXPECT_TRUE(test_rwlock.Release());
}

TEST_F(RWlockTest, AcquireForWriting_ExpectNoMoreAccessForWriting) {
  // Threads IDs
  pthread_t thread[kNum_threads_];
  // Lock rw lock for writing
  EXPECT_TRUE(test_rwlock.AcquireForWriting());
  // Try to lock rw lock for reading
  EXPECT_FALSE(test_rwlock.TryAcquireForWriting());
  // Creating kNumThreads threads, starting them with callback function, waits until all of them finished
  ThreadsDispatcher(thread, &RWlockTest::TryWriteLock_helper);
  EXPECT_TRUE(test_rwlock.Release());
}

}  // namespace utils
}  // namespace components
}  // namespace test
