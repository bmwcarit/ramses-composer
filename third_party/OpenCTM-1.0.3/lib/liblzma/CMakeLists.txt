add_library(lzma Alloc.c LzFind.c LzmaDec.c LzmaEnc.c LzmaLib.c)

target_include_directories(lzma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})