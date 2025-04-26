cmake_minimum_required(VERSION 3.28)

project(PortalGame DESCRIPTION "A Portal Adventure" LANGUAGES CXX)

file(GLOB_RECURSE portalGame_sources *.cpp *.h)

add_executable(${PROJECT_NAME} ${portalGame_sources} ${CMAKE_CURRENT_SOURCE_DIR}/../MainHelper.h ${CMAKE_CURRENT_SOURCE_DIR}/../MainHelper.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/../Data/")

# Linked libraries
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE TIMEngine2)
target_link_libraries(${PROJECT_NAME} PRIVATE TIM_VR)

# For nicer IDE views
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "executable")

source_group(
  TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES ${portalGame_sources})