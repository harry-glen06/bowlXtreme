cmake_minimum_required(VERSION 3.16)
project(bowling VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect Emscripten / web build
if(EMSCRIPTEN)
    set(PLATFORM "Web" CACHE STRING "Platform" FORCE)
endif()

# Collect all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(bowling ${SOURCES})

# ── Raylib ────────────────────────────────────────────────────────────────────
find_package(raylib 5 QUIET)

if(NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

target_link_libraries(bowling PRIVATE raylib)

# ── Platform-specific config ──────────────────────────────────────────────────
if(EMSCRIPTEN)
    # Embed assets into the web bundle
    set(EMSCRIPTEN_FLAGS
        "-Os"
        "-s USE_GLFW=3"
        "-s ASYNCIFY"
        "-s TOTAL_MEMORY=256MB"
        "--preload-file ${CMAKE_SOURCE_DIR}/assets@assets"
        "--preload-file ${CMAKE_SOURCE_DIR}/highscore.txt@highscore.txt"
        "--preload-file ${CMAKE_SOURCE_DIR}/pick_rates.csv@pick_rates.csv"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s FORCE_FILESYSTEM=1"
    )
    string(JOIN " " EMSCRIPTEN_FLAGS_STR ${EMSCRIPTEN_FLAGS})
    set_target_properties(bowling PROPERTIES
        SUFFIX ".html"
        LINK_FLAGS "${EMSCRIPTEN_FLAGS_STR}"
    )
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
elseif(APPLE)
    target_compile_options(bowling PRIVATE -Wall)
elseif(WIN32)
    # Copy DLLs on Windows
    add_custom_command(TARGET bowling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:bowling>
            $<TARGET_FILE_DIR:bowling>
        COMMAND_EXPAND_LISTS
    )
endif()

# ── Assets ────────────────────────────────────────────────────────────────────
if(NOT EMSCRIPTEN AND EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET bowling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:bowling>/assets"
        COMMENT "Copying assets..."
    )
endif()
