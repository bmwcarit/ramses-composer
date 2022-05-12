#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

macro(add_python_test TESTNAME)
	add_racocommand_test("PyAPI_${TESTNAME}" "${CMAKE_CURRENT_SOURCE_DIR}" -r run_test.py "${TESTNAME}")
endmacro()

add_python_test(pyt_general)

add_python_test(exit_code)
set_tests_properties(PyAPI_exit_code PROPERTIES WILL_FAIL True)

add_python_test(python_exception)
set_tests_properties(PyAPI_python_exception PROPERTIES WILL_FAIL True)

add_python_test(no_such_script)
set_tests_properties(PyAPI_no_such_script PROPERTIES WILL_FAIL True)

add_python_test(use_pip)
