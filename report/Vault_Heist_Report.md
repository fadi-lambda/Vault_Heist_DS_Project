# Vault Heist: The Living Vault

### Data Structures Project Report (FAST-NUCES), Deliverable #3

**Engine:** C++20, SFML, CMake + Ninja + vcpkg (MSVC). **Branch:** `main`. **Date:** 22 Nov 2026

---

## 1. Overview

*Vault Heist* is a puzzle game for learning data structures. The player breaks into a "living vault" whose security system is a tree or heap, and each move is a real data-structure operation: a traversal, a BST compare-and-descend, an AVL rotation, or a heap extraction. All operations run on hand-written structures (raw pointers or manual arrays, with no STL containers, `<algorithm>` or enums). Each operation emits an event stream, and the GUI animates and replays that stream step by step.

**Scope delivered**

| Part | Content |
|---|---|
| Act 1 (General Binary Tree) | 1.1 Traversals Protocol, 1.2 Decode the Blueprint, 1.3 The Alarm Spreads, 1.4 Find Where Two Routes Meet, 1.5 Mirror the Vault |
| Act 2 (BST) | 2.1 Search Heist, 2.2 Stash the Loot, 2.3 Steal and Collapse, 2.4 Range Raid, 2.5 Boss: The Lazy Architect |
| Act 3 (AVL) | 3.1 Read the Badges, 3.2 Single Rotations (LL/RR), 3.3 Double Rotations (LR/RL), 3.4 Auto Mode and Prediction, 3.5 Boss: Order-Statistic Heist |
| Act 4 (Max Heap) | 4.1 Activate the Vault, 4.2 Repair the Security Locks, 4.3 Extract the Greatest Treasure, 4.4 Race Against the Guards, 4.5 Final Vault Boss |
| Endless Mode | Timed AVL insert/delete survival on a seeded RNG, with its own leaderboard and save/resume |
| Sandbox | Free-play screen (`SCREEN_SANDBOX`) |

All 20 levels are data-driven. They load from `levels/actN/level_N_M.json` through a hand-rolled JSON reader.

## 2. System Architecture

The design target was the module table in Project.pdf §4.2. The table below describes what was actually built, taken from the responsibilities in the real header files.

| Layer | Module (header) | Responsibility as implemented |
|---|---|---|
| Structures | `TreeNode.h` | Node struct (`id`, key, left/right) shared by BinaryTree and BST |
| | `BinaryTree.h` | General binary tree: 4 traversals, reconstruction from traversal pairs, height, diameter, LCA, mirror. Recursive helpers on raw pointers |
| | `BST.h` (uses `TreeNode`) | Insert/search/delete (3 cases, in-order successor), range query, `kthSmallest` (in-order scan), `isValidBST`, comparison counters for benchmarks |
| | `AVLTree.h` (own `AVLNode`) | Self-balancing tree with height, subtree size, and LL/RR/LR/RL rotations on insert and delete. O(log n) `kth` using subtree sizes, `isValidAVL`, rotation counters |
| | `MaxHeap.h` | Array-based heap (`data/size/capacity`, manual resize): sift-up insert, `extractMax` + heapify, Floyd bottom-up `buildMaxHeap`, in-place heap sort, comparison/swap counters |
| Events | `Event.h`, `EventLog.h` | One flat `Event` struct (`EVENT_*` int constants, rotation case, indices, values), a manual dynamic-array `EventBatch`, and `EventLog` storing batches per operation |
| GUI | `AnimationController.h` | Plays an `EventBatch` (step, play/pause, speed, skip, scrubber/replay) and tweens structural and swap events |
| | `LayoutEngine.h`, `Renderer.h`, `Ui.h` | Tree/heap/array-cell layout, highlight styles, BF/size badges, and a pooled text cache at a fixed 1600×900 logical resolution |
| | `DragDrop*.h`, `ToastQueue.h`, `UndoStack.h`, `ScoreLog.h` | Drag-and-drop (Level 1.2), toasts, undo via serialized snapshots of every structure, persistent score log |
| Game | `Json.h`, `LevelLoader.h`, `LevelData.h` | Minimal JSON reader with no dependency. A unified level schema and computation of the optimal step count from the level data |
| | `Scoring.h` (`ScoringEngine`) | Score/alarm engine. Alarm capacity = ⌈1.5 × optimalSteps⌉ + 2. Penalty kinds and star rating |
| | `HintSystem.h` | One hint per level. Using it caps the rating at 2 stars |
| | `EndlessMode.h` (`EndlessSession`, `EndlessLeaderboard`) | Seeded op generator that runs real `AVLTree` insert/delete with invariant checks. Save/resume persists the seed, RNG state, timer, pending op and the tree as preorder |
| | `SaveManager.h` (`SaveManager`, `SaveData`) | Plain-text save/load manager (`savegame.dat`) for campaign progress (unlocked levels, stars), Endless mode state (seed, wave, score), and tree states in preorder-with-`#` format, plus structural invariant validation via `Validator.h` |
| Bench | `Benchmark.h` | Produces `benchmark_report.txt` (Section 4) |

