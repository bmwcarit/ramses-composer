#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

set(RESOURCE_FILES
	images/text-back.png
    images/text-bottom.png
	images/text-front.png
	images/text-top.png
	images/text-left.png
	images/text-right.png
	images/DuckCM.png
	meshes/Duck.glb
	meshes/meshless.gltf
	meshes/README.md
	meshes/ToyCar/Fabric_baseColor.png
	meshes/ToyCar/Fabric_normal.png
	meshes/ToyCar/Fabric_occlusion.png
	meshes/ToyCar/ToyCar.bin
	meshes/ToyCar/ToyCar.gltf
	meshes/ToyCar/ToyCar_basecolor.png
	meshes/ToyCar/ToyCar_clearcoat.png
	meshes/ToyCar/ToyCar_emissive.png
	meshes/ToyCar/ToyCar_normal.png
	meshes/ToyCar/ToyCar_occlusion_roughness_metallic.png
	scripts/array.lua
	scripts/compile-error.lua
	scripts/Heavy.lua
	scripts/moduleDefinition.lua
	scripts/moduleDependency.lua
	scripts/order.lua
	scripts/runtime-error.lua
	scripts/SimpleScript.lua
	scripts/struct.lua
	scripts/struct-nested.lua
	scripts/struct-simple.lua
	scripts/types-scalar.lua
	shaders/basic.frag
	shaders/basic.vert
	shaders/color.frag
	shaders/color.vert
	shaders/cubemap.frag
	shaders/cubemap.vert
	shaders/default.frag
	shaders/default.vert
	shaders/simple_texture.frag
	shaders/simple_texture.vert
	example_scene.rca
)

foreach(relpath ${RESOURCE_FILES})
	get_filename_component(reldir "${relpath}" DIRECTORY)
	ADD_CUSTOM_COMMAND(OUTPUT "${RACO_RELEASE_DIRECTORY}/projects/${relpath}"
					MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${relpath}"
					COMMAND ${CMAKE_COMMAND} -E make_directory "${RACO_RELEASE_DIRECTORY}/projects/${reldir}"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${relpath}" "${RACO_RELEASE_DIRECTORY}/projects/${relpath}"
					VERBATIM)
	LIST(APPEND RESOURCE_FILES_OUTPUT ${RACO_RELEASE_DIRECTORY}/projects/${relpath})
endforeach(relpath)

set(RACO_PYTHON_EXAMPLE_SCRIPTS
	python/demo.py
	python/print_objects.py
	python/print_scenegraph.py
	python/purge_links.py
	python/use_pip_to_install_module.py
	python/use_pip_to_uninstall_module.py
)

foreach(relpath ${RACO_PYTHON_EXAMPLE_SCRIPTS})
	add_custom_command(OUTPUT "${RACO_RELEASE_DIRECTORY}/${relpath}"
					MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${relpath}"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${relpath}" "${RACO_RELEASE_DIRECTORY}/${relpath}"
					VERBATIM)
	LIST(APPEND RESOURCE_FILES_OUTPUT ${RACO_RELEASE_DIRECTORY}/${relpath})
endforeach(relpath)

ADD_CUSTOM_TARGET(RaCoResources DEPENDS ${RESOURCE_FILES_OUTPUT} SOURCES ${RESOURCE_FILES} ${RACO_PYTHON_EXAMPLE_SCRIPTS})
set_target_properties (RaCoResources PROPERTIES FOLDER Packaging)
