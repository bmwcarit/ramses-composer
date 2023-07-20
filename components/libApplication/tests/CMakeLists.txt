#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Adding the unit test with gtest using our macro from dsathe top level CMakeLists.txt file

set(TEST_SOURCES
    RaCoApplication_test.cpp
    RaCoProject_test.cpp
)
set(TEST_LIBRARIES
    raco::RamsesBase
    raco::ApplicationLib
    raco::Testing
    raco::Utils
)

raco_package_add_headless_test(
    libApplication_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

raco_package_test_resources_process(
    libApplication_test "${CMAKE_SOURCE_DIR}/resources"
    shaders/basic.frag
    shaders/basic.vert
    meshes/RiggedFigure/RiggedFigure.gltf
    meshes/RiggedFigure/RiggedFigure0.bin
    meshes/CesiumMilkTruck/CesiumMilkTruck.gltf
    meshes/CesiumMilkTruck/CesiumMilkTruck_data.bin
    meshes/CesiumMilkTruck/CesiumMilkTruck.png
    meshes/InterpolationTest/InterpolationTest.gltf
    meshes/InterpolationTest/interpolation.bin
    meshes/InterpolationTest/l.jpg
    meshes/Duck.glb
    meshes/meshless.gltf
    meshes/negativeScaleQuad.gltf
    meshes/ToyCar/ToyCar.gltf
    meshes/ToyCar/ToyCar.bin
    scripts/compile-error.lua
    scripts/moduleDefinition.lua
    scripts/moduleDependency.lua
    scripts/SimpleScript.lua
    scripts/types-scalar.lua
    scripts/runtime-error.lua
)

raco_package_test_resources_process(
    libApplication_test "${CMAKE_CURRENT_SOURCE_DIR}/testData"
    loadDoubleModuleReferenceWithoutError.rca
    scripts/m1.lua
    scripts/m2.lua
    scripts/s.lua
    export-interface-link-opt-1.rca
    export-interface-link-opt-2.rca
    export-interface-link-opt-3.rca
    export-interface-link-opt-4.rca
    export-interface-opt-no-ending.rca
    export-interface-opt-ending-valid.rca
    export-interface-opt-ending-invalid.rca
    scripts/script1.lua
    scripts/script2.lua
    interfaces/script1.lua
    interfaces/script2.lua
    interfaces/global.lua
 )
 
raco_package_test_resources_add_compile_definitions(libApplication_test)