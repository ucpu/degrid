cmake_minimum_required(VERSION 3.16)
project(degrid CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/externals/cage/cmake")
include(cage_all)
cage_default_init()
cage_assets_add_data_path("${CMAKE_CURRENT_SOURCE_DIR}/data")

add_subdirectory(externals/cage)

file(GLOB_RECURSE degrid-sources "sources/*")
add_executable(degrid ${degrid-sources})
target_link_libraries(degrid cage-engine)
cage_ide_category(degrid degrid)
cage_ide_sort_files(degrid)
cage_ide_working_dir_in_place(degrid)
