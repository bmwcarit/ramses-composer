#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

raco_find_qt_components(Widgets)

add_library(libRamsesWidgets
    include/ramses_widgets/BuildOptions.h
    include/ramses_widgets/grid_vert.h
    include/ramses_widgets/grid_frag.h
    include/ramses_widgets/PreviewContentWidget.h src/PreviewContentWidget.cpp
    include/ramses_widgets/PreviewFramebufferScene.h src/PreviewFramebufferScene.cpp
    include/ramses_widgets/PreviewMainWindow.h src/PreviewMainWindow.cpp
    include/ramses_widgets/PreviewScrollAreaWidget.h src/PreviewScrollAreaWidget.cpp
    include/ramses_widgets/RamsesPreviewWindow.h src/RamsesPreviewWindow.cpp
    include/ramses_widgets/RendererBackend.h src/RendererBackend.cpp
    include/ramses_widgets/SceneStateEventHandler.h src/SceneStateEventHandler.cpp
    include/ramses_widgets/PreviewBackgroundScene.h src/PreviewBackgroundScene.cpp
    src/PreviewMainWindow.ui
)

target_include_directories(libRamsesWidgets
    PUBLIC
        include/
)
enable_warnings_as_errors(libRamsesWidgets)

set_target_properties(libRamsesWidgets PROPERTIES AUTOMOC TRUE)
set_target_properties(libRamsesWidgets PROPERTIES AUTORCC TRUE)
set_target_properties(libRamsesWidgets PROPERTIES AUTOUIC TRUE)

target_link_libraries(libRamsesWidgets
    PUBLIC
        raco::RamsesBase
        raco::LogSystem
        raco::UserTypes
        Qt5::Widgets
    PRIVATE
        raco::ramses-logic-lib
        raco::ramses-lib
)

add_library(raco::RamsesWidgets ALIAS libRamsesWidgets)