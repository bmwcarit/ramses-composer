#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

if(NOT WIN32)
	message(FATAL_ERROR "This file should only ever be included by Windows builds!")
endif()

# Given that we delete the directory below, do some sanity check first
if(NOT PYTHON_DEPLOYMENT_FOLDER MATCHES "/bin/python-")
	message(FATAL_ERROR "The variable PYTHON_DEPLOYMENT_FOLDER should contain the string '/bin/python-'. This is just a sanity check to avoid deleting random folders. See deploy_python_folder macro.")
endif()

SET(PYTHON_DEPLOYMENT_BATCH_FILE "${CMAKE_SOURCE_DIR}/third_party/python-windows/setup_python_deployment_folder.bat")
STRING(REPLACE "/" "\\" PYTHON_DEPLOYMENT_BATCH_FILE ${PYTHON_DEPLOYMENT_BATCH_FILE})
STRING(REPLACE "/" "\\" PYTHON_DEPLOYMENT_FOLDER_WIN ${PYTHON_DEPLOYMENT_FOLDER})
STRING(REPLACE "/" "\\" Python_ROOT_WIN ${Python_ROOT})
# The scrípt will be run whenever the python_setup_completed.txt is missing. Given that is the file created in the
# last step of the batch file, this seems good enough. Note that changing the source files
# will not cause the copy to be triggered automatically.
add_custom_command(OUTPUT "${PYTHON_DEPLOYMENT_FOLDER}/python_setup_completed.txt"
	COMMAND cmd /c "${PYTHON_DEPLOYMENT_BATCH_FILE} ${PYTHON_DEPLOYMENT_FOLDER_WIN}"
	# We need the *_d.pyd files to run in Debug
	COMMAND copy "${Python_ROOT_WIN}\\DLLs\\*_d.pyd" "${PYTHON_DEPLOYMENT_FOLDER_WIN}")
add_library(RaCoPythonModuleDeployment INTERFACE "${PYTHON_DEPLOYMENT_FOLDER}/python_setup_completed.txt")

macro(deploy_python_folder tgt)
	add_dependencies(${tgt} RaCoPythonModuleDeployment)
endmacro()