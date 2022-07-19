#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

raco_find_qt_components(Core)

add_library(libApplication
    include/application/ExternalProjectsStore.h src/ExternalProjectsStore.cpp
    include/application/RaCoApplication.h src/RaCoApplication.cpp
    include/application/RaCoProject.h src/RaCoProject.cpp
)
target_include_directories(libApplication PUBLIC include/)
target_link_libraries(libApplication
PUBLIC
    raco::Components
    raco::UserTypes
    raco::Core
    raco::LogSystem
    raco::Utils
    Qt5::Core
PRIVATE
    raco::RamsesBase
    raco::MeshLoader
)

set_target_properties(libApplication PROPERTIES AUTOMOC TRUE)
set_target_properties(libApplication PROPERTIES AUTORCC TRUE)
set_target_properties(libApplication PROPERTIES AUTOUIC TRUE)

enable_warnings_as_errors(libApplication)

option(RACO_USE_DEBUG_INSTANCE_COUNTER "Enable debug instance counter" OFF)
if(RACO_USE_DEBUG_INSTANCE_COUNTER)
    target_compile_definitions(libApplication PUBLIC RACO_USE_DEBUG_INSTANCE_COUNTER=true)
    message("RaCo: activated debug instance counter")
endif()

add_library(raco::ApplicationLib ALIAS libApplication)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()
