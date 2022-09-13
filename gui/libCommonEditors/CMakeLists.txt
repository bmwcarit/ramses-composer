#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

raco_find_qt_components(Widgets)

add_library(libCommonEditors
    include/common_editors/DoubleEditor.h src/DoubleEditor.cpp
    include/common_editors/Int64Editor.h src/Int64Editor.cpp
)

target_include_directories(libCommonEditors
    PUBLIC
        include/
)
enable_warnings_as_errors(libCommonEditors)

set_target_properties(libCommonEditors PROPERTIES AUTOMOC TRUE)
set_target_properties(libCommonEditors PROPERTIES AUTORCC TRUE)
set_target_properties(libCommonEditors PROPERTIES AUTOUIC TRUE)

target_link_libraries(libCommonEditors
    PUBLIC
        Qt5::Widgets
        raco::Components
        raco::ApplicationLib
        raco::Style
        raco::PropertyBrowser
    PRIVATE
        raco::LogSystem
)
add_library(raco::CommonEditors ALIAS libCommonEditors)

