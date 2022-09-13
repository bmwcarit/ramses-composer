#[[

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

]]

raco_find_qt_components(Widgets)

add_library(libVisualCurve
    include/visual_curve/VisualCurveWidget.h src/VisualCurveWidget.cpp
    include/visual_curve/VisualCurveScrollArea.h src/VisualCurveScrollArea.cpp
    include/visual_curve/VisualCurveNodeDelegate.h src/VisualCurveNodeDelegate.cpp
    include/visual_curve/VisualCurveNodeTreeView.h src/VisualCurveNodeTreeView.cpp
    include/visual_curve/VisualCurveInfoWidget.h src/VisualCurveInfoWidget.cpp
    include/visual_curve/VisualCurvePosManager.h src/VisualCurvePosManager.cpp
    )

target_include_directories(libVisualCurve
    PUBLIC
    include/
    ../../third_party/glm/
)
enable_warnings_as_errors(libVisualCurve)

set_target_properties(libVisualCurve PROPERTIES AUTOMOC TRUE)
set_target_properties(libVisualCurve PROPERTIES AUTORCC TRUE)
set_target_properties(libVisualCurve PROPERTIES AUTOUIC TRUE)

target_link_libraries(libVisualCurve
    PUBLIC
        raco::Core
        raco::Style
        raco::UserTypes
        raco::Components
        raco::Curve
        raco::GuiData
        raco::node_logic
        raco::signal
        raco::CommonEditors
        raco::TimeAxis
        Qt5::Widgets
        Qt5::Core
        Qt5::Gui
    PRIVATE
        raco::LogSystem
)

add_library(raco::VisualCurve ALIAS libVisualCurve)

