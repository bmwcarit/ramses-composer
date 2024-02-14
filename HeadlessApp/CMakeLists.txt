#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Setup Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

raco_find_qt_components(Core)

set(SOURCES
	main.cpp
	../styles/images.qrc
)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/RaCoHeadless.rc")
add_executable(RaCoCommand ${SOURCES} ${APP_ICON_RESOURCE_WINDOWS})    
target_link_libraries(RaCoCommand
PUBLIC
	raco::ApplicationLib
	raco::RamsesBase
	raco::UserTypes
	raco::LogSystem
	Qt5::Core
	raco::PythonAPI
PRIVATE
    raco::pybind11
)
enable_warnings_as_errors(RaCoCommand)

set_target_properties(RaCoCommand PROPERTIES OUTPUT_NAME "RaCoHeadless" RUNTIME_OUTPUT_DIRECTORY "${HEADLESS_RELEASE_DIRECTORY}/bin/$<CONFIG>")
target_compile_definitions( RaCoCommand PUBLIC -DRACO_OSS_VERSION="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

if(PYTHON_DEBUG_LIBRARY_AVAILABLE)
	target_compile_definitions(RaCoCommand PRIVATE PYTHON_DEBUG_LIBRARY_AVAILABLE)
endif()

seperate_debug_symbols(RaCoCommand "$<TARGET_FILE:RaCoCommand>")

# Copy Headless release root folder files
set(RACO_HEADLESS_RELEASE_ROOT_FILES
	${CMAKE_SOURCE_DIR}/CHANGELOG.md
)

create_folder_structure("${RACO_HEADLESS_RELEASE_ROOT_FILES}" ${HEADLESS_RELEASE_DIRECTORY} RaCoPrepareHeadlessReleaseFolder)

# Create directories required for Headless release
add_custom_command(TARGET RaCoPrepareHeadlessReleaseFolder POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:RaCoEditor>"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${HEADLESS_RELEASE_DIRECTORY}/projects"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/../resources/headless_projects_folder_warning.txt" "${HEADLESS_RELEASE_DIRECTORY}/projects/do_not_remove_this_folder.txt"
)

add_dependencies(RaCoCommand RaCoPrepareHeadlessReleaseFolder)

macro(deploy_to_extra_dir tgt file extradir)
	add_custom_command(TARGET ${tgt} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${file}" "${extradir}"
	)
endmacro()

deploy_raco_cppruntime_dlls(RaCoCommand)
deploy_headless_shared_dlls(RaCoCommand)
deploy_ramses_client_only_shared_dlls(RaCoCommand "$<TARGET_FILE_DIR:RaCoCommand>")
deploy_ramses_client_only_shared_dlls(RaCoCommand "$<TARGET_FILE_DIR:RaCoEditor>")
deploy_qt(RaCoCommand)
deploy_python_dlls(RaCoCommand)
deploy_python_folder_headless(RaCoCommand)

# Copy RaCoHeadless executable, debug symbols and startup shell script to RaCoEditor release folder
deploy_to_extra_dir(RaCoCommand "$<TARGET_FILE:RaCoCommand>" "$<TARGET_FILE_DIR:RaCoEditor>")

# Skip copying debug symbols for Debug Linux build, because they are not stripped
if(WIN32 OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
	set(SYMBOL_EXTENSION "$<IF:$<BOOL:${WIN32}>,pdb,debug>")
	deploy_to_extra_dir(RaCoCommand
		"$<TARGET_FILE_DIR:RaCoCommand>/$<TARGET_FILE_BASE_NAME:RaCoCommand>.${SYMBOL_EXTENSION}"
		"$<TARGET_FILE_DIR:RaCoEditor>"
	)
endif()
if (NOT WIN32)
	deploy_to_extra_dir(RaCoCommand "$<TARGET_FILE:RaCoCommand>.sh" "$<TARGET_FILE_DIR:RaCoEditor>")
endif()

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()

