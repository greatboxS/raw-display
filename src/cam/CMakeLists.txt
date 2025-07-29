cmake_minimum_required(VERSION 3.11)

project(earlycamera)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraAbstraction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QualcommCamera.cpp
)

# Add executable target
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -Wall
        -Wextra
        -Werror
        -pedantic
        -DDEBUG_
        -g
)

# Include directories
target_include_directories(${PROJECT_NAME}
  PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}
                          ${CMAKE_CURRENT_SOURCE_DIR}/../>
        $<INSTALL_INTERFACE:include>
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        qcarcam 
        pthread
)