> **Deviations from the spec table.** (1) `AVLTree` does **not** extend `BST`. It is a standalone class with its own `AVLNode`, which also stores height and subtree size. Only `BinaryTree` and `BST` share `TreeNode`. (2) `SaveManager` works alongside `ScoreLog`, `EndlessLeaderboard`, and `EndlessSession`'s save/resume. (3) The event system was added afterwards as overloads (`insert(key, EventBatch&)`), so the original signatures still work.

**Data flow per move:** input → game logic in `main.cpp` → structure method with an `EventBatch&` → `EventLog` → `AnimationController` playback → `LayoutEngine`/`Renderer`. `ScoringEngine` updates score and alarm from whether the move was valid.

### 2.1 Save/Load System & Memory Safety (Project.pdf §8)

- **`SaveManager` Persistence:** The `SaveManager` class (`include/game/SaveManager.h`, `src/game/SaveManager.cpp`) saves and loads overall game state into a plain-text file (`savegame.dat`, header `VAULT_HEIST_SAVE_V1`). It persists campaign progress (`UNLOCKED` flags and `STARS` ratings across all 20 levels), Endless Mode state (`SEED`, wave, and `ENDLESS` score), active heap and sorted partition arrays (`HEAP`, `SORTED`), and tree states serialized in **preorder-with-`#`** null-marker format (`BT`, `BST`, `AVL`, e.g., `50 30 # # 70 60 # # #`). On load, `Validator` (`include/structures/Validator.h`) verifies all structural invariants before restoring state.
- **Reset Progress Button (2-Click Confirm Flow):** The System Settings screen (`SCREEN_SETTINGS`) includes a **Reset Progress (Campaign & Endless Save)** button using a confirm-then-wipe two-click flow. The first click arms `resetConfirmPending` and displays a warning toast and confirmation prompt (`CONFIRM RESET PROGRESS? (Click again to wipe save)`); only a second click wipes the campaign unlocks/stars (re-locking Levels 2–20), resets Endless Mode progress, and overwrites `savegame.dat` with a clean state. Any other action cancels the pending confirmation.
- **Memory-Leak Fix & Reset Robustness:** `BST`, `AVLTree`, and `BinaryTree` reset operations now explicitly invoke `.clear()` (which recursively frees all allocated nodes via `destroyRecursive` and resets root/size/ID counters) instead of default-constructed copy-assignment (`tree = Tree()`), which was unsafe (shallow pointer copy leaking old nodes or double-freeing) and is now explicitly `= delete`d alongside copy constructors. Zero memory leaks are guaranteed across level transitions, sandbox resets, and save reloads, verified across all 56 unit tests.

## 3. Algorithmic Complexity Analysis

Heights use the convention leaf = 1, null = 0. The measured figures come from `benchmark_report.txt` (XorShift32, seed 20261119, Debug build; the operation counts are exact).

