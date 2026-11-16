# PROJECT_STATE.md

**Project:** Vault Heist: The Living Vault  
**Last updated:** 16 November 2026  
**Primary requirements source:** `Project.pdf`

> This file records the progress confirmed in the project conversations. Do not treat an item as complete unless the team has implemented and verified it. Phase numbers below are retained as the team reported them; the detailed phase definitions were not available in the current context.

## 1. Current status

- **Overall status:** IN PROGRESS
- **Reported completed phases:** Phase 0, Phase 1, Phase 2, Phase 3
- **Phase 2:** Status not confirmed; clarify before assigning work.
- **Current development stage:** Project setup/build work has been carried out, and the application was reported to launch successfully.
- **Next focus:** Confirm the exact scope and evidence for each completed phase, establish the current repository state, then continue with the next uncompleted project milestone.

## 2. Technology decisions

| Area | Decision / known state |
|---|---|
| Language | C++ |
| GUI | SFML |
| Build system | CMake |
| Version control | Git and GitHub |
| Testing | GoogleTest integration has been part of the CMake setup; actual test results need confirmation |
| Environment | Windows, MSVC x64 |
| Dependency management | vcpkg |
| CMake preset | `x64-debug` |
| IDE | Visual Studio 2026 (Insiders build was shown in the setup logs) |

Qt is out of scope. Core data structures must be implemented from scratch; do not replace them with STL tree or priority-queue implementations.

**Still to verify/document:** exact C++ standard, SFML version, GoogleTest version, compiler toolset version, and the current vcpkg manifest/dependency configuration.

## 3. Team responsibilities

| Owner | Responsibility |
|---|---|
| Member 1 | General Binary Tree and BST implementation, plus corresponding unit tests |
| Member 2 | AVL Tree and Max Heap / Heap Sort implementation, plus corresponding unit tests |
| Shared | Architecture and interfaces, SFML GUI, event/animation integration, game flow, levels, scoring/alarm, undo/replay, save/load, integration testing, benchmarks, report and final submission |

Do not change ownership without agreement from both members.

## 4. Repository and build information

- **Repository/project:** `Vault_Heist_DS`
- **Local path shown in recent setup:** `E:\Just MY Work\1. Programming Journey\5. Game Dev\Vault_Heist_DS`
- **Build preset used:** `x64-debug`
- **Dependency root observed:** Visual Studio's vcpkg installation under `...\VC\vcpkg`
- **Build command used:** `cmake --preset x64-debug`
- **Build command used:** `cmake --build --preset x64-debug`
- **Application:** Team reported that the project launched successfully.
- **Git branch / latest commit / remote push:** Not confirmed in the available conversation. Do not assume changes are pushed to GitHub.

The repository contains a `CMakeLists.txt`, `.gitignore`, and `include/structures/BinaryTree.h` according to shared terminal output. Other files may exist; inspect the current repository before changing its structure.

## 5. Phase tracking

| Phase | Status | Evidence / qualification |
|---|---|---|
| Phase 0 | COMPLETE (team-reported) | Team states this phase is complete. Detailed phase checklist/evidence is not present in the current context. |
| Phase 1 | COMPLETE (team-reported) | Setup work included CMake/vcpkg configuration and an `x64-debug` preset. The application was reported to launch. A `BinaryTree.h` file was visible in Git diff output. |
| Phase 2 | NOT CONFIRMED | No reliable status or phase definition available here. Confirm before planning work. |
| Phase 3 | COMPLETE (team-reported) | Team states this phase is complete. Detailed phase checklist/evidence is not present in the current context. |

**Important:** “Team-reported complete” records the team's status, not an independent verification of every requirement. Preserve this distinction until the relevant code, tests, or deliverables are reviewed.

## 6. Build and testing status

- **CMake configuration:** `x64-debug` preset was listed and used in setup.
- **Dependency/toolchain check:** vcpkg toolchain path was checked and found.
- **Application build/run:** Reported successful by the team.
- **Unit tests:** GoogleTest/CMake test setup was being configured. A complete passing test run is not confirmed by the available evidence.
- **CTest:** A test command was attempted, but the final complete output/result is not available here.
- **Known build issue:** Earlier `main.cpp` compile errors involving an undeclared `avl` identifier were shown during setup. The team later reported that the project ran; confirm the current clean build rather than assuming the issue remains or is fully resolved.

