
add_subdirectory(liblzma)

add_library(openctm SHARED 
	compressMG1.c 
	compressMG2.c 
	compressRAW.c 
	openctm.h openctm.c
	openctmpp.h
	stream.c
	internal.h
)

target_link_libraries(openctm lzma)
target_include_directories(openctm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(openctm PRIVATE OPENCTM_BUILD)

