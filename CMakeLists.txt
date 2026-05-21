# ------------------------------------------------------------------------------
#        Mobile Robot Programming Toolkit (MRPT)
#
# Copyright (c) 2005-2026, Jose Luis Blanco-Claraco, contributors
# All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)
project(mrpt_nanogui LANGUAGES C CXX)

# MRPT CMake scripts
find_package(mrpt_common REQUIRED)

# -------------------------------------------------------------------------
# Options & Dependencies
# -------------------------------------------------------------------------
if (NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanovg/src")
  message(FATAL_ERROR "The NanoGUI dependency repositories (NANOVG, etc.) are missing! "
    "You probably did not clone the project with --recursive. It is possible to recover "
    "by calling \"git submodule update --init --recursive\"")
endif()

if (WIN32)
  set(NANOGUI_USE_GLAD_DEFAULT ON)
else()
  set(NANOGUI_USE_GLAD_DEFAULT OFF)
endif()

option(NANOGUI_BUILD_EXAMPLE "Build NanoGUI example applications?" OFF)
option(NANOGUI_USE_GLAD      "Use Glad OpenGL loader library?" ${NANOGUI_USE_GLAD_DEFAULT})

if (BUILD_SHARED_LIBS AND APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

# -------------------------------------------------------------------------
# GLFW Configuration
# -------------------------------------------------------------------------
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_search_module(GLFW glfw3)
endif()

if (GLFW_FOUND)
    message(STATUS "GLFW: System library found.")
else()
    message(STATUS "GLFW: No system library found, building from sources.")
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
    set(GLFW_BUILD_INSTALL OFF CACHE BOOL " " FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL " " FORCE)
    set(GLFW_USE_CHDIR OFF CACHE BOOL " " FORCE)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
    endif()

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw" "ext_build/glfw")
    set(GLFW_LIBRARIES glfw)
endif()

# -------------------------------------------------------------------------
# Resource / Font Compilation
# -------------------------------------------------------------------------
find_file(ENTYPO_TTF_FILE           entypo.ttf          PATHS "${CMAKE_CURRENT_SOURCE_DIR}/resources" REQUIRED)
find_file(ROBOTO_BOLD_TTF_FILE      Roboto-Bold.ttf     PATHS /usr/share/fonts/truetype/roboto-fontface/roboto/ "${CMAKE_CURRENT_SOURCE_DIR}/resources" REQUIRED)
find_file(ROBOTO_REGULAR_TTF_FILE   Roboto-Regular.ttf  PATHS /usr/share/fonts/truetype/roboto-fontface/roboto/ "${CMAKE_CURRENT_SOURCE_DIR}/resources" REQUIRED)

set(resources ${ENTYPO_TTF_FILE} ${ROBOTO_BOLD_TTF_FILE} ${ROBOTO_REGULAR_TTF_FILE})
message(STATUS "Font resources: ${resources}")

string (REGEX REPLACE "([^\\]|^);" "\\1," resources_string "${resources}")
string (REGEX REPLACE "[\\](.)" "\\1" resources_string "${resources_string}")

set(bin2c_cmdline
  -DOUTPUT_C=nanogui_resources.cpp
  -DOUTPUT_H=nanogui_resources.h
  "-DINPUT_FILES=${resources_string}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/resources/bin2c.cmake")

add_custom_command(
  OUTPUT nanogui_resources.cpp nanogui_resources.h
  COMMAND ${CMAKE_COMMAND} ARGS ${bin2c_cmdline}
  DEPENDS ${resources}
  COMMENT "Running bin2c"
  PRE_BUILD VERBATIM)

# -------------------------------------------------------------------------
# Define Sources and Headers
# -------------------------------------------------------------------------
set(LIB_SOURCES
  ext/nanovg/src/nanovg.c
  nanogui_resources.cpp
  src/glutil.cpp src/common.cpp src/widget.cpp src/theme.cpp 
  src/layout.cpp src/screen.cpp src/label.cpp src/window.cpp 
  src/popup.cpp src/checkbox.cpp src/button.cpp src/popupbutton.cpp 
  src/combobox.cpp src/progressbar.cpp src/slider.cpp src/messagedialog.cpp 
  src/textbox.cpp src/imagepanel.cpp src/imageview.cpp src/vscrollpanel.cpp 
  src/colorwheel.cpp src/colorpicker.cpp src/graph.cpp src/stackedwidget.cpp 
  src/tabheader.cpp src/tabwidget.cpp src/glcanvas.cpp src/dockablepanel.cpp 
  src/serializer.cpp
)

set(LIB_PUBLIC_HEADERS
  include/nanogui/glutil.h include/nanogui/common.h include/nanogui/widget.h 
  include/nanogui/theme.h include/nanogui/layout.h include/nanogui/screen.h 
  include/nanogui/label.h include/nanogui/window.h include/nanogui/popup.h 
  include/nanogui/checkbox.h include/nanogui/button.h include/nanogui/popupbutton.h 
  include/nanogui/combobox.h include/nanogui/progressbar.h include/nanogui/slider.h 
  include/nanogui/messagedialog.h include/nanogui/textbox.h include/nanogui/imagepanel.h 
  include/nanogui/imageview.h include/nanogui/vscrollpanel.h include/nanogui/colorwheel.h 
  include/nanogui/colorpicker.h include/nanogui/graph.h include/nanogui/stackedwidget.h 
  include/nanogui/tabheader.h include/nanogui/tabwidget.h include/nanogui/glcanvas.h 
  include/nanogui/dockablepanel.h include/nanogui/formhelper.h include/nanogui/toolbutton.h 
  include/nanogui/opengl.h include/nanogui/nanogui.h 
  include/nanogui/serializer/core.h include/nanogui/serializer/opengl.h include/nanogui/serializer/sparse.h
)

if (APPLE)
  list(APPEND LIB_SOURCES src/darwin.mm)
elseif(APPLE OR CMAKE_SYSTEM MATCHES "Linux")
  add_definitions(-DCORO_SJLJ)
  include_directories(ext/coro)
endif()

if (NANOGUI_USE_GLAD)
  list(APPEND LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/src/glad.c")
  list(APPEND LIB_PUBLIC_HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/include/glad/glad.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/include/KHR/khrplatform.h")
endif()

# -------------------------------------------------------------------------
# Create MRPT Library Target
# -------------------------------------------------------------------------
if (NOT TARGET Eigen3::Eigen)
    find_package(Eigen3 REQUIRED)
endif()

mrpt_add_library(
  TARGET ${PROJECT_NAME}
  SOURCES ${LIB_SOURCES} ${LIB_PUBLIC_HEADERS}
  PUBLIC_LINK_LIBRARIES Eigen3::Eigen
)

# Includes
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext/nanovg/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # For nanogui-config.h
  $<INSTALL_INTERFACE:include/mrpt/nanogui/include>
)

if (NANOGUI_USE_GLAD)
  target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/include>)
  if (MSVC)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/src/glad.c" PROPERTIES COMPILE_FLAGS "/wd4055")
  endif()
endif()

if (NOT GLFW_FOUND)
  target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/include>)
endif()

# Core system libraries
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
elseif (APPLE)
    find_library(cocoa_library Cocoa)
    find_library(opengl_library OpenGL)
    find_library(corevideo_library CoreVideo)
    find_library(iokit_library IOKit)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
elseif(CMAKE_SYSTEM MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "BSD")
    find_package(OpenGL REQUIRED)
    if (NOT CMAKE_VERSION VERSION_LESS "3.8.2")
        target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_gl_LIBRARY})
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE Xxf86vm Xrandr X11 pthread)
    if (NOT CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
        target_link_libraries(${PROJECT_NAME} PRIVATE rt)
    endif()
    if(CMAKE_SYSTEM MATCHES "Linux")
        target_link_libraries(${PROJECT_NAME} PRIVATE dl)
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${GLFW_LIBRARIES})

