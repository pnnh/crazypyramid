# Quickstart: Validating First-Person Visuals

Manual PIE validation guide — this project phase does not use automated tests (Constitution
Development Workflow). Run through this after `/speckit-implement` to confirm the feature
matches spec.md's acceptance scenarios, and that nothing from 001 regressed.

## Prerequisites

- Unreal Editor open on this project, `unreal-mcp` connected on port 8000.
- `Content/VoxelSandbox/Maps/VoxelMVP` open as the active level.
- Project compiled successfully with the updated Voxel/Player code.
- `T_BlockAtlas`, `SM_FirstPersonArm`, `M_FirstPersonArm`, and the updated `M_VoxelChunk` all
  present under `Content/VoxelSandbox/`.

## Run

1. Open `Content/VoxelSandbox/Maps/VoxelMVP`, press **Play**.

## Expected outcomes, mapped to spec.md acceptance scenarios

**User Story 1 — block textures** (spec User Story 1 / SC-001, SC-002)

- Looking at any grass block's top: a grass-pattern texture, not a flat color.
- Looking at the same block's side: a visibly different texture from the top, recognizable as
  "the side of a grass block" (uniform grass-colored side per the resolved clarification — no
  top-to-bottom gradient expected).
- Digging into a hillside to expose dirt and stone: each shows its own distinct texture,
  clearly different from grass and from each other.
- Break a block, then place a new one in the resulting gap (reusing 001's controls): the new
  block shows the correct texture for whatever type is currently selected — never a flat color,
  never the wrong block's texture, never a visibly broken/missing-texture look.

**User Story 2 — first-person arm** (spec User Story 2 / SC-003, SC-005)

- Immediately on spawn: a bare-handed arm is visible anchored in the view (not empty space).
- Turning/moving the camera: the arm follows naturally, with a small idle sway — no clipping
  through the camera, no jitter, no lagging/floating disconnect from the view.
- With the arm visible, repeat several break/place actions from 001's quickstart: aim accuracy
  is unaffected — the arm is never mistaken for the block being aimed at, and break/place still
  hits exactly the block the crosshair/camera center is pointing at.

**Regression check against 001** (spec FR-008)

- Movement (WASD), look (mouse), jump (Space), block-type cycling (per 001's key) all still work
  exactly as before — this feature must not have touched any of that logic.
- Re-run 001's own stress pass (≥20 mixed break/place operations, including across a chunk
  seam) and confirm it still holds with textures: no missing faces, no z-fighting, and now also
  no texture glitches (wrong cell, stretched/misaligned UVs) anywhere in that sequence.

## If something looks wrong

- **A block face shows the wrong texture, or a stretched/garbled texture**: check
  `GetAtlasCellIndex`'s table against `contracts/block-atlas-mapping.md`, and check the UV
  math in `BuildChunkMeshData` converts `CellIndex` to `[UVMin, UVMax]` using the same
  `TotalCells` the atlas was actually laid out with.
- **Visible seams/bleeding between texture cells at a distance**: confirm `T_BlockAtlas` was
  imported with mipmaps disabled and Nearest/Point filtering (research.md).
- **The arm mesh gets hit by break/place aiming**: confirm the arm component is attached under
  `AVoxelCharacter` (not some other actor) so it's covered by the existing
  `AddIgnoredActor(this)` call, and that its collision is set to `NoCollision`.
- **The arm looks static with zero motion**: confirm `AVoxelCharacter::Tick` is actually enabled
  (`PrimaryActorTick.bCanEverTick`) and the sway offset is being applied to the arm component's
  *relative* transform (not accidentally to the whole character or camera).
