file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/exported/")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/actual/")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/diff/")
execute_process(
	COMMAND ${RACO_EXE} -p ${CMAKE_CURRENT_SOURCE_DIR}/projects/merge-left.rca -e exported/merge-left -l 3
	COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
	COMMAND ${RACO_EXE} -p ${CMAKE_CURRENT_SOURCE_DIR}/projects/merge-right.rca -e exported/merge-right -l 3
	COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
	COMMAND ${VIEWER_EXE} --gui on --exec-lua "R.screenshot('actual/merge.png')" exported/merge-left.ramses exported/merge-right.ramses
	COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
	COMMAND ${PY_EXE} ${PY_CMD} merge
	COMMAND_ERROR_IS_FATAL ANY
)
