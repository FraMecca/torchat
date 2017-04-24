cmake_minimum_required(VERSION 2.6)
project(torchat)

# basic settings: c flags, basepath (the repo main directory) is included
set(CMAKE_C_FLAGS "-g -Wall")
set(BASEPATH "${CMAKE_SOURCE_DIR}")
include_directories("${BASEPATH}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# set the dir in which cmake will put the binary files built
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)


# add shared (dinamic) libraries and build them
add_library(jsonhelper SHARED src/jsonhelper.cpp)
target_compile_features(jsonhelper PRIVATE cxx_range_for)
add_library(logger SHARED src/logger.cpp)
target_compile_features(logger PRIVATE cxx_range_for)

# set the executables to be built
add_executable(torchat src/main.c src/socks_helper.c src/util.c src/datastruct.c src/actions.c src/torchatproto.c include/mem.c include/ut_assert.c include/except.c include/base64.c include/proxysocket.c include/fd.c include/iol.c include/utils.c)

# equivalent to -l[lib] in gcc, links the libraries previously built
target_link_libraries(torchat dill pthread dl jsonhelper logger)

# set extra debug flags for debug target 
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -UNDEBUG")
ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
