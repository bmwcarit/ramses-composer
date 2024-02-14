#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

raco_find_qt_components(Core)

add_library(libCore
	include/core/BasicAnnotations.h
	include/core/BasicTypes.h
	include/core/ChangeRecorder.h src/ChangeRecorder.cpp
	include/core/CodeControlledPropertyModifier.h
	include/core/CommandInterface.h src/CommandInterface.cpp
	include/core/Consistency.h src/Consistency.cpp
	include/core/Context.h src/Context.cpp
	include/core/CoreAnnotations.h
    include/core/CoreFormatter.h
	include/core/DynamicEditorObject.h
	include/core/EditorObject.h src/EditorObject.cpp
	include/core/EngineInterface.h
	include/core/ErrorItem.h src/ErrorItem.cpp
	include/core/Errors.h src/Errors.cpp
	include/core/ExternalReferenceAnnotation.h
	include/core/ExtrefOperations.h src/ExtrefOperations.cpp
    include/core/FileChangeCallback.h
    include/core/FileChangeMonitor.h 
	include/core/Handles.h src/Handles.cpp 
	include/core/Iterators.h src/Iterators.cpp 
	include/core/Link.h src/Link.cpp
	include/core/LinkContainer.h src/LinkContainer.cpp
	include/core/LinkGraph.h src/LinkGraph.cpp
	include/core/MeshCacheInterface.h
	include/core/PathManager.h src/PathManager.cpp
	include/core/PathQueries.h src/PathQueries.cpp
	include/core/PrefabOperations.h src/PrefabOperations.cpp
	include/core/Project.h src/Project.cpp
	include/core/ProjectMigration.h src/ProjectMigration.cpp
	include/core/ProjectMigrationToV23.h src/ProjectMigrationToV23.cpp
    include/core/ProjectSettings.h
	include/core/ProjectSettings.h
	include/core/PropertyDescriptor.h src/PropertyDescriptor.cpp
	include/core/ProxyObjectFactory.h src/ProxyObjectFactory.cpp
	include/core/ProxyTypes.h src/ProxyTypes.cpp
	include/core/Queries.h src/Queries.cpp
	include/core/Queries_Tags.h src/Queries_Tags.cpp
	include/core/SceneBackendInterface.h
	include/core/Serialization.h src/Serialization.cpp
    include/core/SerializationKeys.h    
	include/core/TagDataCache.h src/TagDataCache.cpp
	include/core/Undo.h src/Undo.cpp
	include/core/UserObjectFactoryInterface.h
)

target_include_directories(libCore PUBLIC include/)

enable_warnings_as_errors(libCore)


target_link_libraries(libCore
PUBLIC
	raco::DataStorage	
	raco::LogSystem
	raco::Utils
	Qt5::Core
	glm::glm
PRIVATE
	# Linking the user types to the Core causes a circular dependency.
	# We need to either move the classes needed from UserTypes into the core 
	# (as of the middle of September, this is only the Node class),
	# or just move the user type classes all back into libCore.
	raco::UserTypes
)

IF (MSVC)
    # /bigobj is needed to compile UserObjectFactory.cpp
    target_compile_options(libCore PRIVATE "/bigobj")
endif()

add_library(raco::Core ALIAS libCore)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()