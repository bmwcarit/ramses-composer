#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

add_racocommand_test(RaCoHeadless_start "${RACO_RELEASE_DIRECTORY}/bin/$<CONFIG>")

add_racocommand_test(RaCoHeadless_export_no_such_dir "${CMAKE_CURRENT_BINARY_DIR}" "-e" "no_such_dir/no_such_file")
set_tests_properties(RaCoHeadless_export_no_such_dir PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_load_wrong_extension "${CMAKE_CURRENT_BINARY_DIR}" "-p" "no_such_file.abc")
set_tests_properties(RaCoHeadless_load_wrong_extension PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_load_no_such_file "${CMAKE_CURRENT_BINARY_DIR}" "-p" "no_such_file.rca")
set_tests_properties(RaCoHeadless_load_no_such_file PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_load_future_version "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/future-version.rca")
set_tests_properties(RaCoHeadless_load_future_version PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_load_no_json "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/no-json.rca")
set_tests_properties(RaCoHeadless_load_no_json PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_load_invalid_json "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/json-but-invalid-raco.rca")
set_tests_properties(RaCoHeadless_load_invalid_json PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_load_too_short "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/too-short.rca")
set_tests_properties(RaCoHeadless_load_too_short PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_load_fake_zip "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/fake-zip.rca")
set_tests_properties(RaCoHeadless_load_fake_zip PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_load_multi_file_zip "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/multi-file-zip.rca")
set_tests_properties(RaCoHeadless_load_multi_file_zip PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_load_success "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/example_scene.rca")
