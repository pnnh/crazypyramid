# Phase 1 Data Model: Voxel World MVP

Derived from spec.md's Key Entities section, made concrete with the decisions from
`research.md`. This is a conceptual/data model, not final C++ signatures — field names are
illustrative for `/speckit-tasks` and implementation to refine.

## EVoxelBlockType (enum)

| Value | Visible? | Destructible? | Base color (vertex color) | Notes |
|---|---|---|---|---|
| Air | No | N/A | N/A | Absence of a block; never meshed |
| Grass | Yes | Yes | green | Topmost solid voxel of a column |
| Dirt | Yes | Yes | brown | A few voxels below Grass |
| Stone | Yes | Yes | grey | Everything between Dirt and Bedrock |
| Bedrock | Yes | **No** | dark grey/black | Fixed bottom layer(s) of the world (spec FR-003) |

Each type's (color, destructible) pair is a static lookup, not per-instance data — see
`research.md` "Block types & vertical layering."

## VoxelChunk

Represents one chunk's worth of voxel data plus its rendering/collision representation.

- **ChunkCoordinate**: integer (X, Y) — this chunk's position in the world's chunk grid (not
  world units). Used as the key for `VoxelWorld`'s chunk lookup.
- **Voxels**: flat array of `EVoxelBlockType`, length `SizeX * SizeY * SizeZ` (16×16×64),
  indexed `x + y*SizeX + z*SizeX*SizeY`. Owns the ground truth for this chunk's blocks.
- **MeshComponent**: the chunk's `UProceduralMeshComponent` — one mesh section, vertex-colored,
  single material, collision enabled (complex-as-simple).
- **IsDirty**: whether this chunk's mesh needs to be (re)built from `Voxels` (set on creation and
  whenever a local or neighbor-triggered edit touches it — see "Chunk-boundary edit propagation"
  in research.md).

**Relationships**: A `VoxelChunk` has up to 4 horizontal neighbor chunks (N/S/E/W) in
`VoxelWorld`'s grid; it does not store a direct pointer to them (looked up by coordinate via
`VoxelWorld` when needed, e.g. for boundary meshing/dirtying), to avoid dangling-pointer
bookkeeping as chunks are later created/destroyed by a future streaming feature.

**Validation rules**: `Voxels` index must stay within `[0, SizeX*SizeY*SizeZ)` — any voxel access
(read, break, place) by local coordinate must be bounds-checked, and an out-of-range request is a
no-op (matches spec Edge Case: "aiming outside the world does nothing, no crash").

## VoxelWorld

Owns the whole chunk grid and world-level generation parameters for the MVP.

- **WorldSizeInChunks**: (8, 8) for this MVP (see research.md).
- **Seed**: fixed constant for this MVP, feeds the Perlin sampling offset (deterministic
  terrain, per spec Assumptions).
- **GenerationParams**: BaseHeight, Amplitude, Frequency, DirtLayerThickness, BedrockLayerCount —
  the tunable knobs behind the per-column height/layer rule in research.md.
- **Chunks**: map from `ChunkCoordinate` → `VoxelChunk`. For this MVP, populated once at
  `BeginPlay` for the full `WorldSizeInChunks` grid (no runtime load/unload yet), but keyed the
  same way a future streaming feature would key it (satisfies spec FR-013 / Constitution
  Principle IV without a later rewrite).

**Key operations** (conceptual; see contracts/input-actions.md for how the player triggers
these):

- `TryBreakBlock(WorldLocation, Normal) -> bool`: resolves a world hit to a voxel, checks it is
  destructible and in-bounds, sets it to `Air`, marks the owning chunk (and any neighbor chunk
  sharing that voxel's face) dirty, and schedules their async remesh.
- `TryPlaceBlock(WorldLocation, Normal, BlockType) -> bool`: resolves the adjacent target cell,
  checks it is empty, in-bounds, and does not overlap the player's collision volume, sets it to
  `BlockType`, and marks/remeshes chunks the same way.

## Player (state relevant to this feature)

- **SelectedBlockType**: current `EVoxelBlockType` used by the next `TryPlaceBlock` call;
  advanced by the key-cycle input action (spec FR-012) through the ordered list of placeable
  types (`Grass, Dirt, Stone` — `Air`/`Bedrock` are excluded from the cycle since they are not
  placeable by the player).
- Standard first-person character state (position, rotation, velocity) is provided by the
  engine's character-movement component and is not feature-specific data.

## State transitions

```text
Voxel: Air  --place(T)-->  T (T in {Grass, Dirt, Stone})
Voxel: T    --break()-->   Air            (only if T.Destructible == true)
Voxel: Bedrock --break()--> Bedrock        (no-op; Destructible == false)
Chunk: Clean --edit touches this chunk or a face-adjacent neighbor--> Dirty
Chunk: Dirty --async remesh completes--> Clean (mesh section updated on game thread)
```
