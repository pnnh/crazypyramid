# Implementation Plan: Voxel World MVP (体素世界核心闭环)

**Branch**: `001-voxel-world-mvp` | **Date**: 2026-10-20 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/001-voxel-world-mvp/spec.md`

## Summary

Build the smallest end-to-end voxel sandbox loop inside the existing UE 5.8 `MyProject`: a
finite, chunk-organized world with Perlin-noise terrain and three visible block layers (grass /
dirt / stone, over an indestructible bedrock floor), a standard gravity-bound first-person
character that can walk the world, and left-click/right-click block removal/placement that
updates the affected chunk(s)' visible mesh immediately and without a perceptible hitch.

Technical approach: hand-written C++ `AVoxelChunk` actors (one `UProceduralMeshComponent`
each) hold a flat per-chunk voxel array; a `AVoxelWorld` actor owns the chunk grid, runs Perlin
height generation once at `BeginPlay`, and exposes `TryBreakBlock`/`TryPlaceBlock` used by the
player pawn's Enhanced Input bindings. Mesh (re)building runs as a background Task Graph job (`AsyncTask`)
that returns plain mesh-buffer data consumed back on the game thread, so neither the initial
world build nor a single edit blocks the game thread for a perceptible duration (Constitution
Principle IV). Block color is carried as per-vertex color read by one simple unlit/lit master
material, so no texture/art asset is required for the MVP (Assumptions in spec.md). Editor-side
setup (level, PlayerStart, material asset, input assets, placing the `AVoxelWorld` actor) is
done via the local `unreal-mcp` tools; Blender MCP is **not required** for this feature since no
3D model or texture asset is in scope, but remains available as an optional fallback for a later
feature that does need one (e.g. a custom player mesh or a real texture atlas).

## Technical Context

**Language/Version**: C++ (UnrealBuildTool, Engine 5.8 toolchain) for all core voxel systems; Blueprint only for level-level glue (e.g. wiring the Enhanced Input assets already set up in C++, if more convenient than a native pawn subclass).

**Primary Dependencies**: Unreal Engine 5.8 built-in modules — `Engine` (Actor/Pawn/Character, `FMath::PerlinNoise2D`), `ProceduralMeshComponent` (chunk mesh rendering + collision), `EnhancedInput` (already the project default per `DefaultInput.ini`), `InputCore`. No third-party or Marketplace voxel plugin (Constitution Technical Constraints). `unreal-mcp` (local, port 8000) for editor-side asset/actor operations. Blender MCP available but not needed for this feature's scope.

**Storage**: N/A for this feature — world state (voxel edits) lives only in memory for the current PIE session; no save/load (spec Assumptions explicitly defers persistence).

**Testing**: Manual PIE validation against the scenarios in `quickstart.md` (Constitution Development Workflow: automated test coverage is not required yet for this project phase). No contract/unit test suite is introduced by this feature.

**Target Platform**: Unreal Editor Play-In-Editor (PIE) on Desktop (Windows/Mac — project's `HardwareTargetingSettings` already targets Desktop). Packaging a standalone build is out of scope for this feature.

**Project Type**: Single Unreal project, native C++ game module (`MyProject`) — not a multi-project/web/mobile layout.

**Performance Goals**: No quantitative frame-rate target (spec SC explicitly waives numeric performance bars). Qualitative goal only: initial world build and any single break/place edit must not produce a player-perceptible freeze — enforced architecturally by doing mesh (re)generation off the game thread (see Research: Async remesh strategy).

**Constraints**: Mesh rebuild work MUST run off the game thread (Constitution Principle IV); chunk data structure MUST remain addressable by chunk coordinate so a later feature can add distance-based load/unload without changing the chunk/mesh contracts (Constitution Principle IV, spec FR-013); bottom-most voxel layer MUST be non-destructible (spec FR-003).

**Scale/Scope**: One finite world, 8×8 chunks of 16×16×64 voxels each (128×128 voxels horizontally, 64 vertical) at 100 Unreal units per voxel (1 m blocks — matches default UE character scale). See `research.md` for why these numbers were chosen.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Check | Result |
|---|---|---|
| I. Learning-First, Pragmatic Iteration | Core chunk/mesh/terrain-gen logic is hand-written C++, no third-party voxel plugin; includes basic bounds/edge-case handling (out-of-world raycasts, chunk-boundary edits) rather than happy-path only | PASS |
| II. Playable Vertical Slices | Plan delivers the 4 user stories in priority order (P1 observe-world → P1 walk → P2 break → P2 place), each independently demoable in PIE per spec's Independent Test notes | PASS |
| III. Single-Player Scope Discipline (YAGNI) | No multiplayer, inventory UI, crafting, mobs, or day/night included or pre-abstracted; block-type cycling is a plain enum cycle, not a generalized item system | PASS |
| IV. Performance-Aware Voxel Architecture | Chunked data (`AVoxelChunk` per coordinate); remeshing via background Task Graph job (`AsyncTask`) + game-thread apply; `AVoxelWorld` chunk map is keyed by chunk coordinate so a future distance-based streaming feature only adds load/unload calls, not a data rewrite | PASS |
| V. Tool-Assisted Workflow | Editor/asset setup via `unreal-mcp`; core logic under `Source/MyProject`; Speckit flow (constitution → specify → plan, this document) followed | PASS |

No violations — Complexity Tracking table is omitted (nothing to justify).

## Project Structure

### Documentation (this feature)

```text
specs/001-voxel-world-mvp/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md        # Phase 1 output (/speckit-plan command)
├── quickstart.md        # Phase 1 output (/speckit-plan command)
├── contracts/           # Phase 1 output (/speckit-plan command)
│   └── input-actions.md
└── tasks.md             # Phase 2 output (/speckit-tasks command — NOT created by /speckit-plan)
```

### Source Code (repository root)

This feature extends the existing single Unreal project in place — there is no multi-project
split. New C++ files are added under the existing `Source/MyProject` module; new editor assets
(material, input actions, level edits) are created under `Content/` via `unreal-mcp`.

```text
Source/MyProject/
├── Public/
│   ├── Voxel/
│   │   ├── VoxelTypes.h          # EVoxelBlockType enum + per-type rules (color, destructible)
│   │   ├── VoxelChunk.h          # AVoxelChunk: per-chunk voxel array + UProceduralMeshComponent
│   │   ├── VoxelWorld.h          # AVoxelWorld: chunk grid, generation, break/place API
│   │   └── VoxelMeshBuilder.h    # Pure function(s): voxel data -> mesh buffers (thread-safe)
│   └── Player/
│       └── VoxelCharacter.h      # APawn/ACharacter subclass: movement + interact bindings
├── Private/
│   ├── Voxel/
│   │   ├── VoxelChunk.cpp
│   │   ├── VoxelWorld.cpp
│   │   └── VoxelMeshBuilder.cpp
│   └── Player/
│       └── VoxelCharacter.cpp
└── MyProject.Build.cs            # add "ProceduralMeshComponent", "EnhancedInput" dependencies

Content/
├── VoxelSandbox/
│   ├── Materials/M_VoxelChunk (vertex-color-driven base material)
│   ├── Input/ (IA_Move, IA_Look, IA_Jump, IA_Break, IA_Place, IA_CycleBlockType, IMC_VoxelDefault)
│   └── Maps/VoxelMVP (new level used for this feature, built via unreal-mcp SceneTools)
```

**Structure Decision**: Single project, extending the existing native game module in place
(no new project/package boundary). Voxel engine logic and the player pawn are split into two
public sub-namespaces (`Voxel/`, `Player/`) under the existing `Source/MyProject` module purely
for file organization — both still compile into the one `MyProject` module. All new editor-only
assets (material, input, level) live under a dedicated `Content/VoxelSandbox/` folder so this
feature's content is easy to find and does not get mixed with the leftover `Content/Demo`,
`Content/glb`, `OpenWorld`/`NewMap` assets inherited from the project's fork origin.

## Complexity Tracking

*No Constitution Check violations — table intentionally omitted.*
