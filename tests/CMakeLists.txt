add_executable(Vault_Heist_Tests
    TestMain.cpp
    TestMaxHeap.cpp
    TestAVLTree.cpp
    ${PROJECT_SOURCE_DIR}/src/structures/MaxHeap.cpp
    ${PROJECT_SOURCE_DIR}/src/structures/AVLTree.cpp
)

target_include_directories(Vault_Heist_Tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(Vault_Heist_Tests
        PRIVATE
            /Z7
    )
endif()

target_link_libraries(Vault_Heist_Tests
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(Vault_Heist_Tests)