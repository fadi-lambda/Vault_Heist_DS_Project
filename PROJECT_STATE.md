# PROJECT_STATE.md

# Vault Heist: The Living Vault

## 1. Project Status

Current Phase: Phase 5 - Deliverables Complete & Verified
Overall Status: READY FOR DEMONSTRATION & SUBMISSION

Project Deadline: 20 November 2026, 11:59 PM
Team Responsibility: Member 2 (Acts 3 & 4: AVL Tree, Max Heap, Heap Sort, Benchmarking)

---

## 2. Technology Stack

- Language: C++20
- GUI: SFML 3
- Build System: CMake 3.22+
- Build Generator: Ninja
- Compiler: MSVC (C++20)
- Package Manager: vcpkg
- Unit Testing: GoogleTest (v1.18.0)
- Benchmarking: High-Resolution Chrono C++20 Suite

---

## 3. Deliverables Status Checklist

| Deliverable | Required Items | Status | Files |
|---|---|---|---|
| **Deliverable 1** | Executable source code + README (run instructions, controls) | **COMPLETE** | `src/`, `include/`, `README.md` |
| **Deliverable 2** | Unit test suite covering all data structures + test report | **COMPLETE (13/13 Pass)** | `tests/`, `docs/reports/TEST_REPORT.md` |
| **Deliverable 3** | Written project report (8–12 pages) + Benchmarks (Section 10) | **COMPLETE** | `docs/reports/PROJECT_REPORT.md`, `benchmarks/BenchmarkSuite.cpp` |
| **Deliverable 4** | Level data files (JSON) + Guide to adding levels | **COMPLETE** | `levels/act3/*.json`, `levels/act4/*.json`, `docs/LEVEL_CREATION_GUIDE.md` |

---

## 4. Course Constraints Audit

- [x] **No `std::vector`**: Dynamic arrays with `new[]`/`delete[]` exclusively in data structures.
- [x] **No `enum` / `enum class`**: Plain typed constants (`static const int`).
- [x] **No global variables**: All states reside within models or scoped loops.
- [x] **Height convention**: $\text{height}(\text{leaf}) = 1$ and $\text{height}(\text{nullptr}) = 0$.
- [x] **GUI independence**: Data structures (`AVLTree`, `MaxHeap`) have zero GUI/SFML dependencies.
- [x] **All structures from scratch**: Custom tree nodes, rotations, heapify, sift-up/down, order statistics.

---

## 5. Act 3 & Act 4 Campaign Levels

| Level | Title | Data Structure Concept | Mechanics & UI |
|---|---|---|---|
| **3.1** | Read the Badges | Balance Factor diagnosis | Click nodes with $|\text{BF}| > 1$ (red badges). No auto-balance. |
| **3.2** | Manual Single Rotations | LL and RR single rotations | Visual LL/RR imbalance repair with right/left rotations. |
| **3.3** | Manual Double Rotations | LR and RL double rotations | Two-step rotation (inner-child rotation then root rotation). |
| **3.4** | Auto Mode & Prediction | Rebalancing prediction | Predict rotation ($LL, RR, LR, RL, \text{None}$) before each insert. |
| **3.5** | Boss: Order Statistics | Augmented `subtreeSize` | $O(\log n)$ search for $k$-th smallest and rank queries. |
| **4.1** | Activate the Vault | Bottom-up Heap Construction | Sift-down from non-leaf nodes $(n/2 - 1)$ down to 0 in $O(n)$ time. |
| **4.2** | Repair Security Locks | Heap property restoration | Diagnose and repair sabotaged elements using sift-up / sift-down. |
| **4.3** | Extract Greatest Treasure | Root extraction & array partitioning | Extract max into distinct green **SORTED REGION**, heapify remainder. |
| **4.4** | Race Against Guards | Time-limited Heap Sort iterations | Speed extraction cycle under strict alarm capacity. |
| **4.5** | Final Vault Boss | In-place Heap Sort workflow | Phase 1 build Max Heap, Phase 2 extract all, Phase 3 unlock door. |

---

## 6. Unit Testing Summary (13/13 Passed)

Command: `out/build/x64-debug/tests/Vault_Heist_Tests.exe`
- `GoogleTestSetup.BasicTest`: **PASSED** (0 ms)
- `MaxHeapTest.InsertAndExtractMax`: **PASSED** (0 ms)
- `MaxHeapTest.DynamicResizing`: **PASSED** (0 ms)
- `MaxHeapTest.BuildMaxHeapVariousArrays`: **PASSED** (0 ms)
- `MaxHeapTest.HeapSortCorrectness`: **PASSED** (0 ms)
- `AVLTreeTest.LeftLeftRotation`: **PASSED** (0 ms)
- `AVLTreeTest.RightRightRotation`: **PASSED** (0 ms)
- `AVLTreeTest.LeftRightRotation`: **PASSED** (0 ms)
- `AVLTreeTest.RightLeftRotation`: **PASSED** (0 ms)
- `AVLTreeTest.DuplicateRejection`: **PASSED** (0 ms)
- `AVLTreeTest.DeleteWithRebalancing`: **PASSED** (0 ms)
- `AVLTreeTest.OrderStatistics`: **PASSED** (0 ms)
- `AVLTreeTest.LargeSequentialInsertionMaintainsLogNHeight`: **PASSED** (0 ms)

