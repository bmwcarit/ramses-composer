#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

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
	raco::ramses-lib-client-only
	raco::ramses-logic-lib-client-only
	Qt5::Core
	raco::PythonAPI
PRIVATE
    raco::pybind11
)
enable_warnings_as_errors(RaCoCommand)

set_target_properties(RaCoCommand PROPERTIES OUTPUT_NAME "RaCoHeadless" RUNTIME_OUTPUT_DIRECTORY "${RACO_RELEASE_DIRECTORY}/bin/$<CONFIG>")
add_dependencies(RaCoCommand RaCoResources RaCoPrepareReleaseFolder)

target_compile_definitions( RaCoCommand PUBLIC -DRACO_OSS_VERSION="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

if(PYTHON_DEBUG_LIBRARY_AVAILABLE)
	target_compile_definitions(RaCoCommand PRIVATE PYTHON_DEBUG_LIBRARY_AVAILABLE)
endif()

seperate_debug_symbols(RaCoCommand "$<TARGET_FILE:RaCoCommand>")

deploy_headless_shared_dlls(RaCoCommand)
deploy_ramses_client_only_shared_dlls(RaCoCommand)
deploy_qt(RaCoCommand)
deploy_raco_cppruntime_dlls(RaCoCommand)
deploy_viewer(RaCoCommand)
deploy_python_dlls(RaCoCommand)
deploy_python_folder(RaCoCommand)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()

