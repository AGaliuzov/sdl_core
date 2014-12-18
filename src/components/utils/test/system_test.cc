/*
 * Copyright (c) 2014, Ford Motor Company
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

#include "gmock/gmock.h"
#include "utils/system.h"

namespace test {
namespace components {
namespace utils {

using namespace ::utils;

TEST(SystemTest, CommandCreated_WithoutArguments) {
  // Command creation without any arguments
  std::string test_command("ls");
  System object(test_command);

  // Check if the object was created with correct command
  ASSERT_EQ(object.command(), test_command);
  ASSERT_EQ(object.argv().size(), 1);
}

TEST(SystemTest, CommandCreated_WithArguments_Positive) {
  // Command creation with 1 argument
  std::string test_command("ls");
  std::string test_list_args("-la");
  System object(test_command, test_list_args);

  // Check if the object was created with correct command
  ASSERT_EQ(object.command(), test_command);

  // Check if actual number of arguments arec correct
  ASSERT_EQ(object.argv().size(), 1);// Correct number of arguments is 1

}

TEST(SystemTest, AddedArgumentsToExistingCommand_Positive) {
  std::string test_command("ls");
  const char* args[] = {"-la", "cp", "rm", "mv"};
  System object(test_command);

  // Adding arguments
  object.Add(args[0]);
  object.Add(args[1]);
  object.Add(args[2]);
  object.Add(args[3]);

  // Check if the object was created with correct command
  ASSERT_EQ(object.command(), test_command);

  // Check if the object was appended by correct argument
  ASSERT_STREQ(object.argv().back().c_str(), args[3]);

  // Check if actual number of arguments equal args stored in object
  ASSERT_EQ(object.argv().size(), 5);// Correct number of arguments is 5
}

TEST(SystemTest, SynchronousInvokeExistingCommand_Positive) {
  std::string test_command("ls");
  std::string test_list_args("-la");
  System object(test_command, test_list_args);

  // Check if Execute() method is working properly with synchronous command invoke

  ASSERT_TRUE(object.Execute(true));
}

TEST(SystemTest, ASynchronousInvokeExistingCommand_Positive) {
  std::string test_command("ls");
  std::string test_list_args("-la");
  System object(test_command, test_list_args);

  // Check if Execute() method is working properly with asynchronous command invoke
  ASSERT_TRUE(object.Execute());
}

} // namespace utils
} // namespace components
}  // namespace test
