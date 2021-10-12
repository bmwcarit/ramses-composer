#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

add_library(libMeshLoader
	include/mesh_loader/CTMMesh.h src/CTMMesh.cpp
	include/mesh_loader/CTMFileLoader.h src/CTMFileLoader.cpp
	include/mesh_loader/glTFMesh.h src/glTFMesh.cpp
	include/mesh_loader/glTFFileLoader.h src/glTFFileLoader.cpp
)

target_include_directories(libMeshLoader PUBLIC include/)
enable_warnings_as_errors(libMeshLoader)

target_link_libraries(libMeshLoader
PUBLIC
	raco::Core
PRIVATE
	openctm
	tinygltf
	glm
)

add_library(raco::MeshLoader ALIAS libMeshLoader)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()