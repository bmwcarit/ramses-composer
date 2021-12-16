#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Adding the unit test with gtest using our macro from dsathe top level CMakeLists.txt file

set(TEST_SOURCES
    AnimationAdaptor_test.cpp
    AnimationChannelAdaptor_test.cpp
    RamsesBaseFixture.h
    LinkAdaptor_test.cpp
    LuaScriptAdaptor_test.cpp
    LuaScriptModuleAdaptor_test.cpp
    MaterialAdaptor_test.cpp
    MeshAdaptor_test.cpp
    MeshNodeAdaptor_test.cpp
    NodeAdaptor_test.cpp
    OrthographicCameraAdaptor_test.cpp
    PerspectiveCameraAdaptor_test.cpp
    Ramses_test.cpp
    RamsesLogic_test.cpp
    RenderLayerAdaptor_test.cpp
    Resources_test.cpp
    SceneContext_test.cpp
    Utils_test.cpp
    utilities_test.cpp
)
set(TEST_LIBRARIES
    raco::RamsesBase
    raco::Testing
)
raco_package_add_headless_test(
    libRamsesBase_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)
raco_package_add_test_resouces(
    libRamsesBase_test "${CMAKE_SOURCE_DIR}/resources"
    images/DuckCM.png
    shaders/basic.frag
    shaders/basic.vert
    shaders/simple_texture.frag
    shaders/simple_texture.vert
    meshes/Duck.glb
    meshes/CesiumMilkTruck/CesiumMilkTruck.gltf
    meshes/CesiumMilkTruck/CesiumMilkTruck_data.bin
    meshes/InterpolationTest/InterpolationTest.gltf
    meshes/InterpolationTest/interpolation.bin
    meshes/InterpolationTest/l.jpg
)
