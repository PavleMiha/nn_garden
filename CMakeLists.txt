# This is free and unencumbered software released into the public domain. 

cmake_minimum_required( VERSION 3.0 )
project( nn_garden )
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory( deps )

include( deps/bgfx.cmake/cmake/util/ConfigureDebugging.cmake )

add_library( bigg STATIC bigg/src/bigg.cpp bigg/src/bigg_assets.h bigg/src/bigg_imgui.hpp bigg/src/bigg_shaders.hpp bigg/include/bigg.hpp )

target_link_libraries( bigg PUBLIC bgfx imgui glfw glm clip )
target_include_directories( bigg PUBLIC bigg/include )

set(SOURCE_FILES
	src/imnodes.cpp
	src/imgui_canvas.cpp

	src/value.cpp
	src/edit_operation.cpp
	src/computation_graph.cpp
	src/context.cpp
)

set(HEADER_FILES
	include/imnodes.h
	include/imnodes_internal.h

	include/value.h
	include/edit_operation.h
	include/computation_graph.h
	include/context.h

	include/icons_font_awesome.h

	include/json.hpp
)

include_directories(include deps/imgui deps/clip)

add_executable( nn_garden ${SOURCE_FILES} ${HEADER_FILES} src/main.cpp)

configure_file(data/graph.json graph.json COPYONLY)
configure_file(data/imgui.ini  imgui.ini COPYONLY)

configure_file(data/spiral.csv spiral.csv COPYONLY)
configure_file(data/flat.csv   flat.csv COPYONLY)
configure_file(data/wavy.csv   wavy.csv COPYONLY)
configure_file(data/donut.csv  donut.csv COPYONLY)

configure_file(data/fontawesome-webfont.ttf fontawesome-webfont.ttf COPYONLY)

target_link_libraries( nn_garden bigg )

#set_target_properties( imgui_demo PROPERTIES FOLDER "examples" )

add_executable(nn_playground_tests tests/backprop_tests.cpp ${SOURCE_FILES} ${HEADER_FILES})

add_test(FULLTEST nn_playground_tests COMMAND nn_playground_tests)

option( BIGG_EXAMPLES "Build examples." ON )

if( BIGG_EXAMPLES )
	add_subdirectory( examples )
endif()

option( BIGG_ASSET_GEN "Generate binary assets." OFF )
if( BIGG_ASSET_GEN )
	add_subdirectory( assets/gen )
endif()
