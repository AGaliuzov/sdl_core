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

#include <algorithm>
#include <fstream>
#include "gtest/gtest.h"
#include "utils/file_system.h"

namespace test {
namespace components {
namespace utils {

TEST(FileSystemTest, CreateDeleteDirectory) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  // Directory creation
  file_system::CreateDirectory("./Test directory");

  EXPECT_TRUE(file_system::DirectoryExists("./Test directory"));

  EXPECT_TRUE(file_system::IsDirectory("./Test directory"));

  //Directory removing
  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory", false));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
}

TEST(FileSystemTest, CreateDirectoryTwice) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  // Directory creation
  file_system::CreateDirectory("./Test directory");

  EXPECT_TRUE(file_system::DirectoryExists("./Test directory"));
  EXPECT_TRUE(file_system::IsDirectory("./Test directory"));

  //create directory second time
  file_system::CreateDirectory("./Test directory");
  EXPECT_TRUE(file_system::DirectoryExists("./Test directory"));

  //Directory removing
  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory", false));
  //try delete directory again
  EXPECT_FALSE(file_system::RemoveDirectory("./Test directory", false));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
}

TEST(FileSystemTest,DeleteDirectoryRecursively) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  // Create directories
  file_system::CreateDirectory("./Test directory");
  file_system::CreateDirectory("./Test directory/Test directory 2");

  //create file inside directory
  EXPECT_TRUE(file_system::CreateFile("./Test directory/test file"));

  EXPECT_FALSE(file_system::RemoveDirectory("./Test directory", false));
  EXPECT_TRUE(file_system::DirectoryExists("./Test directory"));
  EXPECT_TRUE(file_system::IsDirectory("./Test directory"));

  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory", true));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
}

TEST(FileSystemTest, CreateDirectoryRecursivelyDeleteRecursively) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  // Create directories recursively
  file_system::CreateDirectoryRecursively(
      "./Test directory/Test directory 2/Test directory 3");

  EXPECT_TRUE(file_system::DirectoryExists("./Test directory"));
  EXPECT_TRUE(file_system::IsDirectory("./Test directory"));

  EXPECT_TRUE(
      file_system::DirectoryExists("./Test directory/Test directory 2"));
  EXPECT_TRUE(file_system::IsDirectory("./Test directory/Test directory 2"));

  EXPECT_TRUE(
      file_system::DirectoryExists(
          "./Test directory/Test directory 2/Test directory 3"));
  EXPECT_TRUE(
      file_system::IsDirectory(
          "./Test directory/Test directory 2/Test directory 3"));

  //delete recursively
  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory", true));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
  EXPECT_FALSE(
      file_system::DirectoryExists("./Test directory/Test directory 2"));
  EXPECT_FALSE(
      file_system::DirectoryExists(
          "./Test directory/Test directory 2/Test directory 3"));
}

TEST(FileSystemTest, TwiceCreateDirectoryRecursivelyDeleteRecursivelyOnce) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  // Create directories recursively
  EXPECT_TRUE(
      file_system::CreateDirectoryRecursively(
          "./Test directory/Test directory 2/Test directory 3"));

  //check that all directories are created
  EXPECT_TRUE(file_system::DirectoryExists("./Test directory"));
  EXPECT_TRUE(file_system::IsDirectory("./Test directory"));

  EXPECT_TRUE(
      file_system::DirectoryExists("./Test directory/Test directory 2"));
  EXPECT_TRUE(file_system::IsDirectory("./Test directory/Test directory 2"));

  EXPECT_TRUE(
      file_system::DirectoryExists(
          "./Test directory/Test directory 2/Test directory 3"));
  EXPECT_TRUE(
      file_system::IsDirectory(
          "./Test directory/Test directory 2/Test directory 3"));

  //create directories recursively second time
  EXPECT_TRUE(
      file_system::CreateDirectoryRecursively(
          "./Test directory/Test directory 2/Test directory 3"));

  EXPECT_TRUE(file_system::DirectoryExists("./Test directory"));

  EXPECT_TRUE(
      file_system::DirectoryExists("./Test directory/Test directory 2"));

  EXPECT_TRUE(
      file_system::DirectoryExists(
          "./Test directory/Test directory 2/Test directory 3"));

  //delete recursively
  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory", true));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
  //delete recursively again is impossible
  EXPECT_FALSE(file_system::RemoveDirectory("./Test directory", true));

  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
  EXPECT_FALSE(
      file_system::DirectoryExists("./Test directory/Test directory 2"));
  EXPECT_FALSE(
      file_system::DirectoryExists(
          "./Test directory/Test directory 2/Test directory 3"));
}

