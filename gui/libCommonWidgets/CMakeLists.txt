#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

raco_find_qt_components(Widgets)

add_library(libCommonWidgets
    include/common_widgets/DebugLayout.h
    include/common_widgets/ErrorView.h src/ErrorView.cpp
    include/common_widgets/ExportDialog.h src/ExportDialog.cpp
    include/common_widgets/log_model/LogViewModel.h src/log_model/LogViewModel.cpp
    include/common_widgets/log_model/LogViewSink.h src/log_model/LogViewSink.cpp
    include/common_widgets/log_model/LogViewSortFilterProxyModel.h src/log_model/LogViewSortFilterProxyModel.cpp
    include/common_widgets/LogView.h src/LogView.cpp
    include/common_widgets/MeshAssetImportDialog.h src/MeshAssetImportDialog.cpp
    include/common_widgets/NoContentMarginsLayout.h
    include/common_widgets/PreferencesView.h src/PreferencesView.cpp
    include/common_widgets/PropertyBrowserButton.h src/PropertyBrowserButton.cpp
    include/common_widgets/QtGuiFormatter.h
    include/common_widgets/RaCoClipboard.h src/RaCoClipboard.cpp
    include/common_widgets/RunScriptDialog.h src/RunScriptDialog.cpp
    include/common_widgets/TimingsWidget.h
    include/common_widgets/TracePlayerWidget.h src/TracePlayerWidget.cpp
    include/common_widgets/UndoView.h src/UndoView.cpp
)

target_include_directories(libCommonWidgets
    PUBLIC
        include/
)
enable_warnings_as_errors(libCommonWidgets)

set_target_properties(libCommonWidgets PROPERTIES AUTOMOC TRUE)
set_target_properties(libCommonWidgets PROPERTIES AUTORCC TRUE)
set_target_properties(libCommonWidgets PROPERTIES AUTOUIC TRUE)

target_link_libraries(libCommonWidgets
    PUBLIC
        Qt5::Widgets
        raco::Components
        raco::ApplicationLib
        raco::Style
    PRIVATE
        raco::LogSystem
        raco::Style
)
add_library(raco::CommonWidgets ALIAS libCommonWidgets)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()
