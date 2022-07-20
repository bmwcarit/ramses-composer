#[[

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

]]

raco_find_qt_components(Widgets)

add_library(libNodeLogic
    include/node_logic/NodeLogic.h src/NodeLogic.cpp
    )

target_include_directories(libNodeLogic
    PUBLIC
    include/
)
enable_warnings_as_errors(libNodeLogic)

set_target_properties(libNodeLogic PROPERTIES AUTOMOC TRUE)
set_target_properties(libNodeLogic PROPERTIES AUTORCC TRUE)
set_target_properties(libNodeLogic PROPERTIES AUTOUIC TRUE)

target_link_libraries(libNodeLogic
    PUBLIC
        raco::Core
        raco::UserTypes
        raco::Components
        raco::GuiData
        raco::signal
        Qt5::Widgets
        Qt5::Core
        Qt5::Gui
    PRIVATE
        raco::LogSystem
)

add_library(raco::node_logic ALIAS libNodeLogic)

