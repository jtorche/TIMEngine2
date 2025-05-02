cmake_minimum_required(VERSION 3.28)

project(TIMEngine2Solution DESCRIPTION "TIMEngine2Solution" LANGUAGES CXX)
set(CMAKE_SUPPRESS_REGENERATION true)

add_definitions(-DUSE_VCPP)
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DTIM_DEBUG")

# Only do these if this is the main project, and not if it is included through
# add_subdirectory
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # C++ Standard
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Ensure -std=c++xx instead of -std=g++xx
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Nicely support folders in IDE's
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

option(ENABLE_VR "Check it to add support for VR headset (OpenVR)." OFF)
option(ENABLE_QT "Check it to build TIMEditor with QT." OFF)

# Libraries code
add_subdirectory(TIMEngine2)
add_subdirectory(OpenVR)
add_subdirectory(External/tinyxml)
add_subdirectory(Tests)
add_subdirectory(PortalGame)
add_subdirectory(TIMEditor)