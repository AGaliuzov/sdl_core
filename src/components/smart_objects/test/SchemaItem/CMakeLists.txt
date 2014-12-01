# Copyright (c) 2014, Ford Motor Company
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following
# disclaimer in the documentation and/or other materials provided with the
# distribution.
#
# Neither the name of the Ford Motor Company nor the names of its contributors
# may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

if(BUILD_TESTS)

include_directories (
  ${COMPONENTS_DIR}/utils/include
  ${GMOCK_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/smart_objects/include
  ${COMPONENTS_DIR}/formatters/include/
  ${JSONCPP_INCLUDE_DIRECTORY}
)

# Libraries are the same for all SchemaItem tests
set (LIBRARIES
    gtest
    gtest_main
    gmock
    SmartObjects
    formatters
)

set (SOURCES
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/test_BoolSchemaItemTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/test_NumberSchemaItemTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/test_StringSchemaItemTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/test_ArraySchemaItemTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/test_CObjectSchemaItem.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/AlwaysTrueSchemaItemTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/AlwaysFalseSchemaItemTest.cc
)

add_library(test_smart_object_schema ${SOURCES})
create_test(test_smart_object_schema_test "${SOURCES}" "${LIBRARIES}")
endif()