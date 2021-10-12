#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Adding the unit test with gtest using our macro from dsathe top level CMakeLists.txt file
set(TEST_SOURCES
	Serialization_test.cpp
	Deserialization_test.cpp
    ProjectMigration_test.cpp
)
set(TEST_LIBRARIES
    raco::Serialization
    raco::ApplicationLib
    raco::Testing
    raco::Utils
)
raco_package_add_headless_test(
    libSerialization_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)
raco_package_add_test_resouces(
    libSerialization_test "${CMAKE_CURRENT_SOURCE_DIR}"
    expectations/Node.json
    expectations/NodeRotated.json
    expectations/NodeWithAnnotations.json
    expectations/NodeWithChildMeshNode.json
    expectations/Mesh.json
    expectations/MeshGLTFBaked.json
    expectations/MeshGLTFSubmesh.json
    expectations/MeshNode.json
    expectations/MeshNodeWithMesh.json
    expectations/LuaScript.json
    expectations/LuaScriptInFloat.json
    expectations/LuaScriptInFloatArray.json
    expectations/LuaScriptInStruct.json
    expectations/LuaScriptSpecificPropNames.json
    expectations/LuaScriptWithRefToUserTypeWithAnnotation.json
    expectations/LuaScriptWithAnnotatedDouble.json
    expectations/LuaScriptWithURI.json
    expectations/LuaScriptLinkedToNode.json

    testData/duck.glb
    testData/in-float.lua
    testData/in-struct.lua
    testData/in-float-array.lua
    testData/in-specific-prop-names.lua
    testData/float.lua
    testData/ToyCar.gltf
    testData/uniform-types-vert.glsl
    testData/uniform-types-frag.glsl
    testData/simple_texture.frag
    testData/simple_texture.vert
    testData/DuckCM.png

    migrationTestData/V1.rca
    migrationTestData/V9.rca
    migrationTestData/V10.rca
    migrationTestData/V12.rca
    migrationTestData/V13.rca
    migrationTestData/V14.rca
    migrationTestData/V14b.rca
    migrationTestData/V14c.rca
    migrationTestData/version-current.rca
)
add_compile_definitions(libSerialization_test PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
