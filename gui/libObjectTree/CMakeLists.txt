#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]
set(LIB_NAME libObjectTree)

raco_find_qt_components(Widgets)

add_library(libObjectTree 
    include/object_tree_view/ObjectTreeDock.h src/object_tree_view/ObjectTreeDock.cpp
    include/object_tree_view/ObjectTreeDockManager.h src/object_tree_view/ObjectTreeDockManager.cpp
    include/object_tree_view/ObjectTreeView.h src/object_tree_view/ObjectTreeView.cpp
    include/object_tree_view_model/ObjectTreeNode.h src/object_tree_view_model/ObjectTreeNode.cpp
    include/object_tree_view_model/ObjectTreeViewDefaultModel.h src/object_tree_view_model/ObjectTreeViewDefaultModel.cpp
    include/object_tree_view_model/ObjectTreeViewResourceModel.h
    src/object_tree_view_model/ObjectTreeViewResourceModel.cpp
    include/object_tree_view_model/ObjectTreeViewExternalProjectModel.h src/object_tree_view_model/ObjectTreeViewExternalProjectModel.cpp
    include/object_tree_view_model/ObjectTreeViewPrefabModel.h src/object_tree_view_model/ObjectTreeViewPrefabModel.cpp
    include/object_tree_view_model/ObjectTreeViewTopLevelSortProxyModel.h src/object_tree_view_model/ObjectTreeViewTopLevelSortProxyModel.cpp
)

target_include_directories(libObjectTree
    PUBLIC
        include/
)

enable_warnings_as_errors(libObjectTree)

set_target_properties(libObjectTree PROPERTIES AUTOMOC TRUE)
set_target_properties(libObjectTree PROPERTIES AUTORCC TRUE)
set_target_properties(libObjectTree PROPERTIES AUTOUIC TRUE)

target_link_libraries(libObjectTree
    PUBLIC
        raco::CommonWidgets
        raco::Components
        raco::Core
        raco::Style
        Qt5::Widgets
        qtadvanceddocking
)

add_library(raco::ObjectTree ALIAS libObjectTree)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()