TEST(FileSystemTest, CreateDeleteFile) {
  ASSERT_FALSE(file_system::FileExists("./test file"));
  // File creation
  EXPECT_TRUE(file_system::CreateFile("./test file"));
  EXPECT_FALSE(file_system::IsDirectory("./test file"));

  //delete file
  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  //try delete file again
  EXPECT_FALSE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, CheckIsDirectory) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  //create derictory and check that IsDirectory=true
  file_system::CreateDirectory("./Test directory");
  EXPECT_TRUE(file_system::IsDirectory("./Test directory"));

  //delete derictory and check, that IsDirectory=false
  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory", false));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
  EXPECT_FALSE(file_system::IsDirectory("./Test directory"));

  //create file and check that IsDirectory=false
  ASSERT_FALSE(file_system::FileExists("./test file"));
  EXPECT_TRUE(file_system::CreateFile("./test file"));
  EXPECT_FALSE(file_system::IsDirectory("./test file"));

  //delete file and check that IsDirectory=false
  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
  EXPECT_FALSE(file_system::IsDirectory("./test file"));
}

TEST(FileSystemTest, CreateFileTwice) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  //create file first time
  EXPECT_TRUE(file_system::CreateFile("./test file"));
  EXPECT_TRUE(file_system::FileExists("./test file"));

  //create file second time
  EXPECT_TRUE(file_system::CreateFile("./test file"));
  EXPECT_TRUE(file_system::FileExists("./test file"));

  //delete file
  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, CreateOpenCloseFileStream) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  std::ofstream* test_file = file_system::Open("./test file");
  EXPECT_TRUE(test_file->is_open());
  file_system::Close(test_file);
  EXPECT_FALSE(test_file->is_open());
  delete test_file;

  EXPECT_TRUE(file_system::FileExists("./test file"));

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, CreateAndOpenFileStreamTwice) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  std::ofstream* test_file = file_system::Open("./test file");
  EXPECT_TRUE(test_file->is_open());
  file_system::Close(test_file);
  EXPECT_FALSE(test_file->is_open());
  delete test_file;

  EXPECT_TRUE(file_system::FileExists("./test file"));

  //create file second time
  EXPECT_TRUE(file_system::CreateFile("./test file"));

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, OpenFileWriteInFileStream) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  std::ofstream* test_file = file_system::Open("./test file");
  EXPECT_TRUE(test_file->is_open());

  //write data in file
  uint32_t data_size = 4;
  uint8_t* data = new uint8_t[data_size];
  for (uint i = 0; i < data_size; ++i) {
    data[i] = i;
  }
  EXPECT_TRUE(file_system::Write(test_file, data, data_size));
  file_system::Close(test_file);
  EXPECT_FALSE(test_file->is_open());
  delete test_file;

  // Read data from file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

