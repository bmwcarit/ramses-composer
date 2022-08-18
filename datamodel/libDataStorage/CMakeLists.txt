#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

add_library(libDataStorage
	include/data_storage/AnnotationBase.h
	include/data_storage/ReflectionInterface.h src/ReflectionInterface.cpp 
	include/data_storage/Table.h src/Table.cpp 
	include/data_storage/Value.h src/Value.cpp 
)

target_include_directories(libDataStorage PUBLIC include)
enable_warnings_as_errors(libDataStorage)

target_link_libraries(libDataStorage
PRIVATE
	raco::LogSystem
)

add_library(raco::DataStorage ALIAS libDataStorage)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()