cmake_minimum_required (VERSION 3.5)

project(mrpt-nanogui)

if (POLICY CMP0058)
  cmake_policy(SET CMP0058 NEW)
endif()

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

set(NANOGUI_BUILD_SHARED_DEFAULT ON)

option(NANOGUI_BUILD_EXAMPLE "Build NanoGUI example application?" OFF)
set(NANOGUI_BUILD_SHARED ${NANOGUI_BUILD_SHARED_DEFAULT} CACHE BOOL "Build NanoGUI as a shared library?" FORCE)
option(NANOGUI_BUILD_PYTHON  "Build a Python plugin for NanoGUI?" OFF)
option(NANOGUI_USE_GLAD      "Use Glad OpenGL loader library?" ${NANOGUI_USE_GLAD_DEFAULT})
option(NANOGUI_INSTALL       "Install NanoGUI on `make install`?" ON)

set(NANOGUI_PYTHON_VERSION "" CACHE STRING "Python version to use for compiling the Python plugin")

# Platform-dependent files for libnanogui
set(LIBNANOGUI_EXTRA_SOURCE "")
set(LIBNANOGUI_PYTHON_EXTRA_SOURCE "")

if(APPLE AND NANOGUI_BUILD_SHARED)
  set(CMAKE_MACOSX_RPATH ON)
endif()

include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

macro(CHECK_CXX_COMPILER_AND_LINKER_FLAGS _RESULT _CXX_FLAGS _LINKER_FLAGS)
  set(CMAKE_REQUIRED_FLAGS ${_CXX_FLAGS})
  set(CMAKE_REQUIRED_LIBRARIES ${_LINKER_FLAGS})
  set(CMAKE_REQUIRED_QUIET TRUE)
  check_cxx_source_runs("int main(int argc, char **argv) { return 0; }" ${_RESULT})
  set(CMAKE_REQUIRED_FLAGS "")
  set(CMAKE_REQUIRED_LIBRARIES "")
endmacro()

# GLFW: Build from sources in Windows, use system version in UNIX:
find_package(PkgConfig QUIET)

if (PKG_CONFIG_FOUND)
  pkg_search_module(GLFW glfw3)
endif()

if (GLFW_FOUND)
    message(STATUS "GLFW: System library found.")
    #include_directories(${GLFW_INCLUDE_DIRS})
    #target_link_libraries(simple ${GLFW_LIBRARIES})
else()
    message(STATUS "GLFW: No system library found, building from sources.")

    # Compile GLFW
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
    set(GLFW_BUILD_INSTALL OFF CACHE BOOL " " FORCE)

    # Install required to export cmake targets
    set(GLFW_INSTALL OFF CACHE BOOL " " FORCE)

    set(GLFW_USE_CHDIR OFF CACHE BOOL " " FORCE)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Quench annoying deprecation warnings when compiling GLFW on OSX
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
    endif()

    # Set these variables for glfw, since we cannot control its target directly
    # for it being an external project:
    set(caod_backup ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    set(crod_backup ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw" "ext_build/glfw")
    # Two targets have now been defined: `glfw_objects`, which will be merged into
    # NanoGUI at the end, and `glfw`.  The `glfw` target is the library itself
    # (e.g., libglfw.so), but can be skipped as we do not need to link against it
    # (because we merge `glfw_objects` into NanoGUI).  Skipping is required for
    # XCode, but preferable for all build systems (reduces build artifacts).
    # JLBC 2020: This leads to runtime weird errors in MSVC: disabled.
    #set_target_properties(glfw PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
    #set(OPTIONAL_GLFW_OBJECTS $<TARGET_OBJECTS:glfw_objects>)
    set(GLFW_LIBRARIES glfw)

    # undo changes of global variables:
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${caod_backup})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${crod_backup})
    unset(caod_backup)
    unset(crod_backup)
endif()


