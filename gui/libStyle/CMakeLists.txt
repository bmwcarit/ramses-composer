#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

raco_find_qt_components(Widgets)

add_library(libStyle 
    include/style/Colors.h src/Colors.cpp
    include/style/Icons.h src/Icons.cpp
    include/style/QStyleFormatter.h
	include/style/RaCoStyle.h src/RaCoStyle.cpp
)

target_include_directories(libStyle
    PUBLIC
        include/
)
enable_warnings_as_errors(libStyle)

set_target_properties(libStyle PROPERTIES AUTOMOC TRUE)
set_target_properties(libStyle PROPERTIES AUTORCC TRUE)
set_target_properties(libStyle PROPERTIES AUTOUIC TRUE)

target_link_libraries(libStyle
    PUBLIC
        Qt5::Widgets
    PRIVATE
        raco::LogSystem
        raco::Core
)
add_library(raco::Style ALIAS libStyle)
