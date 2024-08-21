#
# CMakeLists for CryCorder
#

# ####################################################################################################################################################
# CMake  functions
# ####################################################################################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

# ####################################################################################################################################################
# Project Definition
# ####################################################################################################################################################
SET(PROJECT_NAME "CryCorder")

SET(CRYCORDER_VERSION_MAJOR "0")
SET(CRYCORDER_VERSION_MINOR "1")
SET(CRYCORDER_VERSION_PATCH "0")
SET(CRYCORDER_VERSION "${CRYCORDER_VERSION_MAJOR}.${CRYCORDER_VERSION_MINOR}.${CRYCORDER_VERSION_PATCH}")
SET(CRYCORDER_VERSION_STRING "${CRYCORDER_VERSION}")

SET(CRYCORDER_VERSION_DATE "August 2024")

PROJECT(
  ${PROJECT_NAME}
  VERSION ${CRYCORDER_VERSION}
  LANGUAGES C CXX
)

# ####################################################################################################################################################
# Variables
# ####################################################################################################################################################
OPTION(USE_EXTRA_WARNINGS "Enable extra warnings" OFF)
OPTION(USE_WERROR "Warnings as errors" OFF)

# ####################################################################################################################################################
# External Libraries
# ####################################################################################################################################################
# Use pkg-config to find GStreamer
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GSTREAMER REQUIRED gstreamer-1.0)

# ####################################################################################################################################################
# Configure targets
# ####################################################################################################################################################
# Define C++ standard
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

# Define build type if not defined
IF(NOT CMAKE_BUILD_TYPE)
  MESSAGE(STATUS "No build type selected, default to Release")
  SET(CMAKE_BUILD_TYPE "Release")
ENDIF()

# Set common compile flags
SET(COMMON_COMPILE_FLAGS -funroll-loops -Wall -Wextra)

# Add extra warnings if enabled
IF(USE_EXTRA_WARNINGS)
  SET(EXTRA_WARNINGS
      -Wfloat-equal
      -Wundef
      -Wshadow
      -Wpointer-arith
      -Wcast-align
      -Wswitch-default
      -Wswitch-enum
      -Wconversion
      -Wunreachable-code
  )
ELSE()
  SET(EXTRA_WARNINGS "")
ENDIF()

# Set warnings as errors if enabled
IF(USE_WERROR)
  SET(WERROR_FLAGS -Werror)
ELSE()
  SET(WERROR_FLAGS "")
ENDIF()

# Set compile options
SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Add executable
ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)

# Apply include directories and link libraries using target commands
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${GSTREAMER_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${GSTREAMER_LIBRARIES})
TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PRIVATE ${COMMON_COMPILE_FLAGS} ${EXTRA_WARNINGS} ${WERROR_FLAGS})

# ####################################################################################################################################################
# Final information dump
# ####################################################################################################################################################
MESSAGE("")
MESSAGE(STATUS "${PROJECT_NAME} INFORMATION")
MESSAGE(STATUS "Version: " "${CRYCORDER_VERSION_STRING}")
MESSAGE(STATUS "Configuration:")
MESSAGE(STATUS "    C++ Standard: " "${CMAKE_CXX_STANDARD}")
MESSAGE(STATUS "    Build type: " "${CMAKE_BUILD_TYPE}")
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  MESSAGE(STATUS "    Build flags: " "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_COMPILE_FLAGS} ${EXTRA_WARNINGS} ${WERROR_FLAGS}")
ELSE()
  MESSAGE(STATUS "    Build flags: " "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_COMPILE_FLAGS} ${EXTRA_WARNINGS} ${WERROR_FLAGS}")
ENDIF()
MESSAGE("")
