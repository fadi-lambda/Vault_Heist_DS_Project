cmake_minimum_required(VERSION 3.22)

if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    if(MSVC)
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
    endif()
endif()

project(Vault_Heist LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

find_package(SFML CONFIG REQUIRED COMPONENTS Graphics)
find_package(GTest CONFIG REQUIRED)

# Main application
add_executable(Vault_Heist
    src/main.cpp
    src/events/EventLog.cpp
    src/gui/LayoutEngine.cpp
    src/gui/Renderer.cpp
    src/gui/AnimationController.cpp
    src/gui/UndoStack.cpp
    src/gui/ToastQueue.cpp
    src/gui/Ui.cpp
    src/gui/ScoreLog.cpp
    src/game/SaveManager.cpp
    src/structures/MaxHeap.cpp
    src/structures/AVLTree.cpp
    src/structures/BinaryTree.cpp
    src/structures/BST.cpp
)

target_include_directories(Vault_Heist
    PRIVATE include
)

if(MSVC)
    target_compile_options(Vault_Heist PRIVATE /EHsc)
endif()

target_link_libraries(Vault_Heist
    PRIVATE SFML::Graphics
)

# Benchmarks (Section 10)
add_executable(Vault_Heist_Benchmarks
    benchmarks/BenchmarkSuite.cpp
    src/structures/MaxHeap.cpp
    src/structures/AVLTree.cpp
)

target_include_directories(Vault_Heist_Benchmarks
    PRIVATE include
)

if(MSVC)
    target_compile_options(Vault_Heist_Benchmarks PRIVATE /EHsc)
endif()

# Tests
add_subdirectory(tests)
