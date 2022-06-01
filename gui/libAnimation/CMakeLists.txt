#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]
set(LIB_NAME libAnimation)

raco_find_qt_components(Widgets)

add_library(libAnimation
    include/animation/AnimationMainWindow.h src/AnimationMainWindow.cpp
    include/animation/AnimationView.h src/AnimationView.cpp
    include/animation/AnimationTitle.h src/AnimationTitle.cpp
    include/animation/controls/ExpandButton.h src/controls/ExpandButton.cpp
    include/animation/controls/TagContainerEditor.h src/controls/TagContainerEditor.cpp
)

target_include_directories(libAnimation
    PUBLIC
        include/
)

enable_warnings_as_errors(libAnimation)

set_target_properties(libAnimation PROPERTIES AUTOMOC TRUE)


target_link_libraries(libAnimation
    PUBLIC
        raco::CommonWidgets
        raco::Components
        raco::Core
        raco::Style
        raco::GuiData
        raco::signal
        Qt5::Widgets
       # raco::TimeAxis
        qtadvanceddocking
)

add_library(raco::Animation ALIAS libAnimation)

