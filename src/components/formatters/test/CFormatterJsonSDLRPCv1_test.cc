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

#include "formatters/CFormatterJsonSDLRPCv1.hpp"
#include "smart_objects/bool_schema_item.h"
#include "smart_objects/object_schema_item.h"
#include "smart_objects/string_schema_item.h"
#include "smart_objects/number_schema_item.h"

namespace test {
namespace components {
namespace formatters {

using namespace NsSmartDeviceLink::NsJSONHandler::strings;
using namespace NsSmartDeviceLink::NsJSONHandler::Formatters;
using namespace NsSmartDeviceLink::NsSmartObjects;

namespace TestType {
enum eType {
  INVALID_ENUM = -1,
  APPLICATION_NOT_REGISTERED = 0,
  SUCCESS,
  TOO_MANY_PENDING_REQUESTS,
  REJECTED,
  INVALID_DATA,
  OUT_OF_MEMORY,
  ABORTED,
  USER_DISALLOWED,
  GENERIC_ERROR,
  DISALLOWED
};
}

namespace FunctionIdTest {
enum eType {
  INVALID_ENUM = -1,
  RegisterAppInterface,
  UnregisterAppInterface,
  SetGlobalProperties,
};
}

namespace MessageTypeTest {
enum eType {
  INVALID_ENUM = -1,
  request,
  response,
  notification
};
}

using namespace NsSmartDeviceLink::NsSmartObjects;

template<>
const EnumConversionHelper<TestType::eType>::EnumToCStringMap EnumConversionHelper<
    test::components::formatters::TestType::eType>::enum_to_cstring_map_ =
    EnumConversionHelper<test::components::formatters::TestType::eType>::InitEnumToCStringMap();

template<>
const EnumConversionHelper<TestType::eType>::CStringToEnumMap EnumConversionHelper<
    test::components::formatters::TestType::eType>::cstring_to_enum_map_ =
    EnumConversionHelper<test::components::formatters::TestType::eType>::InitCStringToEnumMap();

template<>
const char* const EnumConversionHelper<TestType::eType>::cstring_values_[] = {
    "APPLICATION_NOT_REGISTERED", "SUCCESS", "TOO_MANY_PENDING_REQUESTS",
    "REJECTED", "INVALID_DATA", "OUT_OF_MEMORY", "ABORTED", "USER_DISALLOWED",
    "GENERIC_ERROR", "DISALLOWED" };

template<>
const TestType::eType EnumConversionHelper<TestType::eType>::enum_values_[] = {
    test::components::formatters::TestType::APPLICATION_NOT_REGISTERED,
    test::components::formatters::TestType::SUCCESS,
    test::components::formatters::TestType::TOO_MANY_PENDING_REQUESTS,
    test::components::formatters::TestType::REJECTED,
    test::components::formatters::TestType::INVALID_DATA,
    test::components::formatters::TestType::OUT_OF_MEMORY,
    test::components::formatters::TestType::ABORTED,
    test::components::formatters::TestType::USER_DISALLOWED,
    test::components::formatters::TestType::GENERIC_ERROR,
    test::components::formatters::TestType::DISALLOWED };

template<>
const EnumConversionHelper<FunctionIdTest::eType>::EnumToCStringMap EnumConversionHelper<
    test::components::formatters::FunctionIdTest::eType>::enum_to_cstring_map_ =
    EnumConversionHelper<test::components::formatters::FunctionIdTest::eType>::InitEnumToCStringMap();

template<>
const EnumConversionHelper<FunctionIdTest::eType>::CStringToEnumMap EnumConversionHelper<
    test::components::formatters::FunctionIdTest::eType>::cstring_to_enum_map_ =
    EnumConversionHelper<test::components::formatters::FunctionIdTest::eType>::InitCStringToEnumMap();

template<>
const char* const EnumConversionHelper<FunctionIdTest::eType>::cstring_values_[] =
    { "RegisterAppInterface", "UnregisterAppInterface", "SetGlobalProperties" };

template<>
const FunctionIdTest::eType EnumConversionHelper<FunctionIdTest::eType>::enum_values_[] =
    { test::components::formatters::FunctionIdTest::RegisterAppInterface,
        test::components::formatters::FunctionIdTest::UnregisterAppInterface,
        test::components::formatters::FunctionIdTest::SetGlobalProperties };

template<>
const EnumConversionHelper<MessageTypeTest::eType>::EnumToCStringMap EnumConversionHelper<
    test::components::formatters::MessageTypeTest::eType>::enum_to_cstring_map_ =
    EnumConversionHelper<test::components::formatters::MessageTypeTest::eType>::InitEnumToCStringMap();

template<>
const EnumConversionHelper<MessageTypeTest::eType>::CStringToEnumMap EnumConversionHelper<
    test::components::formatters::MessageTypeTest::eType>::cstring_to_enum_map_ =
    EnumConversionHelper<test::components::formatters::MessageTypeTest::eType>::InitCStringToEnumMap();

template<>
const char* const EnumConversionHelper<MessageTypeTest::eType>::cstring_values_[] =
    { "request", "response", "notification" };

template<>
const MessageTypeTest::eType EnumConversionHelper<MessageTypeTest::eType>::enum_values_[] =
    { test::components::formatters::MessageTypeTest::request,
        test::components::formatters::MessageTypeTest::response,
        test::components::formatters::MessageTypeTest::notification };

CSmartSchema initObjectSchema() {
  std::set<TestType::eType> resultCode_allowedEnumSubsetValues;
  resultCode_allowedEnumSubsetValues.insert(
      TestType::APPLICATION_NOT_REGISTERED);
  resultCode_allowedEnumSubsetValues.insert(TestType::SUCCESS);
  resultCode_allowedEnumSubsetValues.insert(
      TestType::TOO_MANY_PENDING_REQUESTS);
  resultCode_allowedEnumSubsetValues.insert(TestType::REJECTED);
  resultCode_allowedEnumSubsetValues.insert(TestType::INVALID_DATA);
  resultCode_allowedEnumSubsetValues.insert(TestType::OUT_OF_MEMORY);
  resultCode_allowedEnumSubsetValues.insert(TestType::ABORTED);
  resultCode_allowedEnumSubsetValues.insert(TestType::USER_DISALLOWED);
  resultCode_allowedEnumSubsetValues.insert(TestType::GENERIC_ERROR);
  resultCode_allowedEnumSubsetValues.insert(TestType::DISALLOWED);

  // Possible functions in this test scheme
  std::set<FunctionIdTest::eType> functionId_allowedEnumSubsetValues;
  functionId_allowedEnumSubsetValues.insert(
      FunctionIdTest::RegisterAppInterface);
  functionId_allowedEnumSubsetValues.insert(
      FunctionIdTest::UnregisterAppInterface);
  functionId_allowedEnumSubsetValues.insert(
      FunctionIdTest::SetGlobalProperties);

  // Possible message types
  std::set<MessageTypeTest::eType> messageType_allowedEnumSubsetValues;
  messageType_allowedEnumSubsetValues.insert(MessageTypeTest::request);
  messageType_allowedEnumSubsetValues.insert(MessageTypeTest::response);
  messageType_allowedEnumSubsetValues.insert(MessageTypeTest::notification);

  // Create result item
  ISchemaItemPtr success_SchemaItem = CBoolSchemaItem::create(
      TSchemaItemParameter<bool>());
  ISchemaItemPtr resultCode_SchemaItem =
      TEnumSchemaItem<TestType::eType>::create(
          resultCode_allowedEnumSubsetValues,
          TSchemaItemParameter<TestType::eType>());

  // Create info value with min 0 length and max 1000
  ISchemaItemPtr info_SchemaItem = CStringSchemaItem::create(
      TSchemaItemParameter<size_t>(0), TSchemaItemParameter<size_t>(1000),
      TSchemaItemParameter<std::string>());

  ISchemaItemPtr tryAgainTime_SchemaItem = TNumberSchemaItem<int>::create(
      TSchemaItemParameter<int>(0), TSchemaItemParameter<int>(2000000000),
      TSchemaItemParameter<int>());

  // Map of parameters
  std::map<std::string, CObjectSchemaItem::SMember> schemaMembersMap;

  schemaMembersMap["success"] = CObjectSchemaItem::SMember(success_SchemaItem,
                                                           false);
  schemaMembersMap["resultCode"] = CObjectSchemaItem::SMember(
      resultCode_SchemaItem, false);
  schemaMembersMap["info"] = CObjectSchemaItem::SMember(info_SchemaItem, false);
  schemaMembersMap["tryAgainTime"] = CObjectSchemaItem::SMember(
      tryAgainTime_SchemaItem, false);

  std::map<std::string, CObjectSchemaItem::SMember> paramsMembersMap;
  paramsMembersMap[S_FUNCTION_ID] = CObjectSchemaItem::SMember(
      TEnumSchemaItem<FunctionIdTest::eType>::create(
          functionId_allowedEnumSubsetValues),
      true);
  paramsMembersMap[S_MESSAGE_TYPE] = CObjectSchemaItem::SMember(
      TEnumSchemaItem<MessageTypeTest::eType>::create(
          messageType_allowedEnumSubsetValues),
      true);
  paramsMembersMap[S_CORRELATION_ID] = CObjectSchemaItem::SMember(
      TNumberSchemaItem<int>::create(), true);
  paramsMembersMap[S_PROTOCOL_VERSION] = CObjectSchemaItem::SMember(
      TNumberSchemaItem<int>::create(TSchemaItemParameter<int>(1),
                                     TSchemaItemParameter<int>(2)),
      true);
  paramsMembersMap[S_PROTOCOL_TYPE] = CObjectSchemaItem::SMember(
      TNumberSchemaItem<int>::create(), true);

  std::map<std::string, CObjectSchemaItem::SMember> rootMembersMap;
  rootMembersMap[S_MSG_PARAMS] = CObjectSchemaItem::SMember(
      CObjectSchemaItem::create(schemaMembersMap), true);
  rootMembersMap[S_PARAMS] = CObjectSchemaItem::SMember(
      CObjectSchemaItem::create(paramsMembersMap), true);
  return CSmartSchema(CObjectSchemaItem::create(rootMembersMap));
}
;

TEST(CFormatterJsonSDLRPCv1Test, EmptySmartObjectToString) {
  SmartObject srcObj;

  EXPECT_EQ(Errors::eType::OK, srcObj.validate());

  std::string jsonString;
  bool result = CFormatterJsonSDLRPCv1::toString(srcObj, jsonString);

  EXPECT_TRUE(result);

  std::string expectOutputJsonString =
      "{\n \
  \"\" : {\n\
      \"name\" : \"\",\n\
      \"parameters\" : \"\"\n\
   }\n\
}\n";

  EXPECT_EQ(expectOutputJsonString, jsonString);
}

TEST(CFormatterJsonSDLRPCv1Test, SmObjWithRequestWithoutMsgNotValid_ToString) {
  SmartObject srcObj;
  CSmartSchema schema = initObjectSchema();
  srcObj.setSchema(schema);

  srcObj[S_PARAMS][S_MESSAGE_TYPE] = MessageTypeTest::request;
  srcObj[S_PARAMS][S_FUNCTION_ID] = FunctionIdTest::RegisterAppInterface;
  srcObj[S_PARAMS][S_CORRELATION_ID] = 13;
  srcObj[S_PARAMS][S_PROTOCOL_TYPE] = 0;
  srcObj[S_PARAMS][S_PROTOCOL_VERSION] = 1;

  EXPECT_EQ(Errors::eType::MISSING_MANDATORY_PARAMETER, srcObj.validate());

  std::string jsonString;
  bool result = CFormatterJsonSDLRPCv1::toString(srcObj, jsonString);
  EXPECT_TRUE(result);

  std::string expectOutputJsonString =
      "{\n \
  \"request\" : {\n\
      \"correlationID\" : 13,\n\
      \"name\" : \"RegisterAppInterface\",\n\
      \"parameters\" : \"\"\n\
   }\n\
}\n";

  EXPECT_EQ(expectOutputJsonString, jsonString);
}

TEST(CFormatterJsonSDLRPCv1Test, SmObjWithRequestWithEmptyMsgWithTestSchemaToString) {
  SmartObject srcObj;
  CSmartSchema schema = initObjectSchema();
  srcObj.setSchema(schema);

  srcObj[S_PARAMS][S_MESSAGE_TYPE] = MessageTypeTest::request;
  srcObj[S_PARAMS][S_FUNCTION_ID] = FunctionIdTest::RegisterAppInterface;
  srcObj[S_PARAMS][S_CORRELATION_ID] = 13;
  srcObj[S_PARAMS][S_PROTOCOL_TYPE] = 0;
  srcObj[S_PARAMS][S_PROTOCOL_VERSION] = 1;
  srcObj[S_MSG_PARAMS][""] = "";

  EXPECT_EQ(Errors::eType::OK, srcObj.validate());

  std::string jsonString;

  bool result = CFormatterJsonSDLRPCv1::toString(srcObj, jsonString);

  EXPECT_TRUE(result);

  std::string expectOutputJsonString =
      "{\n \
  \"request\" : {\n\
      \"correlationID\" : 13,\n\
      \"name\" : \"RegisterAppInterface\",\n\
      \"parameters\" : {}\n\
   }\n\
}\n";

  EXPECT_EQ(expectOutputJsonString, jsonString);
}

TEST(CFormatterJsonSDLRPCv1Test, SmObjWithRequestWithNonemptyMsgWithTestSchemaToString) {
  SmartObject srcObj;
  CSmartSchema schema = initObjectSchema();
  srcObj.setSchema(schema);

  srcObj[S_PARAMS][S_MESSAGE_TYPE] = MessageTypeTest::request;
  srcObj[S_PARAMS][S_FUNCTION_ID] = FunctionIdTest::RegisterAppInterface;
  srcObj[S_PARAMS][S_CORRELATION_ID] = 13;
  srcObj[S_PARAMS][S_PROTOCOL_TYPE] = 0;
  srcObj[S_PARAMS][S_PROTOCOL_VERSION] = 1;
  srcObj[S_MSG_PARAMS]["info"] = "value";

  std::string jsonString;

  bool result = CFormatterJsonSDLRPCv1::toString(srcObj, jsonString);

  EXPECT_TRUE(result);

  std::string expectOutputJsonString =
      "{\n \
  \"request\" : {\n\
      \"correlationID\" : 13,\n\
      \"name\" : \"RegisterAppInterface\",\n\
      \"parameters\" : {\n\
         \"info\" : \"value\"\n\
      }\n\
   }\n\
}\n";

  EXPECT_EQ(expectOutputJsonString, jsonString);
}

TEST(CFormatterJsonSDLRPCv1Test, SmObjWithRequestWithNonemptyMsgToString) {
  SmartObject srcObj;

  srcObj[S_PARAMS][S_MESSAGE_TYPE] = MessageTypeTest::request;
  srcObj[S_PARAMS][S_FUNCTION_ID] = 5;
  srcObj[S_PARAMS][S_CORRELATION_ID] = 13;
  srcObj[S_PARAMS][S_PROTOCOL_TYPE] = 0;
  srcObj[S_PARAMS][S_PROTOCOL_VERSION] = 1;
  srcObj[S_MSG_PARAMS]["vrSynonyms"][0] = "Synonym 1";

  std::string jsonString;

  bool result = CFormatterJsonSDLRPCv1::toString(srcObj, jsonString);

  EXPECT_TRUE(result);

  std::string expectOutputJsonString =
      "{\n \
  \"0\" : {\n\
      \"correlationID\" : 13,\n\
      \"name\" : \"5\",\n\
      \"parameters\" : {\n\
         \"vrSynonyms\" : [ \"Synonym 1\" ]\n\
      }\n\
   }\n\
}\n";
  EXPECT_EQ(expectOutputJsonString, jsonString);
}

TEST(CFormatterJsonSDLRPCv1Test, SmObjWithResponseWithoutSchemaToString) {
  SmartObject srcObj;

  srcObj[S_PARAMS][S_MESSAGE_TYPE] = MessageTypeTest::response;
  srcObj[S_PARAMS][S_FUNCTION_ID] = 5;
  srcObj[S_PARAMS][S_CORRELATION_ID] = 13;
  srcObj[S_PARAMS][S_PROTOCOL_TYPE] = 0;
  srcObj[S_PARAMS][S_PROTOCOL_VERSION] = 1;
  srcObj[S_MSG_PARAMS]["success"] = true;
  srcObj[S_MSG_PARAMS]["resultCode"] = 0;

  std::string jsonString;

  bool result = CFormatterJsonSDLRPCv1::toString(srcObj, jsonString);

  EXPECT_TRUE(result);

  std::string expectOutputJsonString =
      "{\n \
  \"1\" : {\n\
      \"correlationID\" : 13,\n\
      \"name\" : \"5\",\n\
      \"parameters\" : {\n\
         \"resultCode\" : 0,\n\
         \"success\" : true\n\
      }\n\
   }\n\
}\n";
  EXPECT_EQ(expectOutputJsonString, jsonString);
}

TEST(CFormatterJsonSDLRPCv1Test, SmObjWithNotificationToString) {
  SmartObject srcObj;
  CSmartSchema schema = initObjectSchema();
  srcObj.setSchema(schema);

  srcObj[S_PARAMS][S_MESSAGE_TYPE] = MessageTypeTest::notification;
  srcObj[S_PARAMS][S_FUNCTION_ID] = FunctionIdTest::SetGlobalProperties;
  srcObj[S_PARAMS][S_CORRELATION_ID] = 13;
  srcObj[S_PARAMS][S_PROTOCOL_TYPE] = 0;
  srcObj[S_PARAMS][S_PROTOCOL_VERSION] = 1;
  srcObj[S_MSG_PARAMS][""] = "";
  std::string jsonString;

  bool result = CFormatterJsonSDLRPCv1::toString(srcObj, jsonString);

  EXPECT_TRUE(result);

  std::string expectOutputJsonString =
      "{\n \
  \"notification\" : {\n\
      \"correlationID\" : 13,\n\
      \"name\" : \"SetGlobalProperties\",\n\
      \"parameters\" : {}\n\
   }\n\
}\n";

  EXPECT_EQ(expectOutputJsonString, jsonString);
}

TEST(CFormatterJsonSDLRPCv1Test, SmObjWithResponseToString) {
  SmartObject srcObj;

  CSmartSchema schema = initObjectSchema();
  srcObj.setSchema(schema);

  srcObj[S_PARAMS][S_MESSAGE_TYPE] = MessageTypeTest::response;
  srcObj[S_PARAMS][S_FUNCTION_ID] = FunctionIdTest::RegisterAppInterface;
  srcObj[S_PARAMS][S_CORRELATION_ID] = 13;
  srcObj[S_PARAMS][S_PROTOCOL_TYPE] = 0;
  srcObj[S_PARAMS][S_PROTOCOL_VERSION] = 1;

  srcObj[S_MSG_PARAMS]["success"] = true;
  srcObj[S_MSG_PARAMS]["resultCode"] = TestType::SUCCESS;

  std::string jsonString;

  bool result = CFormatterJsonSDLRPCv1::toString(srcObj, jsonString);

  EXPECT_TRUE(result);

  std::string expectOutputJsonString =
      "{\n \
  \"response\" : {\n\
      \"correlationID\" : 13,\n\
      \"name\" : \"RegisterAppInterface\",\n\
      \"parameters\" : {\n\
         \"resultCode\" : \"SUCCESS\",\n\
         \"success\" : true\n\
      }\n\
   }\n\
}\n";

  EXPECT_EQ(expectOutputJsonString, jsonString);
}

TEST(CFormatterJsonSDLRPCv1Test, SmObjWithResponseWithoutSchemaWithoutParamsToString) {
  SmartObject srcObj;
  srcObj[S_PARAMS][S_MESSAGE_TYPE] = MessageTypeTest::response;
  std::string jsonString;

  bool result = CFormatterJsonSDLRPCv1::toString(srcObj, jsonString);

  EXPECT_TRUE(result);

  std::string expectOutputJsonString =
      "{\n \
  \"1\" : {\n\
      \"name\" : \"\",\n\
      \"parameters\" : \"\"\n\
   }\n\
}\n";

  EXPECT_EQ(expectOutputJsonString, jsonString);
}

TEST(CFormatterJsonSDLRPCv1Test, StringRequestToSmObj) {
  std::string inputJsonString =
      "\
          {\
              \"request\": {\
                  \"correlationID\": 5,\
                  \"name\" : \"RegisterAppInterface\",\n\
                  \"parameters\": {\
                      \"syncMsgVersion\" : {\
                          \"majorVersion\" : 2,\
                          \"minorVersion\" : 10\
                      },\
                      \"appName\": \"some app name\",\
                      \"ttsName\": [{\
                          \"text\": \"ABC\",\
                          \"type\": \"TEXT\"\
                      }],\
                     \"vrSynonyms\": [\"Synonym 1\", \"Synonym 2\"]\
                  }\
              }\
          }";

  SmartObject obj;

  CSmartSchema schema = initObjectSchema();
  obj.setSchema(schema);

  bool result = CFormatterJsonSDLRPCv1::fromString<FunctionIdTest::eType,
      MessageTypeTest::eType>(inputJsonString, obj);

  EXPECT_EQ(CFormatterJsonSDLRPCv1::kSuccess, result);
  EXPECT_EQ(Errors::eType::OK, obj.validate());
  EXPECT_EQ(obj[S_PARAMS][S_MESSAGE_TYPE], MessageTypeTest::request);
  EXPECT_EQ(obj[S_PARAMS][S_FUNCTION_ID], FunctionIdTest::RegisterAppInterface);
  EXPECT_EQ(obj[S_PARAMS][S_CORRELATION_ID], 5);
  EXPECT_EQ(obj[S_PARAMS][S_PROTOCOL_TYPE], 0);
  EXPECT_EQ(obj[S_PARAMS][S_PROTOCOL_VERSION], 1);
  EXPECT_EQ(obj[S_MSG_PARAMS]["appName"], "some app name");

  EXPECT_EQ(obj[S_MSG_PARAMS]["syncMsgVersion"]["majorVersion"], 2);
  EXPECT_EQ(obj[S_MSG_PARAMS]["syncMsgVersion"]["minorVersion"], 10);
  EXPECT_EQ(obj[S_MSG_PARAMS]["ttsName"][0]["text"], "ABC");
  EXPECT_EQ(obj[S_MSG_PARAMS]["ttsName"][0]["type"], "TEXT");
  EXPECT_EQ(obj[S_MSG_PARAMS]["vrSynonyms"][0], "Synonym 1");
  EXPECT_EQ(obj[S_MSG_PARAMS]["vrSynonyms"][1], "Synonym 2");
}

TEST(CFormatterJsonSDLRPCv1Test, StringRequestWithoutNameToSmartObject) {
  std::string inputJsonString =
      "\
          {\
              \"request\": {\
                  \"correlationID\": 5,\
                  \"parameters\": {\
                      \"syncMsgVersion\" : {\
                          \"majorVersion\" : 2,\
                          \"minorVersion\" : 10\
                      },\
                      \"appName\": \"some app name\",\
                      \"ttsName\": [{\
                          \"text\": \"ABC\",\
                          \"type\": \"TEXT\"\
                      }],\
                     \"vrSynonyms\": [\"Synonym 1\", \"Synonym 2\"]\
                  }\
              }\
          }";

  SmartObject obj;

  bool result = CFormatterJsonSDLRPCv1::fromString<FunctionIdTest::eType,
      MessageTypeTest::eType>(inputJsonString, obj);

  EXPECT_EQ(CFormatterJsonSDLRPCv1::kParsingError, result);

  EXPECT_EQ(obj[S_PARAMS][S_MESSAGE_TYPE], MessageTypeTest::request);
  EXPECT_EQ(obj[S_PARAMS][S_FUNCTION_ID], "-1");
  EXPECT_EQ(obj[S_PARAMS][S_CORRELATION_ID], 5);
  EXPECT_EQ(obj[S_MSG_PARAMS]["appName"], "some app name");

  EXPECT_EQ(obj[S_MSG_PARAMS]["syncMsgVersion"]["majorVersion"], 2);
  EXPECT_EQ(obj[S_MSG_PARAMS]["syncMsgVersion"]["minorVersion"], 10);
  EXPECT_EQ(obj[S_MSG_PARAMS]["ttsName"][0]["text"], "ABC");
  EXPECT_EQ(obj[S_MSG_PARAMS]["ttsName"][0]["type"], "TEXT");
  EXPECT_EQ(obj[S_MSG_PARAMS]["vrSynonyms"][0], "Synonym 1");
  EXPECT_EQ(obj[S_MSG_PARAMS]["vrSynonyms"][1], "Synonym 2");
}

TEST(CFormatterJsonSDLRPCv1Test, StringRequestWithIncorrectCorIDToSmartObject) {
  std::string inputJsonString =
      "\
          {\
              \"request\": {\
                  \"correlationID\": \"5\",\
                  \"parameters\": {\
                      \"syncMsgVersion\" : {\
                          \"majorVersion\" : 2,\
                          \"minorVersion\" : 10\
                      },\
                      \"appName\": \"some app name\",\
                      \"ttsName\": [{\
                          \"text\": \"ABC\",\
                          \"type\": \"TEXT\"\
                      }],\
                     \"vrSynonyms\": [\"Synonym 1\", \"Synonym 2\"]\
                  }\
              }\
          }";

  SmartObject obj;

  bool result = CFormatterJsonSDLRPCv1::fromString<FunctionIdTest::eType,
      MessageTypeTest::eType>(inputJsonString, obj);
  EXPECT_EQ(CFormatterJsonSDLRPCv1::kParsingError, result);

  EXPECT_EQ(obj[S_PARAMS][S_MESSAGE_TYPE], MessageTypeTest::request);
  EXPECT_EQ(obj[S_PARAMS][S_FUNCTION_ID], "-1");
  EXPECT_EQ(obj[S_MSG_PARAMS]["appName"], "some app name");
  EXPECT_EQ(obj[S_MSG_PARAMS]["syncMsgVersion"]["majorVersion"], 2);
  EXPECT_EQ(obj[S_MSG_PARAMS]["ttsName"][0]["text"], "ABC");
  EXPECT_EQ(obj[S_MSG_PARAMS]["vrSynonyms"][0], "Synonym 1");
}

TEST(CFormatterJsonSDLRPCv1Test, StringResponceToSmartObject) {
  std::string inputJsonString =
      "{\n \
  \"response\" : {\n\
      \"correlationID\" : 13,\n\
      \"name\" : \"RegisterAppInterface\",\n\
      \"parameters\" : {\n\
         \"resultCode\" : \"SUCCESS\",\n\
         \"success\" : true\n\
      }\n\
   }\n\
}\n";

  SmartObject obj;

  CSmartSchema schema = initObjectSchema();
  obj.setSchema(schema);

  bool result = CFormatterJsonSDLRPCv1::fromString<FunctionIdTest::eType,
      MessageTypeTest::eType>(inputJsonString, obj);
  EXPECT_EQ(CFormatterJsonSDLRPCv1::kSuccess, result);
  EXPECT_EQ(obj[S_PARAMS][S_MESSAGE_TYPE], MessageTypeTest::response);
  EXPECT_EQ(obj[S_PARAMS][S_FUNCTION_ID], 0);
  EXPECT_EQ(obj[S_PARAMS][S_CORRELATION_ID], 13);
  EXPECT_EQ(obj[S_PARAMS][S_PROTOCOL_TYPE], 0);
  EXPECT_EQ(obj[S_PARAMS][S_PROTOCOL_VERSION], 1);
  EXPECT_EQ(obj[S_MSG_PARAMS]["resultCode"], "SUCCESS");
  EXPECT_EQ(obj[S_MSG_PARAMS]["success"], true);
}

TEST(CFormatterJsonSDLRPCv1Test, StringNotificationToSmartObject) {
  std::string inputJsonString =
      "{\n \
  \"notification\" : {\n\
      \"correlationID\" : 13,\n\
      \"name\" : \"SetGlobalProperties\",\n\
      \"parameters\" : {}\n\
   }\n\
}\n";

  SmartObject obj;

  CSmartSchema schema = initObjectSchema();
  obj.setSchema(schema);

  bool result = CFormatterJsonSDLRPCv1::fromString<FunctionIdTest::eType,
      MessageTypeTest::eType>(inputJsonString, obj);
  EXPECT_EQ(CFormatterJsonSDLRPCv1::kSuccess, result);
  EXPECT_EQ(Errors::eType::OK, obj.validate());
  EXPECT_EQ(obj[S_PARAMS][S_MESSAGE_TYPE], MessageTypeTest::notification);
  EXPECT_EQ(obj[S_PARAMS][S_FUNCTION_ID], FunctionIdTest::SetGlobalProperties);
  EXPECT_EQ(obj[S_PARAMS][S_CORRELATION_ID], 13);
  EXPECT_EQ(obj[S_PARAMS][S_PROTOCOL_TYPE], 0);
  EXPECT_EQ(obj[S_PARAMS][S_PROTOCOL_VERSION], 1);
}

TEST(CFormatterJsonSDLRPCv1Test, MetaFormatToString) {
  SmartObject srcObj;

  srcObj[S_PARAMS][S_MESSAGE_TYPE] = MessageTypeTest::request;
  srcObj[S_PARAMS][S_FUNCTION_ID] = FunctionIdTest::RegisterAppInterface;
  srcObj[S_PARAMS][S_CORRELATION_ID] = 13;
  srcObj[S_PARAMS][S_PROTOCOL_TYPE] = 0;
  srcObj[S_PARAMS][S_PROTOCOL_VERSION] = 1;
  srcObj[S_MSG_PARAMS]["info"] = "value";

  std::string jsonString;

  CSmartSchema schema = initObjectSchema();
  srcObj.setSchema(schema);

  meta_formatter_error_code::tMetaFormatterErrorCode result =
      CFormatterJsonSDLRPCv1::MetaFormatToString(srcObj, schema, jsonString);
  EXPECT_EQ(meta_formatter_error_code::kErrorOk, result);
}

} // namespace formatters
} // namespace components
} // namespace test
