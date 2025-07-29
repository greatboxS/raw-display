cmake_minimum_required(VERSION 3.11)

project(earlydrm VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLES2 REQUIRED glesv2)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/MMapAllocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HeapDMAAllocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DrmDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DrmController.cpp
)

set(INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${DRM_INCLUDE_DIRS}
)

if (ANDROID)
    set(SOURCES
        ${SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/IonAllocator.cpp
    )
endif(ANDROID)

set(LIBS
    ${DRM_LIBRARIES}
)

set(FLAGS -DEGL_CONTEXT_VER=2)

# Include directories
add_library(${PROJECT_NAME} STATIC
    ${SOURCES}
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