//  check data
  for (uint i = 0; i < data_size; ++i) {
    EXPECT_EQ(data[i], result[i]);
  }
  delete data;

  //delete file
  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, CannotWriteInClosedFileStream) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  std::ofstream* test_file = file_system::Open("./test file");
  EXPECT_TRUE(test_file->is_open());
  file_system::Close(test_file);
  EXPECT_FALSE(test_file->is_open());

  //write data in file
  uint32_t data_size = 4;
  uint8_t* data = new uint8_t[data_size];
  for (uint i = 0; i < data_size; ++i) {
    data[i] = i;
  }
  EXPECT_TRUE(file_system::Write(test_file, data, data_size));

  delete data;
  delete test_file;

  // Read data from file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_TRUE(result.empty());

  //delete file
  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, CreateWriteInFileStream_CreateFileAgain_FileRewritten) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  std::ofstream* test_file = file_system::Open("./test file");
  EXPECT_TRUE(test_file->is_open());

  //write data in file
  uint32_t data_size = 4;
  uint8_t* data = new uint8_t[data_size];
  for (uint i = 0; i < data_size; ++i) {
    data[i] = i;
  }
  EXPECT_TRUE(file_system::Write(test_file, data, data_size));

  file_system::Close(test_file);
  delete test_file;

  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  delete data;
  EXPECT_TRUE(file_system::CreateFile("./test file"));

  //now file is empty
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_TRUE(result.empty());

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, CreateFileStream_WriteInFile_FileStreamNotClosed) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  std::ofstream* test_file = file_system::Open("./test file");
  EXPECT_TRUE(test_file->is_open());

  //write data in file
  uint32_t data_size = 4;
  std::vector < uint8_t > data;
  for (uint i = 0; i < data_size; ++i) {
    data.push_back(i);
  }
  //write data in file
  EXPECT_TRUE(file_system::Write("./test file", data));
  EXPECT_TRUE(test_file->is_open());

  //close filestream
  file_system::Close(test_file);
  delete test_file;

  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, CreateFileStream_WriteInFileWriteInFileStream_FileIncludeLastData) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  std::ofstream* test_file = file_system::Open("./test file");
  EXPECT_TRUE(test_file->is_open());

  //write data in file
  uint32_t data_size = 4;
  std::vector < uint8_t > data;
  for (uint i = 0; i < data_size; ++i) {
    data.push_back(i);
  }
  //write data in file
  EXPECT_TRUE(file_system::Write("./test file", data));

  EXPECT_TRUE(test_file->is_open());

  //write in filestream
  uint8_t* data_2 = new uint8_t[data_size];
  for (uint i = 0; i < data_size; ++i) {
    data_2[i] = i + data_size;
  }
  EXPECT_TRUE(file_system::Write(test_file, data_2, data_size));
  //close filestream
  file_system::Close(test_file);

  delete test_file;
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  //check data
  EXPECT_EQ(result.size(), data_size);
  for (uint i = 0; i < data_size; ++i) {
    EXPECT_NE(data[i], result[i]);
    EXPECT_EQ(data_2[i], result[i]);
  }

  delete data_2;

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteInFilestreamTwice_FileRewritten) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  std::ofstream* test_file = file_system::Open("./test file");
  EXPECT_TRUE(test_file->is_open());

  //open file second time
  std::ofstream* test_file_2 = file_system::Open("./test file");
  EXPECT_TRUE(test_file_2->is_open());

  uint32_t data_size = 4;
  uint8_t* data = new uint8_t[data_size];
  for (uint i = 0; i < data_size; ++i) {
    data[i] = i;
  }
  uint8_t* data_2 = new uint8_t[data_size];
  for (uint i = 0; i < data_size; ++i) {
    data_2[i] = i + 4;
  }

  //write data in file
  EXPECT_TRUE(file_system::Write(test_file, data, data_size));

  EXPECT_TRUE(file_system::Write(test_file_2, data_2, data_size));

  file_system::Close(test_file);
  file_system::Close(test_file_2);

  EXPECT_FALSE(test_file->is_open());
  EXPECT_FALSE(test_file_2->is_open());

  delete test_file;
  delete test_file_2;
  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());
  //check data
  for (uint i = 0; i < data_size; ++i) {
    EXPECT_NE(data[i], result[i]);
    EXPECT_EQ(data_2[i], result[i]);
  }

  delete data;
  delete data_2;

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteInFilestreamConsequentially_FileRewritten) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  std::ofstream* test_file = file_system::Open("./test file");
  EXPECT_TRUE(test_file->is_open());

  uint32_t data_size = 4;
  uint8_t* data = new uint8_t[data_size];
  for (uint i = 0; i < data_size; ++i) {
    data[i] = i;
  }

  //write data in file
  EXPECT_TRUE(file_system::Write(test_file, data, data_size));

  file_system::Close(test_file);
  EXPECT_FALSE(test_file->is_open());

  //open file second times
  std::ofstream* test_file_2 = file_system::Open("./test file");
  EXPECT_TRUE(test_file_2->is_open());

  //write second time
  uint8_t* data_2 = new uint8_t[data_size];
  for (uint i = 0; i < data_size; ++i) {
    data_2[i] = i + 4;
  }
  EXPECT_TRUE(file_system::Write(test_file_2, data_2, data_size));

  file_system::Close(test_file_2);
  EXPECT_FALSE(test_file_2->is_open());

  delete test_file;
  delete test_file_2;
  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  //check data
  EXPECT_EQ(result.size(), data_size);
  for (uint i = 0; i < data_size; ++i) {
    EXPECT_NE(data[i], result[i]);
    EXPECT_EQ(data_2[i], result[i]);
  }

  delete data;
  delete data_2;

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, CreateFileTwiceWriteInFileTwice) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  EXPECT_TRUE(file_system::CreateFile("./test file"));
  EXPECT_TRUE(file_system::FileExists("./test file"));

  uint32_t data_size = 4;
  std::vector < uint8_t > data;
  for (uint i = 0; i < data_size; ++i) {
    data.push_back(i);
  }

  //write data in file
  EXPECT_TRUE(file_system::Write("./test file", data));
  //create file second time
  EXPECT_TRUE(file_system::CreateFile("./test file"));
  EXPECT_TRUE(file_system::CreateFile("./test file"));

  std::vector < uint8_t > data_2;
  for (uint i = 0; i < data_size; ++i) {
    data_2.push_back(i + data_size);
  }

  //write data in file
  EXPECT_TRUE(file_system::Write("./test file", data_2));

  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  EXPECT_EQ(data_2, result);
  EXPECT_EQ(result.size(), data_size);
  //check data
  for (uint i = 0; i < data_size; ++i) {
    EXPECT_NE(data[i], result[i]);
    EXPECT_EQ(data_2[i], result[i]);
  }

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteInFileTwiceFileRewritten) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  EXPECT_TRUE(file_system::CreateFile("./test file"));
  EXPECT_TRUE(file_system::FileExists("./test file"));

  //write data in file
  uint32_t data_size = 4;
  std::vector < uint8_t > data;
  for (uint i = 0; i < data_size; ++i) {
    data.push_back(i);
  }
  EXPECT_TRUE(file_system::Write("./test file", data));

  //write data in file second time
  std::vector < uint8_t > data_2;
  for (uint i = 0; i < data_size; ++i) {
    data_2.push_back(i + data_size);
  }
  EXPECT_TRUE(file_system::Write("./test file", data_2));

  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  //check data
  EXPECT_EQ(data_size, result.size());
  for (uint i = 0; i < data_size; ++i) {
    EXPECT_NE(data[i], result[i]);
    EXPECT_EQ(data_2[i], result[i]);
  }

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteDataInTheEndOfFile) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  EXPECT_TRUE(file_system::CreateFile("./test file"));
  EXPECT_TRUE(file_system::FileExists("./test file"));

  int32_t data_size = 4;
  std::vector < uint8_t > data;
  for (int i = 0; i < data_size; ++i) {
    data.push_back(i);
  }

  //write data in file
  EXPECT_TRUE(file_system::Write("./test file", data));

  //write in file second time

  std::vector < uint8_t > data_2;
  for (int i = 0; i < data_size; ++i) {
    data_2.push_back(i + data_size);
  }

  //write data in file
  EXPECT_TRUE(file_system::Write("./test file", data_2, std::ios_base::app));

  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  std::vector < uint8_t > data_check;
  for (int i = 0; i < 2 * data_size; ++i) {
    data_check.push_back(i);
  }

  //check data
  EXPECT_EQ(data_check.size(), result.size());
  for (int i = 0; i < 2 * data_size; ++i) {
    EXPECT_EQ(data_check[i], result[i]);
  }

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteInFileStream_WriteInFileInTheEndOfFile_FileIncludeBothData) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  // Create and open file
  std::ofstream* test_file = file_system::Open("./test file");
  EXPECT_TRUE(test_file->is_open());

  //write data in file
  uint32_t data_size = 4;
  std::vector < uint8_t > data;
  for (uint i = 0; i < data_size; ++i) {
    data.push_back(i);
  }
  //write data in file
  EXPECT_TRUE(file_system::Write("./test file", data));
  EXPECT_TRUE(test_file->is_open());

  //close filestream
  file_system::Close(test_file);

  delete test_file;
  //write in file second time
  std::vector < uint8_t > data_2;
  for (uint i = 0; i < data_size; ++i) {
    data_2.push_back(i + data_size);
  }

  //write data in file
  EXPECT_TRUE(file_system::Write("./test file", data_2, std::ios_base::app));

  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  std::vector < uint8_t > data_check;
  for (uint i = 0; i < 2 * data_size; ++i) {
    data_check.push_back(i);
  }

  //check data
  EXPECT_EQ(data_check.size(), result.size());
  for (uint i = 0; i < 2 * data_size; ++i) {
    EXPECT_EQ(data_check[i], result[i]);
  }

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, OpenFileStreamForRead_WriteInFileStream) {
  ASSERT_FALSE(file_system::FileExists("./test file"));
  // File creation
  EXPECT_TRUE(file_system::CreateFile("./test file"));
  std::ofstream* test_file = file_system::Open("./test file",
                                                std::ios_base::in);
  EXPECT_TRUE(test_file->is_open());

  //write data in file
  uint32_t data_size = 4;
  uint8_t* data = new uint8_t[data_size];
  for (uint i = 0; i < data_size; ++i) {
    data[i] = i;
  }

  EXPECT_TRUE(file_system::Write(test_file, data, data_size));

  file_system::Close(test_file);
  EXPECT_FALSE(test_file->is_open());

  // Read data from file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  //  check data
  for (uint i = 0; i < data_size; ++i) {
    EXPECT_EQ(data[i], result[i]);
  }

  delete data;
  delete test_file;

  EXPECT_TRUE(file_system::FileExists("./test file"));

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteFileNotExists) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  std::vector<unsigned char> data= {'t','e','s','t'};
  EXPECT_TRUE(file_system::Write("./test file", data));
  //file now exists
  ASSERT_TRUE(file_system::FileExists("./test file"));
  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteFileReadFile) {
  ASSERT_FALSE(file_system::FileExists("./test file"));
  EXPECT_TRUE(file_system::CreateFile("./test file"));

  std::vector<unsigned char> data= {'t','e','s','t'};
  EXPECT_TRUE(file_system::Write("./test file", data));

  // Read data from file
  std::string result;
  std::string check = "test";
  EXPECT_TRUE(file_system::ReadFile("./test file", result));
  EXPECT_NE(0, result.size());
  EXPECT_EQ(check, result);

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteBinaryDataReadBinaryFile) {
  ASSERT_FALSE(file_system::FileExists("./test file"));
  EXPECT_TRUE(file_system::CreateFile("./test file"));

  std::vector < uint8_t > data;
  data.push_back(1);
  data.push_back(2);
  data.push_back(3);
  data.push_back(4);
  EXPECT_TRUE(file_system::WriteBinaryFile("./test file", data));

  // Read data from file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());
  EXPECT_EQ(data, result);

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
}

