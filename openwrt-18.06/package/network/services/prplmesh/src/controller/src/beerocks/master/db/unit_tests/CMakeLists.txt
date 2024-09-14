# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
#
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

if (BUILD_TESTS)
    project(db_unit_tests VERSION ${prplmesh_VERSION})

    set(unit_tests_sources
        ${db_unit_tests}
        ${CMAKE_CURRENT_LIST_DIR}/db_test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../db.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../node.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../station.cpp
    )

    add_executable(${PROJECT_NAME}
        ${unit_tests_sources}
    )
    if (COVERAGE)
        set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "--coverage -fPIC -O0")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "--coverage")
    endif()
    target_link_libraries(${PROJECT_NAME} mapfcommon bpl btlvf nbapi gtest_main gmock)
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${PLATFORM_INCLUDE_DIR}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

    install(TARGETS ${PROJECT_NAME} DESTINATION tests)
    add_test(NAME ${PROJECT_NAME} COMMAND $<TARGET_FILE:${PROJECT_NAME}>)
endif()