---

## 7. Benchmarking Summary (Section 10)

Command: `out/build/x64-debug/Vault_Heist_Benchmarks.exe`
* **Height Comparison**: At $N = 10,000$, BST degenerates to height **10,000** on sorted input, whereas AVL maintains guaranteed optimal height of **14**.
* **Order Statistics**: 1,000 $k$-th queries on $N = 10,000$: AVL `kth` takes **0.91 ms** ($O(\log n)$) vs BST inorder scan **108.34 ms** ($O(n)$) — **~119x speedup**.
* **Bottom-Up Heap Construction**: Empirical time/element ratio remains flat ($\approx 0.02 \ \mu\text{s}$/element) from $N = 100$ to $N = 10,000$, validating $O(n)$ linear complexity.
* **Heap Sort**: Consistent $O(n \log n)$ execution (~1.5 ms for 10,000 elements) across random, sorted, reverse-sorted, and duplicate-heavy distributions.
* **Sorting Comparison ($N = 5,000$)**: Heap Sort (0.84 ms, $O(1)$ space) vs QuickSort (0.65 ms) vs InsertionSort (30.77 ms).

---

## 8. Directory & File Inventory

```
D:\dsa\Vault_Heist_DS_Project
├── CMakeLists.txt                 # Multi-target CMake build config (/Z7 debug format)
├── README.md                      # Comprehensive user & developer manual (Deliverable #1)
├── PROJECT_STATE.md               # Continuous project progress tracker
├── include/
│   ├── AVLTree.h                  # AVL Tree with height, size, rotations, order statistics
│   └── MaxHeap.h                  # Array-based Max Heap with dynamic resizing
├── src/
│   ├── AVLTree.cpp                # Pure data structure implementation (GUI-independent)
│   ├── MaxHeap.cpp                # Pure MaxHeap & HeapSort implementation
│   └── main.cpp                   # SFML 3 GUI with 10-level campaign & Interactive Action Panel
├── benchmarks/
│   └── BenchmarkSuite.cpp         # Section 10 empirical benchmarking suite
├── tests/
│   ├── CMakeLists.txt             # GoogleTest CMake config
│   ├── TestMain.cpp               # Test runner
│   ├── TestAVLTree.cpp            # 8 AVL unit tests
│   └── TestMaxHeap.cpp            # 4 MaxHeap unit tests
├── levels/
│   ├── act3/                      # JSON data files for levels 3.1 to 3.5
│   └── act4/                      # JSON data files for levels 4.1 to 4.5
└── docs/
    ├── LEVEL_CREATION_GUIDE.md    # Guide to adding new levels (Deliverable #4)
    └── reports/
        ├── TEST_REPORT.md         # Full unit test report (Deliverable #2)
        └── PROJECT_REPORT.md      # 8–12 page academic report draft (Deliverable #3)
```

---

## 9. Recent Fixes & Quality Assurance Overhaul

1. **Neutral Button Styling (No Green Spoilers)**:
   - **Issue**: Correct answer buttons were previously styled with green outlines (`sf::Color(80, 200, 100)`) and fills, spoiling solutions before player interaction.
   - **Resolution**: Refactored all answer and action buttons across all 10 campaign levels to use a consistent neutral slate-blue palette (`btnBg(35, 50, 75)`, `btnOutline(80, 130, 200)`). Removed spoiler hints from button labels.

2. **Level 3.1 (Read the Badges) Hit-Testing & Validation**:
   - **Issue**: Mouse click coordinates in `hits[]` were offset from rendered tree coordinates in `drawManualNode`.
   - **Resolution**: Exact mathematical alignment of click radius (28 px) and coordinates. Node validation checks exact keys (30 and 20). Immediate visual feedback with yellow selection rings.

3. **Level 3.2 & 3.3 (Manual Rotations) Initial Invariants**:
   - **Issue**: Calling standard `AVLTree::insert()` automatically balanced trees upon creation, presenting already-balanced trees in rotation puzzle levels.
   - **Resolution**: Implemented `ManualNode` puzzles without automatic rebalancing. Level 3.2 properly displays LL/RR imbalance; Level 3.3 properly presents genuine LR (Root 30, Left 10, Right 20) and RL imbalances with red violation badges.

4. **Level 3.5 (Order-Statistic Boss) UI & Validation**:
   - **Issue**: Multiple-choice buttons were missing from the screen, leaving players guessing inputs.
   - **Resolution**: Rendered interactive option cards for both Challenge 1 ($k = 3 \implies 25$) and Challenge 2 ($\text{rank}(40) = 5$) with mouse click and keyboard `[1-4]` handlers.

5. **Build & Linking Verification**:
   - Resolved Windows file lock contention on `Vault_Heist.exe`.
   - Clean rebuild of all targets: `Vault_Heist.exe`, `Vault_Heist_Tests.exe` (13/13 passing, 0 ms), and `Vault_Heist_Benchmarks.exe`.