Before marking testing complete, run the current build and test commands and record the actual output.

## 7. Architecture and implementation notes

The Project PDF specifies a layered, GUI-independent design:

1. Core data structures / model
2. Operation event log
3. Animation controller
4. Layout engine and renderer
5. Level manager and game systems
6. UI / input management

The model should emit events; SFML rendering and input must not contain the core tree/heap algorithms.

Required major structures and acts:

- **Act 1:** General Binary Tree — traversals, reconstruction, height, diameter, LCA, paths and mirror.
- **Act 2:** BST — search, insertion, deletion, successor/predecessor, range query and degeneration.
- **Act 3:** AVL — balance factors, all four rotation cases, insertion/deletion, subtree size, kth-smallest and rank.
- **Act 4:** Max Heap and Heap Sort — build, sift-up/sift-down, heapify, extract-max, heap sort, and synchronised tree/array views.

The current public interfaces, event payload definitions, ownership model, and final folder architecture have not been confirmed in the available context. Do not invent them; inspect the repository and agree on them before parallel implementation.

## 8. Pending project deliverables

The Project PDF requires:

- [ ] Complete source code and README with run instructions and game controls.
- [ ] Unit test suite covering all major data structures and algorithms.
- [ ] Test report with genuine test results.
- [ ] Written report of 8–12 pages.
- [ ] Architecture and algorithmic complexity analysis.
- [ ] Benchmark results as specified in Section 10 of the PDF.
- [ ] GUI screenshots.
- [ ] Screenshots of LL, RR, LR and RL AVL rotations.
- [ ] Heap construction, extraction, heapify and final sorted-result demonstrations.
- [ ] Level data files.
- [ ] Short guide explaining how to add levels.
- [ ] Final executable, functional submission packaged as required by the PDF.
- [ ] Final ZIP filename follows `25I-XXXX_25I-XXXX_Section.zip`.
- [ ] Submit through Google Classroom; only one member submits for the group.
- [ ] Both members attend the demonstration and viva.

**Deadline in the PDF:** 20 November 2026, 11:59 PM.

## 9. Git workflow status

- Use `main` as the stable integration branch.
- Use separate feature branches for member-owned work.
- Pull the latest `main` before starting dependent work.
- Commit small, tested changes with descriptive messages.
- Review and build before merging.
- Never commit build output, IDE caches, generated binaries, or credentials.

**Current branch, latest commit, PRs, and push status:** Not confirmed. Check with `git status`, `git branch --show-current`, `git log -1 --oneline`, and `git remote -v`.

## 10. Blockers and decisions to confirm

1. Confirm what Phase 0, Phase 1, Phase 2 and Phase 3 each contain, so the phase tracker maps to the team's actual roadmap.
2. Confirm Phase 2 status.
3. Inspect the current repository and identify the latest branch/commit and whether the recent work has been pushed.
4. Run a fresh configure/build and the full CTest suite; record the exact results.
5. Confirm the C++ standard, SFML version and GoogleTest status.
6. Review `BinaryTree.h` and establish which required operations are implemented and tested.
7. Confirm whether the earlier `avl` compile error is resolved in the current checkout.

## 11. Next actions

### Member 1
- Inspect `BinaryTree.h` and the current Binary Tree/BST files.
- List implemented operations against the PDF API.
- Identify missing operations and corresponding unit tests.
- Do not mark a module TESTED until its tests have actually passed.

### Member 2
- Inspect the current AVL and Max Heap files, if present.
- List implemented operations against the PDF API.
- Identify missing operations and corresponding unit tests.
- Do not assume a module exists or is complete without checking the repository.

### Shared
- Confirm phase definitions and Phase 2 status.
- Check Git branch, commit and remote/push status.
- Run a clean CMake configure/build and CTest; share the full result.
- Reconcile the repository structure and agree on core interfaces before expanding GUI integration.
- Keep this file updated after verified milestones and commit the update.

---

**Status rule:** Use `NOT STARTED`, `IN PROGRESS`, `BLOCKED`, `IMPLEMENTED – NOT TESTED`, `TESTED`, `INTEGRATED`, and `COMPLETE`. Reserve `COMPLETE` for work that satisfies the relevant PDF requirement, is integrated, and has been tested.
