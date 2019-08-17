
file(GLOB_RECURSE TEST_SRC
    "*.h"
    "*.cpp"
)

find_package(Catch2 REQUIRED)
add_executable(tests ${TEST_SRC})
target_link_libraries(tests Catch2::Catch2)

target_include_directories(tests
    PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
)

include(CTest)
include(ParseAndAddCatchTests)
ParseAndAddCatchTests(tests)