if (MSVC)
  # Disable annoying MSVC warnings (all targets)
  add_definitions(/D "_CRT_SECURE_NO_WARNINGS")

  # Parallel build on MSVC (all targets)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

  if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")

    # Disable Eigen vectorization for Windows 32 bit builds (issues with unaligned access segfaults)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DNANOGUI_EIGEN_DONT_ALIGN")
  endif()
endif()

# Compile with compiler warnings turned on
if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|Intel)$")
  CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP14_FLAG)
  CHECK_CXX_COMPILER_FLAG("-std=c++11" HAS_CPP11_FLAG)

  if (HAS_CPP14_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  elseif (HAS_CPP11_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  else()
    message(FATAL_ERROR "Unsupported compiler -- pybind11 requires C++11 support!")
  endif()
endif()

# Various optimizations for shared library release builds
if (NANOGUI_BUILD_SHARED)
  # Disabled by default due to this Eigen issue: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1000780
  option(NANOGUI_ENABLE_LINK_TIME_OPTIMIZATION "Enable lto (link time optimization)" OFF)

  if (U_CMAKE_BUILD_TYPE MATCHES REL AND CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
    # Set the default symbol visibility to hidden
    if (NOT CMAKE_CXX_FLAGS MATCHES "-fvisibility")
      set(CMAKE_CXX_FLAGS "-fvisibility=hidden ${CMAKE_CXX_FLAGS}")
    endif()

    # Enable link time optimization
    if (NOT CMAKE_CXX_FLAGS MATCHES "-flto" AND NANOGUI_ENABLE_LINK_TIME_OPTIMIZATION)
      if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(LTO_CXX_FLAGS "-flto=thin")
        set(LTO_LINKER_FLAGS "-flto=thin")
        if (NOT APPLE AND U_CMAKE_BUILD_TYPE MATCHES MINSIZEREL)
            # Clang Gold plugin does not support -Os
            set(LTO_CXX_FLAGS "${LTO_CXX_FLAGS} -O3")
        endif()
      else()
        set(LTO_CXX_FLAGS "-flto -fno-fat-lto-objects")
        set(LTO_LINKER_FLAGS "-flto")
      endif()

      CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LTO ${LTO_CXX_FLAGS} ${LTO_LINKER_FLAGS})

      if (HAS_LTO)
        message(STATUS "NanoGUI: LTO support enabled.")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LTO_LINKER_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LTO_LINKER_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LTO_CXX_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LTO_CXX_FLAGS}")
      else()
        message(STATUS "NanoGUI: LTO not supported by the compiler.")
      endif()
    endif()
  elseif(MSVC)
    set(Configurations RELEASE RELWITHDEBINFO MINSIZEREL)
    set(LinkTypes EXE SHARED MODULE STATIC)
    foreach(Configuration ${Configurations})
      set("CMAKE_CXX_FLAGS_${Configuration}" "${CMAKE_CXX_FLAGS_${Configuration}} /GL")
      foreach(LinkType ${LinkTypes})
        set("CMAKE_${LinkType}_LINKER_FLAGS_${Configuration}" "${CMAKE_${LinkType}_LINKER_FLAGS_${Configuration}} /LTCG")
      endforeach()
    endforeach()
    message(STATUS "NanoGUI: LTO support enabled.")
  endif()
endif()

# Always use libc++ on Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LIBCPP "-stdlib=libc++" "-stdlib=libc++")
  if (HAS_LIBCPP AND APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LIBCPPABI "-stdlib=libc++" "-stdlib=libc++ -lc++abi")
    if(HAS_LIBCPPABI)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++abi")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc++abi")
      message(STATUS "NanoGUI: using libc++ and libc++abi.")
    else()
      message(STATUS "NanoGUI: using libc++.")
    endif()
  else()
    message(STATUS "NanoGUI: NOT using libc++.")
    if (NOT ("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")) # just use defaults in Emscripten
      # From: https://stackoverflow.com/a/16788372/1631514
      # I would use the native library for each OS i.e. libstdc++ on GNU/Linux and libc++ on Mac OS X.
      # libc++ is not 100% complete on GNU/Linux, and there's no real advantage to using it when libstdc++
      # is more complete. Also, if you want to link to any other libraries written in C++ they will almost certainly have been built with libstdc++ so you'll need to link with that too to use them.
      # Use the libstdc++ lib vs. libc++, to avoid some build errors in MacOS
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libstdc++")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libstdc++")
    endif()
  endif()
