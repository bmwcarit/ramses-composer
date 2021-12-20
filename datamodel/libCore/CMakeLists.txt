#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

raco_find_qt_components(Core)

add_library(libCore
	include/core/ChangeRecorder.h src/ChangeRecorder.cpp
	include/core/Context.h src/Context.cpp
	include/core/CommandInterface.h src/CommandInterface.cpp
	include/core/EditorObject.h src/EditorObject.cpp
    include/core/FileChangeCallback.h src/FileChangeCallback.cpp
    include/core/FileChangeMonitor.h 
	include/core/Handles.h src/Handles.cpp 
	include/core/Project.h src/Project.cpp
	include/core/PropertyDescriptor.h src/PropertyDescriptor.cpp
	include/core/PathQueries.h src/PathQueries.cpp
	include/core/Queries.h src/Queries.cpp
	include/core/Queries_Tags.h src/Queries_Tags.cpp
	include/core/Consistency.h src/Consistency.cpp
	include/core/MeshCacheInterface.h
	include/core/UserObjectFactoryInterface.h src/UserObjectFactoryInterface.cpp
	include/core/Iterators.h src/Iterators.cpp 
	include/core/Undo.h src/Undo.cpp
	include/core/EngineInterface.h
	include/core/ErrorItem.h src/ErrorItem.cpp
	include/core/Errors.h src/Errors.cpp
	include/core/Link.h src/Link.cpp
	include/core/PathManager.h src/PathManager.cpp
	include/core/ProjectSettings.h
	include/core/PrefabOperations.h src/PrefabOperations.cpp
	include/core/ExternalReferenceAnnotation.h
	include/core/ExtrefOperations.h src/ExtrefOperations.cpp
	include/core/SceneBackendInterface.h
	include/core/RamsesProjectMigration.h src/RamsesProjectMigration.cpp
	include/core/Serialization.h src/Serialization.cpp
    include/core/SerializationFunctions.h
    include/core/SerializationKeys.h    
	include/core/CoreAnnotations.h

    include/core/CoreFormatter.h
    include/core/ProjectSettings.h
)

target_include_directories(libCore PUBLIC include/)

enable_warnings_as_errors(libCore)


target_link_libraries(libCore
PUBLIC
	raco::DataStorage	
	raco::LogSystem
	raco::Utils
	Qt5::Core
PRIVATE
	# Linking the user types to the Core causes a circular dependency.
	# We need to either move the classes needed from UserTypes into the core 
	# (as of the middle of September, this is only the Node class),
	# or just move the user type classes all back into libCore.
	raco::UserTypes
)


add_library(raco::Core ALIAS libCore)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# Needed for Ubuntu 18 (GCC 7) - for the experimental file system, "stdc++fs" is required after all other object files.
	# See also https://gitlab.kitware.com/cmake/cmake/-/issues/17834
	target_link_libraries(libCore PUBLIC "stdc++fs")
endif()
if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()