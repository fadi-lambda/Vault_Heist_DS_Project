# PROJECT_STATE.md

# Vault Heist: The Living Vault

## 1. Project Status

Current Phase: Phase 0 - Environment and Project Audit
Overall Status: COMPLETED

Project Deadline: 20 November 2026

## 2. Technology Stack

- Language: C++20
- GUI: SFML 3
- Build: CMake
- Build Generator: Ninja
- IDE: Visual Studio Insiders
- Compiler: MSVC (exact version not recorded)
- Package Manager: vcpkg
- Testing: GoogleTest
- Version Control: Git and GitHub

## 3. Repository Status

- GitHub repository: Created
- Main branch: main
- Remote tracking: origin/main
- CMake project: Configured successfully
- Project structure: Established
- x64 Debug configuration: Successful
- SFML linking: Successful
- SFML test window: Successfully launched
- GoogleTest integration: Successful
- Initial GoogleTest setup test: Passed
- Latest changes: Not yet confirmed committed and pushed

## 4. Team Responsibilities

Member 1:
- General Binary Tree
- Binary Search Tree
- Related unit tests

Member 2:
- AVL Tree
- Max Heap
- Heap Sort
- Related unit tests

Shared:
- Architecture and integration
- Event system
- SFML GUI
- Game systems
- Testing and documentation

## 5. Development Progress

Phase 0: COMPLETED
Phase 1: NOT STARTED
Phase 2: NOT STARTED
Phase 3: NOT STARTED
Phase 4: NOT STARTED
Phase 5: NOT STARTED
Phase 6: NOT STARTED
Phase 7: NOT STARTED
Phase 8: NOT STARTED
Phase 9: NOT STARTED

## 6. Technical Decisions

- C++20 and SFML 3 are the selected technologies.
- CMake is the shared build system.
- Ninja is the selected build generator.
- vcpkg manages external dependencies.
- All major data structures must be implemented from scratch.
- GUI and data-structure logic must remain separate.
- Operations will produce events for visualisation.
- Snapshot-based undo is the initial recommendation.
- GoogleTest is integrated through CMake.

## 7. Current Blockers / Unknowns

- vcpkg path portability between team members needs to be addressed.
- Friend's environment and successful build need verification.
- Shared architecture and interfaces are not finalised.
- Git changes from the initial project restructuring need review and push confirmation.

## 8. Next Actions

Shared:
- Finalise portable vcpkg configuration.
- Verify the friend's development environment.
- Review and commit the initial project restructuring.
- Agree on project architecture.
- Begin Phase 1 foundation.

## 9. Git Status

- Main branch: main
- Remote branch: origin/main
- Feature branches: Not established
- Initial repository: Created
- Latest changes: Pending review, commit, and push confirmation

## 10. Testing Status

- CMake configuration: Passed
- x64 Debug build: Passed
- SFML linking: Passed
- SFML window execution: Passed
- GoogleTest setup test: Passed
- Project-specific data-structure tests: Not yet implemented