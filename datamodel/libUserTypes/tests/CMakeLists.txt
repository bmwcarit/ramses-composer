#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Adding the unit test with gtest using our macro from dsathe top level CMakeLists.txt file

set(TEST_SOURCES
    Animation_test.cpp
    AnimationChannel_test.cpp
    CubeMap_test.cpp
    DefaultValues_test.cpp
    Material_test.cpp
    Mesh_test.cpp
    MeshNode_test.cpp
    LuaScript_test.cpp
    LuaScriptModule_test.cpp
    LuaInterface_test.cpp
    Skin_test.cpp
    Texture_test.cpp
    Timer_test.cpp
)

set(TEST_LIBRARIES
    raco::RamsesBase
    raco::UserTypes
    raco::ApplicationLib
    raco::Testing
)
raco_package_add_headless_test(
    libUserTypes_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)
raco_package_add_test_resources(
    libUserTypes_test "${CMAKE_SOURCE_DIR}/resources"
    meshes/AnimatedMorphCube/AnimatedMorphCube.bin
    meshes/AnimatedMorphCube/AnimatedMorphCube.gltf
    meshes/CesiumMilkTruck/CesiumMilkTruck.gltf
    meshes/CesiumMilkTruck/CesiumMilkTruck_data.bin
    meshes/InterpolationTest/InterpolationTest.gltf
    meshes/InterpolationTest/interpolation.bin
    meshes/InterpolationTest/l.jpg
    meshes/Duck.glb
    meshes/meshless.gltf
    meshes/SimpleSkin/SimpleSkin.gltf
    scripts/array.lua
    scripts/compile-error.lua
    scripts/moduleDefinition.lua
    scripts/moduleDependency.lua
    scripts/struct.lua
    scripts/using-math.lua
    scripts/module-using-math.lua
    scripts/using-module-using-math.lua
    scripts/interface-using-math.lua
    scripts/interface-using-module.lua
    scripts/types-scalar.lua
    shaders/basic.vert
    shaders/basic.frag
    shaders/uniform-array.vert
    shaders/uniform-array.frag
    shaders/include/main_mat_adapter_test.frag
    shaders/include/main_mat_adapter_test.geom
    shaders/include/main_mat_adapter_test.vert
    shaders/include/uniforms_frag.glsl
    shaders/include/uniforms_geom.glsl
    shaders/include/uniforms_vert.glsl
)
