#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

macro(add_python_test TESTNAME)
	add_racocommand_test("PyAPI_${TESTNAME}" "${CMAKE_CURRENT_SOURCE_DIR}" -r run_test.py "${TESTNAME}")
endmacro()

add_python_test(pyt_general)

add_python_test(pyt_animation)

add_python_test(exit_code)
set_tests_properties(PyAPI_exit_code PROPERTIES WILL_FAIL True)

add_python_test(python_exception)
set_tests_properties(PyAPI_python_exception PROPERTIES WILL_FAIL True)

add_python_test(no_such_script)
set_tests_properties(PyAPI_no_such_script PROPERTIES WILL_FAIL True)

add_python_test(use_pip)
set_tests_properties(PyAPI_use_pip PROPERTIES ENVIRONMENT PYTHON_DEPLOYMENT_FOLDER=${HEADLESS_PYTHON_DEPLOYMENT_FOLDER})

add_racocommand_test("PyAPI_run_print_object_version_current" "${CMAKE_CURRENT_SOURCE_DIR}" -r ../resources/python/print_objects.py -p ../datamodel/libCore/tests/migrationTestData/version-current.rca)

add_racocommand_test("PyAPI_run_print_scenegraph_version_current" "${CMAKE_CURRENT_SOURCE_DIR}" -r ../resources/python/print_scenegraph.py -p ../datamodel/libCore/tests/migrationTestData/version-current.rca)

add_racocommand_test("PyAPI_module_import_with_path" "${CMAKE_CURRENT_SOURCE_DIR}" -r module_import.py -y modules/)

add_racocommand_test("PyAPI_module_import_without_path" "${CMAKE_CURRENT_SOURCE_DIR}" -r module_import.py)
set_tests_properties(PyAPI_module_import_without_path PROPERTIES WILL_FAIL True)