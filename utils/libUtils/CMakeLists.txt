#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

add_library(libUtils
    include/utils/CrashDump.h src/CrashDump.cpp
    include/utils/FileUtils.h src/FileUtils.cpp
    include/utils/MathUtils.h src/MathUtils.cpp
    include/utils/ShaderPreprocessor.h src/ShaderPreprocessor.cpp
    include/utils/u8path.h src/u8path.cpp
    include/utils/ZipUtils.h src/ZipUtils.cpp
)
target_include_directories(libUtils PUBLIC include/)
enable_warnings_as_errors(libUtils)

target_compile_definitions(libUtils PUBLIC -DRACO_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
target_compile_definitions(libUtils PUBLIC -DRACO_VERSION_MINOR=${PROJECT_VERSION_MINOR})
target_compile_definitions(libUtils PUBLIC -DRACO_VERSION_PATCH=${PROJECT_VERSION_PATCH})

target_link_libraries(libUtils
PUBLIC
    spdlog
PRIVATE
    glm
    zip
)
add_library(raco::Utils ALIAS libUtils)

if(PACKAGE_TESTS)
	add_subdirectory(tests)
endif()