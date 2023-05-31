#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

if(WIN32)
	set(RACO_EXE "$<TARGET_FILE:RaCoCommand>")
	set(VIEWER_EXE "$<TARGET_FILE_DIR:RaCoEditor>/ramses-logic-viewer.exe")
else()
	set(RACO_EXE "$<TARGET_FILE:RaCoCommand>.sh")
	set(VIEWER_EXE "$<TARGET_FILE_DIR:RaCoEditor>/ramses-logic-viewer.sh")
endif()

macro(add_screenshot_test TESTNAME PROJECTPATH) 
	add_test(
		NAME "screenshot_${TESTNAME}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -D RACO_EXE=${RACO_EXE} -D VIEWER_EXE=${VIEWER_EXE} -D TEST_NAME=${TESTNAME} -D PROJECT_PATH=${PROJECTPATH} -D PY_EXE=${PYTHON_DEPLOYMENT_FOLDER}/python.exe -D PY_CMD=compare_screenshots.py  -P "${CMAKE_CURRENT_SOURCE_DIR}/screenshot_test.cmake"
	)
	set_tests_properties("screenshot_${TESTNAME}" PROPERTIES ATTACHED_FILES_ON_FAIL "${CMAKE_CURRENT_SOURCE_DIR}/actual/${TESTNAME}.png")
endmacro()

# Run screenshot tests only on windows as long as the linux build server can't run the GUI application
if(WIN32)
	# Note: don't include simple_broken here: this project is intentionally different from the expected screenshot to 
	# check if screenshot different will really fail the test.
	set(RACO_SCREENSHOT_PROJECTS
		simple
		generated
		empty
		duck
		morphing
		skinning
	)

	set(RACO_DOC_PROJECTS
		advanced/animations/animations
		advanced/complex_import/complex_import
		advanced/external_references/composite_road
		advanced/external_references/simple_road
		advanced/modules/modules
		advanced/nested_prefabs/toy_cars
		advanced/python_api/broken_link
		basics/hello_world/cube
		basics/monkey/monkey
		basics/offscreen/offscreen
		basics/ordering/1_by_scene_graph
		basics/ordering/2_by_tags_priority
		basics/ordering/3_by_render_pass_slots
		basics/ordering/4_by_render_pass
		basics/ordering/5_nested_render_layers
		basics/prefabs/lantern_road
		basics/stencil_testing/stencil
	)

	foreach(projectname ${RACO_SCREENSHOT_PROJECTS})
		LIST(APPEND RACO_SCREENSHOT_PROJECT_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/projects/${projectname}.rca")
	endforeach(projectname)
	
	foreach(projectname ${RACO_DOC_PROJECTS})
		LIST(APPEND RACO_SCREENSHOT_PROJECT_PATHS "${CMAKE_SOURCE_DIR}/doc/${projectname}.rca")
	endforeach(projectname)

	foreach(projectpath ${RACO_SCREENSHOT_PROJECT_PATHS})
		message(${projectpath})
		get_filename_component(projectname ${projectpath} NAME_WLE)
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/expected/${projectname}.png
			MAIN_DEPENDENCY ${projectpath}
			DEPENDS ramses-logic-viewer RaCoEditor
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/exported/"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/expected/"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/actual/"
			COMMAND ${RACO_EXE} -p ${projectpath} -e ${CMAKE_CURRENT_SOURCE_DIR}/exported/${projectname} -l 3
			COMMAND ${VIEWER_EXE} "${CMAKE_CURRENT_SOURCE_DIR}/exported/${projectname}.ramses" "${CMAKE_CURRENT_SOURCE_DIR}/exported/${projectname}.rlogic" --exec-lua "rlogic.screenshot('expected/${projectname}.png')"
		)

		LIST(APPEND RACO_SCREENSHOT_REF_FILES "${CMAKE_CURRENT_SOURCE_DIR}/expected/${projectname}.png")
		add_screenshot_test(${projectname} ${projectpath})
	endforeach(projectpath)

	add_custom_target(GenerateScreenshotReferenceData DEPENDS ${RACO_SCREENSHOT_REF_FILES})

	add_screenshot_test(simple_broken ${CMAKE_CURRENT_SOURCE_DIR}/projects/simple_broken.rca)
	set_tests_properties(screenshot_simple_broken PROPERTIES WILL_FAIL True)
endif()
