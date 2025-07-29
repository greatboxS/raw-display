cmake_minimum_required(VERSION 3.11)

project(EarlyCameraTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add executable target
add_executable(${PROJECT_NAME} test_egl.cpp)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -Wall
        -Wextra
        -Werror
        -pedantic
        -DDEBUG_
        -g
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        earlycamera
        earlyrender
        earlydrm
        gbm
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
