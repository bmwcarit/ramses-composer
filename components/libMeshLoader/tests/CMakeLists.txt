#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Adding the unit test with gtest using our macro from dsathe top level CMakeLists.txt file

set(TEST_SOURCES
    FileLoader_test.cpp
)
set(TEST_LIBRARIES
    raco::MeshLoader
    raco::RamsesBase
    raco::Testing
)
raco_package_add_headless_test(
    libMeshLoader_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)
raco_package_add_test_resources(
    libMeshLoader_test "${CMAKE_SOURCE_DIR}/resources"
    meshes/AnimatedMorphCube/AnimatedMorphCube.bin
    meshes/AnimatedMorphCube/AnimatedMorphCube.gltf
    meshes/CesiumMilkTruck/CesiumMilkTruck.gltf
    meshes/CesiumMilkTruck/CesiumMilkTruck.png
    meshes/CesiumMilkTruck/CesiumMilkTruck_data.bin
    meshes/MosquitoInAmber/MosquitoInAmber.gltf
    meshes/MosquitoInAmber/MosquitoInAmber.bin
    meshes/MultipleVCols/multiple_VCols.gltf
)