| Structure | Operation | Big-O | Why / evidence |
|---|---|---|---|
| BinaryTree | Traversals, height, diameter, mirror | O(n) | Each node is visited once |
| | LCA (recursive) | O(n) | No ordering, so both subtrees may be searched |
| | Rebuild from inorder + pre/post | O(n²) | Linear search for the root index in the inorder segment |
| BST | Search / insert / delete | O(h): O(log n) avg, **O(n) worst** | On sorted input the tree became a list: height **100 / 1000 / 10000** for N = 100 / 1000 / 10000 (Table 4.1). Avg comparisons per search were **66.87 / 669.73 / 6652.85** (Table 4.2) |
| | Range query | O(h + k) | Subtrees outside [lo, hi] are pruned |
| | kthSmallest (in-order scan) | O(n) | **49.94 / 489.32 / 4991.47** avg comparisons per query, about N/2 (Table 4.4) |
| AVL | Search / insert / delete | **O(log n)** guaranteed | Height **7 / 10 / 14** on sorted input, equal to ⌈log₂(N+1)⌉. Random input gave **8 / 12 / 16** (Table 4.1) |
| | Rotation (LL/RR/LR/RL) | O(1) each | Insert needs at most one (single or double) fix-up. Measured avg rotations per insert was **0.683–0.7300** on random input and **0.9300–0.9986** on sorted input. Delete can rotate at several levels: up to **6** in one delete (N = 10000, random) and **0.3644–0.4988** on average (Table 4.3) |
| | kth (subtree sizes) | O(log n) | **5.87 / 9.19 / 12.52** avg comparisons, matching the BST answers in every case (Table 4.4) |
| MaxHeap | insert / extractMax | O(log n) | Sift-up and sift-down move along one root-to-leaf path (height ⌊log₂ n⌋ + 1) |
| | getMax | O(1) | `data[0]` |
| | buildMaxHeap (Floyd) | **O(n)** | Comparisons/N stays at most **2.00** for every N: up to **1.998** (ascending), **1.840–1.886** (random), **0.990–1.000** (descending, 0 swaps) (Table 4.5) |
| HeapSort | sort | O(n log n) time, **O(1)** extra space | Build is O(n), then n extractions cost O(log n) each. Comparisons went from **1033** to **235230** (random) as N grew 100×, about the n log n growth rate (Table 4.6) |

## 4. Benchmarking Results (Project.pdf §10)

All numbers are copied unchanged from `benchmark_report.txt`.

**4.1 Height comparison (BST vs AVL)**

| N | Input | BST height | AVL height | ⌈log₂(N+1)⌉ | BST valid | AVL valid |
|---|---|---|---|---|---|---|
| 100 | Sorted | 100 | 7 | 7 | YES | YES |
| 100 | Random | 14 | 8 | 7 | YES | YES |
| 1000 | Sorted | 1000 | 10 | 10 | YES | YES |
| 1000 | Random | 21 | 12 | 10 | YES | YES |
| 10000 | Sorted | 10000 | 14 | 14 | YES | YES |
| 10000 | Random | 33 | 16 | 14 | YES | YES |

On sorted input the BST degenerates into a list (height = N), while the AVL tree reaches the optimal height. Even on random input the BST was about twice as tall as the AVL tree.

**4.2 Search cost (10,000 random searches in [1, 1.5N])**

| N | Structure | Input | Total (ms) | Avg/search (µs) | Avg comps |
|---|---|---|---|---|---|
| 100 | BST | Sorted | 6.464 | 0.6464 | 66.87 |
| 100 | AVL | Sorted | 1.240 | 0.1240 | 6.19 |
| 100 | BST | Random | 1.095 | 0.1095 | 6.57 |
| 100 | AVL | Random | 1.177 | 0.1177 | 6.61 |
| 1000 | BST | Sorted | 87.208 | 8.7208 | 669.73 |
| 1000 | AVL | Sorted | 2.461 | 0.2461 | 9.31 |
| 1000 | BST | Random | 2.110 | 0.2110 | 10.30 |
| 1000 | AVL | Random | 2.472 | 0.2472 | 9.43 |
| 10000 | BST | Sorted | 1244.592 | 124.4592 | 6652.85 |
| 10000 | AVL | Sorted | 4.022 | 0.4022 | 12.90 |
| 10000 | BST | Random | 4.078 | 0.4078 | 14.55 |
| 10000 | AVL | Random | 4.368 | 0.4368 | 13.07 |

At N = 10000 on sorted input, the AVL search was about 309× faster (1244.592 ms vs 4.022 ms). On random input the two are close, because a random BST is shallow by chance.

