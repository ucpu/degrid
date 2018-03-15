cmake_minimum_required(VERSION 3.2)
project(grid CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/externals/cage/cmake)
include(cage_all)
cage_disable_source_build()
cage_build_configuration()
cage_output_directories(${CMAKE_BINARY_DIR}/result)
cage_output_data(${CMAKE_BINARY_DIR}/result)
cage_directory_link("${CMAKE_CURRENT_SOURCE_DIR}/externals/data/grid" "${CMAKE_BINARY_DIR}/result/data/grid")

add_subdirectory(externals/cage)

file(GLOB_RECURSE grid-sources "sources/*")
add_executable(grid ${grid-sources})
target_link_libraries(grid cage-client)
cage_ide_groups(grid grid)
cage_ide_working_dir_in_place(grid)
