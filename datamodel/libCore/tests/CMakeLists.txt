#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

set(TEST_SOURCES 
	Context_test.cpp
	Handle_test 
	Iterator_test.cpp
    Node_test.cpp
    Reference_test.cpp
	Link_test.cpp
    Undo_test.cpp
    Prefab_test.cpp
    ExternalReference_test.cpp
    ValueHandle_test.cpp
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

raco_package_add_test_resouces(
    libCore_test "${CMAKE_SOURCE_DIR}/resources"
    shaders/basic.frag
    shaders/basic.vert
    shaders/simple_texture.frag
    shaders/simple_texture.vert
    meshes/Duck.glb
    meshes/defaultQuad.gltf
    scripts/types-scalar.lua
    scripts/struct-simple.lua
    scripts/struct-nested.lua
    scripts/SimpleScript.lua
    scripts/camera-control.lua
)