TEST(FileSystemTest, WriteBinaryDataTwice_FileRewritten) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  EXPECT_TRUE(file_system::CreateFile("./test file"));
  EXPECT_TRUE(file_system::FileExists("./test file"));

  int32_t data_size = 4;
  std::vector < uint8_t > data;
  for (int i = 0; i < data_size; ++i) {
    data.push_back(i);
  }
  //write data in file
  EXPECT_TRUE(file_system::WriteBinaryFile("./test file", data));

  //write in file second time
  std::vector < uint8_t > data_2;
  for (int i = 0; i < data_size; ++i) {
    data_2.push_back(i + data_size);
  }

  //write data in file
  EXPECT_TRUE(file_system::WriteBinaryFile("./test file", data_2));

  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  //check data
  EXPECT_EQ(data_2.size(), result.size());
  for (int i = 0; i < data_size; ++i) {
    EXPECT_EQ(data_2[i], result[i]);
  }

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteBinaryDataFileNotExists) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  int32_t data_size = 4;
  std::vector < uint8_t > data;
  for (int i = 0; i < data_size; ++i) {
    data.push_back(i);
  }

  EXPECT_TRUE(file_system::WriteBinaryFile("./test file", data));
  ASSERT_TRUE(file_system::FileExists("./test file"));
  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteDataAsBinaryData) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  std::vector<unsigned char> data= {'t','e','s','t'};
  EXPECT_TRUE(file_system::WriteBinaryFile("./test file", data));
  ASSERT_TRUE(file_system::FileExists("./test file"));

  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  EXPECT_EQ(data.size(), result.size());

  for (uint i = 0; i < result.size(); ++i) {
    EXPECT_EQ(data[i], result[i]);
  }

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteEmptyData) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  std::vector<unsigned char> data;
  EXPECT_TRUE(file_system::Write("./test file", data));
  ASSERT_TRUE(file_system::FileExists("./test file"));

  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_TRUE(result.empty());

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteEmptyDataAsBinaryData) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  //write empty data
  std::vector<unsigned char> data;
  EXPECT_TRUE(file_system::WriteBinaryFile("./test file", data));
  ASSERT_TRUE(file_system::FileExists("./test file"));

  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_TRUE(result.empty());

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteBinaryData_WriteDataInTheEndOfFile) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  //write binary file
  std::vector<unsigned char> data= {'t','e','s','t'};
  EXPECT_TRUE(file_system::WriteBinaryFile("./test file", data));
  ASSERT_TRUE(file_system::FileExists("./test file"));

  //write in file second time
  int32_t data_size = 4;
  std::vector < uint8_t > data_2;
  for (int i = 0; i < data_size; ++i) {
    data_2.push_back(i);
  }

  //write data in file
  EXPECT_TRUE(file_system::Write("./test file", data_2, std::ios_base::app));

  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  //prepare data for check
  data.insert(data.end(), data_2.begin(), data_2.end());

  //compare data
  EXPECT_EQ(data.size(), result.size());
  for (uint i = 0; i < result.size(); ++i) {
    EXPECT_EQ(data[i], result[i]);
  }

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, CreateFile_WriteDataWithFlagOpenForReading) {
  ASSERT_FALSE(file_system::FileExists("./test file"));
  EXPECT_TRUE(file_system::CreateFile("./test file"));
  //write data in file
  int32_t data_size = 4;
  std::vector < uint8_t > data;
  for (int i = 0; i < data_size; ++i) {
    data.push_back(i);
  }
  EXPECT_TRUE(file_system::Write("./test file", data, std::ios_base::in));
  EXPECT_TRUE(file_system::FileExists("./test file"));

  //check file
  std::vector < uint8_t > result;
  EXPECT_TRUE(file_system::ReadBinaryFile("./test file", result));
  EXPECT_FALSE(result.empty());

  //compare data
  EXPECT_EQ(data.size(), result.size());
  for (uint i = 0; i < result.size(); ++i) {
    EXPECT_EQ(data[i], result[i]);
  }

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, FileDoesNotCreated_WriteFileWithFlagOpenForReadingIsImpossible) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  //write data in file is impossible
  int32_t data_size = 4;
  std::vector < uint8_t > data;
  for (int i = 0; i < data_size; ++i) {
    data.push_back(i);
  }
  EXPECT_FALSE(file_system::Write("./test file", data, std::ios_base::in));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, WriteFileGetSize) {
  ASSERT_FALSE(file_system::FileExists("./test file"));
  EXPECT_TRUE(file_system::CreateFile("./test file"));
  EXPECT_EQ(0, file_system::FileSize("./test file"));

  std::vector<unsigned char> data= {'t','e','s','t'};
  EXPECT_TRUE(file_system::Write("./test file", data));

  EXPECT_NE(0, file_system::FileSize("./test file"));

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, CreateFileCheckDefaultAccess) {
  // File creation
  ASSERT_FALSE(file_system::FileExists("./test file"));
  EXPECT_TRUE(file_system::CreateFile("./test file"));

  //check accesses
  EXPECT_TRUE(file_system::IsAccessible("./test file", R_OK));
  EXPECT_TRUE(file_system::IsAccessible("./test file", W_OK));
  EXPECT_TRUE(file_system::IsReadingAllowed("./test file"));
  EXPECT_TRUE(file_system::IsWritingAllowed("./test file"));

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));
}

