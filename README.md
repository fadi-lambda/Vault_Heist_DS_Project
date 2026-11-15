# Vault Heist: The Living Vault

**Course**: Data Structures & Algorithms  
**Technologies**: C++20, SFML 3, CMake, Ninja, GoogleTest  
**Group Submission**: 2-Member Team Project (Member 2 Focus: Acts 3 & 4)  
**Submission Archive**: `25I-XXXX_25I-XXXX_Section.zip`  
**Deadline**: 20th November 2026, 11:59 PM  

---

## 1. Project Overview

**Vault Heist** is a GUI puzzle-strategy game designed to teach, demonstrate, and benchmark fundamental tree and heap data structures.
The player assumes the role of a master thief infiltrating a high-security vault whose chambers and corridors form a dynamic, living tree structure:
* **The Alarm Meter**: Every room entered or key compared increments the security alarm. A poorly balanced or corrupted tree causes the alarm meter to overflow, triggering alarm failure.
* **Act 3 — The Architect's Wing (AVL Tree)**: Master self-balancing binary search trees. Diagnose balance factor violations, perform manual LL/RR/LR/RL rotations, predict automated rebalancing, and steal high-value targets in $O(\log n)$ using order-statistic augmentation (`subtreeSize`).
* **Act 4 — The Heap Fortress (Max Heap & Heap Sort)**: Penetrate the final vault protected by a priority-based mechanism. Build a Max Heap from unsorted arrays in $O(n)$ bottom-up time, repair security sabotage with sift-up and sift-down, and execute in-place **Heap Sort** to unlock the final vault door.

---

## 2. Strict Course Guidelines Compliance

* **NO `std::vector`**: All heap and array operations utilize manual dynamic array management (`new[]`/`delete[]`).
* **NO `enum` / `enum class`**: Plain typed constants (`static const int`) are utilized across all modules.
* **NO global variables**: Complete game and level states live cleanly inside their models and scopes.
* **Height Convention**: Documented consistently across the project as $\text{height}(\text{leaf}) = 1$ and $\text{height}(\text{nullptr}) = 0$.
* **GUI Independence**: Core data structure classes (`AVLTree`, `MaxHeap`) are 100% decoupled from SFML and GUI drawing logic.

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
cd D:\dsa\Vault_Heist_DS_Project\out\build\x64-debug
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

### Global Controls
* **Mouse**: Click buttons on the Interactive Action Panel or click tree nodes directly.
* **Esc**: Return to Campaign Map or Main Menu / Quit.

### Act 3 Controls (AVL Wing)
* **Level 3.1 ("Read the Badges")**:
  * **Click Node**: Toggle selection ring on unbalanced nodes with **RED** badges ($|\text{BF}| > 1$).
  * **[Enter] / Click Submit**: Submit selection.
  * **[R] / Click Reset**: Clear selections.
* **Level 3.2 ("Single Rotations")**:
  * **[R] / Click Right-Rotate**: Apply Right Rotation on $LL$ imbalance.
  * **[L] / Click Left-Rotate**: Apply Left Rotation on $RR$ imbalance.
  * **[Space] / Click Next**: Advance to the next puzzle.
* **Level 3.3 ("Double Rotations")**:
  * **[1] / Click LR Rotation**: Perform two-step $LR$ rotation (Left-rotate child, Right-rotate root).
  * **[2] / Click RL Rotation**: Perform two-step $RL$ rotation (Right-rotate child, Left-rotate root).
* **Level 3.4 ("Auto Mode & Prediction")**:
  * **[1] LL, [2] RR, [3] LR, [4] RL, [5] None**: Select predicted rotation.
  * **[Space] / Click Confirm & Insert**: Execute insertion and view auto-rebalancing animation.
* **Level 3.5 ("Boss: Order-Statistic Heist")**:
  * **[1] - [4] / Click Answer Buttons**: Select answer to order-statistic questions ($k$-th smallest and rank queries in $O(\log n)$).

### Act 4 Controls (Heap Fortress)
* **Level 4.1 ("Activate the Vault")**:
  * **[S] / Click Sift-Down Next**: Perform bottom-up sift-down step from non-leaf nodes down to root.
  * **[V] / Click Validate**: Validate that every parent is $\ge$ children to activate the vault.
* **Level 4.2 ("Repair Security Locks")**:
  * **[D] / Click Sift-Down Root**: Sink sabotaged root element to its valid leaf position.
  * **[V] / Click Validate**: Verify restored heap property.
* **Level 4.3 ("Extract Greatest Treasure")**:
  * **[E] / Click Extract Max**: Extract root max to the green **SORTED REGION**.
  * **[H] / Click Heapify Remainder**: Restore active heap order.
* **Level 4.4 ("Race Against Guards")**:
  * **[E] / Click Extract & Sift-Down**: High-speed extraction cycles under a strict alarm budget.
* **Level 4.5 ("Final Vault Boss")**:
  * **[B] / Click Build Heap**: Bottom-up Max Heap construction ($O(n)$).
  * **[E] / Click Extract Next**: Repeated root extraction into sorted region ($O(n \log n)$).
  * **[Enter] / Click Unlock Vault**: Open the Final Vault Door!

---

## 5. Deliverables Checklist

| Deliverable | File / Location | Description |
|---|---|---|
| **1. Source Code & README** | `src/`, `include/`, `README.md` | Full executable source code + build & control guide |
| **2. Unit Test Suite & Report** | `tests/`, `docs/reports/TEST_REPORT.md` | 13/13 passing GoogleTests covering all operations |
| **3. Written Report (8–12 pages)**| `docs/reports/PROJECT_REPORT.md` | System architecture, complexity proofs, benchmarks |
| **4. Level Data Files & Guide** | `levels/act3/`, `levels/act4/`, `docs/LEVEL_CREATION_GUIDE.md` | JSON level data files + guide to adding levels |
| **Benchmarking Suite** | `benchmarks/BenchmarkSuite.cpp` | Standalone executable measuring BST vs AVL & Heap Sort |
