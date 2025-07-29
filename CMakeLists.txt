cmake_minimum_required(VERSION 3.11)

add_custom_target(sync_headers ALL
    COMMAND /bin/bash -c "find ${CMAKE_CURRENT_SOURCE_DIR}/src -type f -name *.h -exec rsync -a {} ${CMAKE_CURRENT_SOURCE_DIR}/include \;"
    COMMENT "Rsyncing headers..."
    VERBATIM
)

add_subdirectory(stub)
add_subdirectory(src)
add_subdirectory(example)
