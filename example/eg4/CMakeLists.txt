cmake_minimum_required(VERSION 3.11)

project(EarlyCameraTest4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.11)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLES2 REQUIRED glesv2)

set(INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${DRM_INCLUDE_DIRS}
)

set(LIBS
    ${DRM_LIBRARIES}
    ${EGL_LIBRARIES}
    ${GLES2_LIBRARIES}
)

set(FLAGS -DEGL_CONTEXT_VER=2)

# Add executable target
add_executable(${PROJECT_NAME} main.cpp)

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
        earlymem
)
