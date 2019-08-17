
file(GLOB_RECURSE EXAMPLE_SRC
    "*.cpp"
)

add_executable(example ${EXAMPLE_SRC})

target_include_directories(example
    PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
)
