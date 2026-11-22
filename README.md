# Vault Heist: The Living Vault

**Course**: Data Structures & Algorithms  
**Technologies**: C++20, SFML 3, CMake, Ninja, GoogleTest  
**Group Submission**: 2-Member Team Project  
**Submission Archive**: `25I-XXXX_25I-XXXX_Section.zip`  
**Deadline**: 20th November 2026, 11:59 PM  

---

## 1. Project Overview

**Vault Heist** is a GUI puzzle-strategy game designed to teach, demonstrate, and benchmark fundamental tree and heap data structures.
The player assumes the role of a master thief infiltrating a high-security vault whose chambers and corridors form a dynamic, living tree structure:
* **The Alarm Meter**: Every room entered or key compared increments the security alarm. A poorly balanced or corrupted tree causes the alarm meter to overflow, triggering alarm failure.
* **Act 1 — The Blueprint Wing (General Binary Tree)**: Master the foundational blueprint of the vault. Execute Preorder, Inorder, Postorder, and Level-Order traversals, reconstruct binary trees from Preorder + Inorder sequences via drag-and-drop, calculate bottom-up node heights and tree diameter, trace routes between chambers via Lowest Common Ancestor (LCA), and invert the vault bottom-up.
* **Act 2 — The Sorted Wing (Binary Search Tree)**: Exploit the ordered vault layout. Perform root-to-target comparisons for $O(h)$ searches, insert loot while rejecting duplicate keys, execute 3-case BST deletion (leaf, one-child, two-children with in-order successor collapse), prune subtrees in range queries $[lo, hi]$, and confront BST $O(n)$ degeneration on sorted deliveries.
* **Act 3 — The Architect's Wing (AVL Tree)**: Master self-balancing binary search trees. Diagnose balance factor violations, perform manual LL/RR/LR/RL rotations, predict automated rebalancing, and steal high-value targets in $O(\log n)$ using order-statistic augmentation (`subtreeSize`).
* **Act 4 — The Heap Fortress (Max Heap & Heap Sort)**: Penetrate the final vault protected by a priority-based mechanism. Build a Max Heap from unsorted arrays in $O(n)$ bottom-up time, repair security sabotage with sift-up and sift-down, and execute in-place **Heap Sort** to unlock the final vault door.
* **Endless Survival Mode & Sandbox Simulator**: Test real-time AVL rebalancing against a shrinking deadline timer with seeded RNG and save/resume persistence, or experiment freely in the dual-canvas AVL + Max Heap Sandbox.

---

## 2. Strict Course Guidelines Compliance

* **NO `std::vector`**: All heap, event-batch, and array operations utilize manual dynamic array management (`new[]`/`delete[]`).
* **NO `enum` / `enum class`**: Plain typed constants (`static const int`) are utilized across all modules.
* **NO `<algorithm>`**: All swap, min, max, and clamping operations use custom hand-written helper functions.
* **NO global variables**: Complete game and level states live cleanly inside their models and scopes.
* **Height Convention**: Documented consistently across the project as $\text{height}(\text{leaf}) = 1$ and $\text{height}(\text{nullptr}) = 0$.
* **GUI Independence**: Core data structure classes (`BinaryTree`, `BST`, `AVLTree`, `MaxHeap`) are 100% decoupled from SFML and GUI drawing logic.

---

## 3. How to Build & Run

### Prerequisites
* Windows 10/11 (64-bit)
* Visual Studio 2022 / Community with C++20 support
* CMake 3.22+ & Ninja
* vcpkg with `sfml` and `gtest` installed

### Option A: Build via Visual Studio IDE (Recommended)
1. Open the project folder in Visual Studio (`File -> Open -> Folder...`).
2. Select the `x64-Debug` configuration from the top toolbar.
3. Build all targets: `Build -> Build All` (`Ctrl + Shift + B`).
4. Select `Vault_Heist.exe` as the startup target and press **F5** to run!

### Option B: Build via Developer Command Prompt / PowerShell
Open Developer PowerShell for VS 2022:
```powershell
cd out\build\x64-debug
cmake --build . --target Vault_Heist
cmake --build . --target Vault_Heist_Tests
cmake --build . --target Vault_Heist_Benchmarks

# Run Game:
.\Vault_Heist.exe

# Run Unit Tests:
.\tests\Vault_Heist_Tests.exe

# Run Benchmarks:
.\Vault_Heist_Benchmarks.exe
```

---

