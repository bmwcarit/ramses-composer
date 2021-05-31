#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

raco_find_qt_components(Core)

add_library(libComponents
    include/components/Naming.h
    include/components/EditorObjectFormatter.h
    include/components/DataChangeDispatcher.h src/DataChangeDispatcher.cpp
    include/components/DebugInstanceCounter.h
    include/components/EditorObjectFormatter.h
    src/FileChangeListenerImpl.h src/FileChangeListenerImpl.cpp # hide FileChangeListenerImpl definition from other libraries
    include/components/FileChangeMonitorImpl.h src/FileChangeMonitorImpl.cpp
    include/components/MeshCacheImpl.h src/MeshCacheImpl.cpp
    include/components/RamsesProjectMigration.h src/RamsesProjectMigration.cpp
    include/components/RaCoNameConstants.h
    include/components/RaCoPreferences.h src/RaCoPreferences.cpp
    include/components/QtFormatter.h
)
target_include_directories(libComponents PUBLIC include/)
target_link_libraries(libComponents
PUBLIC
    raco::UserTypes
    raco::Core
    raco::LogSystem
    raco::Serialization
    raco::Utils
    Qt5::Core
PRIVATE
    raco::MeshLoader
)

set_target_properties(libComponents PROPERTIES AUTOMOC TRUE)
set_target_properties(libComponents PROPERTIES AUTORCC TRUE)
set_target_properties(libComponents PROPERTIES AUTOUIC TRUE)

enable_warnings_as_errors(libComponents)

option(RACO_USE_DEBUG_INSTANCE_COUNTER "Enable debug instance counter" OFF)
if(RACO_USE_DEBUG_INSTANCE_COUNTER)
    target_compile_definitions(libComponents PUBLIC RACO_USE_DEBUG_INSTANCE_COUNTER=true)
    message("RaCo: activated debug instance counter")
endif()

add_library(raco::Components ALIAS libComponents)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()
