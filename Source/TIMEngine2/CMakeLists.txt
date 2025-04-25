cmake_minimum_required(VERSION 3.8)

project(TIMEngine2 DESCRIPTION "TIMEngine2 library" LANGUAGES CXX)

add_definitions(-DUSE_VCPP)
add_definitions(-DNO_OGG_VORBIS)

# Note that headers are optional, and do not affect add_library, but they will
# not show up in IDEs unless they are listed in add_library.
file(GLOB_RECURSE core_sources core/*.h core/*.cpp)
file(GLOB_RECURSE resource_sources resource/*.h resource/*.cpp)
file(GLOB_RECURSE scene_sources scene/*.h scene/*.cpp)
file(GLOB_RECURSE renderer_sources renderer/*.h renderer/*.cpp)
file(GLOB_RECURSE interface_sources interface/*.h interface/*.cpp)
file(GLOB_RECURSE openal_sources openAL/*.hpp openAL/*.cpp)
file(GLOB_RECURSE bullet_sources bullet/*.hpp bullet/*.cpp)

add_library(${PROJECT_NAME} ${core_sources} ${resource_sources} ${scene_sources} ${renderer_sources} ${interface_sources} ${openal_sources} ${bullet_sources})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Weird but core is in global include path
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core/)

# External Libs

# GLEW
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../Libs/glew/include/)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../Libs/glew/glew32.lib)

# OpenAL
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../Libs/openal/include/)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../Libs/openal/OpenAL32.lib)

# Bullet
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../Libs/bullet3/include/bullet/)

# Tiny XML
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../External/tinyxml)
target_link_libraries(${PROJECT_NAME} PRIVATE tinyxml)

source_group(
  TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES ${core_sources})
source_group(
  TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES ${resource_sources})
source_group(
  TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES ${scene_sources})
source_group(
  TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES ${renderer_sources})
source_group(
  TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES ${interface_sources})
source_group(
  TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES ${openal_sources})
source_group(
  TREE ${CMAKE_CURRENT_SOURCE_DIR}
  FILES ${bullet_sources})