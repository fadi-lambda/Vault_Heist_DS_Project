# Level Creation Guide — Vault Heist

This guide explains how to author and configure level data files for **Vault Heist** without modifying core C++ game logic, as required by **Deliverable #4**.

---

## 1. Directory Structure & File Naming

All 20 campaign levels are stored under the `levels/` directory, organized by Act (`act1` through `act4`), and loaded by `loadLevelById(levelId)` (`src/game/LevelLoader.cpp`):

* `levels/act1/level_1_1.json` .. `level_1_5.json` — **Act 1: General Binary Tree (The Blueprint Wing)**
* `levels/act2/level_2_1.json` .. `level_2_5.json` — **Act 2: Binary Search Tree (The Sorted Wing)**
* `levels/act3/level_3_1.json` .. `level_3_5.json` — **Act 3: AVL Tree (The Architect's Wing)**
* `levels/act4/level_4_1.json` .. `level_4_5.json` — **Act 4: Max Heap & Heap Sort (The Heap Fortress)**

For a two-digit `level_id` $= 10 \times \text{act} + \text{index}$ (e.g., `11` for Level 1.1, `34` for Level 3.4, `45` for Level 4.5), `levelJsonRelativePath(levelId)` resolves to:
```
levels/act<act>/level_<act>_<index>.json
```

---

## 2. Unified Level JSON Schema

Every level file is a single top-level JSON object parsed by the hand-rolled JSON reader (`include/game/Json.h`, `src/game/Json.cpp`) into `VaultHeist::LevelData` (`include/game/LevelData.h`). All level-specific payload arrays live directly at the top level of the JSON object (no nested `"data"` wrapper).

```json
{
  "level_id": 11,
  "act": 1,
  "title": "1.1 Traversals Protocol",
  "concept": "Preorder, Inorder, Postorder, and Level-Order tree traversals",
  "objective": "Visit all 7 rooms in the exact order required by the active traversal protocol!",
  "controls": "Click nodes in the active traversal sequence  |  [C] Cycle Traversal  |  Hover for stats",
  "start_status": "Follow the active traversal sequence shown above and click each room in order.",
  "nodes": [
    { "key": 50, "left": 30, "right": 70 },
    { "key": 30, "left": 20, "right": 40 },
    { "key": 70, "left": 60, "right": 80 },
    { "key": 20, "left": null, "right": null },
    { "key": 40, "left": null, "right": null },
    { "key": 60, "left": null, "right": null },
    { "key": 80, "left": null, "right": null }
  ],
  "hints": [
    "Preorder (Root -> Left -> Right): visit a room the moment you reach it, then finish its left wing before its right wing."
  ]
}
```

### Common Header & Guidance Fields (Required on All Levels)
| Field | Type | Description |
|---|---|---|
| `level_id` | `int` | Two-digit level identifier (`11`–`15`, `21`–`25`, `31`–`35`, `41`–`45`). |
| `act` | `int` | Act number (`1` to `4`). |
| `title` | `string` | Level title shown on the Campaign Map, Briefing Modal, and Top HUD. |
| `concept` | `string` | Core data-structure concept tested by the level. |
| `objective` | `string` | Mission objective displayed in the Briefing Modal and Top HUD bar. |
| `controls` | `string` | Control summary shown in the Briefing Modal and bottom HUD hint line. |
| `start_status` | `string` | Initial guidance prompt displayed in the bottom panel when the level starts. |
| `hints` | `string[]` | Up to 6 stage-indexed hint strings (`LEVEL_MAX_HINTS = 6`) shown when the player presses **[H]**. |

### Automatic Optimal Steps & Alarm Capacity Formula
Rather than hardcoding `optimal_steps` and `alarm_capacity` in JSON (where they could drift out of sync with edited tree/array data), `LevelLoader::computeOptimalSteps(const LevelData&)` (`src/game/LevelLoader.cpp`) derives `optimalSteps` directly from the level's payload (e.g., `nodeCount`, simulated BST comparison steps, unbalanced node count, puzzle count, or bottom-up sift-down count), and `ScoringEngine::computeAlarmCapacity(optimalSteps)` (`src/game/Scoring.cpp`) computes the alarm limit using the course formula:
$$\text{alarmCapacity} = \lceil 1.5 \times \text{optimalSteps} \rceil + 2$$

---

## 3. Act-by-Act Payload Fields

### 3.1 Act 1 — General Binary Tree (`levels/act1/level_1_1.json` .. `level_1_5.json`)
* **Explicit Tree Levels (`1.1`, `1.3`, `1.4`, `1.5`)**:
  * **`nodes`** (array of objects, up to `LEVEL_MAX_NODES = 32`): Each entry specifies `{ "key": <int>, "left": <int|null>, "right": <int|null> }`. The first entry (`nodes[0]`) is the root, and `null` denotes an empty child pointer.
* **Blueprint Reconstruction (`1.2`)**:
  * **`preorder`** (`int[]`): Preorder traversal sequence of the target binary tree.
  * **`inorder`** (`int[]`): Inorder traversal sequence of the target binary tree (must contain the same distinct keys as `preorder`).

### 3.2 Act 2 — Binary Search Tree (`levels/act2/level_2_1.json` .. `level_2_5.json`)
* **`initial_keys`** (`int[]`, all Act 2 levels): Keys inserted in order to construct the starting BST.
* **`target_keys`** (`int[]`):
  * In **Level 2.1 (Search Heist)**: Keys the player must search for from the root.
  * In **Level 2.2 (Stash the Loot)**: Incoming keys to walk to an empty leaf slot (or reject if duplicate).
  * In **Level 2.5 (Boss: The Lazy Architect)**: Sorted deliveries inserted into the degenerating right-spine chain.
* **`delete_sequence`** (`int[]`, **Level 2.3**): Sequence of keys the player classifies (`Leaf`, `One-Child`, `Two-Children`) and deletes.
* **`range_lo`**, **`range_hi`** (`int`, **Level 2.4**): Inclusive lower and upper bounds $[lo, hi]$ for the BST range query and subtree-pruning puzzle.
* **`k_queries`** (`int[]`, **Level 2.5**): Target key searched down the degenerate chain in Boss Stage 2.

### 3.3 Act 3 — AVL Tree (`levels/act3/level_3_1.json` .. `level_3_5.json`)
* **Badge Identification (`3.1`)**:
  * **`nodes`**: Explicit parent-child node list (`{ "key", "left", "right" }`) containing intentional balance factor violations ($|\text{BF}| > 1$). `findUnbalancedKeys()` automatically identifies all violating keys from the tree heights.
* **Single & Double Rotation Repair (`3.2`, `3.3`)**:
  * **`puzzles`** (array of objects, up to `LEVEL_MAX_PUZZLES = 4`):
    * **`case`**: `"LL"`, `"RR"`, `"LR"`, or `"RL"`.
    * **`root_key`**: Key of the unbalanced root pivot.
    * **`left_child`** or **`right_child`**: Key of the heavy child pivot.
    * **`left_grandchild`**, **`right_grandchild`**, or **`inner_child`**: Key of the grandchild node completing the 3-node imbalance.
    * Optional descriptive metadata fields (`correct_rotation`, `new_root`, `step_1`, `step_2`, `final_root`).
* **Auto Mode & Rotation Prediction (`3.4`)**:
  * **`initial_keys`** (`int[]`): Starting keys inserted into the AVL tree before predictions begin.
  * **`insertion_sequence`** (array of objects, up to `LEVEL_MAX_INSERTS = 16`): Each entry defines `{ "key": <int>, "pivot_key": <int>, "expected_rotation": "LL" | "RR" | "LR" | "RL" | "None" }`.
* **Order-Statistic Heist (`3.5`)**:
  * **`initial_keys`** (`int[]`): Keys used to build the augmented order-statistic AVL tree.
  * **`k_queries`** (`int[]`): Target $k$ values for $O(\log n)$ $k$-th most valuable ($k$-th largest) queries using `subtreeSize` (`sz`).

### 3.4 Act 4 — Max Heap & Heap Sort (`levels/act4/level_4_1.json` .. `level_4_5.json`)
* **`initial_array`** (`int[]`, all Act 4 levels): Starting array of integer keys loaded into the heap/array view.
* **`expected_sorted_result`** (`int[]`, **Levels 4.3, 4.4, 4.5**): Expected ascending sorted array verified when all elements have been extracted into the sorted tail.
* **Optional descriptive metadata fields**:
  * `expected_final_heap` (`int[]`, **Level 4.1**): Resulting array after bottom-up Floyd construction.
  * `violation_index` (`int`), `repair_operation` (`string`), `expected_repaired_heap` (`int[]`, **Level 4.2**): Documents the sabotaged index and repaired heap state.

---

## 4. Testing and Validation

Whenever you edit or add a level file under `levels/act1/`–`levels/act4/`:
1. Confirm all keys in `nodes`, `preorder`/`inorder`, `initial_keys`, or `insertion_sequence` are distinct integers (except where duplicate rejection is explicitly tested in Level 2.2).
2. Run the automated GoogleTest suite (`Vault_Heist_Tests.exe`). The `LevelLoaderTest.LoadsAllTwentyLevelsAndComputesOptimalSteps`, `Act3AvlTests`, and `Act4HeapTests` suites automatically parse all 20 JSON files, verify `computeOptimalSteps()` $> 0$, and validate rotation and heap-sort outcomes against the JSON definitions.
