#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Adding the unit test with gtest using our macro from dsathe top level CMakeLists.txt file

set(TEST_SOURCES
    DataChangeDispatcher_test.cpp
    FileChangeMonitor_test.cpp
)
set(TEST_LIBRARIES
    raco::RamsesBase
    raco::Components
    raco::Testing
    raco::Utils
)
raco_package_add_headless_test(
    libComponents_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)
raco_package_add_test_resources(
    libComponents_test "${CMAKE_SOURCE_DIR}/resources"
    shaders/basic.frag
    shaders/basic.vert
    meshes/CesiumMilkTruck/CesiumMilkTruck.gltf
    meshes/CesiumMilkTruck/CesiumMilkTruck_data.bin
    meshes/CesiumMilkTruck/CesiumMilkTruck.png
    meshes/Duck.glb
    meshes/negativeScaleQuad.gltf
    meshes/ToyCar/ToyCar.gltf
    meshes/ToyCar/ToyCar.bin
    scripts/SimpleScript.lua
    scripts/types-scalar.lua
    scripts/runtime-error.lua
    scripts/compile-error.lua
)
