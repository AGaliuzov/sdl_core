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

#include <unistd.h>
#include "gtest/gtest.h"
#include "utils/macro.h"
#include "utils/bitstream.h"

namespace test {
namespace components {
namespace utils {

using namespace ::utils;

TEST(BitstreamTest, CreateBitstream_CreateBitStreamWithoutDataWithoutDatasize_BitStreamIsGood) {

  //arrange
  uint8_t *data = NULL;
  size_t bits = sizeof(data);
  BitStream* bs = new BitStream(data, bits);

  //assert
  EXPECT_TRUE(bs->IsGood());
  delete bs;
}

TEST(BitstreamTest, CreateBitstream_CreateBitStreamWithoutDataWithBigDataSize_BitStreamIsGood) {

  //arrange
  uint8_t *data = NULL;
  size_t bits = 65535;
  BitStream* bs = new BitStream(data, bits);

  //assert
  EXPECT_TRUE(bs->IsGood());
  delete bs;
}

TEST(BitstreamTest, CreateBitstream_CreateBitStreamWithDataWithLessDataSize_BitStreamIsGood) {

  //arrange
  uint8_t data =255;
  size_t bits = sizeof(char);
  BitStream* bs = new BitStream(&data, bits);

  //assert
  EXPECT_TRUE(bs->IsGood());

  delete bs;
}

TEST(BitstreamTest, ExtractBitstream_CreateBitStreamWithDataWithLessDataSize_BitStreamIsGood) {

  //arrange
  uint8_t data =255;
  size_t bits = sizeof(char);
  BitStream* bs = new BitStream(&data, bits);

  Extract(bs, &data, bits);
  //assert
  EXPECT_TRUE(bs->IsGood());

  delete bs;
}

TEST(BitstreamTest, DISABLED_ExtractBitstream_CreateBitStreamWithZeroDataWithBigDataSize_BitStreamIsGood) {

  //arrange
  uint8_t data =0;
  size_t bits =65535;
  BitStream* bs = new BitStream(&data, bits);

  Extract(bs, &data, bits);
  //assert
  EXPECT_TRUE(bs->IsGood());

  delete bs;
}

TEST(BitstreamTest, DISABLED_ExtractBitstream_CreateBitStreamWithSmallDataWithBiggerDataSize_BitStreamIsGood) {

  //arrange
  uint8_t data =1;
  size_t bits =10;
  BitStream* bs = new BitStream(&data, bits);

  Extract(bs, &data, bits);
  //assert
  EXPECT_TRUE(bs->IsGood());

  delete bs;
}

TEST(BitstreamTest, ExtractBitstream_CreateBitStreamWithDataWithZeroDataSize_BitStreamIsGood) {

  //arrange
  uint8_t data =255;
  size_t bits = 0;
  BitStream* bs = new BitStream(&data, bits);

  Extract(bs, &data, bits);

  //assert
  EXPECT_TRUE(bs->IsGood());

  delete bs;
}

TEST(BitstreamTest, ExtractBitstream_CreateBitStreamWithDataMarkedBad_ExpectIsBad) {

  //arrange
  uint8_t data =255;
  size_t bits = sizeof(int);
  BitStream* bs = new BitStream(&data, bits);
  //assert
  EXPECT_TRUE(bs->IsGood());
  //act
  bs->MarkBad();

  //assert
  EXPECT_TRUE(bs->IsBad());
  //act
  Extract(bs, &data, bits);
  //arrange
  EXPECT_TRUE(bs->IsBad());

  delete bs;
}


}  // namespace utils
}  // namespace components
}  // namespace test