endif()

if (NANOGUI_USE_GLAD)
  # Build and include GLAD on Windows
  list(APPEND LIBNANOGUI_EXTRA_SOURCE
     "${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/src/glad.c"
	 "${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/include/glad/glad.h"
	 "${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/include/KHR/khrplatform.h")
  if (MSVC)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/src/glad.c"
      PROPERTIES COMPILE_FLAGS "/wd4055 ")
  endif()
endif()


if (${CMAKE_SYSTEM_NAME} MATCHES "BSD")
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
  if(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
    include_directories(/usr/X11R6/include/)
    link_directories(/usr/X11R6/lib)
  endif()
endif()

# Run simple cmake converter to put font files into the data segment

# Search fonts from system packages first, rely on built-in as last resource.
# This avoid shipping dupplicated & copyrighted files in Debian packages:
# * Roboto-Bold.ttf,  Roboto-Regular.ttf => apt install fonts-roboto-fontface
# * entypo+ is a custom font from: https://github.com/svenevs/nanogui-entypo
#   (it is NOT the same one as provided by the entypo debian package)
find_file(ENTYPO_TTF_FILE           entypo.ttf          PATHS "${CMAKE_CURRENT_SOURCE_DIR}/resources" REQUIRED)
find_file(ROBOTO_BOLD_TTF_FILE      Roboto-Bold.ttf     PATHS /usr/share/fonts/truetype/roboto-fontface/roboto/ "${CMAKE_CURRENT_SOURCE_DIR}/resources" REQUIRED)
find_file(ROBOTO_REGULAR_TTF_FILE   Roboto-Regular.ttf  PATHS /usr/share/fonts/truetype/roboto-fontface/roboto/ "${CMAKE_CURRENT_SOURCE_DIR}/resources" REQUIRED)

# Glob up resource files
set(resources ${ENTYPO_TTF_FILE} ${ROBOTO_BOLD_TTF_FILE} ${ROBOTO_REGULAR_TTF_FILE})
message(STATUS "Font resources: ${resources}")

# Concatenate resource files into a comma separated string
string (REGEX REPLACE "([^\\]|^);" "\\1," resources_string "${resources}")
string (REGEX REPLACE "[\\](.)" "\\1" resources_string "${resources_string}")

# Create command line for running bin2c cmake script
set(bin2c_cmdline
  -DOUTPUT_C=nanogui_resources.cpp
  -DOUTPUT_H=nanogui_resources.h
  "-DINPUT_FILES=${resources_string}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/resources/bin2c.cmake")

# Run bin2c on resource files
add_custom_command(
  OUTPUT nanogui_resources.cpp nanogui_resources.h
  COMMAND ${CMAKE_COMMAND} ARGS ${bin2c_cmdline}
  DEPENDS ${resources}
  COMMENT "Running bin2c"
  PRE_BUILD VERBATIM)

# Needed to generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Set library type
if (NANOGUI_BUILD_SHARED)
  set(NANOGUI_LIBRARY_TYPE "SHARED")
else()
  set(NANOGUI_LIBRARY_TYPE "STATIC")
endif()

if (APPLE OR CMAKE_SYSTEM MATCHES "Linux")
  # Include coroutine support for running the mainloop in detached mode
  add_definitions(-DCORO_SJLJ)
  include_directories(ext/coro)
  list(APPEND LIBNANOGUI_PYTHON_EXTRA_SOURCE ext/coro/coro.c)
endif()

if (APPLE)
  # Use automatic reference counting for Objective-C portions
  add_compile_options(-fobjc-arc)
endif()

if (APPLE)
    list(APPEND LIBNANOGUI_EXTRA_SOURCE src/darwin.mm)
endif()

# For older cmake versions:
if(NOT MSVC AND NOT COMMAND target_link_options)
  #target_link_options(${PROJECT_NAME} PRIVATE -fPIC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC")
endif()

# Compile main NanoGUI library
add_library(${PROJECT_NAME}  ${NANOGUI_LIBRARY_TYPE}
  # Merge NanoVG into the NanoGUI library
  ext/nanovg/src/nanovg.c
  # Merge GLAD into the NanoGUI library (only if needed)
  ${LIBNANOGUI_EXTRA_SOURCE}
  # Fonts etc.
  nanogui_resources.cpp
  include/nanogui/glutil.h src/glutil.cpp
  include/nanogui/common.h src/common.cpp
  include/nanogui/widget.h src/widget.cpp
  include/nanogui/theme.h src/theme.cpp
  include/nanogui/layout.h src/layout.cpp
  include/nanogui/screen.h src/screen.cpp
  include/nanogui/label.h src/label.cpp
  include/nanogui/window.h src/window.cpp
  include/nanogui/popup.h src/popup.cpp
  include/nanogui/checkbox.h src/checkbox.cpp
  include/nanogui/button.h src/button.cpp
  include/nanogui/popupbutton.h src/popupbutton.cpp
  include/nanogui/combobox.h src/combobox.cpp
  include/nanogui/progressbar.h src/progressbar.cpp
  include/nanogui/slider.h src/slider.cpp
  include/nanogui/messagedialog.h src/messagedialog.cpp
  include/nanogui/textbox.h src/textbox.cpp
  include/nanogui/imagepanel.h src/imagepanel.cpp
  include/nanogui/imageview.h src/imageview.cpp
  include/nanogui/vscrollpanel.h src/vscrollpanel.cpp
  include/nanogui/colorwheel.h src/colorwheel.cpp
  include/nanogui/colorpicker.h src/colorpicker.cpp
  include/nanogui/graph.h src/graph.cpp
  include/nanogui/stackedwidget.h src/stackedwidget.cpp
  include/nanogui/tabheader.h src/tabheader.cpp
  include/nanogui/tabwidget.h src/tabwidget.cpp
  include/nanogui/glcanvas.h src/glcanvas.cpp
  include/nanogui/formhelper.h
  include/nanogui/toolbutton.h
  include/nanogui/opengl.h
  include/nanogui/nanogui.h
  include/nanogui/serializer/core.h
  include/nanogui/serializer/opengl.h
  include/nanogui/serializer/sparse.h
  src/serializer.cpp
  ${OPTIONAL_GLFW_OBJECTS}
)

# Enable GLES3 in nanovg, if we are in Emscripten:
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    target_compile_definitions(${PROJECT_NAME} PRIVATE NANOVG_GLES3_IMPLEMENTATION)
endif()

# create alias to make CMake scripts to work as if the project has been really imported:
add_library(mrpt::nanogui ALIAS ${PROJECT_NAME})

if (NOT TARGET Eigen3::Eigen)
    find_package(Eigen3)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)

