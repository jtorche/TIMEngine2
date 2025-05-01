cmake_minimum_required(VERSION 3.28)

if (ENABLE_QT)
	project(TIMEditor DESCRIPTION "TIMEngine2 Editor" LANGUAGES CXX)
	
	find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)
	qt_standard_project_setup()
	file(GLOB_RECURSE timeditor_sources *.cpp *.h)
	qt6_add_resources(timeditor_resources EditorResources.qrc)
	
	add_executable(${PROJECT_NAME} ${timeditor_sources} ${timeditor_resources})
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/../Data/")
	
	# Linked libraries
	target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
	target_link_libraries(${PROJECT_NAME} PRIVATE TIMEngine2)
	target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)
	target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Gui)
	target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::OpenGLWidgets)
	
	add_custom_command(TARGET TIMEditor POST_BUILD 
		COMMAND Qt6::windeployqt
		ARGS $<TARGET_FILE:TIMEditor>
	)

	# For nicer IDE views
	set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "executable")
	
	source_group(
		TREE ${CMAKE_CURRENT_SOURCE_DIR}
		FILES ${timeditor_sources})
  
endif()