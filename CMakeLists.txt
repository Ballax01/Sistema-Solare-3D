cmake_minimum_required(VERSION 3.21)

project(SistemaSolare3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.0
    GIT_SHALLOW    TRUE
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-3
    GIT_SHALLOW    TRUE
    SYSTEM
)
set(FT_DISABLE_ZLIB     ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(freetype)

add_executable(SistemaSolare3D
    src/main.cpp
    src/glad.c
)

target_include_directories(SistemaSolare3D PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SistemaSolare3D PRIVATE
    SFML::Window
    SFML::Graphics
    SFML::System
    freetype
    opengl32
)