# Target Specific Flags
if (APPLE)
  set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_OPTIONS "-fobjc-arc")
endif()

if (MSVC AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  target_compile_definitions(${PROJECT_NAME} PRIVATE NANOGUI_EIGEN_DONT_ALIGN)
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
  target_compile_definitions(${PROJECT_NAME} PRIVATE NANOVG_GLES3_IMPLEMENTATION)
endif()

if (CMAKE_COMPILER_IS_GNUCC)
  set_source_files_properties(ext/nanovg/src/nanovg.c PROPERTIES COMPILE_FLAGS -Wno-unused-result)
elseif(MSVC)
  set_source_files_properties(ext/nanovg/src/nanovg.c PROPERTIES COMPILE_FLAGS "/wd4005 /wd4456 /wd4457")
endif()

# -------------------------------------------------------------------------
# Dynamic Library Config & Headers
# -------------------------------------------------------------------------
set(NANOGUI_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/nanogui-config.h")
set(NANOGUI_CONFIG_FILE_CONTENT "// Automatically generated do not edit\n\n#pragma once\n\n")
string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#define NANOGUI_BUILD\n#define NVG_BUILD\n")

if (NANOGUI_USE_GLAD)
    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#define NANOGUI_GLAD\n")
endif()

if (BUILD_SHARED_LIBS)
    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#define NANOGUI_SHARED\n")
    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#ifndef NVG_SHARED\n# define NVG_SHARED\n#endif\n")
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLAD_GLAPI_EXPORT NVG_SHARED)
    
    if(NOT GLFW_FOUND)
        set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS "GLFW_DLL")
    endif()
    if (NANOGUI_USE_GLAD)
        string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#ifndef GLAD_GLAPI_EXPORT\n# define GLAD_GLAPI_EXPORT\n#endif\n")
        target_compile_definitions(${PROJECT_NAME} PRIVATE GLAD_GLAPI_EXPORT_BUILD)
    endif()
