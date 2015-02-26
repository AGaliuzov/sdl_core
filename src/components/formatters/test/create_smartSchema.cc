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

#include "create_smartSchema.h"
namespace test {
namespace components {
namespace formatters {

using namespace NsSmartDeviceLink::NsJSONHandler::strings;
using namespace NsSmartDeviceLink::NsJSONHandler::Formatters;
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
};


} // namespace formatters
} // namespace components
} // namespace test
