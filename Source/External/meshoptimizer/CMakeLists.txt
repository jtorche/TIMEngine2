cmake_minimum_required( VERSION 3.16 )
project(meshoptimizer)

################################
# Add common source 

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/.")

file(GLOB_RECURSE meshoptimizer_srcs *.h *.cpp)
# set( meshoptimizer_srcs tinyxml.cpp tinyxmlparser.cpp tinyxmlerror.cpp tinystr.cpp )
# set( meshoptimizer_hdrs tinyxml.h tinystr.h )

add_library( meshoptimizer ${meshoptimizer_srcs})
set_target_properties( meshoptimizer PROPERTIES FOLDER External/meshoptimizer )
