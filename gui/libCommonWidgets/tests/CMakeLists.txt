#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]
set(TEST_SOURCES 
    ErrorView_test.h ErrorView_test.cpp
)

set(TEST_LIBRARIES
    raco::CommonWidgets
    raco::ApplicationLib
    raco::RamsesBase
    raco::Testing
)

raco_package_add_gui_test(
    libCommonWidgets_test 
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)
