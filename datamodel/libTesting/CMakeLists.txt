#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

add_library(libTesting INTERFACE
	include/testing/RacoBaseTest.h
	include/testing/RaCoApplicationTest.h
	include/testing/StructTypes.h
	include/testing/TestEnvironmentCore.h
	include/testing/TestUtil.h
	include/testing/MockUserTypes.h
)
target_include_directories(libTesting INTERFACE include/) 
target_link_libraries(libTesting INTERFACE libRamsesBase)
set_target_properties(libTesting PROPERTIES FOLDER tests)

add_library(raco::Testing ALIAS libTesting)
