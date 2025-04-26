cmake_minimum_required( VERSION 3.16 )
project(tinyxml)

################################
# Add common source 

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/.")

add_definitions(-DTIXML_USE_STL)

set( tinyxml_srcs tinyxml.cpp tinyxmlparser.cpp tinyxmlerror.cpp tinystr.cpp )
set( tinyxml_hdrs tinyxml.h tinystr.h )

add_library( tinyxml ${tinyxml_srcs} ${tinyxml_hdrs} )
set_target_properties( tinyxml PROPERTIES FOLDER External/tinyxml )
