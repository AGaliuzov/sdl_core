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

#include "formatters/formatter_json_rpc.h"
#include <string>
#include <set>
#include "gtest/gtest.h"
#include "formatters/CSmartFactory.h"
#include "interfaces/HMI_API_schema.h"
#include "interfaces/MOBILE_API_schema.h"

namespace test {
namespace components {
namespace formatters_test {

using namespace NsSmartDeviceLink::NsSmartObjects;
using namespace NsSmartDeviceLink::NsJSONHandler::Formatters;
using namespace NsSmartDeviceLink::NsJSONHandler::strings;

TEST(FormatterJsonRPCTest, CorrectRPCv1Request_ToString_Success) {
  // Create SmartObject
  SmartObject obj;
  obj[S_PARAMS][S_FUNCTION_ID] = hmi_apis::FunctionID::VR_IsReady;
  obj[S_PARAMS][S_MESSAGE_TYPE] = hmi_apis::messageType::request;
  obj[S_PARAMS][S_PROTOCOL_VERSION] = 2;
  obj[S_PARAMS][S_PROTOCOL_TYPE] = 1;
  obj[S_PARAMS][S_CORRELATION_ID] = 4444;
  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  // Attach Schema
  hmi_apis::HMI_API factory;
  EXPECT_TRUE(factory.attachSchema(obj, false));

  std::string result;
  // Convert SmartObject to Json string
  EXPECT_TRUE(FormatterJsonRpc::ToString(obj, result));
  EXPECT_EQ(std::string(
                "{\n   \"id\" : 4444,\n   \"jsonrpc\" : \"2.0\",\n   "
                "\"method\" : \"VR.IsReady\"\n}\n"),
            result);
}

TEST(FormatterJsonRPCTest, CorrectRPCv2Request_ToString_Success) {
  // Create SmartObject
  SmartObject obj;
  obj[S_PARAMS][S_FUNCTION_ID] = mobile_apis::FunctionID::AddCommandID;
  obj[S_PARAMS][S_MESSAGE_TYPE] = mobile_apis::messageType::request;
  obj[S_PARAMS][S_CORRELATION_ID] = 4444;
  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  // Attach Schema
  mobile_apis::MOBILE_API factory;
  EXPECT_TRUE(factory.attachSchema(obj, false));

  std::string result;
  // Convert SmartObject to Json string
  EXPECT_TRUE(FormatterJsonRpc::ToString(obj, result));
  EXPECT_EQ(std::string(
                "{\n   \"id\" : 4444,\n   \"jsonrpc\" : \"2.0\",\n   "
                "\"method\" : \"AddCommandID\"\n}\n"),
            result);
}

TEST(FormatterJsonRPCTest, CorrectRPCv1Notification_ToString_Success) {
  // Create SmartObject
  SmartObject obj;
  std::string result;
  obj[S_PARAMS][S_FUNCTION_ID] = hmi_apis::FunctionID::Buttons_OnButtonPress;
  obj[S_PARAMS][S_MESSAGE_TYPE] = hmi_apis::messageType::notification;
  obj[S_PARAMS][S_PROTOCOL_VERSION] = 2;
  obj[S_PARAMS][S_PROTOCOL_TYPE] = 1;
  obj[S_PARAMS][S_CORRELATION_ID] = 4222;
  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  // Attach Schema
  hmi_apis::HMI_API factory;
  EXPECT_TRUE(factory.attachSchema(obj, false));
  // Convert SmartObject to Json string
  EXPECT_TRUE(FormatterJsonRpc::ToString(obj, result));
  EXPECT_EQ(std::string(
                "{\n   \"jsonrpc\" : \"2.0\",\n   \"method\" : "
                "\"Buttons.OnButtonPress\",\n   \"params\" : {}\n}\n"),
            result);
}

TEST(FormatterJsonRPCTest, CorrectResponseToString_Success) {
  // Create SmartObject
  SmartObject obj;
  obj[S_PARAMS][S_FUNCTION_ID] = hmi_apis::FunctionID::VR_AddCommand;
  obj[S_PARAMS][S_MESSAGE_TYPE] = hmi_apis::messageType::response;
  obj[S_PARAMS][S_CORRELATION_ID] = 4440;
  obj[S_PARAMS][kCode] = hmi_apis::Common_Result::SUCCESS;
  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  // Attach Schema
  hmi_apis::HMI_API factory;
  EXPECT_TRUE(factory.attachSchema(obj, false));

  std::string result;
  // Convert SmartObject to Json string
  EXPECT_TRUE(FormatterJsonRpc::ToString(obj, result));
  EXPECT_EQ(std::string(
                "{\n   \"id\" : 4440,\n   \"jsonrpc\" : \"2.0\",\n   "
                "\"result\" : {\n      \"code\" : 0,\n      \"method\" : "
                "\"VR.AddCommand\"\n   }\n}\n"),
            result);
}

TEST(FormatterJsonRPCTest, ErrorResponse_ToString_Success) {
  // Create SmartObject
  SmartObject obj;
  obj[S_PARAMS][S_FUNCTION_ID] = hmi_apis::FunctionID::VR_AddCommand;
  obj[S_PARAMS][S_MESSAGE_TYPE] = hmi_apis::messageType::error_response;
  obj[S_PARAMS][S_CORRELATION_ID] = 4440;
  obj[S_PARAMS][kCode] = hmi_apis::Common_Result::GENERIC_ERROR;
  obj[S_PARAMS][kMessage] = "Some error";
  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  // Attach Schema
  hmi_apis::HMI_API factory;
  EXPECT_TRUE(factory.attachSchema(obj, false));

  std::string result;
  // Convert SmartObject to Json string
  EXPECT_TRUE(FormatterJsonRpc::ToString(obj, result));
  EXPECT_EQ(std::string(
                "{\n   \"error\" : {\n      \"code\" : 22,\n      \"data\" : "
                "{\n         \"method\" : \"VR.AddCommand\"\n      },\n      "
                "\"message\" : \"Some error\"\n   },\n   \"id\" : 4440,\n   "
                "\"jsonrpc\" : \"2.0\"\n}\n"),
            result);
}

TEST(FormatterJsonRPCTest, ResponseWithoutCode_ToString_Success) {
  // Create SmartObject
  SmartObject obj;
  obj[S_PARAMS][S_FUNCTION_ID] = hmi_apis::FunctionID::VR_AddCommand;
  obj[S_PARAMS][S_MESSAGE_TYPE] = hmi_apis::messageType::response;
  obj[S_PARAMS][S_PROTOCOL_VERSION] = 2;
  obj[S_PARAMS][S_PROTOCOL_TYPE] = 1;
  obj[S_PARAMS][S_CORRELATION_ID] = 4444;
  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  // Attach Schema
  hmi_apis::HMI_API factory;
  EXPECT_TRUE(factory.attachSchema(obj, false));

  std::string result;
  // Convert SmartObject to Json string will finish wrong
  EXPECT_FALSE(FormatterJsonRpc::ToString(obj, result));
}

TEST(FormatterJsonRPCTest, RequestWithoutMSGParams_ToString_Success) {
  // Create SmartObject
  SmartObject obj;
  obj[S_PARAMS][S_FUNCTION_ID] = mobile_apis::FunctionID::AddCommandID;
  obj[S_PARAMS][S_MESSAGE_TYPE] = mobile_apis::messageType::request;
  obj[S_PARAMS][S_CORRELATION_ID] = 4444;
  // Attach Schema
  mobile_apis::MOBILE_API factory;
  EXPECT_TRUE(factory.attachSchema(obj, false));

  std::string result;
  // Convert SmartObject to Json string
  EXPECT_TRUE(FormatterJsonRpc::ToString(obj, result));
  EXPECT_EQ(std::string(
                "{\n   \"id\" : 4444,\n   \"jsonrpc\" : \"2.0\",\n   "
                "\"method\" : \"AddCommandID\"\n}\n"),
            result);
}

TEST(FormatterJsonRPCTest, RequestWithoutCorID_ToString_Fail) {
  // Create SmartObject
  SmartObject obj;
  obj[S_PARAMS][S_FUNCTION_ID] = mobile_apis::FunctionID::AddCommandID;
  obj[S_PARAMS][S_MESSAGE_TYPE] = mobile_apis::messageType::request;
  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  // Attach Schema
  mobile_apis::MOBILE_API factory;
  EXPECT_TRUE(factory.attachSchema(obj, false));

  std::string result;
  // Converting SmartObject to Json string is failed
  EXPECT_FALSE(FormatterJsonRpc::ToString(obj, result));
  EXPECT_EQ(std::string(
                "{\n   \"jsonrpc\" : \"2.0\",\n   \"method\" : "
                "\"AddCommandID\"\n}\n"),
            result);
}

TEST(FormatterJsonRPCTest, RequestWithoutType_ToString_Fail) {
  // Create SmartObject
  SmartObject obj;
  obj[S_PARAMS][S_FUNCTION_ID] = mobile_apis::FunctionID::AddCommandID;
  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  // Attach Schema is impossible
  mobile_apis::MOBILE_API factory;
  EXPECT_FALSE(factory.attachSchema(obj, false));

  std::string result;
  // Converting SmartObject to Json string is failed
  EXPECT_FALSE(FormatterJsonRpc::ToString(obj, result));
  EXPECT_EQ(std::string("{\n   \"jsonrpc\" : \"2.0\"\n}\n"), result);
}

TEST(FormatterJsonRPCTest, InvalidRPC_ToString_False) {
  // Create SmartObject with notification id and response message type
  SmartObject obj;
  std::string result;
  obj[S_PARAMS][S_FUNCTION_ID] =
      hmi_apis::FunctionID::BasicCommunication_OnReady;
  obj[S_PARAMS][S_MESSAGE_TYPE] = hmi_apis::messageType::response;
  obj[S_PARAMS][S_PROTOCOL_VERSION] = 2;
  obj[S_PARAMS][S_PROTOCOL_TYPE] = 1;
  obj[S_PARAMS][S_CORRELATION_ID] = 4222;
  obj[S_MSG_PARAMS] = SmartObject(SmartType::SmartType_Map);
  // Attach Schema
  hmi_apis::HMI_API factory;
  EXPECT_FALSE(factory.attachSchema(obj, false));
  // Convert SmartObject to Json string
  EXPECT_FALSE(FormatterJsonRpc::ToString(obj, result));
  // Expect result with default value. No correct conversion was done
  EXPECT_EQ(std::string("{\n   \"jsonrpc\" : \"2.0\"\n}\n"), result);
}

TEST(FormatterJsonRPCTest, Notification_ToSmartObject_Success) {
  // Source Json string
  const std::string json_string(
      "{\n   \"jsonrpc\" : \"2.0\",\n   \"method\" : "
      "\"BasicCommunication.OnReady\",\n   \"params\" : {}\n}\n");
  // Smart Object to keep result
  SmartObject obj;
  // Convert json string to smart object
  int32_t result = FormatterJsonRpc::FromString<hmi_apis::FunctionID::eType,
                                                hmi_apis::messageType::eType>(
      json_string, obj);
  EXPECT_EQ(0, result);
  // Get keys collection from Smart Object
  std::set<std::string> keys = obj["params"].enumerate();
  EXPECT_EQ(4u, keys.size());
}

TEST(FormatterJsonRPCTest, InvalidFormatToSmartObject_False) {
  // Source Json string
  const std::string json_string(
      "{\n   \"method\" : \"BasicCommunication.OnReady\",\n   \"params\" : "
      "{}\n}\n");
  // Smart Object to keep result
  SmartObject obj;
  // Convert json string to smart object
  int32_t result = FormatterJsonRpc::FromString<hmi_apis::FunctionID::eType,
                                                hmi_apis::messageType::eType>(
      json_string, obj);
  EXPECT_EQ(2, result);
  // Get keys collection from Smart Object
  std::set<std::string> keys = obj["params"].enumerate();
  EXPECT_EQ(4u, keys.size());
}

TEST(FormatterJsonRPCTest, RequestToSmartObject_Success) {
  // Source Json string
  const std::string json_string(
      "{\n   \"id\" : 4444,\n   \"jsonrpc\" : \"2.0\",\n   \"method\" : "
      "\"VR.IsReady\"\n}\n");
  // Smart Object to keep result
  SmartObject obj;
  // Convert json string to smart object
  int32_t result = FormatterJsonRpc::FromString<hmi_apis::FunctionID::eType,
                                                hmi_apis::messageType::eType>(
      json_string, obj);
  EXPECT_EQ(0, result);
  // Get keys collection from Smart Object
  std::set<std::string> keys = obj["params"].enumerate();
  EXPECT_EQ(5u, keys.size());
}

TEST(FormatterJsonRPCTest, ResponseToSmartObject_Success) {
  // Source Json string
  const std::string json_string(
      "{\n   \"id\" : 4444,\n   \"jsonrpc\" : \"2.0\",\n   \"method\" : "
      "\"VR.IsReady\"\n}\n");
  // Smart Object to keep result
  SmartObject obj;
  // Convert json string to smart object
  int32_t result = FormatterJsonRpc::FromString<hmi_apis::FunctionID::eType,
                                                hmi_apis::messageType::eType>(
      json_string, obj);
  EXPECT_EQ(0, result);
  // Get keys collection from Smart Object
  std::set<std::string> keys = obj["params"].enumerate();
  EXPECT_EQ(5u, keys.size());
}

}  // namespace formatters_test
}  // namespace components
}  // namespace test
