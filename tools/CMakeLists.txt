# Copyright (c) 2015, Ford Motor Company
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

cmake_minimum_required(VERSION 2.8.3)

include(ExternalProject)

set(intergen_SOURCES "${CMAKE_CURRENT_LIST_DIR}/intergen")
set(intergen_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/intergen")
set(intergen_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/intergen")
set(intergen_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${intergen_INSTALL_DIR})

ExternalProject_Add(intergen
                    SOURCE_DIR ${intergen_SOURCES}
                    PREFIX ${intergen_PREFIX}
                    INSTALL_DIR ${intergen_INSTALL_DIR}
                    CMAKE_ARGS ${intergen_CMAKE_ARGS})

# This will force intergen make call every time core is built
# When intergen source didn't change no files are updated
# It is required to automatically rebuild interfaces if intergen was changed
ExternalProject_Add_Step(intergen forcebuild
                         COMMAND ${CMAKE_COMMAND} -E echo "Force intergen build"
                         DEPENDEES update DEPENDERS build
                         ALWAYS 1)
if(BUILD_TESTS)
  add_subdirectory(intergen/test)
endif()  
add_subdirectory(policy_table_validator)