# The next flag leads to -fPIE, but we ned -fPIC:
#set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
# for gcc and clang, we must build libraries as fPIC:
if(NOT MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
  if (COMMAND target_link_options)
      target_link_options(${PROJECT_NAME} PRIVATE -fPIC)
  endif()
endif()

# Required core libraries on various platforms
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
elseif (APPLE)
    find_library(cocoa_library Cocoa)
    find_library(opengl_library OpenGL)
    find_library(corevideo_library CoreVideo)
    find_library(iokit_library IOKit)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
elseif(CMAKE_SYSTEM MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "BSD")
    # Read: https://cmake.org/cmake/help/latest/module/FindOpenGL.html
    # Prefer the "new" GLVND lib if possible:
    find_package(OpenGL)
    if (NOT "${CMAKE_VERSION}" VERSION_LESS "3.8.2")
        set(NANOGUI_GL_LIB OpenGL::GL)
    else()
        set(NANOGUI_GL_LIB ${OPENGL_gl_LIBRARY})
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${NANOGUI_GL_LIB} Xxf86vm Xrandr X11 pthread) # Xinerama Xcursor Xi
    if (NOT CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
        target_link_libraries(${PROJECT_NAME} PRIVATE rt)
    endif()
    if(CMAKE_SYSTEM MATCHES "Linux")
        target_link_libraries(${PROJECT_NAME} PRIVATE dl)
    endif()
endif()

# Python support: add NANOGUI_PYTHON flag to all targets
if (NANOGUI_BUILD_PYTHON)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DNANOGUI_PYTHON)
endif()

