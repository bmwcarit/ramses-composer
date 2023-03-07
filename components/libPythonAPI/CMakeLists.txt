#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Note: pybind11_add_module doesn't work
add_library(libPythonAPI
	include/python_api/PythonAPI.h src/PythonAPI.cpp
)

target_include_directories(libPythonAPI PUBLIC include/)

target_link_libraries(libPythonAPI
PUBLIC
	raco::ApplicationLib
PRIVATE
    raco::pybind11
)

enable_warnings_as_errors(libPythonAPI)

target_compile_definitions(libPythonAPI PUBLIC -DPYTHON_FOLDER="${PYTHON_DEPLOYMENT_FOLDER_NAME}")
message(STATUS "Python Version for libPythonAPI: ${Python_VERSION} from ${Python_EXECUTABLE}")

add_library(raco::PythonAPI ALIAS libPythonAPI)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()
