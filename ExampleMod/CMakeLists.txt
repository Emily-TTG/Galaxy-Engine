cmake_minimum_required(VERSION 3.10)

project(TestMod)

add_subdirectory(.. ${CMAKE_CURRENT_BINARY_DIR}/GalaxyEngine EXCLUDE_FROM_ALL SYSTEM)

add_library(TestMod SHARED ${CMAKE_CURRENT_LIST_DIR}/Mod.cpp)
set_target_properties(TestMod PROPERTIES PREFIX "")
target_link_libraries(TestMod PRIVATE GalaxyEngineInterface)