**4.3 AVL rotation count**

| N | Input | Insert rots | Avg/insert | Delete rots | Avg/delete | Max (1 delete) |
|---|---|---|---|---|---|---|
| 100 | Sorted | 93 | 0.9300 | 44 | 0.4400 | 1 |
| 100 | Random | 73 | 0.7300 | 42 | 0.4200 | 3 |
| 1000 | Sorted | 990 | 0.9900 | 491 | 0.4910 | 1 |
| 1000 | Random | 683 | 0.6830 | 377 | 0.3770 | 4 |
| 10000 | Sorted | 9986 | 0.9986 | 4988 | 0.4988 | 1 |
| 10000 | Random | 7052 | 0.7052 | 3644 | 0.3644 | 6 |

Rotations per insert stay below 1 (sorted input approaches one RR per insert), so rebalancing is O(1) amortised. Deletes can cascade, which is why one delete needed up to 6 rotations.

**4.4 Order statistics (1,000 kth queries)**

| N | Method | Complexity | Total (ms) | Avg comps | Match |
|---|---|---|---|---|---|
| 100 | AVL kth (subtree size) | O(log n) | 0.253 | 5.87 | YES |
| 100 | BST kthSmallest (inorder) | O(n) | 2.860 | 49.94 | YES |
| 1000 | AVL kth (subtree size) | O(log n) | 0.309 | 9.19 | YES |
| 1000 | BST kthSmallest (inorder) | O(n) | 11.273 | 489.32 | YES |
| 10000 | AVL kth (subtree size) | O(log n) | 0.611 | 12.52 | YES |
| 10000 | BST kthSmallest (inorder) | O(n) | 332.117 | 4991.47 | YES |

Storing subtree sizes makes order-statistic queries logarithmic. The in-order scan's cost grows linearly, at about N/2 comparisons.

**4.5 Max Heap construction (bottom-up)**

| N | Input | Avg time (µs) | Comparisons | Comps/N | Swaps | Valid |
|---|---|---|---|---|---|---|
| 100 | Random | 3.40 | 184 | 1.840 | 75 | YES |
| 100 | Sorted (Asc) | 4.06 | 190 | 1.900 | 96 | YES |
| 100 | Sorted (Desc) | 1.59 | 99 | 0.990 | 0 | YES |
| 1000 | Random | 46.36 | 1873 | 1.873 | 733 | YES |
| 1000 | Sorted (Asc) | 40.84 | 1982 | 1.982 | 992 | YES |
| 1000 | Sorted (Desc) | 15.20 | 999 | 0.999 | 0 | YES |
| 10000 | Random | 562.10 | 18862 | 1.886 | 7519 | YES |
| 10000 | Sorted (Asc) | 401.32 | 19982 | 1.998 | 9992 | YES |
| 10000 | Sorted (Desc) | 152.46 | 9999 | 1.000 | 0 | YES |

Comparisons/N stays at or below 2.00 at every size, which confirms Floyd's O(n) build rather than O(n log n). Descending input is already a heap, so it needs no swaps.

**4.6 Heap sort (all distributions)**

| N | Distribution | Avg time (ms) | Comparisons | Swaps | Sorted |
|---|---|---|---|---|---|
| 100 | Random | 0.0112 | 1033 | 588 | YES |
| 100 | Sorted | 0.0119 | 1081 | 640 | YES |
| 100 | Reverse-Sorted | 0.0099 | 944 | 516 | YES |
| 100 | Duplicate-Heavy | 0.0097 | 969 | 520 | YES |
| 1000 | Random | 0.2891 | 16877 | 9099 | YES |
| 1000 | Sorted | 0.2655 | 17583 | 9708 | YES |
| 1000 | Reverse-Sorted | 0.2476 | 15965 | 8316 | YES |
| 1000 | Duplicate-Heavy | 0.2182 | 14431 | 7415 | YES |
| 10000 | Random | 4.0002 | 235230 | 124156 | YES |
| 10000 | Sorted | 3.1597 | 244460 | 131956 | YES |
| 10000 | Reverse-Sorted | 3.1968 | 226682 | 116696 | YES |
| 10000 | Duplicate-Heavy | 2.9797 | 195814 | 99580 | YES |

