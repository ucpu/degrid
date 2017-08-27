cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(grid LANGUAGES CXX)

file(GLOB_RECURSE grid-sources "src/grid/*")
add_executable(grid ${grid-sources})
target_link_libraries(grid cage-client)
set_target_properties(grid PROPERTIES FOLDER grid)
