#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

set(TEST_SOURCES 
    CommandInterface_test.cpp
    Annotation_test.cpp
	Context_test.cpp
    Datamodel_test.cpp
	Handle_test.cpp
	Iterator_test.cpp
    Node_test.cpp
    Reference_test.cpp
	Link_test.cpp
    Undo_test.cpp
    Prefab_test.cpp
    ExternalReference_test.cpp
    ValueHandle_test.cpp
    PathManager_test.cpp
    Queries_Tags_test.cpp
)

set(TEST_LIBRARIES
    raco::UserTypes
    raco::ApplicationLib
    raco::Testing
)

raco_package_add_headless_test(
    libCore_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

IF (MSVC)
	# /bigobj is needed to compile UserObjectFactory.cpp
	target_compile_options(libCore_test PRIVATE "/bigobj")
endif()

raco_package_add_test_resources(
    libCore_test "${CMAKE_SOURCE_DIR}/resources"
    shaders/basic.frag
    shaders/basic.vert
    shaders/simple_texture.frag
    shaders/simple_texture.vert
    shaders/uniform-array.vert
    shaders/uniform-array.frag
    meshes/Duck.glb
    meshes/defaultQuad.gltf
    meshes/InterpolationTest/InterpolationTest.gltf
    meshes/InterpolationTest/interpolation.bin
    meshes/InterpolationTest/l.jpg
    scripts/camera-control.lua
    scripts/moduleDefinition.lua
    scripts/moduleDependency.lua
    scripts/types-scalar.lua
    scripts/struct-simple.lua
    scripts/struct-nested.lua
    scripts/SimpleScript.lua
    scripts/interface-scalar-types.lua
    scripts/array.lua
)


set(TEST_SOURCES_SERIALIZATION
	Serialization_test.cpp
	Deserialization_test.cpp
    ProjectMigration_test.cpp
)

set(TEST_LIBRARIES_SERIALIZATION
    raco::Core
    raco::ApplicationLib
    raco::Testing
    raco::Utils
)
raco_package_add_headless_test(
    libSerialization_test
    "${TEST_SOURCES_SERIALIZATION}"
    "${TEST_LIBRARIES_SERIALIZATION}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

raco_package_add_test_resources(
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
    testData/InterpolationTest/InterpolationTest.gltf
    testData/InterpolationTest/interpolation.bin
    testData/InterpolationTest/l.jpg

    migrationTestData/V1.rca
    migrationTestData/V9.rca
    migrationTestData/V10.rca
    migrationTestData/V12.rca
    migrationTestData/V13.rca
    migrationTestData/V14.rca
    migrationTestData/V14b.rca
    migrationTestData/V14c.rca
    migrationTestData/V16.rca
    migrationTestData/V18.rca
    migrationTestData/V21.rca
    migrationTestData/V23.rca
    migrationTestData/scripts/out-bool.lua
    migrationTestData/V29.rca
    migrationTestData/V29_tags.rca
    migrationTestData/V29_renderlayer.rca
    migrationTestData/interfaces/intf-struct-vec.lua
    migrationTestData/scripts/array.lua
    migrationTestData/scripts/array-of-array.lua
    migrationTestData/scripts/array-of-structs.lua
    migrationTestData/scripts/types-scalar.lua
    migrationTestData/scripts/struct-simple.lua
    migrationTestData/scripts/struct-nested.lua
    migrationTestData/scripts/interface-scalar-types.lua
    migrationTestData/scripts/uniform-array.lua
    migrationTestData/scripts/uniform-structs.lua
    migrationTestData/shaders/uniform-scalar.vert
    migrationTestData/shaders/uniform-scalar.frag
    migrationTestData/shaders/uniform-array.vert
    migrationTestData/shaders/uniform-array.frag
    migrationTestData/shaders/uniform-struct.vert
    migrationTestData/shaders/uniform-struct.frag
    migrationTestData/V35.rca
    migrationTestData/V35_extref.rca
    migrationTestData/V35_extref_nested.rca
    migrationTestData/V39.rca
    migrationTestData/V40.rca
    migrationTestData/V41.rca
    migrationTestData/V43.rca
    migrationTestData/V44.rca
    migrationTestData/V45.rca
    migrationTestData/V50.rca
    migrationTestData/V51.rca
    migrationTestData/V51_link_vec_struct.rca
    migrationTestData/V52.rca
    migrationTestData/version-current.rca
)
add_compile_definitions(libSerialization_test PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

