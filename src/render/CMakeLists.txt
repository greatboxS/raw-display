cmake_minimum_required(VERSION 3.11)

project(earlyrender VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLES2 REQUIRED glesv2)

# Platform detection or manual option
option(SUPPORT_X11 "Support X11 as display backend" ON)
option(SUPPORT_DRM "Support DRM as display backend" ON)
option(SUPPORT_WAYLAND "Support Wayland as display backend" ON)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RendererAbstraction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderLoop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlitToScreen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UploadTexture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DrawImage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DrawGuidelines.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameBuffer.cpp
)

set(INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${EGL_INCLUDE_DIRS}
    ${GLES2_INCLUDE_DIRS}
)

set(LIBS
    ${EGL_LIBRARIES}
    ${GLES2_LIBRARIES}
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