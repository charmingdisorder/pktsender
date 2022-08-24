cmake_minimum_required(VERSION 3.0)
project(pktsender)

OPTION(DEFINE_DEBUG
  "Build the project using debugging code"
  OFF)
IF(DEFINE_DEBUG)
  MESSAGE("Adding Debug flag...")
  ADD_DEFINITIONS(-DDEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
  MESSAGE("Build type is " ${CMAKE_BUILD_TYPE})
ENDIF(DEFINE_DEBUG)

add_compile_options(-Wall -Wextra -pedantic -g)

set(COMMON_FILES atomic_io.h md5.c)

set(SENDER_SOURCE_FILES ${COMMON_FILES}  pkt_sender.c)
set(RECEIVER_SOURCE_FILES ${COMMON_FILES} pkt_receiver.c)

add_executable(pkt_sender ${SENDER_SOURCE_FILES})
add_executable(pkt_receiver ${RECEIVER_SOURCE_FILES})