The number of comparisons barely depends on input order, as expected for heap sort's Θ(n log n) behaviour. Duplicate-heavy input is the cheapest because ties end sift-down early.

**4.7 Heap sort (O(1) space) vs merge sort (O(n) space)**

| N | Dist. | Heap ms | Heap comps | Heap swaps | Merge ms | Merge comps | Merge moves | OK |
|---|---|---|---|---|---|---|---|---|
| 100 | Random | 0.0112 | 1033 | 588 | 0.0129 | 542 | 1344 | YES |
| 100 | Sorted | 0.0119 | 1081 | 640 | 0.0114 | 356 | 1344 | YES |
| 100 | Reverse | 0.0099 | 944 | 516 | 0.0112 | 316 | 1344 | YES |
| 100 | Dup-Heavy | 0.0097 | 969 | 520 | 0.0121 | 521 | 1344 | YES |
| 1000 | Random | 0.2891 | 16877 | 9099 | 0.2596 | 8722 | 19952 | YES |
| 1000 | Sorted | 0.2655 | 17583 | 9708 | 0.1504 | 5044 | 19952 | YES |
| 1000 | Reverse | 0.2476 | 15965 | 8316 | 0.1511 | 4932 | 19952 | YES |
| 1000 | Dup-Heavy | 0.2182 | 14431 | 7415 | 0.2050 | 8116 | 19952 | YES |
| 10000 | Random | 4.0002 | 235230 | 124156 | 3.4338 | 120395 | 267232 | YES |
| 10000 | Sorted | 3.1597 | 244460 | 131956 | 1.9480 | 69008 | 267232 | YES |
| 10000 | Reverse | 3.1968 | 226682 | 116696 | 1.9645 | 64608 | 267232 | YES |
| 10000 | Dup-Heavy | 2.9797 | 195814 | 99580 | 2.8613 | 112304 | 267232 | YES |

Merge sort makes about half as many comparisons (120395 vs 235230 at N = 10000, random) and runs faster. Heap sort trades that speed for O(1) auxiliary space, while merge sort needs an O(n) buffer.

## 5. GUI Screenshots

All images are real 1600×900 frames from `Vault_Heist.exe --verify-screenshots report_assets/screenshots`.

<div class="grid">

![01 Main menu](../report_assets/screenshots/01_main_menu.png)
![02 Campaign map](../report_assets/screenshots/02_campaign_map.png)
![03 Level briefing (2.2)](../report_assets/screenshots/03_level_briefing.png)
![04 BST play with toast and tooltip](../report_assets/screenshots/04_bst_node_play_toast_tooltip.png)
![07 Leaderboard](../report_assets/screenshots/07_leaderboard.png)
![08 Settings](../report_assets/screenshots/08_settings.png)
![09 Canvas pan and zoom](../report_assets/screenshots/09_canvas_pan_zoom.png)
![10 Level 1.1 Traversals](../report_assets/screenshots/10_act1_1_traversals.png)
![11 Level 1.2 Blueprint drag-and-drop](../report_assets/screenshots/11_act1_2_blueprint_dragdrop.png)
![12 Level 1.3 Heights and diameter](../report_assets/screenshots/12_act1_3_heights_diameter.png)
![13 Level 1.4 LCA path](../report_assets/screenshots/13_act1_4_lca_path.png)
![14 Level 1.5 Mirror the Vault](../report_assets/screenshots/14_act1_5_mirror_vault.png)
![15 Level 2.1 Search Heist](../report_assets/screenshots/15_act2_1_search_heist.png)
![16 Level 2.2 Stash the Loot](../report_assets/screenshots/16_act2_2_stash_the_loot.png)
![17 Level 2.3 Steal and Collapse](../report_assets/screenshots/17_act2_3_steal_and_collapse.png)
![18 Level 2.4 Range Raid](../report_assets/screenshots/18_act2_4_range_raid.png)
![19 Level 2.5 Lazy Architect boss](../report_assets/screenshots/19_act2_5_lazy_architect_boss.png)
![20 Level 3.1 Read the Badges](../report_assets/screenshots/20_act3_1_read_badges.png)
![05 Level 3.4 AVL BF badges](../report_assets/screenshots/05_avl_badges.png)
![24 Level 3.5 Order-statistic boss](../report_assets/screenshots/24_act3_5_order_stat_boss.png)
![06 Level 4.1 Heap and synchronised array](../report_assets/screenshots/06_heap_array_sync.png)
![26 Level 4.4 Race Against the Guards](../report_assets/screenshots/26_act4_4_race_complexity.png)
![28 Endless Mode](../report_assets/screenshots/28_endless_mode_play.png)
![29 Endless leaderboard](../report_assets/screenshots/29_endless_leaderboard.png)

