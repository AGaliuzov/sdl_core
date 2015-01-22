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
#include "utils/singleton.h"
#include <pthread.h>

namespace test {
namespace components {
namespace utils {

using ::utils::Singleton;

class SingletonTest : public ::utils::Singleton<SingletonTest> {
 public:

  void SetValue(int value) {
    test_value = value;
  }
  int GetValue() {
    return test_value;
  }

  FRIEND_BASE_SINGLETON_CLASS (SingletonTest);
 private:
  int test_value = 0;
};

TEST(SingletonTest, CreateAndDestroySingleton) {
  //arrange
  SingletonTest *instance = SingletonTest::instance();

  //assert
  ASSERT_EQ(instance, SingletonTest::instance());
  ASSERT_EQ(0, instance->GetValue());
  ASSERT_TRUE(instance->exists());

  //act
  instance->destroy();

  //assert
  ASSERT_FALSE(instance->exists());
}

TEST(SingletonTest, CreateDeleteSingleton_Create2ObjectsDeleteLast) {
  //arrange
  SingletonTest *instance = SingletonTest::instance();

  //assert
  ASSERT_EQ(instance, SingletonTest::instance());
  ASSERT_EQ(0, instance->GetValue());
  ASSERT_TRUE(instance->exists());

  //act
  instance->SetValue(10);

  //assert
  SingletonTest *instance2 = SingletonTest::instance();
  ASSERT_EQ(10, instance2->GetValue());

  //act
  instance2->SetValue(5);

  //assert
  ASSERT_EQ(5, instance->GetValue());
  ASSERT_EQ(instance, instance2);

  //act
  instance2->destroy();

  //assert
  ASSERT_FALSE(instance->exists());
  ASSERT_FALSE(instance2->exists());
}

TEST(SingletonTest, DestroySingletonTwice) {
  //arrange
  SingletonTest *instance = SingletonTest::instance();

  //assert
  ASSERT_EQ(0, instance->GetValue());
  ASSERT_TRUE(instance->exists());

  //act
  instance->destroy();
  //assert
  ASSERT_FALSE(instance->exists());

  //act
  instance->destroy();
  //assert
  ASSERT_FALSE(instance->exists());
}

TEST(SingletonTest, DeleteSingletonCreateAnother) {
  //arrange
  SingletonTest *instance = SingletonTest::instance();
  instance->SetValue(10);

  //assert
  ASSERT_TRUE(instance->exists());
  //act
  instance->destroy();

  SingletonTest *instance_2 = SingletonTest::instance();

  //assert
  ASSERT_EQ(0, instance_2->GetValue());
  ASSERT_TRUE(instance_2->exists());
  instance_2->destroy();
}

void* func_pthread1(void*) {
  SingletonTest *instance = SingletonTest::instance();
  pthread_exit(instance);
  return NULL;
}

void* func_pthread2(void * value) {
  SingletonTest * instance = reinterpret_cast<SingletonTest *>(value);
  instance->destroy();
  pthread_exit (NULL);
  return NULL;
}

TEST(SingletonTest, CreateSingletonInDifferentThreads) {
  //arrange
  SingletonTest *instance = SingletonTest::instance();
  ASSERT_EQ(0, instance->GetValue());
  ASSERT_TRUE(instance->exists());

  pthread_t thread1;
  pthread_create(&thread1, NULL, func_pthread1, NULL);

  void *instance2;
  pthread_join(thread1, &instance2);
  SingletonTest * instance_2 = reinterpret_cast<SingletonTest *>(instance2);

  //assert
  ASSERT_EQ(instance, instance_2);

  //act
  instance->destroy();
  //assert
  ASSERT_FALSE(instance_2->exists());
}

TEST(SingletonTest, CreateDeleteSingletonInDifferentThreads) {
  //arrange
  pthread_t thread1;
  pthread_create(&thread1, NULL, func_pthread1, NULL);

  pthread_t thread2;
  pthread_create(&thread2, NULL, func_pthread1, NULL);

  void *instance1;
  pthread_join(thread1, &instance1);
  SingletonTest * instance_1 = reinterpret_cast<SingletonTest *>(instance1);

  void *instance2;
  pthread_join(thread2, &instance2);
  SingletonTest * instance_2 = reinterpret_cast<SingletonTest *>(instance2);

  //assert
  ASSERT_TRUE(instance_1->exists());
  ASSERT_TRUE(instance_2->exists());

  ASSERT_EQ(instance_1, instance_2);

  //act
  instance_1->destroy();

  //assert
  ASSERT_FALSE(instance_1->exists());
  ASSERT_FALSE(instance_2->exists());
}

TEST(SingletonTest, DeleteSingletonInDifferentThread) {
  //arrange
  SingletonTest *instance = SingletonTest::instance();
  ASSERT_EQ(0, instance->GetValue());
  ASSERT_TRUE(instance->exists());

  pthread_t thread1;
  pthread_create(&thread1, NULL, func_pthread2, instance);

  pthread_join(thread1, NULL);

  //assert
  ASSERT_FALSE(instance->exists());
}

}  // namespace utils
}  // namespace components
}  // namespace test
