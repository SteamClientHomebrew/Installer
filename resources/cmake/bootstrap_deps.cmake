# bootstrap_deps.cmake - Dependency configuration and fetching for Millennium Installer
# Follows the same FetchContent pattern used in the main Millennium project.

set(CMAKE_POSITION_INDEPENDENT_CODE ON  CACHE BOOL "Enable position independent code")
set(BUILD_SHARED_LIBS               OFF CACHE BOOL "Build static libraries by default")
set(BUILD_STATIC_LIBS               ON  CACHE BOOL "Build static libraries by default")
set(BUILD_TESTS                     OFF CACHE BOOL "Disable building tests")
set(BUILD_EXAMPLES                  OFF CACHE BOOL "Disable building examples")

# -- GLFW -------------------------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

# -- Freetype ----------------------------------------------------------------
set(FT_DISABLE_HARFBUZZ ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON  CACHE BOOL "" FORCE)

# -- CURL --------------------------------------------------------------------
set(BUILD_CURL_EXE       OFF CACHE BOOL "" FORCE)
set(CURL_STATICLIB       ON  CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH2     OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_INSTALL ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP    ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAPS   ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_RTSP    ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_DICT    ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_TELNET  ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_TFTP    ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_POP3    ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_IMAP    ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_SMTP    ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_GOPHER  ON  CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL      OFF CACHE BOOL "" FORCE)

if(WIN32)
    set(CURL_USE_SCHANNEL   ON  CACHE INTERNAL "" FORCE)
    set(CURL_WINDOWS_SSPI   ON  CACHE INTERNAL "" FORCE)
    set(CURL_USE_OPENSSL    OFF CACHE BOOL     "" FORCE)
else()
    set(CURL_USE_OPENSSL    ON  CACHE BOOL     "" FORCE)
    set(CURL_USE_SCHANNEL   OFF CACHE INTERNAL "" FORCE)
    set(CURL_WINDOWS_SSPI   OFF CACHE INTERNAL "" FORCE)
endif()

# -- minizip-ng --------------------------------------------------------------
set(MZ_BZIP2      OFF CACHE BOOL "" FORCE)
set(MZ_FETCH_LIBS OFF CACHE BOOL "" FORCE)
set(MZ_ZLIB       ON  CACHE BOOL "" FORCE)
set(MZ_ICONV      OFF CACHE BOOL "" FORCE)
set(MZ_LZMA       OFF CACHE BOOL "" FORCE)
set(MZ_ZSTD       OFF CACHE BOOL "" FORCE)
set(MZ_PKCRYPT    OFF CACHE BOOL "" FORCE)
set(MZ_WZAES      OFF CACHE BOOL "" FORCE)
set(MZ_OPENSSL    OFF CACHE BOOL "" FORCE)
set(MZ_SIGNING    OFF CACHE BOOL "" FORCE)
set(MZ_COMPAT     ON  CACHE BOOL "" FORCE)

# -- zlib-ng -----------------------------------------------------------------
set(ZLIB_COMPAT         ON  CACHE BOOL "" FORCE)
set(ZLIB_ENABLE_TESTS   OFF CACHE BOOL "" FORCE)
set(ZLIBNG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)

# -- GLEW (glew-cmake) -------------------------------------------------------
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(glew-cmake_BUILD_STATIC ON  CACHE BOOL "" FORCE)
set(ONLY_LIBS               ON  CACHE BOOL "" FORCE)

# -- nlohmann/json -----------------------------------------------------------
set(JSON_BuildTests    OFF CACHE BOOL "" FORCE)
set(JSON_Install       OFF CACHE BOOL "" FORCE)

# ============================================================================
# FetchContent declarations
# ============================================================================
include(FetchContent)

message(STATUS "[Installer] Fetching dependencies...")

