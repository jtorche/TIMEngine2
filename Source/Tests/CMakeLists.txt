cmake_minimum_required(VERSION 3.16)

project(TIMEngine2_Tests DESCRIPTION "Console application to easily test TIMEngine2" LANGUAGES CXX)

file(GLOB_RECURSE tests_sources *.cpp *.h)

add_executable(${PROJECT_NAME} ${tests_sources} ${CMAKE_CURRENT_SOURCE_DIR}/../MainHelper.h ${CMAKE_CURRENT_SOURCE_DIR}/../MainHelper.cpp)

# Linked libraries
target_link_libraries(${PROJECT_NAME} PRIVATE TIMEngine2)

# For nicer IDE views
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "executable")

source_group(
  TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES ${tests_sources})