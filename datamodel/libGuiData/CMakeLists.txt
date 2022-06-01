#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

add_library(libGuiData
    include/CurveData/CurveData.h src/CurveData.cpp
    include/CurveData/CurveManager.h src/CurveManager.cpp
    include/NodeData/nodeManager.h src/nodeManager.cpp
    include/NodeData/nodeDataEx.h src/nodeDataEx.cpp
    include/AnimationData/animationData.h src/animationData.cpp
    include/PropertyData/PropertyData.h src/PropertyData.cpp
    include/MaterialData/materialData.h  src/materialData.cpp
    include/MaterialData/materialManager.h  src/materialManager.cpp
    include/PropertyData/PropertyType.h
)

target_include_directories(libGuiData PUBLIC include)
enable_warnings_as_errors(libGuiData)


target_link_libraries(libGuiData
PUBLIC
        raco::Core
PRIVATE
        raco::LogSystem
)


add_library(raco::GuiData ALIAS libGuiData)
