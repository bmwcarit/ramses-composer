FIND_PACKAGE(Git REQUIRED)
EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE RAMSES_OSS_COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE )

string(REGEX REPLACE "([0-9]+)\.([0-9]+)\.([0-9]+)" "\\1" RAMSES_VERSION_MAJOR "${ramses-sdk_VERSION}")
string(REGEX REPLACE "([0-9]+)\.([0-9]+)\.([0-9]+)" "\\2" RAMSES_VERSION_MINOR "${ramses-sdk_VERSION}")
string(REGEX REPLACE "([0-9]+)\.([0-9]+)\.([0-9]+)" "\\3" RAMSES_VERSION_PATCH "${ramses-sdk_VERSION}")

string(REGEX REPLACE "([0-9]+)\.([0-9]+)\.([0-9]+)" "\\1" RLOGIC_VERSION_MAJOR "${ramses-logic_VERSION}")
string(REGEX REPLACE "([0-9]+)\.([0-9]+)\.([0-9]+)" "\\2" RLOGIC_VERSION_MINOR "${ramses-logic_VERSION}")
string(REGEX REPLACE "([0-9]+)\.([0-9]+)\.([0-9]+)" "\\3" RLOGIC_VERSION_PATCH "${ramses-logic_VERSION}")
