#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Adding the unit test with gtest using our macro from dsathe top level CMakeLists.txt file

set(TEST_SOURCES
        GUIPythonAPI_test.cpp
)
set(TEST_LIBRARIES
    raco::GUIPythonAPI
    raco::PythonAPI
    raco::ObjectTree
    raco::Testing
    raco::RamsesBase
    raco::pybind11
)
raco_package_add_gui_test(
    libGUIPythonAPI_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

deploy_python_dlls(libGUIPythonAPI_test)
deploy_python_folder(libGUIPythonAPI_test)
