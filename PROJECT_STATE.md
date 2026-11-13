# PROJECT_STATE.md

# Vault Heist: The Living Vault

## 1. Project Status

Current Phase: Phase 1 - Core Data Structures (Member 2: Max Heap, Heap Sort, and AVL Tree Completed)
Overall Status: IN PROGRESS

Project Deadline: 20 November 2026

## 2. Technology Stack

- Language: C++20
- GUI: SFML 3
- Build: CMake
- Build Generator: Ninja
- IDE: Visual Studio Insiders
- Compiler: MSVC (C++20)
- Package Manager: vcpkg
- Testing: GoogleTest
- Version Control: Git and GitHub

## 3. Repository Status

- GitHub repository: Created
- Main branch: main
- Remote tracking: origin/main
- CMake project: Configured successfully
- Project structure: Established
- x64 Debug configuration: Successful
- SFML linking: Successful
- SFML test window: Successfully launched
- GoogleTest integration: Successful
- Latest changes: Max Heap and AVL Tree implementations and all unit tests passing

## 4. Team Responsibilities

Member 1:
- General Binary Tree (Act 1)
- Binary Search Tree (Act 2)
- Related unit tests

Member 2 (You):
- AVL Tree (Act 3) - COMPLETED (`include/AVLTree.h`, `src/AVLTree.cpp`)
- Max Heap (Act 4) - COMPLETED (`include/MaxHeap.h`, `src/MaxHeap.cpp`)
- Heap Sort (Act 4) - COMPLETED (in-place $O(n \log n)$)
- Unit Tests (Acts 3 & 4) - COMPLETED & PASSED (`tests/TestMaxHeap.cpp`, `tests/TestAVLTree.cpp`)

Shared:
- Architecture and integration
- Event system (for animation and visualizer)
- SFML GUI
- Game systems (LevelManager, Alarm meter, Score)
- Testing and documentation

## 5. Development Progress

Phase 0: COMPLETED (Environment & setup audit)
Phase 1: IN PROGRESS (Member 2 has completed 100% of assigned core data structures: Max Heap, Heap Sort, and AVL Tree)
Phase 2: NOT STARTED
Phase 3: NOT STARTED
Phase 4: NOT STARTED
Phase 5: NOT STARTED
Phase 6: NOT STARTED
Phase 7: NOT STARTED
Phase 8: NOT STARTED
Phase 9: NOT STARTED

## 6. Technical Decisions

- C++20 and SFML 3 are the selected technologies.
- CMake is the shared build system with Ninja generator.
- vcpkg manages external dependencies.
- Strict Course Guidelines Applied:
  * No `std::vector` allowed: Manual dynamic arrays (`int* data = new int[capacity]`) with automatic resizing and clean destructors (`delete[]`).
  * No `enum` / `enum class` allowed.
  * No global variables allowed.
  * Node heights: `height(leaf) = 1` and `height(nullptr) = 0`.
- All major data structures are implemented strictly from scratch.
- GUI and data-structure logic remain separate.
- GoogleTest is integrated through CMake with `/Z7` debug format for all unit tests.

## 7. Current Blockers / Unknowns

- Member 1's progress on BinaryTree / BST.

## 8. Next Actions

Member 2 (You):
- Commit and push completed AVL Tree implementation and unit tests to GitHub.
- Next phase: Event system integration and SFML visualizer for Heap & AVL.

Shared:
- Review and integrate Member 1 and Member 2 progress.

## 9. Git Status

- Main branch: main
- Remote branch: origin/main
- Uncommitted new files:
  * `include/AVLTree.h`
  * `src/AVLTree.cpp`
  * `tests/TestAVLTree.cpp`
  * `tests/CMakeLists.txt` (modified)
  * `project_state.md` (updated)

## 10. Testing Status

- CMake configuration: Passed
- x64 Debug build: Passed
- SFML linking: Passed
- SFML window execution: Passed
- GoogleTest setup test: Passed
- Max Heap Tests:
  * `MaxHeapTest.InsertAndExtractMax`: PASSED (0 ms)
  * `MaxHeapTest.DynamicResizing`: PASSED (0 ms)
  * `MaxHeapTest.BuildMaxHeapVariousArrays`: PASSED (0 ms)
  * `MaxHeapTest.HeapSortCorrectness`: PASSED (0 ms)
- AVL Tree Tests:
  * `AVLTreeTest.LeftLeftRotation`: PASSED (0 ms)
  * `AVLTreeTest.RightRightRotation`: PASSED (0 ms)
  * `AVLTreeTest.LeftRightRotation`: PASSED (0 ms)
  * `AVLTreeTest.RightLeftRotation`: PASSED (0 ms)
  * `AVLTreeTest.DuplicateRejection`: PASSED (0 ms)
  * `AVLTreeTest.DeleteWithRebalancing`: PASSED (0 ms)
  * `AVLTreeTest.OrderStatistics`: PASSED (0 ms)
  * `AVLTreeTest.LargeSequentialInsertionMaintainsLogNHeight`: PASSED (0 ms)
- Result: **13/13 Tests Passed (100% Success)**
