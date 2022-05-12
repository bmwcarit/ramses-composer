#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Adding the unit test with gtest using our macro from dsathe top level CMakeLists.txt file

set(TEST_SOURCES
    PythonAPI_Simple_test.cpp
)
set(TEST_LIBRARIES
    raco::PythonAPI
    raco::Testing
    raco::RamsesBase
    raco::pybind11
)
raco_package_add_headless_test(
    libPythonAPI_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

raco_package_add_test_resouces(
    libPythonAPI_test "${CMAKE_SOURCE_DIR}/resources"
    example_scene.rca
    empty.rca
    shaders/simple_texture.vert
    shaders/simple_texture.frag
    meshes/Duck.glb
    images/DuckCM.png
)

deploy_python_dlls(libPythonAPI_test)
deploy_python_folder(libPythonAPI_test)