# Generate config.h
set(NANOGUI_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/nanogui-config.h")

set(NANOGUI_CONFIG_FILE_CONTENT "")
string(APPEND NANOGUI_CONFIG_FILE_CONTENT "// Automatically generated do not edit\n\n#pragma once\n\n")

# Compile/link flags for NanoGUI
string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#define NANOGUI_BUILD\n#define NVG_BUILD\n")

if (NANOGUI_USE_GLAD)
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/ext/glad/include>
        $<INSTALL_INTERFACE:include/mrpt/nanogui/include>
        )
    #TODO: Install GLAD in Windows to mrpt/gui/include

    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#define NANOGUI_GLAD\n")
endif()

# Don't export ALL symbols for this project, since it seems to cause problems in MSVC:
set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 0)

# Includes: GLFW
if (GLFW_FOUND)
    target_include_directories(${PROJECT_NAME} PUBLIC ${GLFW_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${GLFW_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC ${GLFW_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/ext/glfw/include>
        $<INSTALL_INTERFACE:include/mrpt/nanogui/include>
        )

    install(TARGETS glfw EXPORT glfw-targets
      RUNTIME DESTINATION bin  COMPONENT Libraries
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Libraries
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Libraries
    )
    export(
      TARGETS glfw
      FILE "${CMAKE_BINARY_DIR}/glfw-targets.cmake"
    )
    file(WRITE "${CMAKE_BINARY_DIR}/glfw-config.cmake"
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/glfw-targets.cmake\")")

    install(
      EXPORT glfw-targets
      DESTINATION share/glfw
    )
endif()

# NANOVG:
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/ext/nanovg/src>
        $<INSTALL_INTERFACE:include/mrpt/nanogui/include>
        )
# NANOGUI itself:
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # For nanogui-config.h
        $<INSTALL_INTERFACE:include/mrpt/nanogui/include>
        )

# Shared library mode: add dllimport/dllexport flags to all symbols
if (NANOGUI_BUILD_SHARED)
    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#define NANOGUI_SHARED\n")
    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#ifndef NVG_SHARED\n")
    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "# define NVG_SHARED\n")
    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#endif\n")

    target_compile_definitions(${PROJECT_NAME} PRIVATE GLAD_GLAPI_EXPORT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NVG_SHARED)
endif()

# Ensure we use a valid .so.x.y.z file name according to common use:
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${MRPT_LIB_PREFIX}${PROJECT_NAME}${MRPT_DLL_VERSION_POSTFIX}
    COMPILE_PDB_NAME "${MRPT_LIB_PREFIX}${PROJECT_NAME}${MRPT_DLL_VERSION_POSTFIX}"
    COMPILE_PDB_NAME_DEBUG "${MRPT_LIB_PREFIX}${PROJECT_NAME}${MRPT_DLL_VERSION_POSTFIX}${CMAKE_DEBUG_POSTFIX}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
    VERSION "${CMAKE_MRPT_VERSION_NUMBER_MAJOR}.${CMAKE_MRPT_VERSION_NUMBER_MINOR}.${CMAKE_MRPT_VERSION_NUMBER_PATCH}"
    SOVERSION ${CMAKE_MRPT_VERSION_NUMBER_MAJOR}.${CMAKE_MRPT_VERSION_NUMBER_MINOR}
    )

