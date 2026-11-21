add_executable(Vault_Heist_Tests
    TestMain.cpp
    TestMaxHeap.cpp
    TestAVLTree.cpp
    BinaryTreeTests.cpp
    BSTTests.cpp
    EventLogTests.cpp
    ${PROJECT_SOURCE_DIR}/src/events/EventLog.cpp
    ${PROJECT_SOURCE_DIR}/src/game/SaveManager.cpp
    ${PROJECT_SOURCE_DIR}/src/structures/MaxHeap.cpp
    ${PROJECT_SOURCE_DIR}/src/structures/AVLTree.cpp
    ${PROJECT_SOURCE_DIR}/src/structures/BinaryTree.cpp
    ${PROJECT_SOURCE_DIR}/src/structures/BST.cpp
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