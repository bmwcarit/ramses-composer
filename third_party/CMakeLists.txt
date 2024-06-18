#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

set(BOOST_INCLUDE_LIBRARIES spirit)
add_subdirectory(boost/ EXCLUDE_FROM_ALL)

set(SPDLOG_WCHAR_FILENAMES ON CACHE BOOL "" FORCE)
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


## TINYGLTF
set(TINYGLTF_USE_CPP14 ON CACHE BOOL "" FORCE)
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
set(TINYGLTF_NO_EXTERNAL_IMAGE OFF CACHE INTERNAL "" FORCE)
add_library(tinygltf INTERFACE)
target_include_directories(tinygltf INTERFACE tinygltf/)
set_target_properties(tinygltf PROPERTIES
    FOLDER third_party/tinygltf
)

## GLM
# Note GLM is now included in Ramses 28 so we don't need this as a submodule anymore


## pybind11
add_subdirectory(pybind11)
add_library(raco_pybind11 INTERFACE)
target_sources(raco_pybind11 PUBLIC raco_pybind11_embed.h)
target_include_directories(raco_pybind11 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(raco_pybind11 INTERFACE pybind11::embed)
if(PYTHON_DEBUG_LIBRARY_AVAILABLE AND WIN32)
    # Needed to avoid pybind11/embed.h to cause linking to the non-debug DLL if the debug DLL is available.
    # See https://github.com/pybind/pybind11/issues/3403#issuecomment-962878324
	target_compile_definitions(raco_pybind11 INTERFACE $<$<CONFIG:Debug>:PYTHON_DEBUG_LIBRARY_AVAILABLE>)
endif()
set_target_properties(raco_pybind11 PROPERTIES
    FOLDER third_party/raco_pybind11
)
add_library(raco::pybind11 ALIAS raco_pybind11)

## zip
set(CMAKE_DISABLE_TESTING  ON CACHE BOOL "" FORCE)

add_subdirectory(zip/)
set_target_properties(zip uninstall PROPERTIES
    FOLDER third_party/zip
)

# Configure ramses-sdk build options
set(ramses-sdk_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ramses-sdk_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_WITH_LTO  OFF CACHE BOOL "" FORCE)

set(ramses-sdk_BUILD_FULL_SHARED_LIB ON CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_HEADLESS_SHARED_LIB ON CACHE BOOL "" FORCE)

set(ramses-sdk_ENABLE_LOGIC ON CACHE BOOL "" FORCE)
set(ramses-sdk_ENABLE_TCP_SUPPORT OFF CACHE BOOL "" FORCE)
set(ramses-sdk_ENABLE_DLT OFF CACHE BOOL "" FORCE)

set(ramses-sdk_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_TOOLS ON CACHE BOOL "" FORCE)
set(ramses-sdk_BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(ramses-sdk_ALLOW_PLATFORM_GLM OFF CACHE BOOL "" FORCE)

add_subdirectory(ramses/ EXCLUDE_FROM_ALL)

add_library(raco::ramses-lib ALIAS ramses-shared-lib)
add_library(raco::ramses-lib-client-only ALIAS ramses-shared-lib-headless)


if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    find_program(LINUXDEPLOYQT linuxdeployqt)
	if(EXISTS "${LINUXDEPLOYQT}")
        add_custom_command(OUTPUT  ${CMAKE_BINARY_DIR}/ramses-viewer
            COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:RaCoEditor>/ramses-viewer.sh"
            COMMAND ${CMAKE_COMMAND} -D TARGET_FILE=$<TARGET_FILE_DIR:RaCoEditor>/ramses-viewer -D ROOT_DIR=${CMAKE_SOURCE_DIR} -P "${CMAKE_SOURCE_DIR}/ubuntustartscript.cmake"
            COMMAND chmod +x "$<TARGET_FILE_DIR:RaCoEditor>/ramses-viewer.sh" 
            DEPENDS ramses-viewer RaCoEditor
        )
        
        add_custom_target(generate_ramses_viewer_launch_script ALL
            DEPENDS ${CMAKE_BINARY_DIR}/ramses-viewer
        )
    endif()
endif()

if(WIN32)
    add_subdirectory(python-windows)
elseif(UNIX)
    add_subdirectory(python-linux)
endif()

register_python_target(RaCoPythonModuleDeployment ${PYTHON_DEPLOYMENT_FOLDER})
register_python_target(RaCoHeadlessPythonModuleDeployment ${HEADLESS_PYTHON_DEPLOYMENT_FOLDER})

macro(deploy_python_folder tgt)
    add_dependencies(${tgt} RaCoPythonModuleDeployment)
endmacro()

macro(deploy_python_folder_headless tgt)
    add_dependencies(${tgt} RaCoHeadlessPythonModuleDeployment)
endmacro()