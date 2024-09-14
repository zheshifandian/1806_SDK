project(beerocks_controller VERSION ${prplmesh_VERSION})
message("${BoldWhite}Preparing ${BoldGreen}${PROJECT_NAME}${BoldWhite} for the ${BoldGreen}${TARGET_PLATFORM}${BoldWhite} platform${ColourReset}")

# Set the base path for the current module
set(MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
set(ACTION_PATH ${MODULE_PATH}/../../../nbapi)

file(GLOB controller_sources ${MODULE_PATH}/*.cpp)
file(GLOB controller_tasks_sources ${MODULE_PATH}/tasks/*.cpp)
file(GLOB controller_operations_sources ${MODULE_PATH}/periodic/*.cpp)
file(GLOB controller_db_sources ${MODULE_PATH}/db/*.cpp)
if (ENABLE_NBAPI)
    file(GLOB controller_actions ${ACTION_PATH}/*.cpp)
endif()

# link to libuci
link_directories(${PLATFORM_STAGING_DIR}/usr/lib)
list(APPEND LINKED_LIBS uci)

#RDKB
if (TARGET_PLATFORM STREQUAL "rdkb")
    file(GLOB RDKB_SOURCES ${MODULE_PATH}/tasks/rdkb/*.cpp)
    list(APPEND controller_tasks_sources  ${RDKB_SOURCES})
endif()

add_executable(${PROJECT_NAME} ${controller_sources} ${controller_tasks_sources}
${controller_operations_sources} ${controller_db_sources} ${controller_actions})

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-z,defs")

list(APPEND LINKED_LIBS bpl bcl btl tlvf elpp btlvf nbapi ieee1905_transport_lib)
if (ENABLE_NBAPI)
    # Include AMBIORIX in the build
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_NBAPI ${UBUS_SOCK}")
    find_package(amxc REQUIRED)
    find_package(amxd REQUIRED)
    list(APPEND LINKED_LIBS amxc amxd)
endif()

target_link_libraries(${PROJECT_NAME} ${LINKED_LIBS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${MODULE_PATH}/../bml
    if (ENABLE_NBAPI)
        ${ACTION_PATH}
    endif()
)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory("db/unit_tests")
