cmake_minimum_required(VERSION 3.11)

project(qcarcam VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/qcarcam.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qcarcam.h
)

# Add executable target
add_library(qcarcam SHARED ${SOURCES})

target_compile_options(qcarcam
    PRIVATE
        -Wall
        -Wextra
        -Werror
        -pedantic
)

# Include directories
target_include_directories(qcarcam
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(qcarcam
    PRIVATE
        pthread
)