endif()

# Write config header
set(do_write_config 1)
if (EXISTS ${NANOGUI_CONFIG_FILE})
	file(READ ${NANOGUI_CONFIG_FILE} TMP_)
	if ("${TMP_}" STREQUAL "${NANOGUI_CONFIG_FILE_CONTENT}")
		set(do_write_config 0)
	endif()
endif()
if (do_write_config)
	file(WRITE ${NANOGUI_CONFIG_FILE} ${NANOGUI_CONFIG_FILE_CONTENT})
endif()

# Extra Custom Installs (mrpt_add_library handles standard 'include/')
install(FILES ${NANOGUI_CONFIG_FILE} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mrpt/nanogui/include/)
install(FILES
    ext/nanovg/src/nanovg_gl_utils.h ext/nanovg/src/stb_image.h
    ext/nanovg/src/nanovg.h ext/nanovg/src/fontstash.h
    ext/nanovg/src/stb_truetype.h ext/nanovg/src/nanovg_gl.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mrpt/nanogui/include/
)
if (NOT GLFW_FOUND)
    install(DIRECTORY ext/glfw/include/GLFW DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mrpt/nanogui/include/)
endif()

# -------------------------------------------------------------------------
# Example Applications
# -------------------------------------------------------------------------
if(NANOGUI_BUILD_EXAMPLE)
  set(GLLIB "")
  if (WIN32)
	  set(GLLIB opengl32)
  elseif (APPLE)
	  set(GLLIB ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
  elseif(CMAKE_SYSTEM MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "BSD")
	  set(GLLIB OpenGL::GL)
  endif()

  foreach(example_name example1 example2 example3 example4 example_icons example_dockpanel)
      add_executable(${example_name} src/${example_name}.cpp)
      target_link_libraries(${example_name} PRIVATE ${PROJECT_NAME} ${GLLIB})
      # Note: For strict MRPT guidelines, you could use mrpt_add_executable(${example_name} ...) here instead.
  endforeach()

  file(COPY resources/icons DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()