## 4. Game Controls & Navigation

### Global Controls & Playback Panel
* **Mouse Left-Click**: Click buttons on the HUD / bottom Action Panel, click tree nodes or heap array cells directly, or drag tokens in Level 1.2.
* **Mouse Wheel**: Zoom in/out on the structure canvas ($35\%$ to $300\%$).
* **Mouse Drag (Empty Canvas / Right-Click / Middle-Click)**: Pan the structure camera across the canvas.
* **Double-Click (Empty Canvas / Right-Click)**: Reset canvas camera zoom and position to $100\%$.
* **Hover Node / Cell**: Inspect live node metadata tooltip (Key, ID, Height, Balance Factor, Subtree Size).
* **[H] / Click `Hint [H]`**: Reveal a stage-aware hint in the bottom panel (once per level; caps rating at 2 stars).
* **[Z] / Click `Undo [Z]`**: Revert the data structure and scoring state to the snapshot before the last move.
* **[Space] / Click `Play` or `Pause`**: Toggle automatic playback of the Event Log animation stream.
* **[Right Arrow] / Click `Step`**: Pause auto-play and step forward one operation event at a time.
* **Click `Replay` / `Skip` / Drag Scrubber or Speed Slider**: Replay the current operation batch, finish the animation immediately, scrub to any event index, or adjust playback speed ($0.25\times$ to $3.0\times$).
* **[Esc] / Click `Map [Esc]` or `Menu [Esc]`**: Return to the Campaign Map, Main Menu, or close modal prompts.

### Act 1 Controls — The Blueprint Wing (General Binary Tree)
* **Level 1.1 ("Traversals Protocol")**:
  * **Click Node**: Visit rooms in the exact order required by the active traversal protocol (Preorder, Inorder, Postorder, or Level-Order).
  * **[C] / Click `Cycle Mode [C]`**: Cycle between Preorder, Inorder, Postorder, and Level-Order protocols before starting the sequence.
* **Level 1.2 ("Decode the Blueprint")**:
  * **Mouse Drag & Drop**: Drag room tokens from the top tray and drop them into the blueprint tree slots using the displayed `PRE` (Preorder) and `IN` (Inorder) sequences.
  * **Auto-Validation**: Filling all 7 blueprint slots automatically validates the reconstructed tree against the target Preorder and Inorder traversals.
* **Level 1.3 ("The Alarm Spreads")**:
  * **[-], [+], [Up/Down Arrows], or [1]–[9] / Click `-` or `+`**: Adjust the numeric stepper for the currently highlighted room's height (in Postorder, where $\text{leaf} = 1$) and then for the total tree **Diameter** (longest path in nodes).
  * **[Enter] / Click `Submit`**: Confirm the entered height or diameter value for the highlighted room.
* **Level 1.4 ("Find Where Two Routes Meet")**:
  * **Step 1 — Click Two Rooms**: Click any starting room (**Room A**) and any distinct destination room (**Room B**) on the tree.
  * **Step 2 — Click LCA**: Click the **Lowest Common Ancestor (LCA)** of Room A and Room B.
  * **Step 3 — Trace Route**: Click every room in order along the simple path connecting Room A and Room B through their LCA.
