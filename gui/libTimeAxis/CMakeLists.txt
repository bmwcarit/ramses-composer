#[[

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

]]

raco_find_qt_components(Widgets)

add_library(libTimeAxis
    include/time_axis/TimeAxisMainWindow.h src/TimeAxisMainWindow.cpp
    include/time_axis/TimeAxisWidget.h src/TimeAxisWidget.cpp
    include/time_axis/TimeAxisScrollArea.h src/TimeAxisScrollArea.cpp
    include/time_axis/AnimationEditorView.h src/AnimationEditorView.cpp
    include/time_axis/TimeAxisCommon.h
    include/time_axis/KeyFrameManager.h src/KeyFrameManager.cpp
#    include/time_axis/VisualCurveWidget.h src/VisualCurveWidget.cpp
#    include/time_axis/VisualCurveScrollArea.h src/VisualCurveScrollArea.cpp
    )

target_include_directories(libTimeAxis
    PUBLIC
    include/
    ../../third_party/glm/
)
enable_warnings_as_errors(libTimeAxis)

set_target_properties(libTimeAxis PROPERTIES AUTOMOC TRUE)
set_target_properties(libTimeAxis PROPERTIES AUTORCC TRUE)
set_target_properties(libTimeAxis PROPERTIES AUTOUIC TRUE)

target_link_libraries(libTimeAxis
    PUBLIC
        raco::Core
        raco::Style
        raco::UserTypes
        raco::Components
        raco::Curve
        raco::GuiData
        raco::node_logic
        raco::signal
        Qt5::Widgets
        Qt5::Core
        Qt5::Gui
    PRIVATE
        raco::LogSystem
)

add_library(raco::TimeAxis ALIAS libTimeAxis)

