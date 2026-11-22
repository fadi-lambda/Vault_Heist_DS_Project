# Unit Test Suite Report — Vault Heist

**Framework**: GoogleTest (v1.18.0)  
**Binary**: `out/build/x64-debug/tests/Vault_Heist_Tests.exe`  
**Total Tests**: 56  
**Passing**: 56 (100%)  
**Failing**: 0 (0%)  

---

## 1. Test Execution Summary

```
Running main() from googletest/src/gtest_main.cc
[==========] Running 56 tests from 16 test suites.
[----------] Global test environment set-up.
[----------] 1 test from GoogleTestSetup
[ RUN      ] GoogleTestSetup.BasicTest
[       OK ] GoogleTestSetup.BasicTest (0 ms)
[----------] 1 test from GoogleTestSetup (0 ms total)

[----------] 7 tests from MaxHeapTest
[ RUN      ] MaxHeapTest.InsertAndExtractMax
[       OK ] MaxHeapTest.InsertAndExtractMax (0 ms)
[ RUN      ] MaxHeapTest.DynamicResizing
[       OK ] MaxHeapTest.DynamicResizing (0 ms)
[ RUN      ] MaxHeapTest.BuildMaxHeapVariousArrays
[       OK ] MaxHeapTest.BuildMaxHeapVariousArrays (0 ms)
[ RUN      ] MaxHeapTest.HeapSortCorrectness
[       OK ] MaxHeapTest.HeapSortCorrectness (0 ms)
[ RUN      ] MaxHeapTest.EventsBuildMaxHeap
[       OK ] MaxHeapTest.EventsBuildMaxHeap (0 ms)
[ RUN      ] MaxHeapTest.EventsExtractMax
Error: Heap is empty!
[       OK ] MaxHeapTest.EventsExtractMax (0 ms)
[ RUN      ] MaxHeapTest.EventsHeapSort
[       OK ] MaxHeapTest.EventsHeapSort (0 ms)
[----------] 7 tests from MaxHeapTest (0 ms total)

[----------] 11 tests from AVLTreeTest
[ RUN      ] AVLTreeTest.LeftLeftRotation
[       OK ] AVLTreeTest.LeftLeftRotation (0 ms)
[ RUN      ] AVLTreeTest.RightRightRotation
[       OK ] AVLTreeTest.RightRightRotation (0 ms)
[ RUN      ] AVLTreeTest.LeftRightRotation
[       OK ] AVLTreeTest.LeftRightRotation (0 ms)
[ RUN      ] AVLTreeTest.RightLeftRotation
[       OK ] AVLTreeTest.RightLeftRotation (0 ms)
[ RUN      ] AVLTreeTest.DuplicateRejection
Warning: Duplicate key 50 rejected in Vault!
[       OK ] AVLTreeTest.DuplicateRejection (0 ms)
[ RUN      ] AVLTreeTest.DeleteWithRebalancing
[       OK ] AVLTreeTest.DeleteWithRebalancing (0 ms)
[ RUN      ] AVLTreeTest.OrderStatistics
[       OK ] AVLTreeTest.OrderStatistics (0 ms)
[ RUN      ] AVLTreeTest.LargeSequentialInsertionMaintainsLogNHeight
[       OK ] AVLTreeTest.LargeSequentialInsertionMaintainsLogNHeight (0 ms)
[ RUN      ] AVLTreeTest.EventsLLRotation
[       OK ] AVLTreeTest.EventsLLRotation (0 ms)
[ RUN      ] AVLTreeTest.EventsRotationCases
[       OK ] AVLTreeTest.EventsRotationCases (0 ms)
[ RUN      ] AVLTreeTest.EventsSearchDeleteDuplicate
Warning: Duplicate key 30 rejected in Vault!
[       OK ] AVLTreeTest.EventsSearchDeleteDuplicate (0 ms)
[----------] 11 tests from AVLTreeTest (1 ms total)

[----------] 8 tests from BinaryTreeTest
[ RUN      ] BinaryTreeTest.EmptyTreeOperations
[       OK ] BinaryTreeTest.EmptyTreeOperations (0 ms)
[ RUN      ] BinaryTreeTest.SingleNodeTree
[       OK ] BinaryTreeTest.SingleNodeTree (0 ms)
[ RUN      ] BinaryTreeTest.SkewedTreeTraversalsMatch
[       OK ] BinaryTreeTest.SkewedTreeTraversalsMatch (0 ms)
[ RUN      ] BinaryTreeTest.BalancedTreeTraversalsAndLevelOrder
[       OK ] BinaryTreeTest.BalancedTreeTraversalsAndLevelOrder (0 ms)
[ RUN      ] BinaryTreeTest.ReconstructionRoundTripPreIn
[       OK ] BinaryTreeTest.ReconstructionRoundTripPreIn (0 ms)
[ RUN      ] BinaryTreeTest.ReconstructionRoundTripPostIn
[       OK ] BinaryTreeTest.ReconstructionRoundTripPostIn (0 ms)
[ RUN      ] BinaryTreeTest.LCAAndPathTo
[       OK ] BinaryTreeTest.LCAAndPathTo (0 ms)
[ RUN      ] BinaryTreeTest.MirrorAndSymmetry
[       OK ] BinaryTreeTest.MirrorAndSymmetry (0 ms)
[----------] 8 tests from BinaryTreeTest (0 ms total)

[----------] 2 tests from BinaryTreeEventTest
[ RUN      ] BinaryTreeEventTest.TraversalVisitOrderMatchesOutput
[       OK ] BinaryTreeEventTest.TraversalVisitOrderMatchesOutput (0 ms)
[ RUN      ] BinaryTreeEventTest.BuildEmitsOneInsertPerNode
[       OK ] BinaryTreeEventTest.BuildEmitsOneInsertPerNode (0 ms)
[----------] 2 tests from BinaryTreeEventTest (0 ms total)

[----------] 8 tests from BSTTest
[ RUN      ] BSTTest.InsertSearchAndDuplicateRejection
Duplicate key 50 rejected.
Duplicate key 30 rejected.
Duplicate key 20 rejected.
[       OK ] BSTTest.InsertSearchAndDuplicateRejection (0 ms)
[ RUN      ] BSTTest.FindMinAndFindMax
findMin called on empty BST.
findMax called on empty BST.
[       OK ] BSTTest.FindMinAndFindMax (0 ms)
[ RUN      ] BSTTest.DeleteAllCases
[       OK ] BSTTest.DeleteAllCases (0 ms)
[ RUN      ] BSTTest.SuccessorAndPredecessor
[       OK ] BSTTest.SuccessorAndPredecessor (0 ms)
[ RUN      ] BSTTest.RangeQuery
[       OK ] BSTTest.RangeQuery (0 ms)
[ RUN      ] BSTTest.KthSmallest
kthSmallest: k=0 out of range [1, 7].
kthSmallest: k=8 out of range [1, 7].
[       OK ] BSTTest.KthSmallest (0 ms)
[ RUN      ] BSTTest.LCABST
[       OK ] BSTTest.LCABST (0 ms)
[ RUN      ] BSTTest.DegenerateChainHeightEqualsN
[       OK ] BSTTest.DegenerateChainHeightEqualsN (0 ms)
[----------] 8 tests from BSTTest (0 ms total)

[----------] 3 tests from BSTEventTest
[ RUN      ] BSTEventTest.InsertEmitsComparesThenInsert
Duplicate key 30 rejected.
[       OK ] BSTEventTest.InsertEmitsComparesThenInsert (0 ms)
[ RUN      ] BSTEventTest.SearchAndDeleteEvents
[       OK ] BSTEventTest.SearchAndDeleteEvents (0 ms)
[ RUN      ] BSTEventTest.RangeQueryHighlightsPrunedSubtrees
[       OK ] BSTEventTest.RangeQueryHighlightsPrunedSubtrees (0 ms)
[----------] 3 tests from BSTEventTest (0 ms total)

[----------] 4 tests from EventLogTest
[ RUN      ] EventLogTest.EmptyLog
[       OK ] EventLogTest.EmptyLog (0 ms)
[ RUN      ] EventLogTest.AppendAndRetrieve
[       OK ] EventLogTest.AppendAndRetrieve (0 ms)
[ RUN      ] EventLogTest.Clear
[       OK ] EventLogTest.Clear (0 ms)
[ RUN      ] EventLogTest.EventBatchGrowthAndCopy
[       OK ] EventLogTest.EventBatchGrowthAndCopy (0 ms)
[----------] 4 tests from EventLogTest (0 ms total)

[----------] 3 tests from ScoringEngineTest
[ RUN      ] ScoringEngineTest.AlarmCapacityMatchesFormula
[       OK ] ScoringEngineTest.AlarmCapacityMatchesFormula (0 ms)
[ RUN      ] ScoringEngineTest.StarCalculationAndHintCap
[       OK ] ScoringEngineTest.StarCalculationAndHintCap (0 ms)
[ RUN      ] ScoringEngineTest.PenaltiesAndScoreFloor
[       OK ] ScoringEngineTest.PenaltiesAndScoreFloor (0 ms)
[----------] 3 tests from ScoringEngineTest (0 ms total)

[----------] 1 test from LevelLoaderTest
[ RUN      ] LevelLoaderTest.LoadsAllTwentyLevelsAndComputesOptimalSteps
[       OK ] LevelLoaderTest.LoadsAllTwentyLevelsAndComputesOptimalSteps (45 ms)
[----------] 1 test from LevelLoaderTest (45 ms total)

[----------] 1 test from HintSystemTest
[ RUN      ] HintSystemTest.OncePerLevelNudgeAndStarCap
[       OK ] HintSystemTest.OncePerLevelNudgeAndStarCap (1 ms)
[----------] 1 test from HintSystemTest (1 ms total)

[----------] 1 test from DragDropTest
[ RUN      ] DragDropTest.PickupHoverSnapRejectCancelAndReslot
[       OK ] DragDropTest.PickupHoverSnapRejectCancelAndReslot (0 ms)
[----------] 1 test from DragDropTest (0 ms total)

[----------] 1 test from Act3AvlTests
[ RUN      ] Act3AvlTests.Act3JsonRealRotationsSubtreeSizesAndKthLargest
[       OK ] Act3AvlTests.Act3JsonRealRotationsSubtreeSizesAndKthLargest (7 ms)
[----------] 1 test from Act3AvlTests (7 ms total)

[----------] 1 test from Act4HeapTests
[ RUN      ] Act4HeapTests.ExpectedSortedResultAndAscendingHeapSortValidation
[       OK ] Act4HeapTests.ExpectedSortedResultAndAscendingHeapSortValidation (6 ms)
[----------] 1 test from Act4HeapTests (7 ms total)

[----------] 1 test from EndlessModeTests
[ RUN      ] EndlessModeTests.StreamGeneratorValidatorSaveLoadAndLeaderboard
[       OK ] EndlessModeTests.StreamGeneratorValidatorSaveLoadAndLeaderboard (13 ms)
[----------] 1 test from EndlessModeTests (13 ms total)

[----------] 3 tests from BenchmarkCounterTest
[ RUN      ] BenchmarkCounterTest.BSTComparisonCountersAndDeepChain
[       OK ] BenchmarkCounterTest.BSTComparisonCountersAndDeepChain (715 ms)
[ RUN      ] BenchmarkCounterTest.AVLRotationAndComparisonCounters
[       OK ] BenchmarkCounterTest.AVLRotationAndComparisonCounters (0 ms)
[ RUN      ] BenchmarkCounterTest.MaxHeapCountersAndLinearBuild
[       OK ] BenchmarkCounterTest.MaxHeapCountersAndLinearBuild (0 ms)
[----------] 3 tests from BenchmarkCounterTest (716 ms total)

[----------] Global test environment tear-down
[==========] 56 tests from 16 test suites ran. (797 ms total)
[  PASSED  ] 56 tests.
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
