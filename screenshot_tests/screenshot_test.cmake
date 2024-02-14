file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/exported/")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/actual/")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/diff/")
execute_process(
	COMMAND ${RACO_EXE} -p ${PROJECT_PATH} -e exported/${TEST_NAME} -l 3
	COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
	COMMAND ${VIEWER_EXE} --gui on --exec-lua "R.screenshot('actual/${TEST_NAME}.png')" exported/${TEST_NAME}.ramses
	COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
	COMMAND ${PY_EXE} ${PY_CMD} ${TEST_NAME}
	COMMAND_ERROR_IS_FATAL ANY
)
