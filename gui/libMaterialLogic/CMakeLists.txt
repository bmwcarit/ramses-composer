#[[

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

]]

raco_find_qt_components(Widgets)

add_library(libMaterialLogic
    include/material_logic/materalLogic.h src/materalLogic.cpp
    )

target_include_directories(libMaterialLogic
    PUBLIC
    include/
)
enable_warnings_as_errors(libMaterialLogic)

set_target_properties(libMaterialLogic PROPERTIES AUTOMOC TRUE)
set_target_properties(libMaterialLogic PROPERTIES AUTORCC TRUE)
set_target_properties(libMaterialLogic PROPERTIES AUTOUIC TRUE)

target_link_libraries(libMaterialLogic
    PUBLIC
        raco::Core
        raco::UserTypes
        raco::Components
        raco::TimeAxis
        raco::GuiData
        raco::PropertyBrowser
        raco::signal
        Qt5::Widgets
        Qt5::Core
        Qt5::Gui
    PRIVATE
        raco::LogSystem
)

add_library(raco::material_logic ALIAS libMaterialLogic)

