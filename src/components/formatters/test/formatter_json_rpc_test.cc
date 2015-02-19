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
#include <iostream>
#include "gtest/gtest.h"
#include "formatters/formatter_json_rpc.h"
#include "formatters/CSmartFactory.hpp"
#include "HMI_API_schema.h"

#include <iostream>

namespace test {
namespace components {
namespace formatters {

using namespace NsSmartDeviceLink::NsSmartObjects;
using namespace NsSmartDeviceLink::NsJSONHandler::Formatters;
using namespace NsSmartDeviceLink::NsJSONHandler::strings;

TEST(FormatterJsonRPCTest, CorrectRPC1_SmartObjectToString_EXPECT_SUCCESS) {

  std::string result;

  SmartObject obj;
  obj[S_PARAMS][S_FUNCTION_ID] = hmi_apis::FunctionID::VR_IsReady;
  obj[S_PARAMS][S_MESSAGE_TYPE] = hmi_apis::messageType::request;
  obj[S_PARAMS][S_PROTOCOL_VERSION] = 2;
  obj[S_PARAMS][S_PROTOCOL_TYPE] = 1;
  obj[S_PARAMS][S_CORRELATION_ID] = 4444;

  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  hmi_apis::HMI_API factory;
  EXPECT_TRUE(factory.attachSchema(obj));
  EXPECT_TRUE(FormatterJsonRpc::ToString(obj, result));
  EXPECT_EQ(
      std::string(
          "{\n   \"id\" : 4444,\n   \"jsonrpc\" : \"2.0\",\n   \"method\" : \"VR.IsReady\"\n}\n"),
      result);
}

TEST(FormatterJsonRPCTest, CorrectRPC2_SmartObjectToString_EXPECT_SUCCESS) {
  SmartObject obj;
  std::string result;
  obj[S_PARAMS][S_FUNCTION_ID] =
      hmi_apis::FunctionID::BasicCommunication_OnReady;
  obj[S_PARAMS][S_MESSAGE_TYPE] = hmi_apis::messageType::notification;
  obj[S_PARAMS][S_PROTOCOL_VERSION] = 2;
  obj[S_PARAMS][S_PROTOCOL_TYPE] = 1;
  obj[S_PARAMS][S_CORRELATION_ID] = 4222;

  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  hmi_apis::HMI_API factory;
  EXPECT_TRUE(factory.attachSchema(obj));
  EXPECT_TRUE(FormatterJsonRpc::ToString(obj, result));
  EXPECT_EQ(
      std::string(
          "{\n   \"jsonrpc\" : \"2.0\",\n   \"method\" : \"BasicCommunication.OnReady\",\n   \"params\" : {}\n}\n"),
      result);
}

TEST(FormatterJsonRPCTest, IncorrectRPC_SmartObjectToString_EXPECT_FALSE) {
  SmartObject obj;
  std::string result;
  obj[S_PARAMS][S_FUNCTION_ID] =
      hmi_apis::FunctionID::BasicCommunication_OnReady;
  obj[S_PARAMS][S_MESSAGE_TYPE] = hmi_apis::messageType::response;
  obj[S_PARAMS][S_PROTOCOL_VERSION] = 2;
  obj[S_PARAMS][S_PROTOCOL_TYPE] = 1;
  obj[S_PARAMS][S_CORRELATION_ID] = 4222;

  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  hmi_apis::HMI_API factory;
  EXPECT_FALSE(factory.attachSchema(obj));
  EXPECT_FALSE(FormatterJsonRpc::ToString(obj, result));
  // Expect result with default value. No correct conversion was done
  EXPECT_EQ(std::string("{\n   \"jsonrpc\" : \"2.0\"\n}\n"), result);
}

TEST(FormatterJsonRPCTest, FromString) {
  const std::string json_string(
      "{\n   \"jsonrpc\" : \"2.0\",\n   \"method\" : \"BasicCommunication.OnReady\",\n   \"params\" : {}\n}\n");
  SmartObject obj;
  Json::Value object_value;
  // Get keys collection from Smart Object
  std::set<std::string> keys = obj.enumerate();
  std::set<std::string>::iterator it1 = keys.begin();
  Json::Reader().parse(json_string, object_value);
  EXPECT_EQ(
      0,
      (FormatterJsonRpc::FromString<hmi_apis::FunctionID::eType,
          hmi_apis::messageType::eType>(json_string, obj)));
  // Get membes names(keys) from Json object
  Json::Value::Members mems = object_value.getMemberNames();
  const std::string result = object_value.toStyledString();
  EXPECT_EQ(3, mems.size());
  EXPECT_EQ(json_string, result);
}

//TEST(GenericJsonFormatter, FromString) {
//  namespace smartobj = NsSmartDeviceLink::NsSmartObjects;
//  namespace formatters = NsSmartDeviceLink::NsJSONHandler::Formatters;
//
//  smartobj::SmartObject result;
//
//  ASSERT_FALSE(formatters::GenericJsonFormatter::FromString("", result));
//  ASSERT_FALSE(formatters::GenericJsonFormatter::FromString("\"str", result));
//  ASSERT_FALSE(formatters::GenericJsonFormatter::FromString("[10", result));
//  ASSERT_FALSE(formatters::GenericJsonFormatter::FromString("{10}", result));
//
//  ASSERT_TRUE(formatters::GenericJsonFormatter::FromString("null", result));
//  ASSERT_EQ(smartobj::SmartType_Null, result.getType());
//
//  ASSERT_TRUE(formatters::GenericJsonFormatter::FromString("true", result));
//  ASSERT_EQ(smartobj::SmartType_Boolean, result.getType());
//  ASSERT_EQ(true, result.asBool());
//
//  ASSERT_TRUE(formatters::GenericJsonFormatter::FromString("1", result));
//  ASSERT_EQ(smartobj::SmartType_Integer, result.getType());
//  ASSERT_EQ(1, result.asInt());
//
//  ASSERT_TRUE(formatters::GenericJsonFormatter::FromString("0.5", result));
//  ASSERT_EQ(smartobj::SmartType_Double, result.getType());
//  ASSERT_DOUBLE_EQ(0.5, result.asDouble());
//
//  ASSERT_TRUE(formatters::GenericJsonFormatter::FromString("\"str\"", result));
//  ASSERT_EQ(smartobj::SmartType_String, result.getType());
//  ASSERT_STREQ("str", result.asString().c_str());
//
//  ASSERT_TRUE(formatters::GenericJsonFormatter::FromString("[true, null, 10]",
//                                                           result));
//  ASSERT_EQ(smartobj::SmartType_Array, result.getType());
//  ASSERT_EQ(smartobj::SmartType_Boolean, result.getElement(0U).getType());
//  ASSERT_EQ(true, result.getElement(0U).asBool());
//  ASSERT_EQ(smartobj::SmartType_Null, result.getElement(1U).getType());
//  ASSERT_EQ(smartobj::SmartType_Integer, result.getElement(2U).getType());
//  ASSERT_EQ(10, result.getElement(2U).asInt());
//
//  ASSERT_TRUE(
//    formatters::GenericJsonFormatter::FromString("{"
//                                                 " \"intField\": 100500,"
//                                                 " \"subobject\": {"
//                                                 "  \"arrayField\": [1, null],"
//                                                 "  \"strField\": \"str\""
//                                                 " }"
//                                                 "}",
//                                                 result));
//  ASSERT_EQ(smartobj::SmartType_Map, result.getType());
//  ASSERT_EQ(smartobj::SmartType_Integer,
//            result.getElement("intField").getType());
//  ASSERT_EQ(100500, result.getElement("intField").asInt());
//  ASSERT_EQ(smartobj::SmartType_Map, result.getElement("subobject").getType());
//  ASSERT_EQ(smartobj::SmartType_Array,
//            result.getElement("subobject").getElement("arrayField").getType());
//  ASSERT_EQ(smartobj::SmartType_Integer,
//            result.getElement("subobject").getElement("arrayField").getElement(0U).getType());
//  ASSERT_EQ(1, result.getElement("subobject").getElement("arrayField").getElement(0U).asInt());
//  ASSERT_EQ(smartobj::SmartType_Null,
//            result.getElement("subobject").getElement("arrayField").getElement(1U).getType());
//  ASSERT_EQ(smartobj::SmartType_String,
//            result.getElement("subobject").getElement("strField").getType());
//  ASSERT_STREQ(
//    "str",
//    result.getElement("subobject").getElement("strField").asString().c_str());
//}

}// formatters
}  // components
}  // test
