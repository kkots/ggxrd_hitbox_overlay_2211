# this CMakeLists.txt is for Linux compilation
cmake_minimum_required(VERSION "${MIN_VER_CMAKE}" FATAL_ERROR)
project(ggxrd_hitbox_patcher_console)
set(CMAKE_CXX_STANDARD 14)
include_directories(${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/../common)
add_executable(ggxrd_hitbox_patcher_console
	ggxrd_hitbox_patcher_console.cpp
	${PROJECT_SOURCE_DIR}/../common/ggxrd_hitbox_patcher_common.cpp
	${PROJECT_SOURCE_DIR}/../common/InjectorCommonOut.cpp)
target_compile_definitions(ggxrd_hitbox_patcher_console PRIVATE "-DFOR_LINUX=\"1\"")

# compile instructions
# cd into the directory with the CMakeLists.txt
#
# cmake -DCMAKE_BUILD_TYPE=Release .
# make
#
# The executable named "ggxrd_hitbox_patcher_console" appears in the current directory.
# To launch, use:
#
# ./ggxrd_hitbox_patcher_console