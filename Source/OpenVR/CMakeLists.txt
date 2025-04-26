cmake_minimum_required(VERSION 3.16)

if (ENABLE_VR)
	project(TIM_VR DESCRIPTION "VR module for TIM_Engine2" LANGUAGES CXX)

	file(GLOB_RECURSE openvr_sources *.h *.cpp)
	add_library(${PROJECT_NAME} ${openvr_sources})
	
	target_link_libraries(${PROJECT_NAME} PUBLIC TIMEngine2)
	
	source_group(
		TREE ${CMAKE_CURRENT_SOURCE_DIR}
		FILES ${openvr_sources})
endif()