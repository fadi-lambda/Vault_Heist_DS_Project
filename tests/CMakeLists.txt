add_executable(Vault_Heist_Tests
    TestMain.cpp
)

target_include_directories(Vault_Heist_Tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(Vault_Heist_Tests
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(Vault_Heist_Tests)