#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

raco_find_qt_components(Core)

add_library(libSerialization
    include/serialization/Serialization.h
    include/serialization/SerializationFunctions.h
    include/serialization/SerializationKeys.h    
    src/Serialization.cpp
)
target_include_directories(libSerialization PUBLIC include/)
target_link_libraries(libSerialization
PUBLIC
    raco::DataStorage
    Qt5::Core
    raco::LogSystem
PRIVATE
    raco::Utils
)

enable_warnings_as_errors(libSerialization)

add_library(raco::Serialization ALIAS libSerialization)
add_subdirectory(tests/)
