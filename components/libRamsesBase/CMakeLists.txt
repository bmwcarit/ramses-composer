#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

raco_find_qt_components(Core)

add_library(libLodepng
    ../../third_party/ramses-logic/external/ramses/external/lodepng/lodepng.h
    ../../third_party/ramses-logic/external/ramses/external/lodepng/lodepng.cpp
)

set_target_properties(libLodepng PROPERTIES FOLDER third_party/libLodepng)

add_library(libRamsesBase
    include/ramses_base/BaseEngineBackend.h src/ramses_base/BaseEngineBackend.cpp
    include/ramses_base/BuildOptions.h
    include/ramses_base/CoreInterfaceImpl.h src/ramses_base/CoreInterfaceImpl.cpp
    include/ramses_base/HeadlessEngineBackend.h src/ramses_base/HeadlessEngineBackend.cpp
    include/ramses_base/LogicEngine.h
    include/ramses_base/RamsesHandles.h
    include/ramses_base/RamsesFormatter.h
    include/ramses_base/Utils.h src/ramses_base/Utils.cpp
    src/ramses_base/EnumerationDescriptions.h

    include/ramses_adaptor/AnchorPointAdaptor.h src/ramses_adaptor/AnchorPointAdaptor.cpp
    include/ramses_adaptor/AnimationAdaptor.h src/ramses_adaptor/AnimationAdaptor.cpp
    include/ramses_adaptor/AnimationChannelAdaptor.h src/ramses_adaptor/AnimationChannelAdaptor.cpp
    include/ramses_adaptor/BaseCameraAdaptorHelpers.h src/ramses_adaptor/BaseCameraAdaptorHelpers.cpp
    include/ramses_adaptor/BuildOptions.h
    include/ramses_adaptor/BlitPassAdaptor.h src/ramses_adaptor/BlitPassAdaptor.cpp
    include/ramses_adaptor/CubeMapAdaptor.h src/ramses_adaptor/CubeMapAdaptor.cpp
    include/ramses_adaptor/Factories.h src/ramses_adaptor/Factories.cpp
    include/ramses_adaptor/LinkAdaptor.h src/ramses_adaptor/LinkAdaptor.cpp
    include/ramses_adaptor/LuaInterfaceAdaptor.h src/ramses_adaptor/LuaInterfaceAdaptor.cpp
    include/ramses_adaptor/LuaScriptAdaptor.h src/ramses_adaptor/LuaScriptAdaptor.cpp
    include/ramses_adaptor/LuaScriptModuleAdaptor.h src/ramses_adaptor/LuaScriptModuleAdaptor.cpp
    include/ramses_adaptor/MaterialAdaptor.h src/ramses_adaptor/MaterialAdaptor.cpp
    include/ramses_adaptor/MeshAdaptor.h src/ramses_adaptor/MeshAdaptor.cpp
    include/ramses_adaptor/MeshNodeAdaptor.h src/ramses_adaptor/MeshNodeAdaptor.cpp
    include/ramses_adaptor/NodeAdaptor.h src/ramses_adaptor/NodeAdaptor.cpp
    include/ramses_adaptor/ObjectAdaptor.h src/ramses_adaptor/ObjectAdaptor.cpp
    include/ramses_adaptor/OrthographicCameraAdaptor.h src/ramses_adaptor/OrthographicCameraAdaptor.cpp
    include/ramses_adaptor/PerspectiveCameraAdaptor.h src/ramses_adaptor/PerspectiveCameraAdaptor.cpp
    include/ramses_adaptor/RenderBufferAdaptor.h src/ramses_adaptor/RenderBufferAdaptor.cpp
    include/ramses_adaptor/RenderBufferMSAdaptor.h src/ramses_adaptor/RenderBufferMSAdaptor.cpp
    include/ramses_adaptor/RenderLayerAdaptor.h src/ramses_adaptor/RenderLayerAdaptor.cpp
    include/ramses_adaptor/RenderPassAdaptor.h src/ramses_adaptor/RenderPassAdaptor.cpp
    include/ramses_adaptor/RenderTargetAdaptor.h src/ramses_adaptor/RenderTargetAdaptor.cpp
    include/ramses_adaptor/SceneAdaptor.h src/ramses_adaptor/SceneAdaptor.cpp
    include/ramses_adaptor/SkinAdaptor.h src/ramses_adaptor/SkinAdaptor.cpp
    include/ramses_adaptor/SceneBackend.h src/ramses_adaptor/SceneBackend.cpp
    include/ramses_adaptor/TextureExternalAdaptor.h src/ramses_adaptor/TextureExternalAdaptor.cpp
    include/ramses_adaptor/TextureSamplerAdaptor.h src/ramses_adaptor/TextureSamplerAdaptor.cpp
    include/ramses_adaptor/TimerAdaptor.h src/ramses_adaptor/TimerAdaptor.cpp
    include/ramses_adaptor/utilities.h src/ramses_adaptor/utilities.cpp
)

target_include_directories(libRamsesBase
    PUBLIC
        include/
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ramses-logic/external/ramses/external/lodepng>
)
enable_warnings_as_errors(libRamsesBase)

# At this point we don't know yet if we will link against ramses-client-only or ramses with renderer
# thus we link against the ramses interfaces and ramses-logic-static which also links against ramses interfaces (see CMakeLists.txt in third_party folder)
# The actuall dynamic ramses lib is linked by the headless executable (client-only) and preview widget (with renderer).
target_link_libraries(libRamsesBase
PUBLIC
    ramses-client-api
    ramses-framework-api
    ramses-logic-api
    raco::UserTypes
    raco::Components
PRIVATE
    raco::Utils
    raco::LogSystem
    libLodepng
)
add_library(raco::RamsesBase ALIAS libRamsesBase)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()