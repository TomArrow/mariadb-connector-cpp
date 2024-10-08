# ************************************************************************************
#   Copyright (C) 2021 MariaDB Corporation AB
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Library General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Library General Public License for more details.
#
#   You should have received a copy of the GNU Library General Public
#   License along with this library; if not see <http://www.gnu.org/licenses>
#   or write to the Free Software Foundation, Inc.,
#   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
# *************************************************************************************/

cmake_minimum_required(VERSION 3.1)

PROJECT(mariadbcpp-example
        LANGUAGES CXX)
IF(WIN32)
  MESSAGE(SEND_ERROR "Not supported on Windows")
ELSE()
  SET(CMAKE_CXX_STANDARD 11)
  SET(CMAKE_CXX_STANDARD_REQUIRED ON)

  SET(LIBRARY_NAME "libmariadbcpp.so")

  EXECUTE_PROCESS(COMMAND tar zxf Z:/ProgrOld/eternaljk2mv/jk2mv/build/msvc16_x64/mariadb-connector-cpp-1.0.4-win32.tar.gz)
  ADD_CUSTOM_TARGET(LIST_CONTENTS
      COMMAND ls -lR ${CMAKE_BINARY_DIR}/mariadb-connector-cpp-1.0.4-win32//mariadb/*.so* ${CMAKE_BINARY_DIR}/mariadb-connector-cpp-1.0.4-win32/)

  FILE(GLOB LIBS_IN_PACK
       ${CMAKE_BINARY_DIR}/mariadb-connector-cpp-1.0.4-win32//mariadb/lib*.so*)
  MESSAGE(STATUS "Configuring to search for headers in the ${CMAKE_BINARY_DIR}/mariadb-connector-cpp-1.0.4-win32/include")
  INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/mariadb-connector-cpp-1.0.4-win32/include")
  ADD_EXECUTABLE(example example.cpp)

  MESSAGE(STATUS "Configuring to link against libs in the package ${LIBS_IN_PACK}")
  FOREACH(PACKEDLIB ${LIBS_IN_PACK})
    TARGET_LINK_LIBRARIES(example ${PACKEDLIB})
  ENDFOREACH()
  ADD_DEPENDENCIES(example LIST_CONTENTS)
ENDIF()
