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
  void rdlockThread() {
    EXPECT_FALSE(test_rwlock.AcquireForReading());
    EXPECT_FALSE(test_rwlock.Release());
  }

  void tryrdlockThread() {
    bool temp = test_rwlock.TryAcquireForReading();
    EXPECT_TRUE(temp);
    if (!temp) {
      EXPECT_FALSE(test_rwlock.Release());
    }
  }

  void trywrlockThread() {
    bool temp = test_rwlock.TryAcquireForWriting();
    EXPECT_TRUE(temp);
    if (!temp) {
      EXPECT_FALSE(test_rwlock.Release());
    }
  }

  static void* rdlockThread_helper(void *context) {
    RWlockTest *temp = reinterpret_cast<RWlockTest *>(context);
    temp->rdlockThread();
    return NULL;
  }

  static void* tryrdlockThread_helper(void *context) {
    RWlockTest *temp = reinterpret_cast<RWlockTest *>(context);
    temp->tryrdlockThread();
    return NULL;
  }

  static void* trywrlockThread_helper(void *context) {
    RWlockTest *temp = reinterpret_cast<RWlockTest *>(context);
    temp->trywrlockThread();
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
  // Try to lock rw lock for reading
  EXPECT_FALSE(test_rwlock.AcquireForReading());
  // Try to lock rw lock for reading again
  EXPECT_FALSE(test_rwlock.AcquireForReading());
  // Creating reading threads
  for (uint8_t i = 0; i < kNum_threads_; ++i) {
    bool thread_created = pthread_create(&thread[i], NULL,
                                         &RWlockTest::rdlockThread_helper,
                                         this);
    ASSERT_FALSE(thread_created)<< "thread is not created!";
  }

  for (uint8_t i = 0; i < kNum_threads_; ++i) {
    pthread_join(thread[i], NULL);
  }
  // Releasing RW locks
  EXPECT_FALSE(test_rwlock.Release());
  EXPECT_FALSE(test_rwlock.Release());
}

TEST_F(RWlockTest, AcquireForWriting_ExpectNoAccessForReading) {
  // Threads IDs
  pthread_t thread[kNum_threads_];
  // Try to lock rw lock for writing
  EXPECT_FALSE(test_rwlock.AcquireForWriting());
  // Try to lock rw lock for reading
  EXPECT_TRUE(test_rwlock.TryAcquireForReading());
  // Create reading threads
  for (uint8_t i = 0; i < kNum_threads_; ++i) {
    bool thread_created = pthread_create(&thread[i], NULL,
                                         &RWlockTest::tryrdlockThread_helper,
                                         this);
    ASSERT_FALSE(thread_created)<< "thread is not created!";
  }
  for (uint8_t i = 0; i < kNum_threads_; ++i) {
    pthread_join(thread[i], NULL);
  }
  EXPECT_FALSE(test_rwlock.Release());
}

TEST_F(RWlockTest, AcquireForReading_ExpectNoAccessForWriting) {
  // Threads IDs
  pthread_t thread[kNum_threads_];
  // Try to lock rw lock for writing
  EXPECT_FALSE(test_rwlock.AcquireForReading());
  // Try to lock rw lock for reading
  EXPECT_TRUE(test_rwlock.TryAcquireForWriting());
  // Create reading threads
  for (uint8_t i = 0; i < kNum_threads_; ++i) {
    bool thread_created = pthread_create(&thread[i], NULL,
                                         &RWlockTest::trywrlockThread_helper,
                                         this);
    ASSERT_FALSE(thread_created)<< "thread is not created!";
  }
  for (uint8_t i = 0; i < kNum_threads_; ++i) {
    pthread_join(thread[i], NULL);
  }
  EXPECT_FALSE(test_rwlock.Release());
}

}  // namespace utils
}  // namespace components
}  // namespace test