if (NANOGUI_USE_GLAD AND NANOGUI_BUILD_SHARED)
    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#ifndef GLAD_GLAPI_EXPORT\n")
    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "# define GLAD_GLAPI_EXPORT\n")
    string(APPEND NANOGUI_CONFIG_FILE_CONTENT "#endif\n")
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLAD_GLAPI_EXPORT_BUILD)
endif()

if (NANOGUI_INSTALL AND EXISTS "${MRPT_SOURCE_DIR}/parse-files/mrpt-xxx-config.cmake.in")
	install(TARGETS ${PROJECT_NAME} EXPORT mrpt-nanogui-targets
		RUNTIME DESTINATION bin  COMPONENT Libraries
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Libraries
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Libraries
		)

	# Create module CMake config file:
	# For local usage from the BUILD directory (without "install"):
	# 1/3: autogenerated target file:
	export(
		TARGETS ${PROJECT_NAME}
		FILE "${CMAKE_BINARY_DIR}/mrpt-nanogui-targets.cmake"
		NAMESPACE mrpt::
	)
	# 2/3: config file with manual list of dependencies:
	set(MRPT_MODULE_NAME nanogui)
  if (NOT GLFW_FOUND) # for windows builds, mainly
    set(ALL_DEPS_LIST glfw)
  endif()
	configure_file(
		"${MRPT_SOURCE_DIR}/parse-files/mrpt-xxx-config.cmake.in"
		"${CMAKE_BINARY_DIR}/mrpt-nanogui-config.cmake" IMMEDIATE @ONLY)
	# 3/3: version file:
	write_basic_package_version_file(
		"${CMAKE_BINARY_DIR}/mrpt-nanogui-config-version.cmake"
		VERSION ${CMAKE_MRPT_FULL_VERSION}
		COMPATIBILITY AnyNewerVersion
	)

	# mrpt-xxx-config.cmake file:
	# Makes the project importable from installed dir:
	# 1/3: autogenerated target file:
	install(
		EXPORT mrpt-nanogui-targets
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
		NAMESPACE mrpt::
	)
	# 2/3: config file with manual list of dependencies:
	# 3/3: version file:
	install(
		FILES
			"${CMAKE_BINARY_DIR}/mrpt-nanogui-config.cmake"
			"${CMAKE_BINARY_DIR}/mrpt-nanogui-config-version.cmake"
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
	)

	# Install public headers:
	set(HEADERS_DIR "${MRPT_SOURCE_DIR}/libs/nanogui/include/")
	if (EXISTS "${HEADERS_DIR}")  # This is mainly to avoid problems with "virtual module" names
		install(
		DIRECTORY
			"${HEADERS_DIR}"
		DESTINATION
			${CMAKE_INSTALL_INCLUDEDIR}/mrpt/nanogui/include/
		)
	endif()
	install(
	FILES
		${NANOGUI_CONFIG_FILE}
	DESTINATION
		${CMAKE_INSTALL_INCLUDEDIR}/mrpt/nanogui/include/
	)

    install(DIRECTORY
        include/nanogui
        DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/mrpt/nanogui/include/
        FILES_MATCHING PATTERN "*.h")

    install(FILES
        ext/nanovg/src/nanovg_gl_utils.h
        ext/nanovg/src/stb_image.h
        ext/nanovg/src/nanovg.h
        ext/nanovg/src/fontstash.h
        ext/nanovg/src/stb_truetype.h
        ext/nanovg/src/nanovg_gl.h
        DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/mrpt/nanogui/include/
        )

    # GLFW3 if not present in the system:
    if (NOT GLFW_FOUND)
        #
        install(DIRECTORY
            ext/glfw/include/GLFW
            DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}/mrpt/nanogui/include/
            )
    endif()
endif()

if (NANOGUI_BUILD_SHARED)
  # When GLFW is merged into the NanoGUI library, this flag must be specified
  set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_DEFINITIONS "GLFW_DLL;NVG_SHARED")
endif()

# Quench warnings while compiling NanoVG
if (CMAKE_COMPILER_IS_GNUCC)
  set_source_files_properties(ext/nanovg/src/nanovg.c PROPERTIES COMPILE_FLAGS -Wno-unused-result)
