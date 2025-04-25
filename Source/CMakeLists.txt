cmake_minimum_required(VERSION 3.16)

project(TIMEngine2Solution DESCRIPTION "TIMEngine2Solution" LANGUAGES CXX)

# Only do these if this is the main project, and not if it is included through
# add_subdirectory
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # C++ Standard
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Ensure -std=c++xx instead of -std=g++xx
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Nicely support folders in IDE's
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

SET(CMAKE_CXX_FLAGS_DEBUG "/DTIM_DEBUG")

# Libraries code
add_subdirectory(TIMEngine2)
add_subdirectory(External/tinyxml)
add_subdirectory(Tests)