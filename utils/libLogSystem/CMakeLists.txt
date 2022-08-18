#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

add_library(libLogSystem
include/log_system/log.h src/log.cpp
)

target_include_directories(libLogSystem
PUBLIC
    include/
)

target_link_libraries(libLogSystem
PUBLIC
    spdlog
)

enable_warnings_as_errors(libLogSystem)

add_library(raco::LogSystem ALIAS libLogSystem)
