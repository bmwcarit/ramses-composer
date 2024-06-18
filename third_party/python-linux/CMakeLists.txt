#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]

# Create a build target ${targetname} copying python folder contents to ${deploymentfolder}.
# Declared as function for ${CMAKE_CURRENT_FUNCTION_LIST_DIR} to return current file directory even when called from another file.
function(register_python_target targetname deploymentfolder)
	if(NOT UNIX)
		message(FATAL_ERROR "This file should only ever be included by Linux builds!")
	endif()

	# Given that we delete the directory below, do some sanity check first
	if(NOT ${deploymentfolder} MATCHES "/bin/python-")
		message(FATAL_ERROR "The parameter deploymentfolder should contain the string '/bin/python-'. This is just a sanity check to avoid deleting random folders. See register_python_target function.")
	endif()

	# The script will be run whenever the python_setup_completed.txt is missing. Given that is the file created in the
	# last step of the batch file, this seems good enough. Note that changing the source files
	# will not cause the copy to be triggered automatically.
	add_custom_command(OUTPUT "${deploymentfolder}/python_setup_completed.txt"
		COMMAND ${CMAKE_COMMAND} -E rm -rf "${deploymentfolder}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${deploymentfolder}"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "/usr/lib/python3.8" "${deploymentfolder}/python3.8"
		# TODO: this does not work in CC ci (no Internet access)
		# COMMAND ${CMAKE_COMMAND} -E env  PIP_PREFIX="${deploymentfolder}/python3.8" python3.8 "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/get-pip.py" --ignore-installed
		COMMAND ${CMAKE_COMMAND} -E touch "${deploymentfolder}/python_setup_completed.txt")
	add_library(${targetname} INTERFACE "${deploymentfolder}/python_setup_completed.txt")
	set_target_properties (${targetname} PROPERTIES FOLDER Packaging)
endfunction()