* **Level 1.5 ("Mirror the Vault")**:
  * **Click Node**: Click internal rooms in **bottom-up** order (children's subtrees before their parent) to swap their left and right children, or click leaves to verify them.
  * **[A] or [M] / Click `Auto Mirror [A]`**: Trigger automated recursive postorder tree inversion and animate all swaps.

### Act 2 Controls — The Sorted Wing (Binary Search Tree)
* **Level 2.1 ("Search Heist")**:
  * **[1] or [L] / Click `< Go Left`**: Compare target key with current room and descend to the left child ($\text{target} < \text{current}$).
  * **[2], [F], or [Enter] / Click `= Found`**: Confirm the current room holds the target key ($\text{target} == \text{current}$).
  * **[3] or [D] / Click `Go Right >`**: Compare target key with current room and descend to the right child ($\text{target} > \text{current}$).
* **Level 2.2 ("Stash the Loot")**:
  * **[1] or [L] / Click `< Go Left`**: Walk left from the current room when the incoming loot key is smaller; inserts automatically upon stepping into an empty child slot.
  * **[2], [F], or [Enter] / Click `= Reject Dup`**: Reject a duplicate incoming key when the current room already holds that exact key.
  * **[3] or [D] / Click `Go Right >`**: Walk right from the current room when the incoming loot key is larger.
* **Level 2.3 ("Steal and Collapse")**:
  * **[1] / Click `Leaf`**: Classify the highlighted deletion target as a Leaf node ($0$ children) and remove it.
  * **[2] / Click `One-Child`**: Classify the highlighted target as a One-Child node ($1$ child) and splice its child upward.
  * **[3] / Click `Two-Children`**: Classify the highlighted target as a Two-Children node ($2$ children) to replace it with its in-order successor and animate the two-phase collapse.
* **Level 2.4 ("Range Raid")**:
  * **[1] or [R] / Click `Report Room`**: Report the current room when its key falls inside the target range $[lo, hi]$.
  * **[2] or [L] / Click `Prune Left`**: Prune and grey-out the entire left subtree when $\text{currentKey} \le lo$ (awarding bonus points).
  * **[3] or [P] / Click `Prune Right`**: Prune and grey-out the entire right subtree when $\text{currentKey} \ge hi$ (awarding bonus points).
* **Level 2.5 ("Boss: The Lazy Architect")**:
  * **Stage 1 (Sorted Deliveries)**: Use **[1/L] `< Go Left`** and **[3/D] `Go Right >`** to insert sorted keys $\{20, 30, 40, 50\}$ from root $10$, watching the BST degenerate into an $O(n)$ right-spine chain as the alarm capacity tightens.
  * **Stage 2 (Chain Search)**: Use **[1/L] `< Go Left`**, **[2/F/Enter] `= Found`**, and **[3/D] `Go Right >`** to search for the target key down the height-$n$ degenerate chain before the alarm overflows.

### Act 3 Controls — The Architect's Wing (AVL Tree)
* **Level 3.1 ("Read the Badges")**:
  * **Click Node**: Click every unbalanced node on the tree showing a **RED** violation badge ($|\text{BF}| > 1$, where $\text{BF} = \text{height}(\text{left}) - \text{height}(\text{right})$).
* **Level 3.2 ("Single Rotations (LL / RR)")**:
  * **Step 1 — Click Node**: Click the unbalanced pivot node ($|\text{BF}| > 1$) on the tree.
  * **Step 2 — [1] or [L] / Click `LL: Rotate Right (1)`**: Apply a single Right Rotation around the selected pivot to repair a Left-Left ($LL$, $\text{BF} = +2$) imbalance.
  * **Step 2 — [2] or [R] / Click `RR: Rotate Left (2)`**: Apply a single Left Rotation around the selected pivot to repair a Right-Right ($RR$, $\text{BF} = -2$) imbalance.
* **Level 3.3 ("Double Rotations (LR / RL)")**:
  * **Step 1 — [1] / Click `Case: LR (1)` or [2] / Click `Case: RL (2)`**: Inspect the opposite-sign BF badges and classify the zig-zag imbalance case ($LR$ or $RL$).
  * **Step 2 — Click Child Pivot**: Click the heavy child pivot node first to execute the first rotation (straightening the zig-zag into a single-lean chain).
  * **Step 3 — Click Root Pivot**: Click the unbalanced root pivot node second to complete the double rotation and restore $|\text{BF}| \le 1$.
* **Level 3.4 ("Auto Mode and Prediction")**:
  * **[1] / Click `LL (1)`**: Predict a single Right Rotation ($LL$ case) for the incoming key and trigger insertion.
  * **[2] / Click `RR (2)`**: Predict a single Left Rotation ($RR$ case) for the incoming key and trigger insertion.
  * **[3] / Click `LR (3)`**: Predict a Left-Right double rotation ($LR$ case) for the incoming key and trigger insertion.
  * **[4] / Click `RL (4)`**: Predict a Right-Left double rotation ($RL$ case) for the incoming key and trigger insertion.
  * **[5] or [N] / Click `None (N)`**: Predict that no rotation is needed ($|\text{BF}| \le 1$ everywhere) and trigger insertion.
* **Level 3.5 ("Boss: Order-Statistic Heist")**:
  * **Click Node**: Inspect the `sz` (`subtreeSize`) badges and click the **$k$-th most valuable ($k$-th largest)** treasure directly in $O(\log n)$ comparisons, including after security insertions and automatic rotations update `sz`.

### Act 4 Controls — The Heap Fortress (Max Heap & Heap Sort)
* **Level 4.1 ("Activate the Vault")**:
  * **Click Node or Array Cell**: Click each violating non-leaf node (or its larger swap child) in bottom-up Floyd order from $\lfloor n/2 \rfloor - 1$ down to root $[0]$ to sift down and construct a valid Max Heap in $O(n)$ time.
* **Level 4.2 ("Repair the Security Locks")**:
  * **Click Node or Array Cell**: Click the sabotaged root $[0]$ (or its larger child $[1]$) on either the binary tree view or the synchronized array strip to sift down and restore the Max Heap invariant.
* **Level 4.3 ("Extract the Greatest Treasure")**:
  * **Click Root `[0]` (Tree Node or Array Cell)**: Repeatedly extract the maximum element at index $[0]$ into the green ascending **SORTED REGION** tail and watch the remainder re-heapify via sift-down.
* **Level 4.4 ("Race Against the Guards")**:
  * **Click Root `[0]` (Tree Node or Array Cell)**: Rapidly extract all treasures from root $[0]$ within a strict $\Theta(n \log n)$ step budget before the alarm meter fills.
* **Level 4.5 ("Final Vault Boss")**:
  * **Phase 1 — [B] / Click `Build Max Heap [B]` (or last non-leaf node)**: Execute bottom-up Floyd `buildMaxHeap` ($O(n)$) across the unsorted treasure array.
  * **Phase 2 — Click Root `[0]` (Tree Node or Array Cell)**: Repeatedly extract root $[0]$ ($O(n \log n)$) until the active heap is empty and all treasures are sorted in ascending order.

### Endless Survival Mode Controls
* **[Enter] or [X] / Click `EXECUTE: INSERT/DELETE <key> [Enter]`**: Execute the incoming seeded AVL operation (`INSERT` or `DELETE`), animate rotations, and validate all AVL height/BF/subtree-size invariants before the shrinking per-operation deadline bar expires.
* **[P] / Click `Pause [P]` or `Resume [P]`**: Pause or resume the live survival and deadline timers.
* **[F5] / Click `Save Run [F5]`**: Persist the active Endless session (seed, RNG state, survival time, pending op, and AVL tree in preorder-with-`#` format) to `endless_save.txt`.
* **[F9] / Click `Load Run [F9]`**: Resume a previously saved Endless session from `endless_save.txt` with full structural invariant verification.
* **[R] / Click `Restart [R]`**: Start a fresh Endless run with a new seed.
* **[L] / Click `Scores [L]`**: Open the Endless Survival Leaderboard (`endless_leaderboard.txt`), where **[R]** starts a new run, **[S]** replays the last seed, and **[C]** switches to Campaign scores.

### Sandbox Simulator Controls
* **[I] / Click `[I] AVL Insert`**: Insert the next key into the left-canvas `AVLTree` and animate any resulting single or double rotations in the Event Log.
* **[D] / Click `[D] AVL Delete`**: Delete the minimum key from the `AVLTree` with automatic rebalancing and event playback.
* **[K] / Click `[K] AVL kth`**: Query `avl.kth(k)` in $O(\log n)$ using augmented subtree sizes and display the result toast.
* **[E] / Click `[E] Heap Extract`**: Call `extractMax()` on the right-canvas `MaxHeap`, move the maximum into the green sorted tail, and animate sift-down.
* **[Z] / Click `[Z] Undo`**: Undo the last AVL or Max Heap operation using serialized structure snapshots.
* **[R] / Click `[R] Reset`**: Reset both the `AVLTree` and `MaxHeap` canvases to their initial Sandbox states.

---

## 5. Deliverables Checklist

| Deliverable | File / Location | Description |
|---|---|---|
| **1. Source Code & README** | `src/`, `include/`, `README.md` | Full executable source code + build & control guide |
| **2. Unit Test Suite & Report** | `tests/`, `docs/reports/TEST_REPORT.md` | 56/56 passing GoogleTests across 16 test suites covering all operations |
| **3. Written Report** | `report/Vault_Heist_Report.md`, `report/Vault_Heist_Report.pdf` | System architecture, Save/Load system, complexity proofs, benchmarks, 33 UI screenshots |
| **4. Level Data Files & Guide** | `levels/act1/`–`levels/act4/`, `docs/LEVEL_CREATION_GUIDE.md` | 20 JSON level data files + guide to authoring levels |
| **Benchmarking Suite** | `benchmarks/BenchmarkSuite.cpp`, `src/benchmark/Benchmark.cpp` | Standalone & integrated benchmarks measuring BST vs AVL & Heap Sort |
