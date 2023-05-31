#[[
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
]]
cmake_minimum_required(VERSION 3.19)

SET(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo")

project(RaCoOS VERSION 1.9.0)

SET(RACO_RELEASE_DIRECTORY ${CMAKE_BINARY_DIR}/release)

SET(HEADLESS_RELEASE_DIRECTORY ${CMAKE_BINARY_DIR}/release_headless)

# The build and deployment process for Python works differently for Linux and Windows.
# Windows:
# * RaCo builds and runs without Python being installed.
# * RaCo is build using Python 3.8.10 from third_party/python-windows/python-3.8.10-for-build.
#   This is a normal full Python installation folder, all options except Debug Binaries off.
# * The build process uses the third_party/python-windows/python-3.8.10-embed-amd64 folder (downloadable from Python as the "embeddable package")
#   to generate a suitable Python folder in the release folder (containing pip and virtualenv), see third_party\python-windows\CMakeLists.txt
# * On startup, RaCoHeadless will just use the Python folder "${HEADLESS_RELEASE_DIRECTORY}/bin/python-3.8.10-with-pip-and-virtualenv" generated in the steps above.
# * Python will be started "in isolation", so all other Python installations/environment variables on the machine will be ignored.
# Linux:
# * RaCo requires Python 3.8 to be installed for building.
# 
# * In a later version, RaCoHeadless will automatically create a virtualenv on startup, making it easier to install custom modules etc.

set(RequiredPythonVersion 3.8)
if(WIN32)
	set(PYTHON_THIRD_PARTY_FOLDER "${CMAKE_SOURCE_DIR}/third_party/python-windows")
	set(Python_ROOT "${PYTHON_THIRD_PARTY_FOLDER}/python-${RequiredPythonVersion}.10-for-build")
endif()
find_package (Python ${RequiredPythonVersion} EXACT REQUIRED COMPONENTS Interpreter Development) # Python version needs to match the one in third_party
# Check if a Python debug library is available. On Windows Python can be installed with or without debug libraries,
# and if the debug library is available, pybind11 will link to it, but not properly without a bit of our help.
# PYTHON_DEBUG_LIBRARY_AVAILABLE is used in all projects including pybind11.
if(Python_LIBRARY_DEBUG)
	set(PYTHON_DEBUG_LIBRARY_AVAILABLE ON)	
else()
	set(PYTHON_DEBUG_LIBRARY_AVAILABLE OFF)	
endif()
if(NOT ${Python_FOUND})
	message(FATAL_ERROR "Cannot find Python ${RequiredPythonVersion}!")
endif()
message(STATUS "Python Version for build: ${Python_VERSION} from ${Python_EXECUTABLE}")
if(WIN32)
	if(NOT "${Python_EXECUTABLE}" MATCHES ".*/third_party/.*")
		message(FATAL_ERROR "Found ${Python_EXECUTABLE} is not the one in the third_party folder!")
	endif()
endif()
set(PYTHON_DEPLOYMENT_FOLDER_NAME "python-${Python_VERSION}-with-pip")
set(PYTHON_DEPLOYMENT_FOLDER "${RACO_RELEASE_DIRECTORY}/bin/${PYTHON_DEPLOYMENT_FOLDER_NAME}")
set(HEADLESS_PYTHON_DEPLOYMENT_FOLDER "${HEADLESS_RELEASE_DIRECTORY}/bin/${PYTHON_DEPLOYMENT_FOLDER_NAME}")


# Configuration - where is your Qt?
set(RACO_QT_BASE $ENV{RACO_QT_BASE})
if(NOT RACO_QT_BASE)
	# Standard installation directory for the current Qt installer in ParadoxCat should be "C:\Qt"
	# Can be overridden by setting the environment variable "QTBASEDIR". The "QTBASEDIR" environment variable
	# is shared with other products, so please don't randomly change that.
	set(QT_ROOT $ENV{QTBASEDIR})
	if(NOT QT_ROOT)
		if(WIN32)
			set(QT_ROOT "C:/Qt")
		else()
			set(QT_ROOT "/usr/local/opt/Qt")
		endif()
		message(STATUS "Set QT_ROOT to ${QT_ROOT} from value hardcoded into CMakeLists.txt.")
	else()
		message(STATUS "Set QT_ROOT to ${QT_ROOT} from QTBASEDIR environment variable.")
	endif()
	if(WIN32)
		set(RACO_QT_BASE "${QT_ROOT}/5.15.2/msvc2019_64")
	else()
		set(RACO_QT_BASE "${QT_ROOT}/5.15.2/gcc_64")
	endif()
	message(STATUS "Set RACO_QT_BASE to ${RACO_QT_BASE} using default subdirectory for QT_ROOT.")
else()
	message(STATUS "Set RACO_QT_BASE to ${RACO_QT_BASE} from environment variable.")
endif()

# Setup more details for Qt5
set(Qt5_DIR "${RACO_QT_BASE}/lib/cmake/Qt5" CACHE PATH "FORCED in root CMakeLists.txt - Qt5 cmake file directory" FORCE)
set(RACO_QT_WINDEPLOY_PATH "${RACO_QT_BASE}/bin/")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# Enable multi-processor compilation for Visual Studio
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
	# Compile errors might occur with Visual Studio 2019 16.6 or earlier.
	if(${MSVC_VERSION} LESS 1927)
		message(SEND_ERROR "MSVC_VERSION reports version number ${MSVC_VERSION}. Minimum required version is 1927 (=Visual Studio 2019 16.7).")
	endif()
endif()

# We only allow building against (and test with) one exact Qt version for now. 
set(REQUIRED_QT_VERSION 5.15.2)

macro(raco_find_qt_components QTCOMPONENTS)
	find_package(Qt5 ${REQUIRED_QT_VERSION} EXACT REQUIRED COMPONENTS ${ARGV})
endmacro()

function(seperate_debug_symbols exetarget TargetBinaryFile)
        IF(NOT WIN32)
                IF("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
                        ADD_CUSTOM_COMMAND(TARGET ${exetarget} POST_BUILD
                                COMMAND objcopy --only-keep-debug "${TargetBinaryFile}" "${TargetBinaryFile}.debug"
                                COMMAND objcopy --strip-debug "${TargetBinaryFile}"
                                COMMAND objcopy --add-gnu-debuglink="${TargetBinaryFile}.debug" "${TargetBinaryFile}")
                ENDIF()
        endif()
endfunction()

# Adding third party libraries

# GoogleTest

# Option - Do we want tests?
option(PACKAGE_TESTS "Build the tests" ON)

macro(deploy_qt tgt)
	IF(WIN32)
		# Post build commands - copy the DLLs with the windeployqt tool. Sadly, because of a bug we need to run it twice for debug builds
		# See https://stackoverflow.com/questions/59828611/windeployqt-doesnt-deploy-qwindowsd-dll-for-a-debug-application
		# Also add the Qt licensing file and our licensing file.
		# We are making use of the add_custom_command feature for build event commands that "COMMAND" statements with an empty COMMAND string are completely ignored.
		add_custom_command(TARGET ${tgt} POST_BUILD 
			COMMAND "$<$<CONFIG:Debug>:${RACO_QT_WINDEPLOY_PATH}/windeployqt.exe>" --debug --no-libraries --no-opengl-sw --no-system-d3d-compiler --svg --no-translations --no-compiler-runtime "$<TARGET_FILE:${tgt}>"
			COMMAND "${RACO_QT_WINDEPLOY_PATH}/windeployqt.exe" "$<$<CONFIG:Debug>:>" --no-opengl-sw --no-system-d3d-compiler --svg --no-translations --no-compiler-runtime "$<TARGET_FILE:${tgt}>"
			# Qt deploys the folder imageformats which we do not need except for svg support - createReadHandlerHelper in C:\Qt\5.15.2\Src\qtbase\src\gui\image\qimagereader.cpp contains hardcoded support for the extensions png, bmp, dib, xpm, xbm, pbm, pbmraw, pgm, pgmraw, ppm and ppmraw (which is more than we need).
			COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:${tgt}>/imageformats/qgifd.dll" "$<TARGET_FILE_DIR:${tgt}>/imageformats/qgif.dll"
			COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:${tgt}>/imageformats/qicnsd.dll" "$<TARGET_FILE_DIR:${tgt}>/imageformats/qicns.dll"
			COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:${tgt}>/imageformats/qicod.dll" "$<TARGET_FILE_DIR:${tgt}>/imageformats/qico.dll"
			COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:${tgt}>/imageformats/qjpegd.dll" "$<TARGET_FILE_DIR:${tgt}>/imageformats/qjpeg.dll"
			COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:${tgt}>/imageformats/qtiffd.dll" "$<TARGET_FILE_DIR:${tgt}>/imageformats/qtiff.dll"
			COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:${tgt}>/imageformats/qtgad.dll" "$<TARGET_FILE_DIR:${tgt}>/imageformats/qtga.dll"
			COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:${tgt}>/imageformats/qwbmpd.dll" "$<TARGET_FILE_DIR:${tgt}>/imageformats/qwbmp.dll"
			COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:${tgt}>/imageformats/qwebpd.dll" "$<TARGET_FILE_DIR:${tgt}>/imageformats/qwebp.dll"
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/third_party/qtsharedlibs_license.txt" "$<TARGET_FILE_DIR:${tgt}>/qtsharedlibs_license.txt"
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE.txt" "$<TARGET_FILE_DIR:${tgt}>/LICENSE.txt"
			)
	ELSE()
		set(LINUXDEPLOYQTREQUIRED $ENV{LINUXDEPLOYQTREQUIRED})
		find_program(LINUXDEPLOYQT linuxdeployqt)
		if(EXISTS "${LINUXDEPLOYQT}")
			add_custom_command(TARGET ${tgt} POST_BUILD 	
				COMMAND "${LINUXDEPLOYQT}" "$<TARGET_FILE:${tgt}>" -qmake="${RACO_QT_BASE}/bin/qmake" -no-copy-copyright-files -no-strip -no-translations -unsupported-allow-new-glibc
				COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_FILE_DIR:${tgt}>/AppRun" "$<TARGET_FILE:${tgt}>.sh"
				COMMAND ${CMAKE_COMMAND} -D TARGET_FILE=$<TARGET_FILE:${tgt}> -D ROOT_DIR=${CMAKE_SOURCE_DIR} -P "${CMAKE_SOURCE_DIR}/ubuntustartscript.cmake"
				COMMAND chmod +x "$<TARGET_FILE:${tgt}>.sh"
				COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/third_party/qtsharedlibs_license.txt" "$<TARGET_FILE_DIR:${tgt}>/lib/qtsharedlibs_license.txt"
				COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE.txt" "$<TARGET_FILE_DIR:${tgt}>/LICENSE.txt"
				WORKING_DIRECTORY "$<TARGET_FILE_DIR:${tgt}>"
				)
			
		elseif(LINUXDEPLOYQTREQUIRED)
			message(SEND_ERROR "Cannot find linuxdeployqt in the path, will not deploy Qt shared libraries. This is an error because the environment variable 'LINUXDEPLOYQTREQUIRED' is set, indicating that this build is used to create a release (which will need the Qt shared libs deployed to work properly).")
		else()
			message(WARNING "Cannot find linuxdeployqt in the path, will not deploy Qt shared libraries. You can ignore this message on a developer machine, but not if you want to build a release. If you have linuxdeployqt, you can either add its path to your system path or to the CMAKE_PROGRAM_PATH environment variable")
		endif()
	ENDIF()
endmacro()
macro(deploy_raco_cppruntime_dlls tgt)
	IF(WIN32)
		# Add the MSVC runtime libraries (for release only, Microsoft licensing terms preclude distributing the debug DLLs)
		# We are making use of the add_custom_command feature for build event commands that "COMMAND" statements with an empty COMMAND string are completely ignored.
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP OFF CACHE BOOL "" FORCE)
		include(InstallRequiredSystemLibraries)
		foreach(MSVC_RUNTIME_LIB IN LISTS CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
			add_custom_command(TARGET ${tgt} POST_BUILD 
				COMMAND "$<$<NOT:$<CONFIG:Debug>>:${CMAKE_COMMAND}>" -E copy_if_different "${MSVC_RUNTIME_LIB}" "$<TARGET_FILE_DIR:${tgt}>"
				)
		endforeach()
	ENDIF()
endmacro()

#
# Create macros to copy necessary DLLs (we might be better off using the RUNTIME_OUTPUT_DIRECTORY property in a controlled manner?)
#
macro(deploy_dlls_and_strip_symbols exetarget dlltarget outdir)
	add_custom_command(TARGET ${exetarget} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${dlltarget}> "${outdir}"
	)
	IF(WIN32)
		add_custom_command(TARGET ${exetarget} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PDB_FILE:${dlltarget}> "${outdir}"
		)
	ENDIF()
	seperate_debug_symbols(${exetarget} "${outdir}/$<TARGET_FILE_NAME:${dlltarget}>")
endmacro()
macro(deploy_ramses_client_only_shared_dlls tgt outdir)
	deploy_dlls_and_strip_symbols(${tgt} raco::ramses-lib-client-only "${outdir}")
	deploy_dlls_and_strip_symbols(${tgt} raco::ramses-logic-lib-client-only "${outdir}")
endmacro()
macro(deploy_ramses_with_renderer_shared_dlls tgt)
	deploy_dlls_and_strip_symbols(${tgt} raco::ramses-lib "$<TARGET_FILE_DIR:${tgt}>")
	deploy_dlls_and_strip_symbols(${tgt} raco::ramses-logic-lib "$<TARGET_FILE_DIR:${tgt}>")
endmacro()
macro(deploy_headless_shared_dlls tgt)
	deploy_dlls_and_strip_symbols(${tgt} openctm "$<TARGET_FILE_DIR:${tgt}>")
endmacro()
macro(deploy_gui_shared_dlls tgt)
	deploy_headless_shared_dlls(${tgt})
	deploy_dlls_and_strip_symbols(${tgt} qtadvanceddocking "$<TARGET_FILE_DIR:${tgt}>")
endmacro()
macro(deploy_viewer tgt)
	deploy_dlls_and_strip_symbols(${tgt} ramses-logic-viewer "$<TARGET_FILE_DIR:${tgt}>")
endmacro()
macro(deploy_python_dlls tgt)
	IF(WIN32)
		add_custom_command(TARGET ${tgt} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Python_ROOT}/python38$<$<CONFIG:Debug>:_d>.dll" "$<TARGET_FILE_DIR:${tgt}>"
		)
	else()
		# Make sure RaCoHeadless starts if Python is not used and Python is not installed.
	add_custom_command(TARGET ${tgt} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "/usr/lib/x86_64-linux-gnu/libpython3.8.so.1.0" "$<TARGET_FILE_DIR:${tgt}>"
	)
	ENDIF()
endmacro()

if(PACKAGE_TESTS)
	enable_testing()
	include(GoogleTest)	
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # needed to build on Windows
	set(raco_test_resources_base_path "${CMAKE_CURRENT_SOURCE_DIR}/resources")
	#if (WINDOWS)
		#string(REPLACE "/" "\\\\" raco_test_resources_base_path "${raco_test_resources_base_path}")
	#endif()
	add_subdirectory(third_party/googletest)

	# See https://cmake.org/cmake/help/v3.10/module/GoogleTest.html
	# Keep cache clean
	mark_as_advanced(
		BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
		gmock_build_tests gtest_build_samples gtest_build_tests
		gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
	)
	# Hide all the folders from Visual Studio's Solution Explorer
	set_target_properties(gtest PROPERTIES FOLDER third_party/googletest)
	set_target_properties(gtest_main PROPERTIES FOLDER third_party/googletest)
	set_target_properties(gmock PROPERTIES FOLDER third_party/googletest)
	set_target_properties(gmock_main PROPERTIES FOLDER third_party/googletest)
	# Define a macro to easily setup tests
	macro(raco_package_add_test TESTNAME FILES LIBRARIES TEST_WORKING_DIRECTORY)
	    add_executable(${TESTNAME} ${FILES})
		target_link_libraries(${TESTNAME} gtest gmock gtest_main ${LIBRARIES} raco::ramses-logic-lib-client-only)
		gtest_discover_tests(${TESTNAME}
			WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
			PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${TEST_WORKING_DIRECTORY}"
			DISCOVERY_MODE PRE_TEST
		)
		set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
		target_compile_definitions(${TESTNAME} PRIVATE -DRACO_TEST_RESOURCES_BASE_PATH="${raco_test_resources_base_path}")
		# TODO: Working directory for tests (should be also config specific), required for tests which modify files
	endmacro()
	macro(raco_package_add_qt_test TESTNAME FILES LIBRARIES TEST_WORKING_DIRECTORY)
		raco_package_add_test(${TESTNAME} "${FILES}" "${LIBRARIES}" "${TEST_WORKING_DIRECTORY}")
		IF(WIN32)
			# In Windows we need the DLLs to be next to the executable to start Qt.
			# In Linux even if we deploy the DLLs next to the tests, we would have to start the
			# executable with the correct LD_LIBRARY_PATH environment variable (or the shell script
			# deploy_qt generates) for the test to start without Qt in the path. We also
			# would have to deploy the "offscreen" plug-in to allow the GUI tests to run
			# on a machine without OpenGL. And deploying Qt causes a qt.conf to be created
			# which messes with Qt setup even if the paths are all there.
			# So do not deploy Qt in Linux for the tests. If you want to run the tests,
			# make sure Qt is installed and in the path.
			deploy_qt(${TESTNAME})
		ENDIF()
	endmacro()
	macro(raco_package_add_headless_test TESTNAME FILES LIBRARIES TEST_WORKING_DIRECTORY)
		raco_package_add_qt_test(${TESTNAME} "${FILES}" "${LIBRARIES}" "${TEST_WORKING_DIRECTORY}")
		deploy_headless_shared_dlls(${TESTNAME})
		deploy_ramses_client_only_shared_dlls(${TESTNAME} "$<TARGET_FILE_DIR:${TESTNAME}>")
	endmacro()
	macro(raco_package_add_gui_test TESTNAME FILES LIBRARIES TEST_WORKING_DIRECTORY)
		raco_package_add_qt_test(${TESTNAME} "${FILES}" "${LIBRARIES}" "${TEST_WORKING_DIRECTORY}")
		deploy_gui_shared_dlls(${TESTNAME})
		deploy_ramses_client_only_shared_dlls(${TESTNAME} "$<TARGET_FILE_DIR:${TESTNAME}>")
	endmacro()
	function(raco_package_test_resources_process TESTNAME SOURCE_DIRECTORY)
		foreach(fname ${ARGN}) 
			set_property(GLOBAL APPEND PROPERTY "${TESTNAME}_RESOURCE_FILES" "${fname}")
			set_property(GLOBAL APPEND PROPERTY "${TESTNAME}_RESOURCE_DIRECTORIES" "${SOURCE_DIRECTORY}")
		endforeach(fname)
	endfunction()
	function(raco_package_test_resources_add_compile_definitions TESTNAME)
		get_property("${TESTNAME}_RESOURCE_FILES" GLOBAL PROPERTY "${TESTNAME}_RESOURCE_FILES")
		get_property("${TESTNAME}_RESOURCE_DIRECTORIES" GLOBAL PROPERTY "${TESTNAME}_RESOURCE_DIRECTORIES")

		list(JOIN "${TESTNAME}_RESOURCE_FILES" "!" RESOURCES_FILE_LIST)
		list(JOIN "${TESTNAME}_RESOURCE_DIRECTORIES" "!" RESOURCES_DIRECTORY_LIST)
		#message("${TESTNAME} --> ${RESOURCES_FILE_LIST}")
		#message("${TESTNAME} ==> ${RESOURCES_DIRECTORY_LIST}")
		target_compile_definitions(${TESTNAME} PRIVATE RACO_LOCAL_TEST_RESOURCES_FILE_LIST="${RESOURCES_FILE_LIST}")
		target_compile_definitions(${TESTNAME} PRIVATE RACO_LOCAL_TEST_RESOURCES_DIRECTORY_LIST="${RESOURCES_DIRECTORY_LIST}")
	endfunction()
	# Do not call raco_package_add_test_resources multiple times for the same TESTNAME.
	# We must avoid multiple calls to raco_package_test_resources_add_compile_definitions since each adds another compiler flag
	# leading to platform dependent behaviour, i.e. the compiler picks one over the others.
	# Instead call raco_package_test_resources_process multiple times and then add a single call to raco_package_test_resources_add_compile_definitions
	function(raco_package_add_test_resources TESTNAME SOURCE_DIRECTORY)
		raco_package_test_resources_process(${TESTNAME} ${SOURCE_DIRECTORY} ${ARGN})
		raco_package_test_resources_add_compile_definitions(${TESTNAME})
	endfunction()
	macro(add_racocommand_test TESTNAME WORKINGDIR)
		if(WIN32)
			add_test(
				NAME ${TESTNAME}
				WORKING_DIRECTORY ${WORKINGDIR}
				COMMAND "$<TARGET_FILE:RaCoCommand>" ${ARGN}
			)
		else()
			add_test(
				NAME ${TESTNAME}
				WORKING_DIRECTORY ${WORKINGDIR}
				COMMAND "$<TARGET_FILE:RaCoCommand>.sh" ${ARGN}
			)
		endif()
	endmacro()
endif()

macro(enable_warnings_as_errors TARGET_NAME)
	if (MSVC)
		# warning level 4 and all warnings as errors
		target_compile_options(${TARGET_NAME} PRIVATE "/WX")
	else()
		# lots of warnings and all warnings as errors
		target_compile_options(${TARGET_NAME} PRIVATE "-Werror")
	endif()
endmacro()


#
# Adding third party projects
#
add_subdirectory(third_party/)

# CXX Standard for our own targets
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# Disable any non-standard C++ extensions Visual Studio might feature to avoid surprising build problems in the Linux build
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
endif()

#
# Macros creating necessary files and folders in the packaged release folder
#
macro(add_file_copy SRC_FILE DEST_FILE)
	add_custom_command(OUTPUT "${DEST_FILE}"
		MAIN_DEPENDENCY "${SRC_FILE}"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_FILE}" "${DEST_FILE}"
		VERBATIM
	)
endmacro()

function(create_folder_structure FILE_LIST DEST_DIR TARGET_NAME)
	foreach(filepath ${FILE_LIST})
		get_filename_component(filename ${filepath} NAME)
		add_file_copy(${filepath} ${DEST_DIR}/${filename})
		LIST(APPEND OUTPUT_FILE_LIST ${destpath})
	endforeach(filepath)

	add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILE_LIST} SOURCES ${FILE_LIST})
	set_target_properties (${TARGET_NAME} PROPERTIES FOLDER Packaging)
endfunction()

#
# Adding the subprojects
#
add_subdirectory(utils)
add_subdirectory(datamodel)

#
# From here on, everything needs Qt
#
add_subdirectory(components)
add_subdirectory(HeadlessApp)

add_subdirectory(gui)

include(cmake/ramsesversions.cmake)

add_subdirectory(EditorApp)
add_subdirectory(resources)

add_subdirectory(PyAPITests)
add_subdirectory(screenshot_tests)
add_subdirectory(doc)