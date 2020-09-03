cmake_minimum_required(VERSION 3.10)
project (sxplayer)

set (CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file (GLOB_RECURSE SXPLAYER_SRC_FILES *.c *.h)
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
list(FILTER SXPLAYER_SRC_FILES EXCLUDE REGEX ".*decoder_vt.c")
endif()

add_library(sxplayer ${SXPLAYER_SRC_FILES})
include_directories(
	../external/win64/include
	../external/win64/include/pthread
)

