#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Adding the unit test with gtest using our macro from dsathe top level CMakeLists.txt file

set(TEST_SOURCES
    AdaptorTestUtils.h
    MaterialAdaptorTestBase.h

    AnimationAdaptor_test.cpp
    AnimationChannelAdaptor_test.cpp
    BlitPassAdaptor_test.cpp
    CubeMapAdaptor_test.cpp
    EngineInterface_test.cpp
    RamsesBaseFixture.h
    LinkAdaptor_test.cpp
    LinkOptimization_test.cpp
    LuaScriptAdaptor_test.cpp
    LuaInterfaceAdaptor_test.cpp
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
    SkinAdaptor_test.cpp
    TimerAdaptor_test.cpp
    TextureAdaptor_test.cpp
    TextureExternalAdaptor_test.cpp
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
raco_package_add_test_resources(
    libRamsesBase_test "${CMAKE_SOURCE_DIR}/resources"
    images/blue_1024.png
    images/blue_1024_16i.png
    images/DuckCM.png
    images/green_512.png
    images/green_512_16f.png
    images/green_512_16f_no_alpha.png
    images/green_512_16i.png
    images/green_512_gray.png
    images/green_512_gray_16f.png
    images/green_512_gray_alpha.png
    images/red_128.png
    images/text-back.png
    images/text-back-palette.png
    images/yellow_256.png
    scripts/types-scalar.lua
    scripts/uniform-structs.lua
    scripts/uniform-array.lua
    shaders/basic.frag
    shaders/basic.vert
    shaders/multisampler.frag
    shaders/multisampler.vert
    shaders/simple_texture.frag
    shaders/simple_texture.vert
    shaders/texture-external.vert
    shaders/texture-external.frag
    shaders/uniform-array.vert
    shaders/uniform-array.frag
    shaders/uniform-samplers.vert
    shaders/uniform-samplers.frag
    shaders/uniform-scalar.vert
    shaders/uniform-scalar.frag
    shaders/uniform-struct.vert
    shaders/uniform-struct.frag
    shaders/skinning-template.vert
    shaders/skinning-template.frag
    shaders/include/main_mat_adapter_test.vert
    shaders/include/main_mat_adapter_test.frag
    shaders/include/main_mat_adapter_test.geom
    shaders/include/uniforms_frag.glsl
    shaders/include/uniforms_vert.glsl
    shaders/include/uniforms_geom.glsl
    meshes/Duck.glb
    meshes/meshrefless.gltf
    meshes/meshless.gltf
    meshes/CesiumMilkTruck/CesiumMilkTruck.gltf
    meshes/CesiumMilkTruck/CesiumMilkTruck_data.bin
    meshes/InterpolationTest/InterpolationTest.gltf
    meshes/InterpolationTest/interpolation.bin
    meshes/InterpolationTest/l.jpg
    meshes/SimpleSkin/SimpleSkin.gltf
    meshes/AnimatedMorphCube/AnimatedMorphCube.bin
    meshes/AnimatedMorphCube/AnimatedMorphCube.gltf
)
