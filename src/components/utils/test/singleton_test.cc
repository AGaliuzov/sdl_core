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

namespace test {
namespace components {
namespace utils {

using ::utils::Singleton;

class SingletonTest : public ::utils::Singleton<SingletonTest> {
 public:
  virtual ~SingletonTest() {};
  SingletonTest() {};

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
  SingletonTest *Instance = SingletonTest::instance();

  //assert
  ASSERT_EQ(Instance, SingletonTest::instance());
  ASSERT_EQ(0, Instance->GetValue());
  ASSERT_TRUE(Instance->exists());

  //act
  Instance->destroy();

  //assert
  ASSERT_FALSE(Instance->exists());
}

TEST(SingletonTest, CreateDeleteSingleton_Create2ObjectsDeleteLast) {
  //arrange
  SingletonTest *Instance = SingletonTest::instance();

  //assert
  ASSERT_EQ(Instance, SingletonTest::instance());
  ASSERT_EQ(0, Instance->GetValue());
  ASSERT_TRUE(Instance->exists());

  //act
  Instance->SetValue(10);

  //assert
  SingletonTest *Instance2 = SingletonTest::instance();
  ASSERT_EQ(10, Instance2->GetValue());

  //act
  Instance2->SetValue(5);

  //assert
  ASSERT_EQ(5, Instance->GetValue());
  ASSERT_EQ(Instance, Instance2);

  //act
  Instance2->destroy();

  //assert
  ASSERT_FALSE(Instance->exists());
  ASSERT_FALSE(Instance2->exists());
}

TEST(SingletonTest, DestroySingletonTwice) {
  //arrange
  SingletonTest *Instance = SingletonTest::instance();

  //assert
  ASSERT_EQ(0, Instance->GetValue());
  ASSERT_TRUE(Instance->exists());

  //act
  Instance->destroy();
  //assert
  ASSERT_FALSE(Instance->exists());

  //act
  Instance->destroy();
  //assert
  ASSERT_FALSE(Instance->exists());
}

TEST(SingletonTest, DeleteSingletonCreateAnother) {
  //arrange
  SingletonTest *Instance = SingletonTest::instance();
  Instance->SetValue(10);

  //assert
  ASSERT_TRUE(Instance->exists());
  //act
  Instance->destroy();

  SingletonTest *Instance_2 = SingletonTest::instance();

  //assert
  ASSERT_EQ(0, Instance_2->GetValue());
  ASSERT_TRUE(Instance_2->exists());
  Instance_2->destroy();
}

}  // namespace utils
}  // namespace components
}  // namespace test
