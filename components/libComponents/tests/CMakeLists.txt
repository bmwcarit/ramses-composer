#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

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

set(TEST_SOURCES_TRACEPLAYER
    TracePlayer_test.cpp
)

set(TEST_LIBRARIES_TRACEPLAYER
    raco::Testing
    raco::RamsesBase
    raco::ApplicationLib
    raco::Components
)

raco_package_add_headless_test(
    libTracePlayer_test
    "${TEST_SOURCES_TRACEPLAYER}"
    "${TEST_LIBRARIES_TRACEPLAYER}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

raco_package_add_test_resources(
    libTracePlayer_test "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    raco_traces/invalid_empty.rctrace
    raco_traces/invalid_wrongExtension.so
    raco_traces/invalid_wrongFormat.rctrace
    raco_traces/invalid_wrongRootType.rctrace
    raco_traces/valid_20211123.rctrace
    raco_traces/valid_10frames_500ms.rctrace
    lua_scripts/saInfo.lua
    lua_scripts/SyntheticData.lua
    lua_scripts/TracePlayerData.lua
    lua_scripts/dummyAllTypes.lua
    lua_scripts/dummyAllTypes_NoInteger.lua
    raco_traces/g05_demo.rctrace
    raco_traces/g05_demo_withExtras.rctrace
    lua_scripts/SceneControls.lua
    lua_scripts/SceneControls_withExtras.lua
    lua_scripts/modules/anim_utils.lua
    lua_scripts/modules/car_utils.lua
    lua_scripts/SceneControls_withArrays.lua
    raco_traces/g05_demo_Arrays.rctrace
    lua_scripts/DummyScript.lua
    raco_traces/g05_demo_withDummy.rctrace
)

add_compile_definitions(libTracePlayer_test PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")