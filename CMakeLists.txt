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
set(SFML_BUILD_GRAPHICS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO    OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK  OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

add_executable(SistemaSolare3D
    src/main.cpp
    src/glad.c
)

target_include_directories(SistemaSolare3D PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SistemaSolare3D PRIVATE
    SFML::Window
    SFML::System
    opengl32
)
