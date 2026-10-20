# Phase 1 Data Model: First-Person Visuals

Extends 001's data model (`specs/001-voxel-world-mvp/data-model.md`) — this feature adds new
fields/lookups, it does not replace anything there. Conceptual/data model, not final C++
signatures.

## EVoxelBlockType → Atlas Cell (extends the existing block-type table)

| Value | Atlas cell(s) | Notes |
|---|---|---|
| Air | — | Never meshed, no cell needed (unchanged from 001) |
| Grass | Top: "GrassTop" cell; all other 5 face directions: "GrassSide" cell | Only block type that differentiates by face direction |
| Dirt | "Dirt" cell, all 6 directions | Uniform |
| Stone | "Stone" cell, all 6 directions | Uniform |
| Bedrock | "Bedrock" cell, all 6 directions | Uniform — new in this feature (001 had no bedrock-specific visual) |

`EVoxelFaceDirection` (new, conceptual): `{ Top, Side }` — collapses the mesh builder's existing
6 physical face directions down to just these 2 for the purpose of atlas lookup, since nothing
in this feature needs to distinguish Bottom from the 4 horizontal sides, or needs more than
Grass to distinguish Top at all.

## BlockAtlasCell (new)

Represents one cell's position within `T_BlockAtlas`.

- **CellIndex**: integer, 0-based, left-to-right position in the atlas strip.
- **UVMin / UVMax**: derived, not stored — computed from `CellIndex` and the atlas's fixed total
  cell count (`CellIndex / TotalCells` to `(CellIndex+1) / TotalCells` along U, full `[0,1]`
  along V for a horizontal-strip layout). A single `TotalCells` constant is the only thing that
  needs to change if more cells are added later.

## AVoxelChunk (extends 001's entity — no new fields, behavior change only)

- Mesh data (`FVoxelMeshData.UVs`) now carries real per-corner UV coordinates computed from the
  block type + face direction of each emitted quad, instead of a constant `(0,0)`. Vertex colors
  are still emitted alongside (harmless; the updated material simply doesn't read them anymore).
- No new fields on the chunk itself — this is purely a change to what `BuildChunkMeshData`
  writes into the same mesh-data shape 001 already defined.

## First-Person Arm (new)

Represents the player-visible arm attached under the camera.

- **ArmMeshComponent**: a `UStaticMeshComponent` on `AVoxelCharacter`, using `SM_FirstPersonArm`
  and `M_FirstPersonArm`, attached to `FirstPersonCamera` with a fixed relative transform
  (positioned lower-right of view, bare-handed).
- **SwayPhase**: a running time accumulator (or just `GetWorld()->GetTimeSeconds()` sampled
  directly) feeding a sine function that produces a small relative-location/rotation offset each
  tick — purely cosmetic, not gameplay state, and not something other systems ever need to read.
- **Collision**: `NoCollision` — the arm never needs to physically interact with anything
  (belt-and-braces alongside the aim trace already ignoring the owning actor; see research.md).

## State transitions

```text
Face meshed by BuildChunkMeshData: (BlockType, FaceDirection) --lookup--> CellIndex --derive--> UVMin/UVMax
Arm component: Tick(DeltaTime) --sine(WorldTime)--> RelativeLocation/RelativeRotation offset (purely visual, no state machine)
```