# Independent dependencies (built via FetchContent_MakeAvailable directly)
FetchContent_Declare(zlib           GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git           GIT_TAG 2.2.4               GIT_SHALLOW TRUE)
FetchContent_Declare(glfw           GIT_REPOSITORY https://github.com/glfw/glfw.git                 GIT_TAG 3.4                 GIT_SHALLOW TRUE)
FetchContent_Declare(glew           GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git       GIT_TAG glew-cmake-2.2.0    GIT_SHALLOW TRUE)
FetchContent_Declare(nlohmann_json  GIT_REPOSITORY https://github.com/nlohmann/json.git             GIT_TAG v3.11.3             GIT_SHALLOW TRUE)
FetchContent_Declare(mini           GIT_REPOSITORY https://github.com/metayeti/mINI.git             GIT_TAG 0.9.17              GIT_SHALLOW TRUE)

# Dependencies that need zlib built first (SOURCE_SUBDIR fakedir defers add_subdirectory)
FetchContent_Declare(freetype       GIT_REPOSITORY https://github.com/freetype/freetype.git         GIT_TAG VER-2-13-3          GIT_SHALLOW TRUE SOURCE_SUBDIR fakedir)
FetchContent_Declare(curl           GIT_REPOSITORY https://github.com/curl/curl.git                 GIT_TAG curl-8_11_1         GIT_SHALLOW TRUE SOURCE_SUBDIR fakedir)
FetchContent_Declare(minizip_ng     GIT_REPOSITORY https://github.com/zlib-ng/minizip-ng.git        GIT_TAG 4.0.7               GIT_SHALLOW TRUE SOURCE_SUBDIR fakedir)

# Header-only / no-CMake dependencies (SOURCE_SUBDIR fakedir skips add_subdirectory)
FetchContent_Declare(imgui          GIT_REPOSITORY https://github.com/ocornut/imgui.git             GIT_TAG master              GIT_SHALLOW TRUE SOURCE_SUBDIR fakedir)
FetchContent_Declare(stb            GIT_REPOSITORY https://github.com/nothings/stb.git              GIT_TAG master              GIT_SHALLOW TRUE SOURCE_SUBDIR fakedir)
FetchContent_Declare(imgui_markdown GIT_REPOSITORY https://github.com/juliettef/imgui_markdown.git  GIT_TAG main                GIT_SHALLOW TRUE SOURCE_SUBDIR fakedir)
FetchContent_Declare(imspinner      GIT_REPOSITORY https://github.com/dalerank/imspinner.git        GIT_TAG master              GIT_SHALLOW TRUE SOURCE_SUBDIR fakedir)

# ============================================================================
# Build independent dependencies
# ============================================================================
set(INDEPENDENT_DEPS zlib glfw glew nlohmann_json mini)

foreach(dep ${INDEPENDENT_DEPS})
    string(TIMESTAMP start_time "%s")
    FetchContent_MakeAvailable(${dep})
    string(TIMESTAMP end_time "%s")
    math(EXPR duration "${end_time} - ${start_time}")
    message(STATUS "[Installer] ${dep} completed in ${duration} seconds")
endforeach()

# ============================================================================
# Download deferred dependencies (SOURCE_SUBDIR fakedir = download only)
# ============================================================================
FetchContent_MakeAvailable(freetype curl minizip_ng imgui stb imgui_markdown imspinner)

# ============================================================================
# Patch curl warnings for MSVC (must happen before add_subdirectory)
# ============================================================================
if(MSVC AND EXISTS "${curl_SOURCE_DIR}/CMake/PickyWarnings.cmake")
    file(READ "${curl_SOURCE_DIR}/CMake/PickyWarnings.cmake" _pw_content)
    string(REPLACE
        "  list(APPEND _picky \"-W4\")"
        "  if(PICKY_COMPILER)\n    list(APPEND _picky \"-W4\")\n  endif()"
        _pw_content "${_pw_content}"
    )
    file(WRITE "${curl_SOURCE_DIR}/CMake/PickyWarnings.cmake" "${_pw_content}")
    unset(_pw_content)
endif()
set(PICKY_COMPILER OFF CACHE BOOL "" FORCE)

# Patch minizip-ng /W3 in Debug builds (same D9025 root cause)
if(MSVC AND EXISTS "${minizip_ng_SOURCE_DIR}/CMakeLists.txt")
    file(READ "${minizip_ng_SOURCE_DIR}/CMakeLists.txt" _mz_content)
    string(REPLACE "add_compile_options($<$<CONFIG:Debug>:/W3>)" "" _mz_content "${_mz_content}")
    file(WRITE "${minizip_ng_SOURCE_DIR}/CMakeLists.txt" "${_mz_content}")
    unset(_mz_content)
endif()

# ============================================================================
# Configure zlib variables for dependent packages (freetype, curl, minizip-ng)
# ============================================================================
# Set as CACHE FORCE so FindZLIB.cmake's find_path/find_library skip their
# searches and use our FetchContent-built zlib directly.
set(ZLIB_FOUND TRUE)
if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(ZLIB_LIBRARY "${zlib_BINARY_DIR}/zlibstaticd${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE FILEPATH "" FORCE)
    else()
        set(ZLIB_LIBRARY "${zlib_BINARY_DIR}/zlibstatic${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE FILEPATH "" FORCE)
    endif()
elseif(UNIX)
    set(ZLIB_LIBRARY "${zlib_BINARY_DIR}/libz${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE FILEPATH "" FORCE)
endif()

set(ZLIB_INCLUDE_DIR  "${zlib_BINARY_DIR}" CACHE PATH "" FORCE)
set(ZLIB_LIBRARIES    "${ZLIB_LIBRARY}")
set(ZLIB_INCLUDE_DIRS "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}")

# ============================================================================
# Build deferred dependencies (suppress third-party warnings)
# ============================================================================
set(_saved_c_flags   "${CMAKE_C_FLAGS}")
set(_saved_cxx_flags "${CMAKE_CXX_FLAGS}")

if(MSVC)
    string(APPEND CMAKE_C_FLAGS   " /w")
    string(APPEND CMAKE_CXX_FLAGS " /w")
else()
    string(APPEND CMAKE_C_FLAGS   " -w")
    string(APPEND CMAKE_CXX_FLAGS " -w")
endif()

add_subdirectory("${freetype_SOURCE_DIR}"   "${freetype_BINARY_DIR}"   SYSTEM)
add_subdirectory("${curl_SOURCE_DIR}"       "${curl_BINARY_DIR}"       SYSTEM)
add_subdirectory("${minizip_ng_SOURCE_DIR}" "${minizip_ng_BINARY_DIR}" SYSTEM)

set(CMAKE_C_FLAGS   "${_saved_c_flags}")
set(CMAKE_CXX_FLAGS "${_saved_cxx_flags}")
