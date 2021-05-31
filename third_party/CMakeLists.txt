add_subdirectory(spdlog/)
set_target_properties(spdlog PROPERTIES FOLDER third_party)

set(BUILD_STATIC OFF CACHE BOOL "" FORCE)
add_subdirectory(Qt-Advanced-Docking-System/ EXCLUDE_FROM_ALL)
set_target_properties(qtadvanceddocking PROPERTIES FOLDER third_party)

# Mesh file libraries
## CTM
add_subdirectory(OpenCTM-1.0.3/)
set_target_properties(openctm PROPERTIES FOLDER third_party/openctm POSITION_INDEPENDENT_CODE ON)
set_target_properties(lzma PROPERTIES FOLDER third_party/openctm POSITION_INDEPENDENT_CODE ON)


## ASSIMP
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "We do not need the Assimp tools" FORCE) # for build speed reasons
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "We do not need the Assimp tests" FORCE) # for build speed reasons
set(ASSIMP_BUILD_NO_EXPORT ON CACHE BOOL "We do not need to export 3D using Assimp" FORCE) # for build speed reasons
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "We only need one specific importer" FORCE) # for build speed reasons
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "Re-enable support for glTF. Supported format." FORCE) # for build speed reasons
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "Force building of zlibstatic through assimp even if it already can be found (can happen on Ubuntu)." FORCE)
add_subdirectory(assimp EXCLUDE_FROM_ALL)
set_target_properties(assimp PROPERTIES
    FOLDER third_party/assimp
)

if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER third_party/zlibstatic)
endif()

# Configure ramses-sdk build options
set(ramses-sdk_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_IVI_TEST_APPS OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_DEMO_ECM OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_SMOKE_TESTS OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ramses-sdk_ENABLE_WAYLAND_SHELL  OFF CACHE BOOL "" FORCE)
set(ramses-sdk_ENABLE_DLT OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_WITH_LTO  OFF CACHE BOOL "" FORCE)
set(ramses-sdk_FORCE_OFF_SOMEIP_IC ON CACHE BOOL "" FORCE)
set(ramses-sdk_FORCE_USE_SOMEIP_IC OFF CACHE BOOL "" FORCE)

# These need to be set
set(ramses-sdk_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_FULL_SHARED_LIB ON CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_CLIENT_ONLY_SHARED_LIB ON CACHE BOOL "" FORCE)
set(ramses-sdk_CONSOLE_LOGLEVEL "info" CACHE STRING "" FORCE)
# TODO: why are we building ramses-daemon?
set(ramses-sdk_BUILD_TOOLS ON CACHE BOOL "" FORCE)

# TODO: check if we need to enable this
set(ramses-sdk_ENABLE_TCP_SUPPORT OFF CACHE BOOL "" FORCE)

add_subdirectory(ramses-logic/external/ramses/ EXCLUDE_FROM_ALL)

# replace ramses-logic RAMSES_TARGET with an INTERFACE ONLY LIBRARY
# needed so we can choose which ramses-shared-lib to link against
# on HeadlessApp or RaCoEditor level, by doing this we can no longer build
# all targets in the project (e.g. the ramses-logic dynamic target will fail)
add_library(ramses-api INTERFACE)
target_link_libraries(ramses-api
INTERFACE
    ramses-client-api
    ramses-framework-api
)

set(ramses-logic_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ramses-logic_ALLOW_RAMSES_BUILD OFF CACHE BOOL "" FORCE)
set(ramses-logic_RAMSES_TARGET "ramses-api" CACHE STRING "" FORCE)
set(ramses-logic_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
add_subdirectory(ramses-logic/ EXCLUDE_FROM_ALL)

add_library(ramses-logic-api INTERFACE)
target_include_directories(ramses-logic-api INTERFACE
    ramses-logic/include
)

function(add_ramses_logic_target TARGET RAMSES_TARGET)
    add_library(${TARGET} SHARED)
    target_link_libraries(${TARGET} PRIVATE ramses-logic-obj)
    target_link_libraries(${TARGET} PUBLIC ${RAMSES_TARGET})
    target_include_directories(${TARGET} PUBLIC ramses-logic/include)
    set_target_properties(${TARGET} PROPERTIES
        PUBLIC_HEADER "${public_headers}"
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
    folderize_target(${TARGET} "third_party/ramses-logic")
endfunction()

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    # es-3-0 / 4-5 / 4-2-core
    add_ramses_logic_target(ramses-logic-shared-lib-windows-wgl-4-5 ramses-shared-lib-windows-wgl-4-5)

    add_library(raco::ramses-logic-lib ALIAS ramses-logic-shared-lib-windows-wgl-4-5)
    add_library(raco::ramses-lib ALIAS ramses-shared-lib-windows-wgl-4-5)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    add_ramses_logic_target(ramses-logic-shared-lib-x11-egl-es-3-0 ramses-shared-lib-x11-egl-es-3-0)

    add_library(raco::ramses-logic-lib ALIAS ramses-logic-shared-lib-x11-egl-es-3-0)
    add_library(raco::ramses-lib ALIAS ramses-shared-lib-x11-egl-es-3-0)
endif()

add_ramses_logic_target(ramses-logic-shared-lib-client-only ramses-shared-lib-client-only)
add_library(raco::ramses-logic-lib-client-only ALIAS ramses-logic-shared-lib-client-only)
add_library(raco::ramses-lib-client-only ALIAS ramses-shared-lib-client-only)
