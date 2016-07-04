## gl3w ##
execute_process(
	COMMAND git submodule update --init src/gl3w
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
# Download include/GL/glcorearb.h & generate src/gl3w.c and include/GL/gl3w.h
execute_process(
	COMMAND ./gl3w_gen.py
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/gl3w
)
add_subdirectory(gl3w)

## imgui ##

add_library(imgui SHARED imgui/imgui.cpp imgui/imgui_draw.cpp)
install(TARGETS imgui DESTINATION ${INSTALL_LIBRARY_DIR})

## utf8.h ##
set(UTF8H_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/utf8")


add_subdirectory(openboardview)
