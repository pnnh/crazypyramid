---

description: "Task list for First-Person Visuals implementation"
---

# Tasks: First-Person Visuals (第一人称手臂 + 方块贴图集)

**Input**: Design documents from `/specs/002-first-person-visuals/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/block-atlas-mapping.md, quickstart.md

**Tests**: Not included. Per the project constitution's Development Workflow section, this
project phase relies on manual PIE validation (see `quickstart.md`) rather than an automated
test suite — no test tasks are generated below.

**Organization**: Tasks are grouped by user story (per spec.md priorities). The two stories are
architecturally independent (texture atlas vs. first-person arm touch different files/assets),
so unlike 001 there is no shared blocking "Foundational" work — see Dependencies below.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency on an incomplete task)
- **[Story]**: Which user story this task belongs to (US1, US2, per spec.md)
- All file paths are relative to the repository root (`/Users/Larry/Projects/unreal/crazypyramid`)

## Path Conventions

Extends the existing single Unreal project in place (see plan.md "Project Structure"):

- C++: `Source/MyProject/Public/Voxel/`, `Source/MyProject/Private/Voxel/`,
  `Source/MyProject/Public/Player/`, `Source/MyProject/Private/Player/`
- Content (created via `unreal-mcp`/Pixellab MCP/Blender MCP): `Content/VoxelSandbox/`

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Prepare the content folders this feature's new assets land in.

- [X] T001 Create directories `Content/VoxelSandbox/Textures/` and `Content/VoxelSandbox/Meshes/` via `unreal-mcp` (`editor_toolset.toolsets.asset.AssetTools.create_folder`)

**Checkpoint**: Folders exist; both user stories below can start immediately and in parallel with each other.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: N/A for this feature — User Story 1 (texture atlas) and User Story 2 (first-person
arm) touch disjoint files and assets with no shared blocking work. Each story's own tasks below
are self-contained once Setup is done.

---

## Phase 3: User Story 1 - 看到更真实的方块材质 (Priority: P1) 🎯 MVP

**Goal**: Replace flat vertex-color blocks with a 5-cell pixel-art texture atlas (Grass Top,
Grass Side, Dirt, Stone, Bedrock), sampled via real per-face UVs.

**Independent Test**: Play `VoxelMVP` without the arm feature done; observe that grass
top/side, dirt, and stone all show distinct textures (not flat colors), and that breaking/
placing blocks keeps showing correct textures — per spec.md User Story 1 acceptance scenarios.

### Implementation for User Story 1

- [X] T002 [P] [US1] Generate 5 pixel-art textures (GrassTop, GrassSide, Dirt, Stone, Bedrock) via Pixellab MCP, composite them left-to-right into one atlas PNG (e.g. via a short local script), save to the scratchpad directory
- [X] T003 [US1] Import the composited atlas PNG via `unreal-mcp` `TextureTools`/`AssetTools` as `Content/VoxelSandbox/Textures/T_BlockAtlas`, with `MipGenSettings=NoMipmaps`, Nearest/Point filtering, and a 2D/UI-style (non-photographic) compression setting (depends on T002)
- [X] T004 [P] [US1] Add `EVoxelFaceDirection {Top, Side}` and `VoxelBlockRules::GetAtlasCellIndex(EVoxelBlockType, EVoxelFaceDirection)` + a `TotalCells` constant to `Source/MyProject/Public/Voxel/VoxelTypes.h` / `Source/MyProject/Private/Voxel/VoxelTypes.cpp`, matching `contracts/block-atlas-mapping.md`'s table exactly
- [X] T005 [US1] In `Source/MyProject/Private/Voxel/VoxelMeshBuilder.cpp`, replace the hardcoded `(0,0)` per-corner UV with a real UV computed from `GetAtlasCellIndex` + `TotalCells` for each face's block type and direction (depends on T004)
- [X] T006 [US1] Update the `M_VoxelChunk` material via `unreal-mcp` `MaterialTools`: add a Texture Sample node reading `T_BlockAtlas` via the mesh's UV channel, and wire it (instead of the existing VertexColor node) into Base Color (depends on T003)
- [X] T007 [US1] Verify in PIE: grass top/side, dirt, stone, and bedrock all show correct, distinct textures; break/place a few blocks and confirm the new/remaining blocks keep correct textures (depends on T005, T006)

**Checkpoint**: Pressing Play shows a texture-atlas-rendered world. User Story 1 is independently demoable.

---

## Phase 4: User Story 2 - 看到自己的第一人称手臂 (Priority: P2)

**Goal**: A bare-handed, static-mesh first-person arm is visible under the camera, with a
simple code-driven idle sway, and never interferes with the break/place aim trace.

**Independent Test**: Play `VoxelMVP` without the texture-atlas feature done; confirm the arm
is visible, follows the camera naturally with a slight sway, and break/place still aims
correctly — per spec.md User Story 2 acceptance scenarios.

### Implementation for User Story 2

- [X] T008 [P] [US2] Generate a simple bare-handed first-person forearm/hand static mesh (no skeleton) via Blender MCP, export as FBX to the scratchpad directory
- [X] T009 [US2] Import the FBX via `unreal-mcp` `AssetTools` as `Content/VoxelSandbox/Meshes/SM_FirstPersonArm` (static mesh, not skeletal) (depends on T008)
- [X] T010 [P] [US2] Create a simple lit skin-tone material `Content/VoxelSandbox/Materials/M_FirstPersonArm` via `unreal-mcp` `MaterialTools`
- [X] T011 [US2] In `Source/MyProject/Public/Player/VoxelCharacter.h` / `Private/Player/VoxelCharacter.cpp`, add an `ArmMeshComponent` (`UStaticMeshComponent`) member; in the constructor, attach it under `FirstPersonCamera` with a fixed relative transform (lower-right of view), assign `SM_FirstPersonArm` + `M_FirstPersonArm`, and set collision to `NoCollision` (depends on T009, T010)
- [X] T012 [US2] Re-add a `Tick` override on `AVoxelCharacter` that applies a small sine-wave-driven relative location/rotation offset to `ArmMeshComponent` each frame (idle sway) (depends on T011)
- [X] T013 [US2] Verify in PIE: arm is visible on spawn, follows camera look/movement naturally with a slight idle sway, and break/place aiming accuracy is unaffected by its presence (depends on T012)

**Checkpoint**: Player sees their own arm, with correct aim behavior preserved. User Story 2 is independently demoable. Combined with Phase 3, the full feature is complete.

---

## Phase 5: Polish & Cross-Cutting Concerns

**Purpose**: Final verification that both stories hold together and 001 hasn't regressed.

- [X] T014 Verify the project compiles cleanly (no new errors/warnings) with all Voxel/Player changes
- [X] T015 Run the full `quickstart.md` validation pass in PIE — both user stories, plus the 001-regression check (movement/look/jump/break/place still work, and 001's ≥20-operation stress pass still holds with textures)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately.
- **Foundational (Phase 2)**: Empty for this feature — nothing blocks either story beyond Setup.
- **User Story 1 (Phase 3)** and **User Story 2 (Phase 4)**: Both depend only on Setup; they
  touch disjoint files/assets and can be worked in either order, or fully in parallel.
- **Polish (Phase 5)**: Depends on both user stories being complete.

### Parallel Opportunities

- T002 (Pixellab textures) and T004 (C++ enum/lookup table) can run in parallel — different
  files/systems, no dependency on each other.
- T008 (Blender arm mesh) and T010 (arm material) can run in parallel with each other, and with
  all of Phase 3 (US1), since US1 and US2 are fully independent.

---

## Parallel Example: Cross-Story

```bash
# Once Setup (T001) is done, these can all proceed together:
Task: "Generate 5 Pixellab textures and composite the atlas PNG"
Task: "Add EVoxelFaceDirection + GetAtlasCellIndex to VoxelTypes"
Task: "Generate the first-person arm mesh via Blender MCP"
Task: "Create the M_FirstPersonArm material"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1 (Setup).
2. Complete Phase 3 (User Story 1 - texture atlas).
3. **STOP and VALIDATE**: Play and confirm blocks look textured, not flat-colored — this alone
   already resolves the single biggest "doesn't look real" visual complaint.

### Incremental Delivery

1. Setup → foundation ready (trivially, since there's no shared Foundational work this time).
2. Add User Story 1 → validate → textured world (biggest visual win, ship this first).
3. Add User Story 2 → validate → player can see their own arm → full feature complete.

## Notes

- This is a solo project (Constitution Development Workflow): `[P]` here means "no ordering
  dependency," not "assign to a different person."
- Commit at each checkpoint (end of a phase), per the constitution's guidance to commit at
  meaningful, demoable increments rather than after every single task.
