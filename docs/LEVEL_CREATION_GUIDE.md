# Level Creation Guide — Vault Heist

This guide explains how to author and configure new levels for **Vault Heist** without modifying core game logic, as required by **Deliverable #4**.

---

## 1. Directory Structure

Levels are stored under the `levels/` directory organized by act:
* `levels/act1/` — General Binary Tree (The Blueprint Wing)
* `levels/act2/` — Binary Search Tree (The Sorted Wing)
* `levels/act3/` — AVL Tree (The Architect's Wing)
* `levels/act4/` — Max Heap & Heap Sort (The Heap Fortress)

---

## 2. Level JSON Schema

Every level definition file follows a standard JSON format:

```json
{
  "level_id": 31,
  "act": 3,
  "title": "Level Title",
  "concept": "Core Data Structure concept tested",
  "objective": "Clear description of player mission",
  "alarm_capacity": 20,
  "optimal_steps": 5,
  "data": { ... }
}
```

### Key Fields:
* **`level_id`**: Integer code (e.g., `31` for Act 3 Level 1, `45` for Act 4 Level 5).
* **`act`**: Act number (`1` to `4`).
* **`alarm_capacity`**: Alarm limit before level failure.
  $$\text{alarmCapacity} = \lceil 1.5 \times \text{optimalSteps} \rceil + 2$$
* **`optimal_steps`**: Theoretical minimal number of comparisons/swaps/operations to solve the puzzle.
* **`data`**: Level-specific payload (tree node lists, initial array values, or puzzle cases).

---

## 3. Creating an Act 3 Level (AVL Tree)

To define an AVL Tree puzzle (e.g. rotation repair, order statistic, or badge identification):

1. **Badge Identification**:
   * Supply a list of nodes with their explicit parent-child links.
   * Ensure at least one node has $|\text{BF}| > 1$ ($\text{BF} = \text{height}(\text{left}) - \text{height}(\text{right})$).
   * Specify `expected_violation_keys`.

2. **Rotation Repair**:
   * Supply the unbalance type: `"LL"`, `"RR"`, `"LR"`, or `"RL"`.
   * For single rotations: define `root_key`, `child_key`, `grandchild_key`.
   * For double rotations: define the two-step rotation sequence (`step_1`, `step_2`).

3. **Order Statistics**:
   * Supply an array of keys to insert into the AVL tree.
   * Define the query type: `"KTH_SMALLEST"` (with target $k$) or `"RANK"` (with target key).

---

## 4. Creating an Act 4 Level (Max Heap)

To define a Max Heap puzzle:

1. **Bottom-Up Heap Construction**:
   * Supply an unsorted integer array `initial_array`.
   * The start index for bottom-up construction is $(n / 2) - 1$.
   * Specify `expected_final_heap`.

2. **Sabotage / Repair**:
   * Supply a heap array with 1 or 2 deliberate violations (where parent < child).
   * Specify `violation_index` and the required fix (`"SIFT_DOWN"` or `"SIFT_UP"`).

3. **Heap Sort Campaign**:
   * Supply an unsorted array of treasures.
   * Set `alarm_capacity` based on the $O(n \log n)$ swap count.

---

## 5. Testing and Validation

Before committing a new level file:
1. Validate that the JSON file is well-formed.
2. Confirm that tree values in reconstruction or BST levels are distinct.
3. Verify that `alarm_capacity` strictly follows the course formula:
   $$\text{alarm\_capacity} = \lceil 1.5 \times \text{optimal\_steps} \rceil + 2$$
