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

add_racocommand_test(RaCoHeadless_load_future_feature_level "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/future-feature-level.rca")
set_tests_properties(RaCoHeadless_load_future_feature_level PROPERTIES WILL_FAIL True)

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

add_racocommand_test(RaCoHeadless_export_lua_saving_mode_source_code_success "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/example_scene.rca" "-e" "lua_saving_mode_source" "-s" "source_code")
add_racocommand_test(RaCoHeadless_export_lua_saving_mode_byte_code_success "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/example_scene.rca" "-e" "lua_saving_mode_byte" "-s" "byte_code")
add_racocommand_test(RaCoHeadless_export_lua_saving_mode_source_and_byte_code_success "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/example_scene.rca" "-e" "lua_saving_mode_source_byte" "-s" "source_and_byte_code"
)
add_racocommand_test(RaCoHeadless_export_lua_saving_mode_long_option "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/example_scene.rca" "-e" "lua_saving_mode_long_option_source" "--luasavingmode" "source_code")
add_racocommand_test(RaCoHeadless_export_lua_saving_mode_fail "${CMAKE_CURRENT_BINARY_DIR}" "-p" "${CMAKE_SOURCE_DIR}/resources/example_scene.rca" "-e" "lua_saving_mode_fail" "-s" "invalid_setting")
set_tests_properties(RaCoHeadless_export_lua_saving_mode_fail PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_export_raco_warn "${CMAKE_CURRENT_BINARY_DIR}" -p "${CMAKE_SOURCE_DIR}/resources/export-raco-warning-ramses-ok.rca" -e "export-raco-warning-ramses-ok")
add_racocommand_test(RaCoHeadless_export_raco_error "${CMAKE_CURRENT_BINARY_DIR}" -p "${CMAKE_SOURCE_DIR}/resources/export-raco-error-ramses-ok.rca" -e "export-raco-error-ramses-ok")
set_tests_properties(RaCoHeadless_export_raco_error PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_export_ramses_warn "${CMAKE_CURRENT_BINARY_DIR}" -p "${CMAKE_SOURCE_DIR}/resources/export-raco-ok-ramses-warning.rca" -e "export-raco-ok-ramses-warning")
add_racocommand_test(RaCoHeadless_export_ramses_error "${CMAKE_CURRENT_BINARY_DIR}" -p "${CMAKE_SOURCE_DIR}/resources/export-raco-ok-ramses-error.rca" -e "export-raco-ok-ramses-error")
set_tests_properties(RaCoHeadless_export_ramses_error PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_export_strict_raco_warn "${CMAKE_CURRENT_BINARY_DIR}" -p "${CMAKE_SOURCE_DIR}/resources/export-raco-warning-ramses-ok.rca" -e "export-raco-warning-ramses-ok" -w)
add_racocommand_test(RaCoHeadless_export_strict_raco_error "${CMAKE_CURRENT_BINARY_DIR}" -p "${CMAKE_SOURCE_DIR}/resources/export-raco-error-ramses-ok.rca" -e "export-raco-error-ramses-ok" -w)
set_tests_properties(RaCoHeadless_export_strict_raco_error PROPERTIES WILL_FAIL True)

add_racocommand_test(RaCoHeadless_export_strict_ramses_warn "${CMAKE_CURRENT_BINARY_DIR}" -p "${CMAKE_SOURCE_DIR}/resources/export-raco-ok-ramses-warning.rca" -e "export-raco-ok-ramses-warning" -w)
set_tests_properties(RaCoHeadless_export_strict_ramses_warn PROPERTIES WILL_FAIL True)
add_racocommand_test(RaCoHeadless_export_strict_ramses_error "${CMAKE_CURRENT_BINARY_DIR}" -p "${CMAKE_SOURCE_DIR}/resources/export-raco-ok-ramses-error.rca" -e "export-raco-ok-ramses-error" -w)
set_tests_properties(RaCoHeadless_export_strict_ramses_error PROPERTIES WILL_FAIL True)
