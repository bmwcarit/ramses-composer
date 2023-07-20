#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]
set(TEST_SOURCES
    Value_test.cpp
    Property_test.cpp
)

set(TEST_LIBRARIES
    raco::DataStorage
    raco::Testing
)

raco_package_add_test(
    libDataStorage_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(libDataStorage_test raco::ramses-logic-lib-client-only)