</div>

Levels 3.2–3.4 and 4.3/4.5 appear in Sections 5.1–5.2. **Level 4.2 (Repair the Security Locks) and the Sandbox screen have no `saveShot` call in the verify block, so they have no screenshot.**

### 5.1 All Four AVL Rotation Types

<div class="grid2">

![LL: shot 21, Level 3.2 puzzle 1. Pivot 30 (BF = +2) with a left-left chain 30→20→10. The panel offers "LL: Rotate Right".](../report_assets/screenshots/21_act3_2_single_rotations.png)
![RR: shot 22b, Level 3.2 puzzle 2. Pivot 10 (BF = −2) with a right-right chain 10→20→30. "RR: Rotate Left".](../report_assets/screenshots/22b_act3_2_rr_rotation.png)
![LR: shot 22, Level 3.3 puzzle 1. Root 30 (BF = +2), left child 10 (BF = −1), zig-zag. "LR case confirmed", rotate child 10 first.](../report_assets/screenshots/22_act3_3_double_rotations.png)
![RL: shot 22c, Level 3.3 puzzle 2. Root 10 (BF = −2), right child 30 (BF = +1), zig-zag. "RL case confirmed", rotate child 30 first.](../report_assets/screenshots/22c_act3_3_rl_rotation.png)

</div>

Shot 23 (Level 3.4, "Auto Mode and Prediction") shows the player predicting LL/RR/LR/RL/None before each real insert, with the insert's event log (VISIT, CMP, INSERT, HEIGHT, SIZE) in the right panel. Shots 22b and 22c were added for this report so that RR and RL each have a screenshot.

### 5.2 Max Heap: Construction → Extraction + Heapify → Sorted Result

<div class="grid2">

![Step 1, construction (shot 26b, Level 4.5): bottom-up buildMaxHeap on 8 keys. The event log shows HEAPIFY start [3], [2], [1], [0] (the last non-leaf down to the root) and its swaps, giving the array 95 80 70 30 60 40 15 25.](../report_assets/screenshots/26b_act4_5_build_max_heap.png)
![Step 2, extract-max + heapify (shot 25, Level 4.3): SWAP [0]↔[3], EXTRACT max = 70, HEAP SIZE 4→3, HEAPIFY start [0], SWAP [0]↔[1]. The sorted tail is 70 75 80 95 (green).](../report_assets/screenshots/25_act4_3_extract_ascending.png)
![Step 3, heap sort in progress (shot 27, Level 4.5): after 4 extractions the active heap is 40 30 25 15 and the ascending tail is 60 70 80 95. The log shows EXTRACT max = 60 followed by heapify.](../report_assets/screenshots/27_act4_5_boss_heapsort.png)
![Step 4, final sorted result (shot 27b, Level 4.5): Active 0, Sorted 8. The array is 15 25 30 40 60 70 80 95 in ascending order, with "All treasures heap-sorted ascending!".](../report_assets/screenshots/27b_act4_5_sorted_result.png)

</div>

Shots 26b and 27b were added for this report so that construction and the fully sorted array each have a screenshot. Shot 26 (Level 4.4) also displays the "7 extractions, O(log n) each = Θ(n log n)" readout.

## 6. Conclusion

All four structures were built from scratch and connected to an event-driven GUI, backed by 56 passing GoogleTest unit tests. The benchmarks match the theory: BST height grows to O(n) on sorted input while AVL height stays at ⌈log₂(N+1)⌉; AVL inserts average fewer than one rotation; Floyd's build uses at most 2 comparisons per element; and heap sort is Θ(n log n) in time and O(1) in extra space.
