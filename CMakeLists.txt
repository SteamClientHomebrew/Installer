cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(Installer VERSION 1.0 LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(BUILD_SHARED_LIBS OFF)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_STATIC ON)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -fpermissive -fno-diagnostics-show-option")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fvisibility=hidden -s")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -fvisibility=hidden")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
endif()

add_compile_definitions(
  CURL_STATICLIB
  FMT_HEADER_ONLY
)

# Define source files
set(IMGUI_DIR "../..")
set(SOURCES
    src/main.cc
    src/texture.cc
    src/wndproc.cc
    src/renderer.cc
    src/routes/home.cc
    src/routes/install_prompt.cc
    src/routes/installer.cc
    src/routes/uninstall_select.cc
    src/components/titlebar.cc
    src/components/checkbox.cc
    src/components/bottombar.cc
    src/components/message.cc
    src/animate.cc
    src/router.cc
    src/dpi.cc
    src/installer/task_scheduler.cc
    src/installer/unzip.cc
    vendor/imgui/src/imgui.cpp
    vendor/imgui/src/imgui_demo.cpp
    vendor/imgui/src/imgui_draw.cpp
    vendor/imgui/src/imgui_tables.cpp
    vendor/imgui/src/imgui_widgets.cpp
    vendor/imgui/src/imgui_impl_sdl2.cpp
    vendor/imgui/src/imgui_impl_sdlrenderer2.cpp
    vendor/imgui/include/misc/freetype/imgui_freetype.cpp
    vendor/imgui/src/imgui_stdlib.cpp
)

# Add the executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

find_program(WINDRES windres)
if (WINDRES)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/version.o
        COMMAND ${WINDRES} -i ${CMAKE_SOURCE_DIR}/scripts/version.rc -o ${CMAKE_BINARY_DIR}/version.o
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/version.rc
    )

    add_custom_target(resource DEPENDS ${CMAKE_BINARY_DIR}/version.o)
    add_dependencies(${PROJECT_NAME} resource)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/version.o)
endif()


# Include directories
include_directories(include)
include_directories(vendor/imgui/include)
include_directories(vendor/imgui/extern)
include_directories(vendor/nlohmann/include)
include_directories(vendor/ini/src)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    message(STATUS "Building on Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)

    target_compile_options(${PROJECT_NAME} PRIVATE ${GLFW_CFLAGS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GLFW_LDFLAGS} GL)

elseif(APPLE)
    message(STATUS "Building on macOS")
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    find_library(GLFW_LIBRARY glfw REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${OPENGL_LIBRARY}
        ${GLFW_LIBRARY}
    )

elseif(WIN32)
    message(STATUS "Building on Windows")
    find_package(Freetype REQUIRED)
    find_package(Stb REQUIRED)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(CURL REQUIRED)
    find_package(unofficial-minizip CONFIG REQUIRED) # used for extracting zip files

    target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})
    
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Freetype::Freetype
        SDL2::SDL2-static
        gdi32
        opengl32
        imm32
        CURL::libcurl
        dwmapi
        unofficial::minizip::minizip
        winmm
    )
endif()

# Add compilation flags
target_compile_options(${PROJECT_NAME} PRIVATE
    -g
    -Wall
    -Wformat
)
