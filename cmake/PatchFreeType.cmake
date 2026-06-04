# Aggiorna cmake_minimum_required di FreeType per compatibilità con CMake 4.x
# (FreeType 2.13.x usa VERSION 2.8.12, rifiutata da CMake 4.0+)
cmake_minimum_required(VERSION 3.5)
if(NOT DEFINED PATCH_FILE)
    message(FATAL_ERROR "Uso: cmake -DPATCH_FILE=<path> -P PatchFreeType.cmake")
endif()
file(READ "${PATCH_FILE}" content)
string(REGEX REPLACE
    "cmake_minimum_required[ \t]*\\([ \t]*VERSION[ \t]+[0-9.]+([ \t]*\\.\\.\\.[ \t]*[0-9.]+)?[ \t]*\\)"
    "cmake_minimum_required(VERSION 3.5)"
    content "${content}")
file(WRITE "${PATCH_FILE}" "${content}")
message(STATUS "FreeType CMakeLists.txt patchato per CMake 4.x")
