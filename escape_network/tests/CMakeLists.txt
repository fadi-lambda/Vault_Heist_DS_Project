add_executable(EscapeNetwork_Tests
    GraphTests.cpp
    GraphAlgorithmsTests.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/Graph.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/GraphAlgorithms.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/Level1_1.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/Level1_2.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/Level2_1.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/Level2_2.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/Level3_2.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/StatePanel.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/Json.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/LevelManager.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/ScoreManager.cpp
    ${PROJECT_SOURCE_DIR}/escape_network/src/Validator.cpp
)

target_include_directories(EscapeNetwork_Tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/escape_network/include
)

if(MSVC)
    target_compile_options(EscapeNetwork_Tests
        PRIVATE
            /Z7
            /EHsc
    )
endif()

target_link_libraries(EscapeNetwork_Tests
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(EscapeNetwork_Tests)
