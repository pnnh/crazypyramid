# Contract: Block Type/Face → Atlas Cell Mapping

This feature has no network/service API. Its external "contract" — the stable interface the
mesh builder and the material are both built against — is this lookup table. Implementation
(exact cell pixel coordinates inside `T_BlockAtlas`, exact `EVoxelFaceDirection` enum spelling)
is an implementation detail, but the *mapping itself* must hold regardless of how it's coded,
since both the C++ mesh builder and the imported texture asset have to agree on it.

| Block Type | Face Direction | Atlas Cell | Notes |
|---|---|---|---|
| Grass | Top | GrassTop | Only this (type, direction) pair uses this cell |
| Grass | Side (the other 5 physical directions) | GrassSide | Includes Bottom — a grass block's underside uses the side texture, not a separate "grass bottom"; acceptable since Grass columns always have Dirt directly beneath in 001's generation, so a bare grass bottom face is never actually visible in normal play |
| Dirt | Top or Side (any direction) | Dirt | Uniform on all 6 faces |
| Stone | Top or Side (any direction) | Stone | Uniform on all 6 faces |
| Bedrock | Top or Side (any direction) | Bedrock | Uniform on all 6 faces |
| Air | N/A | N/A | Never meshed (unchanged from 001) |

## Pre/post-conditions

**`VoxelBlockRules::GetAtlasCellIndex(BlockType, FaceDirection)`**

- Pre: `BlockType` is a solid type (`IsSolid(BlockType) == true`); `Air` must never be passed in
  (callers only invoke this for faces already known to be solid, matching 001's existing
  `IsSolid` guard in the meshing loop).
- Post: returns a valid `CellIndex` in `[0, TotalCells)` for every `(BlockType, FaceDirection)`
  combination in the table above. The function is total over all non-Air block types — there is
  no "no cell found" failure case a caller needs to handle.

## Visual contract: what must still hold after this feature

These restate (don't replace) the relevant guarantees from 001's `contracts/input-actions.md`,
confirming this feature doesn't weaken them:

- The aim trace used by `TryBreakBlock`/`TryPlaceBlock` must never resolve to the first-person
  arm mesh as "the block being aimed at" — guaranteed structurally (see research.md), not by any
  new runtime check this feature adds.
- Breaking/placing a block must still result in a mesh with the *correct* atlas UVs for the
  new/remaining block's type — i.e. `RequestRemesh`'s rebuild path uses the exact same
  `GetAtlasCellIndex` lookup as the initial world-generation meshing path. There is only one
  meshing code path (`BuildChunkMeshData`); this is automatically true rather than a second
  thing to keep in sync.
