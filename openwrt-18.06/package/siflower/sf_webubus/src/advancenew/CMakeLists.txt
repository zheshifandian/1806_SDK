cmake_minimum_required(VERSION 2.6)

PROJECT(advance C)

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

ADD_LIBRARY(advance MODULE advance.c)
TARGET_LINK_LIBRARIES(advance ubox ubus ${crypt})
SET_TARGET_PROPERTIES(advance PROPERTIES OUTPUT_NAME advance PREFIX "")

INSTALL(TARGETS advance LIBRARY DESTINATION lib)
