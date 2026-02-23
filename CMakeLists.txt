cmake_minimum_required(VERSION 3.16)
project(bowling VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(bowling ${SOURCES})

# ── SFML ──────────────────────────────────────────────────────────────────────
find_package(SFML 3 COMPONENTS Graphics Window System Audio QUIET)

if(SFML_FOUND)
    target_link_libraries(bowling PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
    )
else()
    # Fallback: try FetchContent (downloads SFML automatically)
    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        3.0.0
    )
    FetchContent_MakeAvailable(SFML)
    target_link_libraries(bowling PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
    )
endif()

# ── Assets ────────────────────────────────────────────────────────────────────
# Copy assets folder next to the executable after build
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET bowling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:bowling>/assets"
        COMMENT "Copying assets..."
    )
endif()

# ── Windows: copy SFML DLLs next to exe ───────────────────────────────────────
if(WIN32)
    add_custom_command(TARGET bowling POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:bowling>
            $<TARGET_FILE_DIR:bowling>
        COMMAND_EXPAND_LISTS
    )
endif()
