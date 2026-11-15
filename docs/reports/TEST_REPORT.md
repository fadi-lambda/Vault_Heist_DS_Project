# Unit Test Suite Report — Vault Heist

**Framework**: GoogleTest (v1.18.0)  
**Binary**: `out/build/x64-debug/tests/Vault_Heist_Tests.exe`  
**Total Tests**: 13  
**Passing**: 13 (100%)  
**Failing**: 0 (0%)  

---

## 1. Test Execution Summary

```
Running main() from googletest/src/gtest_main.cc
[==========] Running 13 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 1 test from GoogleTestSetup
[ RUN      ] GoogleTestSetup.BasicTest
[       OK ] GoogleTestSetup.BasicTest (0 ms)
[----------] 1 test from GoogleTestSetup (0 ms total)

[----------] 4 tests from MaxHeapTest
[ RUN      ] MaxHeapTest.InsertAndExtractMax
[       OK ] MaxHeapTest.InsertAndExtractMax (0 ms)
[ RUN      ] MaxHeapTest.DynamicResizing
[       OK ] MaxHeapTest.DynamicResizing (0 ms)
[ RUN      ] MaxHeapTest.BuildMaxHeapVariousArrays
[       OK ] MaxHeapTest.BuildMaxHeapVariousArrays (0 ms)
[ RUN      ] MaxHeapTest.HeapSortCorrectness
[       OK ] MaxHeapTest.HeapSortCorrectness (0 ms)
[----------] 4 tests from MaxHeapTest (0 ms total)

[----------] 8 tests from AVLTreeTest
[ RUN      ] AVLTreeTest.LeftLeftRotation
[       OK ] AVLTreeTest.LeftLeftRotation (0 ms)
[ RUN      ] AVLTreeTest.RightRightRotation
[       OK ] AVLTreeTest.RightRightRotation (0 ms)
[ RUN      ] AVLTreeTest.LeftRightRotation
[       OK ] AVLTreeTest.LeftRightRotation (0 ms)
[ RUN      ] AVLTreeTest.RightLeftRotation
[       OK ] AVLTreeTest.RightLeftRotation (0 ms)
[ RUN      ] AVLTreeTest.DuplicateRejection
[       OK ] AVLTreeTest.DuplicateRejection (0 ms)
[ RUN      ] AVLTreeTest.DeleteWithRebalancing
[       OK ] AVLTreeTest.DeleteWithRebalancing (0 ms)
[ RUN      ] AVLTreeTest.OrderStatistics
[       OK ] AVLTreeTest.OrderStatistics (0 ms)
[ RUN      ] AVLTreeTest.LargeSequentialInsertionMaintainsLogNHeight
[       OK ] AVLTreeTest.LargeSequentialInsertionMaintainsLogNHeight (0 ms)
[----------] 8 tests from AVLTreeTest (0 ms total)

[----------] Global test environment tear-down
[==========] 13 tests from 3 test suites ran. (0 ms total)
[  PASSED  ] 13 tests.
```

---

## 2. Test Case Breakdown

### 2.1 AVL Tree Suite (`AVLTreeTest`)

| Test Name | Concept Tested | Assertions Verified | Status |
|---|---|---|---|
| `LeftLeftRotation` | LL Single Right Rotation | Root becomes pivot, height=2, size=3, `isBalanced()==true` | **PASSED** |
| `RightRightRotation` | RR Single Left Rotation | Root becomes pivot, height=2, size=3, `isBalanced()==true` | **PASSED** |
| `LeftRightRotation` | LR Double Rotation | Double rotation restores balance, correct child ordering | **PASSED** |
| `RightLeftRotation` | RL Double Rotation | Double rotation restores balance, correct child ordering | **PASSED** |
| `DuplicateRejection` | Section 5.3 & 5.4 rule | Second insertion of identical key returns `false`, size unchanged | **PASSED** |
| `DeleteWithRebalancing` | 3 Delete Cases | Leaf deletion, 1-child bypass, 2-child successor swap + rebalance | **PASSED** |
| `OrderStatistics` | $O(\log n)$ kth & rank | `kth(1..7)` matches sorted array, `rank()` counts correctly, `countInRange` matches | **PASSED** |
| `LargeSequentialInsertion`| Balance Invariant | 63 sequential inserts maintain height $\le 7$ ($O(\log n)$) | **PASSED** |

### 2.2 Max Heap Suite (`MaxHeapTest`)

| Test Name | Concept Tested | Assertions Verified | Status |
|---|---|---|---|
| `InsertAndExtractMax` | siftUp & extractMax | Insertions maintain max heap property; extraction yields descending order | **PASSED** |
| `DynamicResizing` | Manual Array Doubling | Capacity dynamically doubles without `std::vector`; data preserved | **PASSED** |
| `BuildMaxHeapVariousArrays` | Section 9.1 array cases | Verified on single, reverse-sorted, already-heap, duplicates, random | **PASSED** |
| `HeapSortCorrectness` | Section 9.1 sorting | Empty, single, random with negatives and duplicates sorted ascending | **PASSED** |

---

## 3. Invariant Validation (Section 5.6)
Every test verifies the core invariants defined in Section 5.6:
1. BST ordering across all left and right subtrees.
2. $\text{Stored Height} == 1 + \max(\text{leftH}, \text{rightH})$.
3. $\text{Stored Size} == 1 + \text{leftS} + \text{rightS}$.
4. $|\text{Balance Factor}| \le 1$ for all nodes.
5. Max Heap parent-child inequality: $\text{data}[i] \ge \text{data}[2i+1]$ and $\text{data}[i] \ge \text{data}[2i+2]$.
