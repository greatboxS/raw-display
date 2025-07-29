cmake_minimum_required(VERSION 3.11)

project(earlymem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_DMA_HEAP "Use dma-heap allocator (kernel >= 5.6)" ON)

if(USE_DMA_HEAP)
    add_definitions(-DUSE_DMA_HEAP=1)
    set(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/DmaHeapDevice.cpp
    )
else()
    add_definitions(-DUSE_ION=1)
    set(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/IonDevice.cpp
    )
endif()

set(INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

# Include directories
add_library(${PROJECT_NAME} OBJECT ${SOURCES}
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -Wall
        -Wextra
        # -Wno-unused-variable
        -Wno-unused-parameter
        -Wno-unused-function
        -Werror
        -pedantic
        -DDEBUG_
        -g
    PRIVATE
        ${FLAGS}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${INCLUDES}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${LIBS}
)