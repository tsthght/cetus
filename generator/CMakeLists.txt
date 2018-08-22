#  $%BEGINLICENSE%$
#  Copyright (c) 2007, 2012, Oracle and/or its affiliates. All rights reserved.
# 
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; version 2 of the
#  License.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
#  02110-1301  USA
# 
#  $%ENDLICENSE%$
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckLibraryExists)
INCLUDE(FindPkgConfig)
INCLUDE(CheckTypeSize)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

PROJECT(generator)

IF(NOT GLIB_INCLUDE_DIRS) 
    SET(__pkg_config_checked_GLIB 0)
    PKG_CHECK_MODULES(GLIB REQUIRED glib-2.0>=2.28)
    ADD_DEFINITIONS(-DHAVE_GLIB)
ENDIF(NOT GLIB_INCLUDE_DIRS)

MESSAGE(STATUS "GLIB_INCLUDE_DIRS="${GLIB_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(
include
${GLIB_INCLUDE_DIRS}
)

AUX_SOURCE_DIRECTORY(src DIR_SRCS)
SET(READER_SOURCE
${DIR_SRCS}
)

ADD_EXECUTABLE(${PROJECT_NAME} ${READER_SOURCE})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
${GLIB_LIBRARIES}
)