TEST(FileSystemTest, GetFileModificationTime) {
  ASSERT_FALSE(file_system::FileExists("./test file"));

  EXPECT_TRUE(file_system::CreateFile("./test file"));

  uint64_t modif_time = file_system::GetFileModificationTime("./test file");
  EXPECT_LE(0, modif_time);

  std::vector < uint8_t > data(1,1);
  EXPECT_TRUE(file_system::WriteBinaryFile("./test file", data));

  EXPECT_LE(0, file_system::GetFileModificationTime("./test file"));
  EXPECT_LE(modif_time, file_system::GetFileModificationTime("./test file"));

  EXPECT_TRUE(file_system::DeleteFile("./test file"));
  EXPECT_FALSE(file_system::FileExists("./test file"));

}

TEST(FileSystemTest, ListFiles) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  file_system::CreateDirectory("./Test directory");

  std::vector < std::string > list;
  list = file_system::ListFiles("./Test directory");
  EXPECT_TRUE(list.empty());

  EXPECT_TRUE(file_system::CreateFile("./Test directory/test file"));
  EXPECT_TRUE(file_system::CreateFile("./Test directory/test file 2"));

  list = file_system::ListFiles("./Test directory");
  EXPECT_FALSE(list.empty());

  std::sort(list.begin(), list.end());
  EXPECT_EQ("test file", list[0]);
  EXPECT_EQ("test file 2", list[1]);

  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory", true));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));

  EXPECT_FALSE(file_system::FileExists("./Test directory/test file"));
  EXPECT_FALSE(file_system::FileExists("./Test directory/test file 2"));
}

