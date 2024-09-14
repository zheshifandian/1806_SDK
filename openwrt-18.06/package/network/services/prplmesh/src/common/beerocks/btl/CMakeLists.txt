project(btl VERSION ${prplmesh_VERSION})
message("${BoldWhite}Preparing ${BoldGreen}${PROJECT_NAME}${BoldWhite} for the ${BoldGreen}${TARGET_PLATFORM}${BoldWhite} platform${ColourReset}")

# Set the base path for the current module
set(MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# BTL source files
set(btl_sources
    ${MODULE_PATH}/source/broker_client_factory_factory.cpp
    ${MODULE_PATH}/source/broker_client_factory_impl.cpp
    ${MODULE_PATH}/source/broker_client_impl.cpp
    ${MODULE_PATH}/source/message_parser_stream_impl.cpp
    ${MODULE_PATH}/source/message_serializer_stream_impl.cpp
)

# Build the library
add_library(${PROJECT_NAME} ${btl_sources})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${prplmesh_VERSION} SOVERSION ${prplmesh_VERSION_MAJOR})
target_link_libraries(${PROJECT_NAME} PUBLIC bcl tlvf elpp btlvf mapfcommon ieee1905_transport_messages)

# Include paths
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Install
install(
    TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
