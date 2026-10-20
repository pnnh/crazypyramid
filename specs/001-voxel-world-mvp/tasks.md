---

description: "Task list for Voxel World MVP implementation"
---

# Tasks: Voxel World MVP (体素世界核心闭环)

**Input**: Design documents from `/specs/001-voxel-world-mvp/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/input-actions.md, quickstart.md

**Tests**: Not included. Per the project constitution's Development Workflow section, this
project phase relies on manual PIE validation (see `quickstart.md`) rather than an automated
test suite — no test tasks are generated below.

**Organization**: Tasks are grouped by user story (per spec.md priorities) so each story can be
implemented and PIE-tested independently, in priority order (P1 → P1 → P2 → P2).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency on an incomplete task)
- **[Story]**: Which user story this task belongs to (US1–US4, per spec.md)
- All file paths are relative to the repository root (`/Users/Larry/Projects/unreal/crazypyramid`)

## Path Conventions

Single Unreal project, extending the existing `MyProject` native module in place (see plan.md
"Project Structure" — no new project/package boundary):

- C++: `Source/MyProject/Public/Voxel/`, `Source/MyProject/Private/Voxel/`,
  `Source/MyProject/Public/Player/`, `Source/MyProject/Private/Player/`
- Content (created via `unreal-mcp`, not hand-authored files): `Content/VoxelSandbox/`

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Prepare the folders, build dependencies, and base level this feature needs.

- [X] T001 Create directories `Source/MyProject/Public/Voxel/`, `Source/MyProject/Private/Voxel/`, `Source/MyProject/Public/Player/`, `Source/MyProject/Private/Player/`
- [X] T002 [P] Add `"ProceduralMeshComponent"` and `"EnhancedInput"` to `PublicDependencyModuleNames` in `Source/MyProject/MyProject.Build.cs`
- [X] T003 [P] Create new level asset `Content/VoxelSandbox/Maps/VoxelMVP` via `unreal-mcp` (`editor_toolset.toolsets.scene.SceneTools` / `AssetTools`), as the dedicated level for this feature (keeps it separate from the inherited `OpenWorld`/`NewMap`/`Demo` content)

**Checkpoint**: Module compiles with the new dependencies; `VoxelMVP` level exists and can be opened.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core voxel data/mesh/world types every user story builds on.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [X] T004 [P] Define `EVoxelBlockType` (Air, Grass, Dirt, Stone, Bedrock) and a static color/`IsDestructible` lookup in `Source/MyProject/Public/Voxel/VoxelTypes.h` (+ `.cpp` if the lookup needs out-of-line storage), per data-model.md "EVoxelBlockType"
- [X] T005 [P] Define the plain mesh-buffer struct (`FVoxelMeshData`: vertex positions, triangle indices, normals, per-vertex colors) in `Source/MyProject/Public/Voxel/VoxelMeshBuilder.h`, with no `UObject`/engine-actor dependencies so it is safe to build off the game thread
- [X] T006 [US-shared] Implement `BuildChunkMeshData(...)` — naive per-face-culling meshing over a chunk's voxel array (+ access to neighbor-chunk border voxels), emitting per-vertex color from `EVoxelBlockType`'s lookup — in `Source/MyProject/Private/Voxel/VoxelMeshBuilder.cpp` (depends on T004, T005)
- [X] T007 [P] Implement `AVoxelChunk` (16×16×64 `EVoxelBlockType` array, `ChunkCoordinate`, `UProceduralMeshComponent`, `IsDirty` flag, bounds-checked local-coordinate get/set) in `Source/MyProject/Public/Voxel/VoxelChunk.h` + `Source/MyProject/Private/Voxel/VoxelChunk.cpp` (depends on T004)
- [X] T008 Implement the async remesh pipeline on `AVoxelChunk` — `AsyncTask` (Task Graph) running `BuildChunkMeshData`, game-thread continuation calling `CreateMeshSection`/`UpdateMeshSection` with collision enabled (complex-as-simple) — in `Source/MyProject/Private/Voxel/VoxelChunk.cpp` (depends on T006, T007)
- [X] T009 [P] Implement `AVoxelWorld` (chunk map keyed by `ChunkCoordinate`, `WorldSizeInChunks`/`Seed`/`GenerationParams` properties, world↔chunk↔local coordinate conversion helpers) in `Source/MyProject/Public/Voxel/VoxelWorld.h` + `Source/MyProject/Private/Voxel/VoxelWorld.cpp` (depends on T007)
- [X] T010 [P] Create the vertex-color-driven master material `M_VoxelChunk` via `unreal-mcp` `MaterialTools`, saved to `Content/VoxelSandbox/Materials/M_VoxelChunk`, matching the vertex-color contract from T006
- [X] T011 [P] Create the Enhanced Input assets (`IMC_VoxelDefault`, `IA_Move`, `IA_Look`, `IA_Jump`, `IA_Break`, `IA_Place`, `IA_CycleBlockType`) via `unreal-mcp` `AssetTools`, saved to `Content/VoxelSandbox/Input/`, per `contracts/input-actions.md`

**Checkpoint**: Voxel types, chunk/world classes, async remesh, material, and input assets all exist — user story implementation can begin.

---

## Phase 3: User Story 1 - 进入并观察一个真实的体素世界 (Priority: P1) 🎯 MVP

**Goal**: On Play, the world is generated with noise-driven terrain and at least 3 visually distinct block layers over an indestructible bedrock floor.

**Independent Test**: Press Play in `VoxelMVP`; without moving or interacting, visually confirm hills (not flat ground) and layered Grass/Dirt/Stone (+ Bedrock at the bottom) — per spec.md User Story 1 acceptance scenarios.

### Implementation for User Story 1

- [X] T012 [US1] Implement per-column height generation using `FMath::PerlinNoise2D` plus the Grass/Dirt/Stone/Bedrock layering rule (research.md "Terrain generation", "Block types & vertical layering") in `AVoxelWorld`'s generation logic, `Source/MyProject/Private/Voxel/VoxelWorld.cpp` (depends on T009, T004)
- [X] T013 [US1] Wire world generation to populate every chunk's voxel array across the full 8×8 grid at `BeginPlay`, then trigger each chunk's initial async remesh (T008's pipeline) in `Source/MyProject/Private/Voxel/VoxelWorld.cpp` (depends on T012, T008)
- [X] T014 [P] [US1] Place an `AVoxelWorld` instance in `Content/VoxelSandbox/Maps/VoxelMVP` via `unreal-mcp` `SceneTools.add_to_scene_from_class`, with default `Seed`/`GenerationParams` set (depends on T009, T003)
- [X] T015 [US1] Assign `M_VoxelChunk` as the default material each `AVoxelChunk` applies to its mesh section, in `Source/MyProject/Private/Voxel/VoxelChunk.cpp` (depends on T010, T007)

**Checkpoint**: Pressing Play shows a hilly, visibly layered voxel world. User Story 1 is independently demoable.

---

## Phase 4: User Story 2 - 行走探索世界 (Priority: P1)

**Goal**: The player can move a standard gravity/collision first-person character through the world, correctly blocked by terrain.

**Independent Test**: Press Play, use WASD + mouse to walk to several different points in the world without clipping through terrain or falling out of the world — per spec.md User Story 2 acceptance scenarios.

### Implementation for User Story 2

- [X] T016 Implement `AVoxelCharacter` (a `ACharacter` subclass with a first-person camera component) in `Source/MyProject/Public/Player/VoxelCharacter.h` + `Source/MyProject/Private/Player/VoxelCharacter.cpp`
- [X] T017 [US2] Bind `IA_Move`/`IA_Look`/`IA_Jump` (from T011) to standard character movement/look/jump in `Source/MyProject/Private/Player/VoxelCharacter.cpp` (depends on T016, T011)
- [X] T018 [P] [US2] Confirm/extend chunk collision (complex-as-simple, enabled on every mesh section including rebuilt ones) in `Source/MyProject/Private/Voxel/VoxelChunk.cpp` so `AVoxelCharacter` collides with generated terrain (depends on T008)
- [X] T019 [US2] Set `AVoxelCharacter` as the default pawn class for `VoxelMVP` (World Settings or a minimal GameMode override) and place a `PlayerStart` above the generated terrain, via `unreal-mcp` `SceneTools`/`EditorToolset` (depends on T016, T014)

**Checkpoint**: Player can walk and look around the generated world with correct collision. User Stories 1+2 are both independently demoable.

---

## Phase 5: User Story 3 - 挖除瞄准的方块 (Priority: P2)

**Goal**: Left-click removes the aimed block; neighboring faces (including across a chunk boundary) render correctly afterward.

**Independent Test**: Press Play, aim at various blocks (including ones next to a chunk seam and the bottom Bedrock layer) and left-click — per spec.md User Story 3 acceptance scenarios and the Edge Cases section.

### Implementation for User Story 3

- [X] T020 [US3] Implement an aim-resolution helper (camera-forward line trace, max reach per research.md, resolving the hit to a target voxel's chunk+local coordinate and face normal) in `Source/MyProject/Private/Player/VoxelCharacter.cpp` (depends on T018, T016)
- [X] T021 [P] [US3] Implement `AVoxelWorld::TryBreakBlock(WorldLocation, Normal)` per `contracts/input-actions.md` pre/post-conditions (bounds check, `IsDestructible` check, set `Air`, mark owning chunk dirty) in `Source/MyProject/Private/Voxel/VoxelWorld.cpp` (depends on T009, T004)
- [X] T022 [US3] Add chunk-boundary dirty propagation — when the edited voxel is on a chunk face, also mark the face-adjacent neighbor chunk dirty and re-queue its async remesh — in `Source/MyProject/Private/Voxel/VoxelWorld.cpp` (depends on T021)
- [X] T023 [US3] Bind `IA_Break` to call `TryBreakBlock` using the aim-resolution helper's hit result, in `Source/MyProject/Private/Player/VoxelCharacter.cpp` (depends on T020, T022)

**Checkpoint**: Breaking blocks works end-to-end, including at chunk seams and against Bedrock (no-op). User Stories 1–3 are independently demoable.

---

## Phase 6: User Story 4 - 在瞄准面上放置方块 (Priority: P2)

**Goal**: Right-click places the currently selected block type on the aimed face; rendering is correct and placement cannot overlap the player.

**Independent Test**: Press Play, cycle block types, and right-click various exposed faces (including one that would overlap the player) — per spec.md User Story 4 acceptance scenarios.

### Implementation for User Story 4

- [X] T024 [P] [US4] Add `SelectedBlockType` state and the ordered placeable-type cycle (`Grass, Dirt, Stone`) to `AVoxelCharacter` in `Source/MyProject/Public/Player/VoxelCharacter.h` / `Source/MyProject/Private/Player/VoxelCharacter.cpp` (depends on T016)
- [X] T025 [P] [US4] Implement `AVoxelWorld::TryPlaceBlock(WorldLocation, Normal, BlockType)` per `contracts/input-actions.md` pre/post-conditions (bounds check, target-cell-empty check, player-overlap check, set type, reuse T022's neighbor-dirtying) in `Source/MyProject/Private/Voxel/VoxelWorld.cpp` (depends on T022, T009)
- [X] T026 [US4] Bind `IA_Place` to call `TryPlaceBlock` with `Player.SelectedBlockType`, using the aim-resolution helper's hit result, in `Source/MyProject/Private/Player/VoxelCharacter.cpp` (depends on T020, T025)
- [X] T027 [US4] Bind `IA_CycleBlockType` to advance `SelectedBlockType` through the placeable cycle, in `Source/MyProject/Private/Player/VoxelCharacter.cpp` (depends on T024)

**Checkpoint**: Placing blocks works end-to-end, including the player-overlap rejection. All four user stories are independently demoable — full MVP loop complete.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Final verification that the whole feature holds together.

- [X] T028 Verify the project compiles cleanly (no new errors/warnings) with all Voxel/Player code added
- [X] T029 Run the full `quickstart.md` validation pass in PIE — all 4 user stories plus the SC-005 stress pass (short dig-and-refill tunnel crossing a chunk seam)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately.
- **Foundational (Phase 2)**: Depends on Setup — BLOCKS all user stories.
- **User Story 1 (Phase 3)**: Depends on Foundational only.
- **User Story 2 (Phase 4)**: Depends on Foundational; does not require US1's generation logic (T012/T013) to be done, only the chunk/collision/material classes from Foundational — but is normally done after US1 since you need a generated world to usefully walk around in.
- **User Story 3 (Phase 5)**: Depends on Foundational + US2's aim-resolution prerequisite (collision from T018) — needs a world to aim at, so practically follows US1+US2.
- **User Story 4 (Phase 6)**: Depends on US3's `TryBreakBlock`/neighbor-dirtying pattern (T022) being in place, which `TryPlaceBlock` (T025) reuses.
- **Polish (Phase 7)**: Depends on all four user stories being complete.

### Parallel Opportunities

- T002 and T003 (Setup) can run in parallel.
- T004, T005, T007, T009, T010, T011 (Foundational) can each run in parallel once their own listed dependency is met (see each task's "(depends on ...)" note) — they touch different files.
- T014 (place world actor) can run in parallel with T012/T013 (generation logic) since it's a content-editor operation, not a C++ change to the same files.
- T018 (chunk collision) can run in parallel with T016/T017 (character class) in Phase 4.
- T021 (`TryBreakBlock`) can run in parallel with T020 (aim-resolution helper) in Phase 5 — different files.
- T024 (`SelectedBlockType`) and T025 (`TryPlaceBlock`) can run in parallel in Phase 6 — different files.

---

## Parallel Example: Foundational Phase

```bash
# Once T004/T005 land, these can proceed together:
Task: "Implement AVoxelChunk in Source/MyProject/Public/Voxel/VoxelChunk.h + Private/Voxel/VoxelChunk.cpp"
Task: "Create M_VoxelChunk material via unreal-mcp MaterialTools"
Task: "Create Enhanced Input assets via unreal-mcp AssetTools"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1 (Setup) and Phase 2 (Foundational).
2. Complete Phase 3 (User Story 1).
3. **STOP and VALIDATE**: Play `VoxelMVP` and confirm the world looks right (hills + layers) per
   its Independent Test — this alone already proves the hand-written chunk/mesh/noise pipeline
   works, which is this project's core learning goal.

### Incremental Delivery

1. Setup + Foundational → foundation ready.
2. Add User Story 1 → validate → this is already a demoable "look at the world" slice.
3. Add User Story 2 → validate → now a walkable world.
4. Add User Story 3 → validate → now you can dig.
5. Add User Story 4 → validate → full break/place loop, matching spec.md's complete MVP scope.

## Notes

- This is a solo project (Constitution Development Workflow): no [P] task here implies "assign
  to a different person" — it only means "no ordering dependency, safe to do in either order (or
  back-to-back) without rework."
- Per the constitution, commit at each checkpoint (end of a phase) rather than after every single
  task — each checkpoint is a meaningful, demoable increment.
