cmake_minimum_required(VERSION 2.6)
project(torchat)

# basic settings: c flags, basepath (the repo main directory) is included
set(CMAKE_C_FLAGS "-Wall -lm -Wno-unused-parameter")
set(BASEPATH "${CMAKE_SOURCE_DIR}")
include_directories("${BASEPATH}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# set the dir in which cmake will put the binary files built
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)


# add shared (dinamic) libraries and build them
#add_library(jsonhelper SHARED src/jsonhelper.cpp)
#target_compile_features(jsonhelper PRIVATE cxx_range_for)
#add_library(logger SHARED src/logger.cpp)
#target_compile_features(logger PRIVATE cxx_range_for)
add_library(confuse SHARED IMPORTED)
set_property(TARGET confuse PROPERTY IMPORTED_LOCATION
	${BASEPATH}/build/libconfuse.so.1)

# set the executables to be built
file(GLOB SrcFiles "src/*.c")
add_executable(torchat ${SrcFiles} include/argparse/src/argparse.c
	include/except.c)


# equivalent to -l[lib] in gcc, links the libraries previously built
target_link_libraries(torchat confuse) 

# set extra debug flags for debug target 
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