TEST(FileSystemTest, ListFilesIncludeSubdirectory) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  file_system::CreateDirectoryRecursively("./Test directory/Test directory 2/");

  std::vector < std::string > list;
  list = file_system::ListFiles("./Test directory");
  EXPECT_FALSE(list.empty());
  EXPECT_EQ(1, list.size());
  EXPECT_EQ("Test directory 2", list[0]);

  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory", true));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
}

TEST(FileSystemTest, ListFilesDoesNotIncludeFilesInSubdirectory) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  file_system::CreateDirectoryRecursively("./Test directory/Test directory 2/");

  std::vector < std::string > list;
  list = file_system::ListFiles("./Test directory");
  EXPECT_FALSE(list.empty());

  EXPECT_TRUE(
      file_system::CreateFile("./Test directory/Test directory 2/test file"));
  EXPECT_TRUE(
      file_system::CreateFile("./Test directory/Test directory 2/test file 2"));

  list = file_system::ListFiles("./Test directory");
  EXPECT_FALSE(list.empty());

  std::sort(list.begin(), list.end());
  EXPECT_EQ("Test directory 2", list[0]);
  EXPECT_EQ(1, list.size());

  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory", true));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
}

TEST(FileSystemTest, GetAvailableDiskSpace) {

  //get available disk space before directory with file creaction and after
  uint64_t available_space = file_system::GetAvailableDiskSpace(".");
  EXPECT_NE(0, available_space);
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  file_system::CreateDirectory("./Test directory");

  std::vector<unsigned char> data= {'t','e','s','t'};
  EXPECT_TRUE(file_system::Write("./Test directory/test file", data));

  EXPECT_GE(available_space, file_system::GetAvailableDiskSpace("."));
  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory"));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
}

