#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]
set(LIB_NAME libProperty)

raco_find_qt_components(Widgets)

add_library(libProperty 
    include/property/PropertyMainWindow.h src/PropertyMainWindow.cpp
    include/property/PropertyTableView.h src/PropertyTableView.cpp
    include/property/PropertyTitleBar.h src/PropertyTitleBar.cpp
    include/property/PropertyTableDelegate.h src/PropertyTableDelegate.cpp
)

target_include_directories(libProperty
    PUBLIC
        include/
)

enable_warnings_as_errors(libProperty)

set_target_properties(libProperty PROPERTIES AUTOMOC TRUE)


target_link_libraries(libProperty
    PUBLIC
        raco::CommonWidgets
        raco::Components
        raco::Core
        raco::Style
        raco::GuiData
        raco::signal
        Qt5::Widgets
        qtadvanceddocking
)

add_library(raco::Property ALIAS libProperty)

