# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
#
# This code is subject to the terms of the BSD+Patent license.
# See LICENSE file for more details.

# Set agent data model path
set(DATAMODELS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/odl)

message(STATUS "Test agent data model:")

# Parse agent.odl file
execute_process(COMMAND amxo-cg ${DATAMODELS_PATH}/agent.odl RESULT_VARIABLE DATAMODEL_PARSE_RESULT)

# If parser find errors - generate FATAL_ERROR
if(${DATAMODEL_PARSE_RESULT})
    message(FATAL_ERROR "Failed to parse agent data model!")
endif()

install(DIRECTORY ${DATAMODELS_PATH} DESTINATION config)
file(COPY ${DATAMODELS_PATH} DESTINATION "${CMAKE_MULTIAP_OUTPUT_DIRECTORY}/config/odl")

message(STATUS "Agent data model tested successfully!")
