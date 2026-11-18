add_executable(Vault_Heist_Tests
    TestMain.cpp
    TestMaxHeap.cpp
    TestAVLTree.cpp
    BinaryTreeTests.cpp
    BSTTests.cpp
    EventLogTests.cpp
    FoundationTests.cpp
    ${PROJECT_SOURCE_DIR}/src/events/EventLog.cpp
    ${PROJECT_SOURCE_DIR}/src/game/Json.cpp
    ${PROJECT_SOURCE_DIR}/src/game/LevelLoader.cpp
    ${PROJECT_SOURCE_DIR}/src/game/Scoring.cpp
    ${PROJECT_SOURCE_DIR}/src/game/HintSystem.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/DragDrop.cpp
    ${PROJECT_SOURCE_DIR}/src/structures/MaxHeap.cpp
    ${PROJECT_SOURCE_DIR}/src/structures/AVLTree.cpp
    ${PROJECT_SOURCE_DIR}/src/structures/BinaryTree.cpp
    ${PROJECT_SOURCE_DIR}/src/structures/BST.cpp
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