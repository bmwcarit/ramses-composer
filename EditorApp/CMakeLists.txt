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
set(CMAKE_AUTOUIC ON)

raco_find_qt_components(Widgets)

set(SOURCES
	DebugActions.h DebugActions.cpp
    mainwindow.h mainwindow.cpp mainwindow.ui
    versiondialog.h versiondialog.cpp versiondialog.ui
    SavedLayoutsDialog.h SavedLayoutsDialog.cpp SavedLayoutsDialog.ui
	OpenRecentMenu.h OpenRecentMenu.cpp
    RaCoDockManager.h RaCoDockManager.cpp
	EditMenu.h EditMenu.cpp
	ForceHighEndGPU.cpp
	main.cpp
	../styles/icons.qrc
	../styles/images.qrc
)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/RamsesComposer.rc")
add_executable(RaCoEditor WIN32 ${SOURCES} ${APP_ICON_RESOURCE_WINDOWS})

target_compile_definitions( RaCoEditor PUBLIC -DRACO_OSS_COMMIT="sha1 ${RAMSES_OSS_COMMIT_HASH}" )
target_compile_definitions( RaCoEditor PUBLIC -DRACO_OSS_VERSION="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
target_compile_definitions( RaCoEditor PUBLIC -DRAMSES_VERSION="${RAMSES_VERSION_MAJOR}.${RAMSES_VERSION_MINOR}.${RAMSES_VERSION_PATCH}" )
target_compile_definitions( RaCoEditor PUBLIC -DRLOGIC_VERSION="${RLOGIC_VERSION_MAJOR}.${RLOGIC_VERSION_MINOR}.${RLOGIC_VERSION_PATCH}" )
enable_warnings_as_errors(RaCoEditor)

set_target_properties(RaCoEditor PROPERTIES OUTPUT_NAME "RamsesComposer" RUNTIME_OUTPUT_DIRECTORY "${RACO_RELEASE_DIRECTORY}/bin/$<CONFIG>")

# Copy release root folder files
set(RACO_RELEASE_ROOT_FILES
	${CMAKE_SOURCE_DIR}/README.md
	${CMAKE_SOURCE_DIR}/CHANGELOG.md
	${CMAKE_SOURCE_DIR}/third_party/ramses-logic/tools/migrate/migrate_to_v1_0.py
)

create_folder_structure("${RACO_RELEASE_ROOT_FILES}" ${RACO_RELEASE_DIRECTORY} RaCoPrepareReleaseFolder)

# Create default directories for configuration files and project files.
add_custom_command(TARGET RaCoPrepareReleaseFolder POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${RACO_RELEASE_DIRECTORY}/projects"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${RACO_RELEASE_DIRECTORY}/python"
)

add_dependencies(RaCoEditor RaCoResources RaCoPrepareReleaseFolder)

target_link_libraries(RaCoEditor
PUBLIC
	raco::LogSystem
	raco::PropertyBrowser
	raco::CommonWidgets
	raco::GUIPythonAPI
	raco::Style
	raco::Utils
	raco::Core 
	raco::ObjectTree
	Qt5::Widgets
	qtadvanceddocking
	raco::PythonAPI
	raco::RamsesWidgets
PRIVATE
    raco::pybind11
)
add_compile_definitions(RaCoEditor PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

seperate_debug_symbols(RaCoEditor "$<TARGET_FILE:RaCoEditor>")

deploy_gui_shared_dlls(RaCoEditor)
deploy_ramses_with_renderer_shared_dlls(RaCoEditor)
deploy_qt(RaCoEditor)
deploy_raco_cppruntime_dlls(RaCoEditor)
deploy_viewer(RaCoEditor)
deploy_python_dlls(RaCoEditor)
deploy_python_folder(RaCoEditor)