cmake_minimum_required(VERSION 3.2)
project(grid CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/externals/cage/cmake")
include(cage_all)
cage_default_init()
cage_directory_link("${CMAKE_CURRENT_SOURCE_DIR}/data/grid" "${CMAKE_BINARY_DIR}/result/data/grid")

add_subdirectory(externals/cage)

file(GLOB_RECURSE grid-sources "sources/*")
add_executable(grid ${grid-sources})
target_link_libraries(grid cage-client)
cage_ide_category(grid grid)
cage_ide_sort_files(grid)
cage_ide_working_dir_in_place(grid)
