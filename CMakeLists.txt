cmake_minimum_required(VERSION 3.21)

project(SistemaSolare3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_ROOT "" CACHE PATH "Percorso opzionale alla cartella principale di SFML")
if(SFML_ROOT AND NOT SFML_DIR)
    set(SFML_DIR "${SFML_ROOT}/lib/cmake/SFML")
endif()

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

if(WIN32)
    add_custom_command(TARGET SistemaSolare3D POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:SistemaSolare3D>
        $<TARGET_FILE_DIR:SistemaSolare3D>
        COMMAND_EXPAND_LISTS
    )
endif()
