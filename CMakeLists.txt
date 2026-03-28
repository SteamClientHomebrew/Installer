cmake_minimum_required(VERSION 3.31.6)
# Set the project name and version
project(Installer VERSION 1.0 LANGUAGES CXX)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
add_compile_definitions(UNICODE _UNICODE)
# Read the Millennium version from the version file
file(STRINGS "resources/versioning/version" VERSION_LINES LIMIT_COUNT 2)
list(GET VERSION_LINES 1 MILLENNIUM_VERSION)
string(STRIP "${MILLENNIUM_VERSION}" MILLENNIUM_VERSION)

# Make cmake re-run if the version file changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "resources/versioning/version")

# Generate version.h from version.h.in
configure_file(src/include/version.h.in ${CMAKE_BINARY_DIR}/version.h)

add_compile_definitions(MILLENNIUM_VERSION="${MILLENNIUM_VERSION}")

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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

# Fetch all dependencies via FetchContent
include(${CMAKE_CURRENT_SOURCE_DIR}/resources/cmake/bootstrap_deps.cmake)

# ImGui source files (from FetchContent-populated source directory)
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

# Define source files
set(SOURCES
    src/main.cc
    src/window/texture.cc
    src/window/wndproc.cc
    src/window/renderer.cc
    src/util/updater.cc
    src/routes/router.cc
    src/routes/home.cc
    src/routes/install_prompt.cc
    src/routes/installer.cc
    src/routes/uninstall_select.cc
    src/components/titlebar.cc
    src/components/checkbox.cc
    src/components/bottombar.cc
    src/components/message.cc
    src/util/animate.cc
    src/util/semver.cc
    src/window/dpi.cc
    src/installer/task_scheduler.cc
    src/installer/unzip.cc
    ${IMGUI_SOURCES}
)

# Add the executable
if(WIN32)
    set(RESOURCE_FILE "${CMAKE_SOURCE_DIR}/resources/win32/Installer.rc")
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${RESOURCE_FILE})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

# Include directories
include_directories(src/include)
include_directories(SYSTEM ${imgui_SOURCE_DIR})
include_directories(SYSTEM ${imgui_SOURCE_DIR}/backends)
include_directories(SYSTEM ${imgui_SOURCE_DIR}/misc/cpp)
include_directories(SYSTEM ${imgui_markdown_SOURCE_DIR})
include_directories(SYSTEM ${imspinner_SOURCE_DIR})
include_directories(SYSTEM ${nlohmann_json_SOURCE_DIR}/include)
include_directories(SYSTEM ${mini_SOURCE_DIR}/src)
include_directories(SYSTEM ${stb_SOURCE_DIR})
include_directories(SYSTEM ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})

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

    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${minizip_ng_SOURCE_DIR})

    target_link_libraries(${PROJECT_NAME} PRIVATE
        freetype
        glfw
        gdi32
        opengl32
        libglew_static
        imm32
        CURL::libcurl
        dwmapi
        minizip
        winmm
        bcrypt
    )
endif()
