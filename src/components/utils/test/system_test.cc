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

TEST(SystemTest, CreateCommand_without_arguments) {
  // Command creation without any arguments
  std::string test_command("ls");
  System *object = new System(test_command);

  ASSERT_TRUE(object->GetCommand() == test_command);
  ASSERT_TRUE(object->GetArgumentsList().size() == 1);
  ASSERT_FALSE(object->GetArgumentsList().empty());

  delete object;
}

TEST(SystemTest, CreateCommand_with_arguments) {
  // Command creation with 1 argument
  std::string test_command("ls");
  std::string test_list_args("-la");
  System *object = new System(test_command, test_list_args);

  // Check if the object was created with correct command
  ASSERT_TRUE(object->GetCommand() == test_command);

  // Check if actual number of arguments arec correct
  ASSERT_TRUE(object->GetArgumentsList().size() == 1);// Correct number of arguments is 1
  ASSERT_FALSE(object->GetArgumentsList().size() == 3);
  delete object;
}

TEST(SystemTest, AddArgumentToExistingCommand) {
  // Command creation with 1 argument
  std::string test_command("ls");
  std::string test_list_args("-la");
  System *object = new System(test_command);

  object->Add(test_list_args);

  // Check if the object was created with correct command
  ASSERT_TRUE(object->GetCommand() == test_command);

  // Check if the object was appended by correct argument
  ASSERT_TRUE(object->GetArgumentsList().back() == test_list_args);

  // Check if actual number of arguments are correct
  ASSERT_TRUE(object->GetArgumentsList().size() == 2);// Correct number of arguments is 2
  delete object;
}

TEST(SystemTest, ExecuteExistingCommandWithArgs) {
  // Command creation with 1 argument
  std::string test_command("ls");
  std::string test_list_args("-la");
  System *object = new System(test_command, test_list_args);

  ASSERT_TRUE(object->Execute());
  ASSERT_TRUE(object->Execute(true));

  delete object;
}


}  // namespace utils
}  // namespace components
}  // namespace test