TEST(FileSystemTest, ConvertPathForURL) {
  std::string path = "./Test directory";
  EXPECT_NE(path, file_system::ConvertPathForURL(path));
  std::string path_brackets = "./Test_directory_with(brackets)";
  EXPECT_NE(path_brackets, file_system::ConvertPathForURL(path));
  std::string another_path = "./Test_directory/new_directory_without_spaces";
  EXPECT_EQ(another_path, file_system::ConvertPathForURL(another_path));
}

TEST(FileSystemTest, DirectorySize) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  file_system::CreateDirectory("./Test directory");
  EXPECT_TRUE(file_system::DirectoryExists("./Test directory"));
  //get size of empty directory
  EXPECT_EQ(0, file_system::DirectorySize("./Test directory"));
  EXPECT_TRUE(file_system::CreateFile("./Test directory/test file"));

  //get size of nonempty directory with empty file
  EXPECT_EQ(0, file_system::DirectorySize("./Test directory"));

  std::vector<unsigned char> data= {'t','e','s','t'};

  EXPECT_TRUE(file_system::Write("./Test directory/test file", data));
  //get size of nonempty directory with nonempty file
  EXPECT_NE(0, file_system::DirectorySize("./Test directory"));

  EXPECT_TRUE(file_system::DeleteFile("./Test directory/test file"));
  EXPECT_EQ(0, file_system::DirectorySize("./Test directory"));
  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory"));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
}

