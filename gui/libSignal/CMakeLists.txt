#[[

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

]]

raco_find_qt_components(Widgets)

add_library(libSignal
    include/signal/SignalProxy.h src/SignalProxy.cpp
    )

target_include_directories(libSignal
    PUBLIC
    include/
)
enable_warnings_as_errors(libSignal)

set_target_properties(libSignal PROPERTIES AUTOMOC TRUE)
set_target_properties(libSignal PROPERTIES AUTORCC TRUE)
set_target_properties(libSignal PROPERTIES AUTOUIC TRUE)

target_link_libraries(libSignal
    PUBLIC
        raco::Core
        raco::UserTypes
        raco::Components
        #raco::TimeAxis
        raco::GuiData
        raco::PropertyBrowser
        Qt5::Widgets
        Qt5::Core
        Qt5::Gui
    PRIVATE
        raco::LogSystem
)

add_library(raco::signal ALIAS libSignal)

