#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

set(TEST_SOURCES
    UtilsBaseTest.h
    FileUtils_test.cpp
    ShaderPreprocessor_test.cpp
    u8path_test.cpp
)

set(TEST_LIBRARIES
    raco::Testing
    raco::Utils
)

raco_package_add_headless_test(
    libUtils_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

raco_package_add_test_resources(
    libUtils_test "${CMAKE_SOURCE_DIR}/resources"
    shaders/basic.vert
    shaders/basic.frag
    shaders/include/func1.glsl
    shaders/include/func2.glsl
    shaders/include/func2_diamond.glsl
    shaders/include/loop_inc
    shaders/include/loop_main
    shaders/include/main.glsl
    shaders/include/main_diamond.glsl
    shaders/include/main_nested.glsl
    shaders/include/main_subdir.glsl
    shaders/include/subdirectory/func3.glsl
)
