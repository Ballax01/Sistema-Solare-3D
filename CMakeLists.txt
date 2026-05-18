cmake_minimum_required(VERSION 3.20)

project(SistemaSolare3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_DIR "C:/libs/SFML-3.0.0/lib/cmake/SFML")

find_package(SFML 3.0 REQUIRED COMPONENTS Window Graphics System)

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
    opengl32
)

add_custom_command(TARGET SistemaSolare3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "C:/libs/SFML-3.0.0/bin"
    $<TARGET_FILE_DIR:SistemaSolare3D>
)