elseif(MSVC)
  set_source_files_properties(ext/nanovg/src/nanovg.c PROPERTIES COMPILE_FLAGS "/wd4005 /wd4456 /wd4457")
endif()

# Build example application if desired
if(NANOGUI_BUILD_EXAMPLE)
  add_executable(example1      src/example1.cpp)
  add_executable(example2      src/example2.cpp)
  add_executable(example3      src/example3.cpp)
  add_executable(example4      src/example4.cpp)
  add_executable(example_icons src/example_icons.cpp)

  set(GLLIB "")
  if (WIN32)
	  set(GLLIB opengl32)
  elseif (APPLE)
	  set(GLLIB ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
  elseif(CMAKE_SYSTEM MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "BSD")
	  set(GLLIB GL)
  endif()

  target_link_libraries(example1      ${PROJECT_NAME} ${GLLIB})
  target_link_libraries(example2      ${PROJECT_NAME} ${GLLIB})
  target_link_libraries(example3      ${PROJECT_NAME} ${GLLIB})
  target_link_libraries(example4      ${PROJECT_NAME} ${GLLIB})
  target_link_libraries(example_icons ${PROJECT_NAME} ${GLLIB})

  # Copy icons for example application
  file(COPY resources/icons DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (NANOGUI_BUILD_PYTHON)
  # Detect Python

  # Try to autodetect Python (can be overridden manually if needed)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ext/pybind11/tools")
  set(Python_ADDITIONAL_VERSIONS 3.7 3.6 3.5 3.4)
  find_package(PythonLibsNew ${NANOGUI_PYTHON_VERSION})
  if (NOT PYTHONLIBS_FOUND)
    # Python not found -- disable the plugin
    set(NANOGUI_BUILD_PYTHON OFF CACHE BOOL "Build a Python plugin for NanoGUI?" FORCE)
    message(WARNING "NanoGUI: not building the Python plugin!")
  else()
    message(STATUS "NanoGUI: building the Python plugin.")
  endif()
endif()

if (NANOGUI_BUILD_PYTHON)
  # Need PIC code in libnanogui even when compiled as a static library
  set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

  include_directories("ext/pybind11/include" ${PYTHON_INCLUDE_DIR})
  add_library(nanogui-python-obj OBJECT
    python/main.cpp
    python/constants_glfw.cpp
    python/constants_entypo.cpp
    python/eigen.cpp
    python/widget.cpp
    python/layout.cpp
    python/basics.cpp
    python/button.cpp
    python/tabs.cpp
    python/textbox.cpp
    python/theme.cpp
    python/glcanvas.cpp
    python/formhelper.cpp
    python/misc.cpp
    python/glutil.cpp
    python/nanovg.cpp
    python/python.h python/py_doc.h
    ${LIBNANOGUI_PYTHON_EXTRA_SOURCE})

  add_library(nanogui-python SHARED $<TARGET_OBJECTS:nanogui-python-obj>)
  set_property(TARGET nanogui-python-obj PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_target_properties(nanogui-python PROPERTIES OUTPUT_NAME "nanogui")
  target_link_libraries(nanogui-python ${PROJECT_NAME} Eigen3::Eigen)

  target_link_libraries(nanogui-python-obj PUBLIC Eigen3::Eigen)
  target_include_directories(nanogui-python-obj PUBLIC ${${PROJECT_NAME}_SOURCE_DIR}/include)
  target_include_directories(nanogui-python-obj PUBLIC ${PROJECT_NAME})
  target_include_directories(nanogui-python-obj PUBLIC $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/ext/nanovg/src>)

  # Quench warnings on GCC
  if (CMAKE_COMPILER_IS_GNUCC)
    set_property(TARGET nanogui-python-obj APPEND PROPERTY COMPILE_OPTIONS "-Wno-unused-variable")
  endif()

  set_target_properties(nanogui-python PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python)
  # The prefix and extension are provided by FindPythonLibsNew.cmake
  set_target_properties(nanogui-python PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}")
  set_target_properties(nanogui-python PROPERTIES SUFFIX "${PYTHON_MODULE_EXTENSION}")

  if (WIN32)
    # Set output path
    set_target_properties(nanogui-python PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE "Release/python")
    set_target_properties(nanogui-python PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG "Debug/python")
    set_target_properties(nanogui-python PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "MinSizeRel/python")
    set_target_properties(nanogui-python PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "RelWithDebInfo/python")
    set_target_properties(nanogui-python PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "Release/python")
    set_target_properties(nanogui-python PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "Debug/python")
    set_target_properties(nanogui-python PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "MinSizeRel/python")
    set_target_properties(nanogui-python PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "RelWithDebInfo/python")

    # Link against the Python shared library
    target_link_libraries(nanogui-python ${PYTHON_LIBRARY})

    if (MSVC)
      # Optimize for size, /bigobj is needed for due to the heavy template metaprogramming in pybind11
      set_property(TARGET nanogui-python-obj APPEND PROPERTY COMPILE_OPTIONS
        "/bigobj" "$<$<CONFIG:Release>:/Os>" "$<$<CONFIG:MinSizeRel>:/Os>"
        "$<$<CONFIG:RelWithDebInfo>:/Os>")
    endif()
  elseif(UNIX)
    # Optimize for size
    if (U_CMAKE_BUILD_TYPE MATCHES REL)
      set_property(TARGET nanogui-python-obj APPEND PROPERTY COMPILE_OPTIONS "-Os")
    endif()

    # Strip unnecessary sections of the binary on Linux/Mac OS
    if(APPLE)
      set_target_properties(nanogui-python PROPERTIES MACOSX_RPATH ".")
      set_target_properties(nanogui-python PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")

      if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEB)
        add_custom_command(TARGET nanogui-python POST_BUILD COMMAND strip -u -r $<TARGET_FILE:nanogui-python>)
      endif()
    else()
      if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEB)
        add_custom_command(TARGET nanogui-python POST_BUILD COMMAND strip $<TARGET_FILE:nanogui-python>)
      endif()
    endif()
  endif()

  if (NANOGUI_INSTALL)
    install(TARGETS nanogui-python
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
  endif()
endif()

get_directory_property(NANOGUI_HAS_PARENT PARENT_DIRECTORY)
if(NANOGUI_HAS_PARENT)
  # This project is included from somewhere else. Export NANOGUI_EXTRA_LIBS variable
  # 2020: JLBC: changed to cmake exported targets, no need for custom variables.
else()
  # Create documentation for python plugin (optional target for developers)

  string(REPLACE " " ";" MKDOC_CXX_FLAGS_LIST ${CMAKE_CXX_FLAGS})
  get_property(MKDOC_INCLUDE_DIRECTORIES DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
  get_property(MKDOC_COMPILE_DEFINITIONS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)

  foreach (value ${MKDOC_INCLUDE_DIRECTORIES})
    list(APPEND MKDOC_CXX_FLAGS_LIST -I${value})
  endforeach()

  # Make sure platform specific code gets kept in py_doc.h (specifically __doc_nanogui_chdir_to_bundle_parent)
  list(APPEND MKDOC_COMPILE_DEFINITIONS "DOXYGEN_DOCUMENTATION_BUILD")
  foreach (value ${MKDOC_COMPILE_DEFINITIONS})
    list(APPEND MKDOC_CXX_FLAGS_LIST -D${value})
  endforeach()

  add_custom_target(mkdoc COMMAND
    python3 ${PROJECT_SOURCE_DIR}/docs/mkdoc_rst.py
      ${MKDOC_CXX_FLAGS_LIST}
      ${PROJECT_SOURCE_DIR}/include/nanogui/*.h
      > ${CMAKE_CURRENT_SOURCE_DIR}/python/py_doc.h)

endif()

# Write nanogui-config.h
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

# vim: set et ts=2 sw=2 ft=cmake nospell:
