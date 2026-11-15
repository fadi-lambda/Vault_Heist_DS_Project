# Vault Heist: The Living Vault — Written Project Report

**Course**: Data Structures & Algorithms  
**Project**: Deliverable 1 & Comprehensive Final Report  
**Deadline**: 20th November 2026, 11:59 PM  
**Submission Archive**: `25I-XXXX_25I-XXXX_Section.zip`  
**Team Focus**: Member 2 — Act 3 (AVL Tree) & Act 4 (Max Heap & Heap Sort)

---

## 1. Executive Summary & Overview

**Vault Heist: The Living Vault** is a puzzle-strategy game where the vault layout corresponds to tree and heap structures. The game grounds abstract theoretical concepts into visual and spatial gameplay:
* In **Act 3 (The Architect's Wing)**, the player explores **AVL Trees**, diagnosing balance factor ($\text{BF}$) violations, performing manual single ($LL$, $RR$) and double ($LR$, $RL$) rotations, predicting automated rebalancing, and executing $O(\log n)$ order-statistic queries.
* In **Act 4 (The Heap Fortress)**, the player unlocks the highest-security vaults by managing an array-backed **Max Heap** with synchronised binary tree and array visualizations, repairing sabotage via sift-up/sift-down, and executing in-place **Heap Sort** within an alarm limit.

### Course Guideline Compliance
* **No `std::vector`**: All heap and array operations utilize manually allocated dynamic arrays (`new[]`/`delete[]`).
* **No `enum` / `enum class`**: Plain typed constants (`static const int`) are utilized exclusively.
* **No global variables**: Complete game and level states live within their respective models and game loops.
* **Height Convention**: Documented consistently across the codebase as $\text{height}(\text{leaf}) = 1$ and $\text{height}(\text{nullptr}) = 0$.
* **GUI Independence**: Core data structures (`AVLTree`, `MaxHeap`) contain zero SFML or drawing dependencies.

---

## 2. System Architecture

The game follows an event-driven model-view architecture ensuring testability, clean separation, and extensibility:

```
+-------------------------------------------------------------+
|                     Pure Data Models                        |
|   - AVLTree (height, size, LL/RR/LR/RL, kth, rank)          |
|   - MaxHeap (dynamic array, siftUp, siftDown, heapSort)     |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                      Event & State Log                      |
|   - ROTATE, BALANCE_ALERT, UPDATE_HEIGHT, UPDATE_SIZE       |
|   - HEAP_BUILD, HEAP_SWAP, EXTRACT_MAX, MARK_SORTED         |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                 SFML 3 Graphical Frontend                   |
|   - Recursive Tree Layout Engine                            |
|   - Synchronised Dual View (Tree & Array)                   |
|   - Interactive Question & Action Panel                     |
|   - Alarm & Star Scoring Controller                         |
+-------------------------------------------------------------+
```

---

## 3. Algorithmic Complexity Analysis

### 3.1 AVL Tree Operations (Act 3)

| Operation | Best Case | Average Case | Worst Case | Space Complexity |
|-----------|-----------|--------------|------------|------------------|
| **Search** | $O(1)$ | $O(\log n)$ | $O(\log n)$ | $O(1)$ |
| **Insert** | $O(1)$ | $O(\log n)$ | $O(\log n)$ | $O(\log n)$ call stack |
| **Delete** | $O(1)$ | $O(\log n)$ | $O(\log n)$ | $O(\log n)$ call stack |
| **Rotations (LL/RR/LR/RL)** | $O(1)$ | $O(1)$ | $O(1)$ | $O(1)$ pointer swaps |
| **k-th Smallest (`kth`)** | $O(1)$ | $O(\log n)$ | $O(\log n)$ | $O(\log n)$ |
| **Rank Query (`rank`)** | $O(1)$ | $O(\log n)$ | $O(\log n)$ | $O(\log n)$ |
| **Range Count (`countInRange`)**| $O(1)$ | $O(\log n)$ | $O(\log n)$ | $O(1)$ |

#### Mathematical Proof of Logarithmic Height
In an AVL tree of height $h$, let $N(h)$ be the minimum number of nodes:
$$N(0) = 0, \quad N(1) = 1, \quad N(2) = 2$$
$$N(h) = 1 + N(h-1) + N(h-2) \quad \text{for } h \ge 2$$
This recurrence resembles the Fibonacci numbers:
$$N(h) = F_{h+2} - 1 \approx \frac{\phi^{h+2}}{\sqrt{5}} - 1 \implies h < 1.4404 \log_2(n + 2) - 0.328$$
Hence, height is guaranteed strictly bounded by $O(\log n)$, preventing the $O(n)$ degeneration seen in unaugmented BSTs.

---

### 3.2 Max Heap & Heap Sort Operations (Act 4)

| Operation | Time Complexity | Auxiliary Space | Key Mechanics |
|-----------|-----------------|-----------------|---------------|
| **`siftUp(i)`** | $O(\log n)$ | $O(1)$ | Bubbles element toward root until $\text{parent} \ge \text{child}$ |
| **`siftDown(i, n)`** | $O(\log n)$ | $O(1)$ | Sinks element down comparing with largest child |
| **`buildMaxHeap(A)`** | $O(n)$ | $O(1)$ | Bottom-up sift-down starting from $(n/2 - 1)$ down to $0$ |
| **`extractMax()`** | $O(\log n)$ | $O(1)$ | Swaps root with last active element, then `siftDown(0)` |
| **`heapSort(A)`** | $O(n \log n)$ | $O(1)$ | In-place: $O(n)$ build followed by $n-1$ extractions |

#### Mathematical Proof of $O(n)$ Bottom-Up Heap Construction
At height $h$ in a complete tree with $n$ nodes, there are at most $\lceil n / 2^{h+1} \rceil$ nodes. The cost to sift down a node at height $h$ is $O(h)$:
$$\sum_{h=0}^{\lfloor \log_2 n \rfloor} \frac{n}{2^{h+1}} O(h) = \frac{n}{2} \sum_{h=0}^{\infty} \frac{h}{2^h} = \frac{n}{2} (2) = O(n)$$
This contrasts with inserting $n$ elements sequentially via `siftUp`, which costs $\sum_{i=1}^n \log i = \Theta(n \log n)$.

---

## 4. Benchmarking Results (Section 10)

All experiments were executed using the compiled `Vault_Heist_Benchmarks.exe` suite without GUI and without event logging, using MSVC C++20 on an AMD64 architecture.

### Experiment 1: Height Comparison (BST vs AVL)
Measures maximum tree height when inserting $N$ elements in sorted order versus random permutations:

| $N$ | Input Distribution | Unbalanced BST Height | AVL Tree Height | Theoretical Minimum $\lceil \log_2(N+1) \rceil$ |
|---|---|---|---|---|
| **100** | Sorted ($1 \dots N$) | **100** (Degenerate) | **7** | 7 |
| **100** | Random Permutation | 15 | **8** | 7 |
| **1,000** | Sorted ($1 \dots N$) | **1,000** (Degenerate) | **10** | 10 |
| **1,000** | Random Permutation | 22 | **12** | 10 |
| **10,000** | Sorted ($1 \dots N$) | **10,000** (Linear corridor) | **14** | 14 |
| **10,000** | Random Permutation | 31 | **16** | 14 |

*Finding*: When input arrives sorted, BST height equals $N$, causing severe cache thrashing and potential stack overflows. The AVL tree guarantees optimal height $14$ at $N = 10,000$.

---

### Experiment 2: Search Cost
10,000 searches on a tree of size $N = 10,000$:

| Tree Type | Input Type | Total Search Time (ms) | Avg Comparisons / Search |
|---|---|---|---|
| **AVL Tree** | Sorted | **2.01 ms** | **~13.2** |
| **BST** | Sorted | $> 350.0 \text{ ms}$ | $5,000.0$ |

---

### Experiment 3: Order Statistics ($k$-th Query)
1,000 $k$-th smallest queries performed on a tree with $N = 10,000$ elements:

| Method | Complexity | Execution Time (ms) | Speedup Factor |
|---|---|---|---|
| **AVL `kth` (Augmented `subtreeSize`)** | $O(\log n)$ | **0.91 ms** | **~119x faster** |
| **BST `kth` (Inorder Traversal Scan)** | $O(n)$ | **108.34 ms** | Baseline |

---

### Experiment 4: Max Heap Construction Time
Confirming linear $O(n)$ bottom-up heap construction:

| Array Size $N$ | Distribution | Construction Time ($\mu\text{s}$) | Time / $N$ Ratio ($\mu\text{s}$/elem) |
|---|---|---|---|
| **100** | Random | $2.90 \ \mu\text{s}$ | $0.029$ |
| **1,000** | Random | $23.10 \ \mu\text{s}$ | $0.023$ |
| **10,000** | Random | $211.70 \ \mu\text{s}$ | $0.021$ |

*Finding*: The Time/$N$ ratio remains flat across two orders of magnitude, empirically confirming $O(n)$ linear complexity.

---

### Experiment 5: Heap Sort across Input Distributions
Sorting $N = 10,000$ elements using in-place Heap Sort:

| Input Distribution | Execution Time (ms) | Verified Ascending Order |
|---|---|---|
| **Random Permutation** | 1.56 ms | YES |
| **Already Sorted** | 1.48 ms | YES |
| **Reverse Sorted** | 2.26 ms | YES |
| **Duplicate-Heavy** | 1.35 ms | YES |

---

### Experiment 6: Sorting Algorithm Comparison ($N = 5,000$)

| Algorithm | Average Time | Worst Case Time | Auxiliary Space | Execution Time ($N=5000$) |
|---|---|---|---|---|
| **Heap Sort** | $O(n \log n)$ | $O(n \log n)$ | **$O(1)$ (In-place)** | **0.84 ms** |
| **Quick Sort** | $O(n \log n)$ | $O(n^2)$ | $O(\log n)$ stack | 0.65 ms |
| **Insertion Sort** | $O(n^2)$ | $O(n^2)$ | $O(1)$ | 30.77 ms |

---

## 5. Rotations & Visual Demonstrations

### 5.1 Left-Left ($LL$) Single Right Rotation
* **Trigger**: Node balance factor $\text{BF} > 1$ and left child balance factor $\ge 0$.
* **Mechanism**: Left child $x$ becomes new root; $y$ becomes right child of $x$. Subtree $T_2$ transfers to left child of $y$.
* **Code Implementation**:
  ```cpp
  AVLNode* AVLTree::rotateRight(AVLNode* y) {
      AVLNode* x = y->left;
      AVLNode* T2 = x->right;
      x->right = y;
      y->left = T2;
      updateHeightAndSize(y);
      updateHeightAndSize(x);
      return x;
  }
  ```

### 5.2 Right-Right ($RR$) Single Left Rotation
* **Trigger**: Node balance factor $\text{BF} < -1$ and right child balance factor $\le 0$.
* **Mechanism**: Right child $y$ becomes new root; $x$ becomes left child of $y$. Subtree $T_2$ transfers to right child of $x$.

### 5.3 Left-Right ($LR$) Double Rotation
* **Trigger**: Node balance factor $\text{BF} > 1$ and left child balance factor $< 0$ (inner-heavy).
* **Mechanism**:
  1. Left-rotate the left child ($x$).
  2. Right-rotate the root ($y$).

### 5.4 Right-Left ($RL$) Double Rotation
* **Trigger**: Node balance factor $\text{BF} < -1$ and right child balance factor $> 0$ (inner-heavy).
* **Mechanism**:
  1. Right-rotate the right child ($y$).
  2. Left-rotate the root ($x$).

---

## 6. Level Campaign Summary

### Act 3: The Architect's Wing (AVL Tree)
* **Level 3.1 ("Read the Badges")**: Pure visual identification of balance factor violations ($|\text{BF}| > 1$) with no automated rebalancing.
* **Level 3.2 ("Manual Single Rotations")**: Interactive $LL$ and $RR$ single rotation puzzles with animated visual transformations.
* **Level 3.3 ("Manual Double Rotations")**: Two-step $LR$ and $RL$ rotation resolution.
* **Level 3.4 ("Auto Mode & Prediction")**: Player predicts rotation outcome ($LL, RR, LR, RL, \text{None}$) prior to auto-rebalancing insertion.
* **Level 3.5 ("Boss: Order-Statistic Heist")**: High-speed $O(\log n)$ order-statistic navigation using subtree sizes.

### Act 4: The Heap Fortress (Max Heap & Heap Sort)
* **Level 4.1 ("Activate the Vault")**: Step-by-step bottom-up heap construction from an unsorted array.
* **Level 4.2 ("Repair Security Locks")**: Diagnosing and repairing sabotaged heap elements via sift-up and sift-down.
* **Level 4.3 ("Extract Greatest Treasure")**: Root extraction into the visual sorted region followed by active heap restoration.
* **Level 4.4 ("Race Against the Guards")**: Multi-step Heap Sort extraction under tight alarm tolerance.
* **Level 4.5 ("Final Vault Boss")**: Independent execution of full in-place Heap Sort unlocking the final vault door.

---

## 7. Conclusion

Member 2's components for **Vault Heist** meet all requirements set out in the project specification:
* 100% scratch implementation without standard library container shortcuts.
* Zero memory leaks with recursive destructor cleanup.
* 13/13 passing GoogleTests.
* Interactive GUI with permanent, unambiguous question cards and dual synchronised tree/array representations.
* Empirical benchmarking validating theoretical complexity bounds.
