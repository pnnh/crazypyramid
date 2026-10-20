# Quickstart: Validating the Voxel World MVP

Manual PIE validation guide — this project phase does not use automated tests (Constitution
Development Workflow). Run through this after `/speckit-implement` to confirm the feature
matches spec.md's acceptance scenarios.

## Prerequisites

- Unreal Editor open on this project (`MyProject.uproject`), `unreal-mcp` connected on port
  8000 (used during implementation to create the level/material/input assets — not required to
  just play the result).
- The `VoxelMVP` level (see plan.md Project Structure) open as the active level.
- Project compiled successfully with the new `Source/MyProject/Voxel` and `Player` code.

## Run

1. In the Unreal Editor, open `Content/VoxelSandbox/Maps/VoxelMVP`.
2. Click **Play** (Play-In-Editor, standard viewport or new window — either is fine for manual
   validation).

## Expected outcomes, mapped to spec.md acceptance scenarios

**User Story 1 — observe the world** (spec User Story 1 / SC-001)

- On Play, you immediately see a world with visible hills/depressions (not a flat plane) and, by
  looking at any exposed vertical cross-section (e.g. a hillside), at least Grass/Dirt/Stone
  visibly stacked by depth, with a distinct Bedrock layer visible if you look at the world's
  lowest exposed layer.
- Moving the mouse turns the camera smoothly, no stutter.

**User Story 2 — walk the world** (spec User Story 2 / SC-002)

- WASD moves the character; the character cannot walk through hills (collision blocks it) and
  does not fall through the ground.
- Walking off a small ledge/hill causes the character to fall/step down naturally (standard
  character-movement gravity), not clip through terrain.

**User Story 3 — break a block** (spec User Story 3 / SC-003)

- Aim at any visible block, left-click: it disappears immediately, and the newly exposed face(s)
  of neighboring blocks render correctly (no hole into "the inside of the world", no missing
  face, no flickering double-face).
- Aim at the bottom-most (Bedrock) layer, left-click: nothing happens.
- Aim at the sky / outside the world bounds, left-click: nothing happens, no error in the Output
  Log.

**User Story 4 — place a block** (spec User Story 4 / SC-004)

- Aim at any exposed block face, right-click: a new block of the currently selected type appears
  just outside that face, rendering correctly against its new neighbors.
- Press the cycle-block-type input (see contracts/input-actions.md) a few times and place again:
  the newly placed block's color changes accordingly, confirming the cycle actually changes
  `Player.SelectedBlockType`.
- Try to place a block at a position that would overlap the player's own capsule (e.g. aim
  almost straight down at your feet): placement is rejected (spec FR-009) — no block appears,
  player is not pushed/stuck.

**Stress pass** (spec SC-005)

- Dig a short 1×1 tunnel of ~10 blocks, then place ~10 blocks back. Confirm no perceptible
  stutter during any individual break/place, and no corrupted-looking geometry (misaligned
  faces, chunk seams) afterward — including at any point where the tunnel crosses a chunk
  boundary (visible as an invisible seam every 16 blocks given the chosen chunk size).

## If something looks wrong

- **Holes/missing faces at a chunk seam specifically**: check the chunk-boundary edit
  propagation logic in research.md — the neighbor chunk likely wasn't marked dirty/remeshed.
- **Visible stutter on break/place**: confirm the remesh is actually going through the
  `AsyncTask` (Task Graph) async path (research.md "Async remesh strategy") and not a synchronous
  fallback left in during implementation.
- **Can place a block inside yourself**: the player-overlap check in `TryPlaceBlock`
  (data-model.md) is missing or using the wrong collision query.
