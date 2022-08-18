#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

if(NOT UNIX)
	message(FATAL_ERROR "This file should only ever be included by Linux builds!")
endif()

# Given that we delete the directory below, do some sanity check first
if(NOT PYTHON_DEPLOYMENT_FOLDER MATCHES "/bin/python-")
	message(FATAL_ERROR "The variable PYTHON_DEPLOYMENT_FOLDER should contain the string '/bin/python-'. This is just a sanity check to avoid deleting random folders. See deploy_python_folder macro.")
endif()

# The script will be run whenever the python_setup_completed.txt is missing. Given that is the file created in the
# last step of the batch file, this seems good enough. Note that changing the source files
# will not cause the copy to be triggered automatically.
add_custom_command(OUTPUT "${PYTHON_DEPLOYMENT_FOLDER}/python_setup_completed.txt"
	COMMAND ${CMAKE_COMMAND} -E rm -rf "${PYTHON_DEPLOYMENT_FOLDER}"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${PYTHON_DEPLOYMENT_FOLDER}"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "/usr/lib/python3.8" "${PYTHON_DEPLOYMENT_FOLDER}/python3.8"
	COMMAND ${CMAKE_COMMAND} -E env  PIP_PREFIX="${PYTHON_DEPLOYMENT_FOLDER}/python3.8" python3.8 "${CMAKE_CURRENT_SOURCE_DIR}/get-pip.py"
	COMMAND ${CMAKE_COMMAND} -E touch "${PYTHON_DEPLOYMENT_FOLDER}/python_setup_completed.txt")
add_library(RaCoPythonModuleDeployment INTERFACE "${PYTHON_DEPLOYMENT_FOLDER}/python_setup_completed.txt")

macro(deploy_python_folder tgt)
	add_dependencies(${tgt} RaCoPythonModuleDeployment)
endmacro()