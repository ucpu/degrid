cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(grid LANGUAGES CXX)

file(GLOB_RECURSE grid-sources "sources/grid/*")
add_executable(grid ${grid-sources})
target_link_libraries(grid cage-client)
set_ide_groups(grid grid)
set_ide_working_dir_in_place(grid)
