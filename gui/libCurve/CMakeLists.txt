#[[

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

]]

raco_find_qt_components(Widgets)

add_library(libCurve
    include/curve/CurveWindow.h src/CurveWindow.cpp
    include/curve/CurveTree.h src/CurveTree.cpp
    include/curve/PointPropertyView.h src/PointPropertyView.cpp
    include/curve/CurveNameWidget.h src/CurveNameWidget.cpp
    include/curve/CurveLogic.h src/CurveLogic.cpp
    )

target_include_directories(libCurve
    PUBLIC
    include/
)
enable_warnings_as_errors(libCurve)

set_target_properties(libCurve PROPERTIES AUTOMOC TRUE)
set_target_properties(libCurve PROPERTIES AUTORCC TRUE)
set_target_properties(libCurve PROPERTIES AUTOUIC TRUE)

target_link_libraries(libCurve
    PUBLIC
        raco::Core
        raco::UserTypes
        raco::Components
        raco::GuiData
        raco::node_logic
        raco::signal
        Qt5::Widgets
        Qt5::Core
        Qt5::Gui
    PRIVATE
        raco::LogSystem
)

add_library(raco::Curve ALIAS libCurve)

