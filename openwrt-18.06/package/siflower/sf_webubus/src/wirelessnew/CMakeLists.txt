cmake_minimum_required(VERSION 2.6)

PROJECT(wireless C)

ADD_DEFINITIONS(-Os -Wall -Werror --std=gnu99 -g3 -Wmissing-declarations -Iinclude)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

IF(APPLE)
  INCLUDE_DIRECTORIES(/opt/local/include)
  LINK_DIRECTORIES(/opt/local/lib)
ENDIF()

FIND_LIBRARY(crypt NAMES crypt)
IF(crypt STREQUAL "LIBS-NOTFOUND")
  SET(crypt "")
ENDIF()

ADD_LIBRARY(wireless MODULE wireless.c)
TARGET_LINK_LIBRARIES(wireless ubox ubus ${crypt})
SET_TARGET_PROPERTIES(wireless PROPERTIES OUTPUT_NAME wireless PREFIX "")

INSTALL(TARGETS wireless LIBRARY DESTINATION lib)
