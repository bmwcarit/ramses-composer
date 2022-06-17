#[[

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

]]

raco_find_qt_components(Widgets)

add_library(libDataConvert
    include/data_Convert/ProgramDefine.h
    include/data_Convert/ProgramManager.h src/ProgramManager.cpp
    include/proto/Common.pb.h src/Common.pb.cpp
    include/proto/HmiBase.pb.h src/HmiBase.pb.cpp
    include/proto/HmiWidget.pb.h src/HmiWidget.pb.cpp
    include/proto/Numeric.pb.h src/Numeric.pb.cpp
    include/proto/Scenegraph.pb.h src/Scenegraph.pb.cpp
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
        protobuf::libprotobuf-lite
        protobuf::libprotobuf
        protobuf::libprotoc
        raco::LogSystem
)

add_library(raco::dataConvert ALIAS libDataConvert)

