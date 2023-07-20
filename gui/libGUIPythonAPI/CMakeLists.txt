#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

add_library(libGUIPythonAPI
    include/gui_python_api/GUIPythonAPI.h src/GUIPythonAPI.cpp
)

target_include_directories(libGUIPythonAPI PUBLIC include/)

target_link_libraries(libGUIPythonAPI
PUBLIC
	raco::ObjectTree
	raco::RamsesWidgets
PRIVATE
	raco::pybind11)

enable_warnings_as_errors(libGUIPythonAPI)

add_library(raco::GUIPythonAPI ALIAS libGUIPythonAPI)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()
