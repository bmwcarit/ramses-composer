#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# This test covers the links to the source code repository using the substitution mechanism
add_test(
    NAME "doc_link_check_substituted"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${Python_EXECUTABLE} link_check.py
)

add_test(
    NAME "doc_test_link_checker"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${Python_EXECUTABLE} link_check_test.py
)

find_program(SPHINX_COMMAND sphinx-build)

if (EXISTS ${SPHINX_COMMAND})
    # This test covers both internal relative links to files as well as external absolute links
    add_test(
        NAME "doc_link_check_verbatim"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${SPHINX_COMMAND} -T -E -W --keep-going -D "linkcheck_ignore=https://github.com/bmwcarit/ramses-composer/.*" -b linkcheck ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/doc
    )

    file(GLOB_RECURSE
        SPHINX_INPUT_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.md
    )

    set(SPHINX_OUTPUT_DIR ${CMAKE_BINARY_DIR}/sphinx-docs)
    set(SPHINX_OUTPUT_FILE ${SPHINX_OUTPUT_DIR}/index.html)

    add_custom_command(OUTPUT ${SPHINX_OUTPUT_FILE}
        COMMAND ${SPHINX_COMMAND} -T -E -W --keep-going -b html ${CMAKE_CURRENT_SOURCE_DIR} ${SPHINX_OUTPUT_DIR}
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/conf.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${SPHINX_INPUT_FILES}
    )

    add_custom_target(sphinx-docs ALL
        DEPENDS ${SPHINX_OUTPUT_FILE}
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/conf.py
    )
endif()