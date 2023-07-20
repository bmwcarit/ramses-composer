#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Adding the unit test with gtest using our macro from dsathe top level CMakeLists.txt file

set(TEST_SOURCES
    PropertyBrowserItem_test.cpp

    BoolEditor_test.cpp
	DoubleEditor_test.cpp
	EditorTestFixture.h
    EnumerationEditor_test.cpp
    IntEditor_test.cpp
	LinkEditor_test.cpp
    PrimitiveEditorsDataChange_test.cpp
    RefEditor_test.cpp
    SpinBox_test.cpp
    URIEditor_test.cpp
	VecNTEditor_test.cpp
    TagContainerEditor_test.cpp
)
set(TEST_LIBRARIES
    raco::PropertyBrowser
    raco::ApplicationLib
    raco::RamsesBase
    raco::Style
    raco::Testing
    Qt5::Test
)
raco_package_add_gui_test(
    libPropertyBrowser_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}
)

raco_package_add_test_resources(
    libPropertyBrowser_test "${CMAKE_SOURCE_DIR}/resources"
    meshes/Duck.glb
	scripts/types-scalar.lua
    scripts/interface-scalar-types.lua
)
