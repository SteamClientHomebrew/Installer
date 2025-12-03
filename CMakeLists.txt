cmake_minimum_required(VERSION 3.50)
# Set the project name and version
project(Installer VERSION 1.0 LANGUAGES CXX)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
add_compile_definitions(UNICODE _UNICODE)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(BUILD_SHARED_LIBS OFF)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_STATIC ON)

add_compile_definitions(
  CURL_STATICLIB
  FMT_HEADER_ONLY
  IMGUI_USE_WCHAR32
)

# Additional size optimizations for release builds
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(
        IMGUI_DISABLE_DEMO_WINDOWS
        IMGUI_DISABLE_DEBUG_TOOLS
        IMGUI_DISABLE_STYLE_EDITOR
    )
endif()

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
    vendor/imgui/src/imgui_draw.cpp
    vendor/imgui/src/imgui_tables.cpp
    vendor/imgui/src/imgui_widgets.cpp
    vendor/imgui/src/imgui_impl_glfw.cpp
    vendor/imgui/src/imgui_impl_opengl3.cpp
    vendor/imgui/include/misc/freetype/imgui_freetype.cpp
    vendor/imgui/src/imgui_stdlib.cpp
)

# Add the executable
if(WIN32)
    set(RESOURCE_FILE "${CMAKE_SOURCE_DIR}/win32/Installer.rc")
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${RESOURCE_FILE})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
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
    find_package(glfw3 CONFIG REQUIRED)
    find_package(GLEW CONFIG REQUIRED)
    find_package(CURL REQUIRED)
    find_package(unofficial-minizip CONFIG REQUIRED) # used for extracting zip files

    target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Freetype::Freetype
        glfw
        gdi32
        opengl32
        GLEW::GLEW
        imm32
        CURL::libcurl
        dwmapi
        unofficial::minizip::minizip
        winmm
        bcrypt
    )
endif()