TEST(FileSystemTest, DeleteAllContentInDirectory) {
  ASSERT_FALSE(file_system::DirectoryExists("./Test directory"));
  file_system::CreateDirectory("./Test directory");

  //create files in directory
  EXPECT_TRUE(file_system::CreateFile("./Test directory/test file"));
  EXPECT_TRUE(file_system::CreateFile("./Test directory/test file 2"));

  EXPECT_TRUE(file_system::FileExists("./Test directory/test file"));
  EXPECT_TRUE(file_system::FileExists("./Test directory/test file 2"));

  EXPECT_TRUE(file_system::DirectoryExists("./Test directory"));

  //create subdirectories
  file_system::CreateDirectoryRecursively(
      "./Test directory/Test directory 2/Test directory 3");

  EXPECT_TRUE(
      file_system::DirectoryExists("./Test directory/Test directory 2"));
  EXPECT_TRUE(
      file_system::DirectoryExists(
          "./Test directory/Test directory 2/Test directory 3"));

  file_system::remove_directory_content("./Test directory");

  //directory does not include files and subdirectories
  EXPECT_FALSE(file_system::FileExists("./Test directory/test file"));
  EXPECT_FALSE(file_system::FileExists("./Test directory/test file 2"));

  EXPECT_FALSE(
      file_system::DirectoryExists(
          "./Test directory/Test directory 2/Test directory 3"));
  EXPECT_FALSE(
      file_system::DirectoryExists("./Test directory/Test directory 2"));

  std::vector < std::string > list;
  list = file_system::ListFiles("./Test directory");
  EXPECT_TRUE(list.empty());

  EXPECT_TRUE(file_system::DirectoryExists("./Test directory"));

  EXPECT_TRUE(file_system::RemoveDirectory("./Test directory", true));
  EXPECT_FALSE(file_system::DirectoryExists("./Test directory"));
}

}  // namespace utils
}  // namespace components
}  // namespace test
