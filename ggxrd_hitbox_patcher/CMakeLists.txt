# this CMakeLists.txt is for Linux compilation
cmake_minimum_required(VERSION "${MIN_VER_CMAKE}" FATAL_ERROR)
project(ggxrd_hitbox_patcher)
set(CMAKE_CXX_STANDARD 14)
add_executable(ggxrd_hitbox_patcher ggxrd_hitbox_patcher.cpp)
target_compile_definitions(ggxrd_hitbox_patcher PRIVATE "-DFOR_LINUX=\"1\"")

# compile instructions
# cd into the directory with the CMakeLists.txt
#
# cmake .
# make
#
# The executable named "ggxrd_hitbox_patcher" appears in the current directory.
# To launch, use:
#
# ./ggxrd_hitbox_patcher