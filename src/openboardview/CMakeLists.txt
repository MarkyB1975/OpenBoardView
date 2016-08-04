if(NOT WIN32)
	find_package(SDL2 REQUIRED) # is this really required by windows?
endif()

option(ENABLE_GL3 "Build OpenGL 3 renderer." ON)
option(ENABLE_GLES2 "Build OpenGL ES 2 renderer." ON)

# Link with Cocoa on OS X and GTK3 on other Unix (or MinGW)
# Used for open file dialog
if(UNIX)
	if(APPLE)
		find_library(COCOA_LIBRARY Cocoa)
		find_package(OpenGL REQUIRED)
		set(OPENGL_INCLUDE_DIR ${OPENGL_INCLUDE_DIR}/Headers)
	else(APPLE)
		find_package(PkgConfig REQUIRED)
		pkg_search_module(GTK REQUIRED gtk+-3.0 gtk+-2.0) # gtk2 fallback if gtk3 not found

		if(GTK_FOUND)
			message(STATUS "Found GTK version ${GTK_VERSION}")
			link_directories(${GTK_LIBRARY_DIRS}) # not linked since we load it at runtime
			add_definitions(${GTK_CFLAGS_OTHER})
			add_definitions(-DENABLE_GTK)
		else()
			message(WARNING "GTK not found, file picker will be unavailable.")
		endif()
	endif(APPLE)
endif(UNIX)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../imgui
	${CMAKE_CURRENT_SOURCE_DIR}/../json11
	${GLAD_INCLUDE_DIRS}
	${GTK_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIRS}
	${SQLITE3_INCLUDE_DIRS}
	${CMAKE_BINARY_DIR}/src/zlib
)

if(NOT WIN32)
	include_directories(${SDL2_INCLUDE_DIR})
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
if(MINGW)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNICODE -D_UNICODE")
endif()

set(SOURCES
	annotations.cpp
	confparse.cpp
	vectorhulls.cpp
	history.cpp
	BRD2File.cpp
	BVRFile.cpp
	BDVFile.cpp
	BoardView.cpp
	BRDBoard.cpp
	BRDFile.cpp
	FZFile.cpp
	NetList.cpp
	PartList.cpp
	TextureDDS.cpp
)

#if(WIN32)
#	LIST(APPEND SOURCES
#		imgui_impl_dx9.cpp
#	)
#else()
	if(ENABLE_GL3)
		LIST(APPEND SOURCES
			imgui_impl_sdl_gl3.cpp
		)
	endif()
	if(ENABLE_GLES2)
		LIST(APPEND SOURCES
			imgui_impl_sdl_gles2.cpp
		)
	endif()
#endif()

if(UNIX)
	if(APPLE)
		set(SOURCES ${SOURCES}
			osx.mm
		)
	endif ()
	set(SOURCES ${SOURCES}
		unix.cpp
		main_opengl.cpp
	)
elseif (MINGW) # Should this just be WIN32?
	set(SOURCES ${SOURCES}
		win32.cpp
		main_opengl.cpp
#		main_dx9.cpp
		../vcpp/OpenBoardView.utf8.rc
	)
endif()

# Must be defined in the same directory as the add_executable including the file
set_source_files_properties(${ASSETS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

#SET(CMAKE_EXE_LINKER_FLAGS "-static")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

add_executable(openboardview
	MACOSX_BUNDLE
	WIN32
	${SOURCES}
	${ASSETS}
)

set_target_properties(openboardview PROPERTIES LINK_SEARCH_END_STATIC 1)
set_target_properties(openboardview PROPERTIES MACOSX_BUNDLE_ICON_FILE "brd")

target_link_libraries(openboardview
	imgui
	json11
	${GLAD_LIBRARIES}
	${COCOA_LIBRARY}
	${SDL2_LIBRARY}
	${ZLIB_LIBRARIES}
	${SQLITE3_LIBRARIES}
	mingw32
	SDL2main
	SDL2
	version
	winmm
)

if(UNIX)
	target_link_libraries(openboardview
		dl
	)
endif()

if(ENABLE_GL3)
	add_definitions(-DENABLE_GL3)
endif()
if(ENABLE_GLES2)
	add_definitions(-DENABLE_GLES2)
endif()

install(TARGETS
	openboardview
	RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
	BUNDLE DESTINATION ${INSTALL_BUNDLE_DIR})

if (MINGW)
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/${INSTALL_ASSET_DIR} DESTINATION ${INSTALL_RUNTIME_DIR})
endif ()

add_custom_target(install_obv
	${CMAKE_MAKE_PROGRAM} install
	DEPENDS openboardview
	COMMENT "Installing ${PROJECT_NAME}")
