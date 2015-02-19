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

#include <climits>
#include <string>
#include <algorithm>
#include "gtest/gtest.h"
#include "json/reader.h"
#include "formatters/CFormatterJsonBase.hpp"
#include "formatters/generic_json_formatter.h"

namespace test {
namespace components {
namespace formatters {

using namespace NsSmartDeviceLink::NsSmartObjects;
using namespace NsSmartDeviceLink::NsJSONHandler::Formatters;

TEST(CFormatterJsonBaseTest, JSonStringValueToSmartObj) {
  // Arrange value
  std::string string_val("test_string");
  Json::Value string_value(string_val);  // Json value from string
  SmartObject object;
  // Convert json to smart object
  CFormatterJsonBase::jsonValueToObj(string_value, object);
  // Check conversion was successful
  EXPECT_EQ(string_val, object.asString());
}

TEST(CFormatterJsonBaseTest, JSonDoubleValueToSmartObj) {
  // Arrange value
  double dval = 3.512;
  Json::Value double_value(dval);  // Json value from double
  SmartObject object;
  // Convert json to smart object
  CFormatterJsonBase::jsonValueToObj(double_value, object);
  // Check conversion was successful
  EXPECT_DOUBLE_EQ(dval, object.asDouble());
}

TEST(CFormatterJsonBaseTest, JSonIntValueToSmartObj) {
  // Arrange value
  int ival = -7;
  Json::Value int_value(ival);  // Json value from signed int
  SmartObject object;
  // Convert json to smart object
  CFormatterJsonBase::jsonValueToObj(int_value, object);
  // Check conversion was successful
  EXPECT_EQ(ival, object.asInt());
}

TEST(CFormatterJsonBaseTest, JSonUnsignedIntValueToSmartObj) {
  // Arrange value
  Json::UInt ui_val = 11;
  Json::Value uint_value(ui_val);  // Json value from unsigned int
  SmartObject object;
  // Convert json to smart object
  CFormatterJsonBase::jsonValueToObj(uint_value, object);
  // Check conversion was successful
  EXPECT_EQ(ui_val, object.asUInt());
}

TEST(CFormatterJsonBaseTest, JSonBoolValueToSmartObj) {
  // Arrange value
  bool bval1 = true;
  bool bval2 = false;
  Json::Value boolean_value1(bval1);  // Json value from bool
  Json::Value boolean_value2(bval2);  // Json value from bool
  SmartObject object1;
  SmartObject object2;
  // Convert json to smart object
  CFormatterJsonBase::jsonValueToObj(boolean_value1, object1);
  CFormatterJsonBase::jsonValueToObj(boolean_value2, object2);
  // Check conversion was successful
  EXPECT_TRUE(object1.asBool());
  EXPECT_FALSE(object2.asBool());
}

TEST(CFormatterJsonBaseTest, JSonCStringValueToSmartObj) {
  // Arrange value
  const char* cstr_val = "cstring_test";
  Json::Value cstring_value(cstr_val);  // Json value from const char*
  SmartObject object;
  // Convert json to smart object
  CFormatterJsonBase::jsonValueToObj(cstring_value, object);
  // Check conversion was successful
  EXPECT_STREQ(cstr_val, object.asCharArray());
}

TEST(CFormatterJsonBaseTest, JSonArrayValueToSmartObj) {
  // Arrange value
  const char* json_array = "[\"test1\", \"test2\", \"test3\"]";  // Array in json format
  Json::Value arr_value;  // Json value from array. Will be initialized later
  SmartObject object;
  Json::Reader reader;  // Json reader - Needed for correct parsing
  // Parse array to json value
  ASSERT_TRUE(reader.parse(json_array, arr_value));
  // Convert json array to SmartObject
  CFormatterJsonBase::jsonValueToObj(arr_value, object);
  // Check conversion was successful
  EXPECT_TRUE(arr_value.isArray());
  EXPECT_EQ(3u, object.asArray()->size());
  SmartArray *ptr = NULL;  // Smart Array pointer;
  EXPECT_NE(ptr, object.asArray());
}

TEST(CFormatterJsonBaseTest, JSonObjectValueToSmartObj) {
  // Arrange value
  const char* json_object =
      "{ \"json_test_object\": [\"test1\", \"test2\", \"test3\"], \"json_test_object2\": [\"test11\", \"test12\", \"test13\" ]}";  // Json object
  Json::Value object_value;  // Json value from object. Will be initialized later
  SmartObject object;
  Json::Reader reader;  // Json reader - Needed for correct parsing
  ASSERT_TRUE(reader.parse(json_object, object_value));  // If parsing not successful - no sense to continue
  CFormatterJsonBase::jsonValueToObj(object_value, object);
  // Check conversion was successful
  EXPECT_TRUE(object_value.isObject());
  EXPECT_TRUE(object_value.type() == Json::objectValue);
  // Get keys collection from Smart Object
  std::set<std::string> keys = object.enumerate();
  std::set<std::string>::iterator it1 = keys.begin();
  // Get membes names(keys) from Json object
  Json::Value::Members mems = object_value.getMemberNames();
  std::vector<std::string>::iterator it;
  // Compare sizes
  EXPECT_EQ(mems.size(), keys.size());
  // Sort mems
  std::sort(mems.begin(), mems.end());
  // Full data compare
  for (it = mems.begin(); it != mems.end(); ++it) {
    EXPECT_EQ(*it, *it1);
    ++it1;
  }
  ASSERT(it == mems.end() && it1 == keys.end());
}

TEST(CFormatterJsonBaseTest, StringSmartObjectToJSon) {
  // Arrange value
  std::string string_val("test_string");
  SmartObject object(string_val);
  Json::Value string_value;  // Json value from string
  // Convert smart object to json
  CFormatterJsonBase::objToJsonValue(object, string_value);
  // Check conversion was successful
  EXPECT_EQ(string_val, string_value.asString());
}

TEST(CFormatterJsonBaseTest, DoubleSmartObjectToJSon) {
  // Arrange value
  double dval = 3.512;
  Json::Value double_value;  // Json value from double
  SmartObject object(dval);
  // Convert json to smart object
  CFormatterJsonBase::objToJsonValue(object, double_value);
  // Check conversion was successful
  EXPECT_DOUBLE_EQ(dval, double_value.asDouble());
}

TEST(CFormatterJsonBaseTest, IntSmartObjectToJSon) {
  // Arrange value
  int ival = -7;
  Json::Value int_value;  // Json value from signed int
  SmartObject object(ival);
  // Convert json to smart object
  CFormatterJsonBase::objToJsonValue(object, int_value);
  // Check conversion was successful
  EXPECT_EQ(ival, int_value.asInt());
}

TEST(CFormatterJsonBaseTest, UnsignedIntSmartObjectToJSon) {
  // Arrange value
  Json::UInt ui_val = 11;
  Json::Value uint_value;  // Json value from unsigned int
  SmartObject object(ui_val);
  // Convert json to smart object
  CFormatterJsonBase::objToJsonValue(object, uint_value);
  // Check conversion was successful
  EXPECT_EQ(ui_val, uint_value.asUInt());
}

TEST(CFormatterJsonBaseTest, BoolSmartObjectToJSon) {
  // Arrange value
  bool bval1 = true;
  bool bval2 = false;
  Json::Value boolean_value1;  // Json value from bool
  Json::Value boolean_value2;  // Json value from bool
  SmartObject object1(bval1);
  SmartObject object2(bval2);
  // Convert json to smart object
  CFormatterJsonBase::objToJsonValue(object1, boolean_value1);
  CFormatterJsonBase::objToJsonValue(object2, boolean_value2);
  // Check conversion was successful
  EXPECT_TRUE(boolean_value1.asBool());
  EXPECT_FALSE(boolean_value2.asBool());
}

TEST(CFormatterJsonBaseTest, CStringSmartObjectToJSon) {
  // Arrange value
  const char* cstr_val = "cstring_test";
  Json::Value cstring_value;  // Json value from const char*
  SmartObject object(cstr_val);
  // Convert json to smart object
  CFormatterJsonBase::objToJsonValue(object, cstring_value);
  // Check conversion was successful
  EXPECT_STREQ(cstr_val, cstring_value.asCString());
}

TEST(CFormatterJsonBaseTest, ArraySmartObjectToJSon) {
  // Arrange value
  const char* json_array = "[\"test1\", \"test2\", \"test3\"]";  // Array in json format
  Json::Value arr_value;  // Json value from array. Will be initialized later
  Json::Value result;  // Json value from array. Will be initialized later
  SmartObject object;
  Json::Reader reader;  // Json reader - Needed for correct parsing
  // Parse array to json value
  ASSERT_TRUE(reader.parse(json_array, arr_value));  // Convert json array to SmartObject
  // Convert json array to SmartObject
  CFormatterJsonBase::jsonValueToObj(arr_value, object);
  // Convert SmartObject to JSon
  CFormatterJsonBase::objToJsonValue(object, result);
  // Check conversion was successful
  EXPECT_TRUE(result.isArray());
  EXPECT_EQ(3u, result.size());
}

TEST(CFormatterJsonBaseTest, JSonObjectValueToObj) {
  // Arrange value
  const char* json_object =
      "{ \"json_test_object\": [\"test1\", \"test2\", \"test3\"], \"json_test_object2\": [\"test11\", \"test12\", \"test13\" ]}";  // Json object
  Json::Value object_value;  // Json value from json object. Will be initialized later
  Json::Value result;  // Json value from Smart object. Will keep conversion result
  SmartObject object;
  Json::Reader reader;  // Json reader - Needed for correct parsing
  // Parse json object to correct json value
  ASSERT_TRUE(reader.parse(json_object, object_value));  // If parsing not successful - no sense to continue
  // Convert json array to SmartObject
  CFormatterJsonBase::jsonValueToObj(object_value, object);
  // Convert SmartObject to JSon
  CFormatterJsonBase::objToJsonValue(object, result);
  // Check conversion was successful
  EXPECT_TRUE(result.isObject());
  EXPECT_TRUE(result.type() == Json::objectValue);
  EXPECT_TRUE(result == object_value);
  // Get keys collection from Smart Object
  std::set<std::string> keys = object.enumerate();
  std::set<std::string>::iterator it1 = keys.begin();
  // Get membes names(keys) from Json object
  Json::Value::Members mems = result.getMemberNames();
  std::vector<std::string>::iterator it;
  // Compare sizes
  EXPECT_EQ(mems.size(), keys.size());
  // Sort mems
  std::sort(mems.begin(), mems.end());
  // Full data compare
  for (it = mems.begin(); it != mems.end(); ++it) {
    EXPECT_EQ(*it, *it1);
    ++it1;
  }
  ASSERT(it == mems.end() && it1 == keys.end());
}

}  // namespace formatters
}  // namespace components
}  // namespace test
