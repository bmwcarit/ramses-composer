#[[

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

]]

raco_find_qt_components(Widgets)

add_library(libDataConvert
    include/data_Convert/ProgramDefine.h
    include/data_Convert/ProgramManager.h src/ProgramManager.cpp
    )

target_include_directories(libDataConvert
    PUBLIC
    include/
)
enable_warnings_as_errors(libDataConvert)

set_target_properties(libDataConvert PROPERTIES AUTOMOC TRUE)
set_target_properties(libDataConvert PROPERTIES AUTORCC TRUE)
set_target_properties(libDataConvert PROPERTIES AUTOUIC TRUE)

target_link_libraries(libDataConvert
    PUBLIC
        raco::Core
        raco::UserTypes
        raco::Components
        raco::TimeAxis
        raco::GuiData
        raco::PropertyBrowser
        Qt5::Widgets
        Qt5::Core
        Qt5::Gui
    PRIVATE
        raco::LogSystem
)

add_library(raco::dataConvert ALIAS libDataConvert)

