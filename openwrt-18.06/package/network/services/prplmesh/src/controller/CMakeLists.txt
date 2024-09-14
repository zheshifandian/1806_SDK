add_definitions(-DBEEROCKS_VERSION="${prplmesh_VERSION}")
add_definitions(-DBEEROCKS_BUILD_DATE="${BUILD_DATE}")
add_definitions(-DBEEROCKS_REVISION="${PRPLMESH_REVISION}")

add_subdirectory("src/beerocks")
add_subdirectory("config")
if (ENABLE_NBAPI)
    add_subdirectory("nbapi")
endif()
