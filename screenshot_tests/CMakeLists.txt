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

macro(add_screenshot_test TESTNAME) 
	add_test(
		NAME "screenshot_${TESTNAME}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -D RACO_EXE=${RACO_EXE} -D VIEWER_EXE=${VIEWER_EXE} -D TEST_NAME=${TESTNAME} -D PY_EXE=${PYTHON_DEPLOYMENT_FOLDER}/python.exe -D PY_CMD=compare_screenshots.py  -P "${CMAKE_CURRENT_SOURCE_DIR}/screenshot_test.cmake"
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

	foreach(projectname ${RACO_SCREENSHOT_PROJECTS})
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/expected/${projectname}.png
			MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/projects/${projectname}.rca
			DEPENDS ramses-logic-viewer RaCoEditor
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/exported/"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/expected/"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/actual/"
			COMMAND ${RACO_EXE} -p ${CMAKE_CURRENT_SOURCE_DIR}/projects/${projectname}.rca -e ${CMAKE_CURRENT_SOURCE_DIR}/exported/${projectname} -l 3
			COMMAND ${VIEWER_EXE} "${CMAKE_CURRENT_SOURCE_DIR}/exported/${projectname}.ramses" "${CMAKE_CURRENT_SOURCE_DIR}/exported/${projectname}.rlogic" --exec-lua "rlogic.screenshot('expected/${projectname}.png')"
		)

		LIST(APPEND RACO_SCREENSHOT_REF_FILES "${CMAKE_CURRENT_SOURCE_DIR}/expected/${projectname}.png")
		add_screenshot_test(${projectname})
	endforeach(projectname)

	add_custom_target(GenerateScreenshotReferenceData DEPENDS ${RACO_SCREENSHOT_REF_FILES})

	add_screenshot_test(simple_broken)
	set_tests_properties(screenshot_simple_broken PROPERTIES WILL_FAIL True)
endif()
