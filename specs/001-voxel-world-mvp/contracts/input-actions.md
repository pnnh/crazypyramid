# Contract: Player Input Actions

This feature has no network/service API. Its external "contract" — the stable interface the
rest of the system (and the player) is built against — is the set of Enhanced Input Actions the
player pawn binds to, and what each one calls into the `VoxelWorld`/chunk systems. Implementation
(exact `.uasset` names, Input Mapping Context) is created via `unreal-mcp`, but the semantics
below must hold regardless of how the assets are named.

| Input Action | Trigger | Default binding | Calls | Notes |
|---|---|---|---|---|
| `IA_Move` | Axis2D (held) | WASD | `CharacterMovementComponent` add-movement (engine-standard) | Blocked by solid-voxel collision (chunk mesh collision) |
| `IA_Look` | Axis2D (held) | Mouse X/Y | Standard camera/controller yaw+pitch | No gameplay-specific logic |
| `IA_Jump` | Digital (pressed) | Space | `ACharacter::Jump()` (engine-standard) | Only relevant because movement uses a gravity/collision character per spec FR-014 |
| `IA_Break` | Digital (pressed) | Left mouse button | Line trace from camera → `VoxelWorld::TryBreakBlock(HitLocation, HitNormal)` | No-op if trace misses or hits a non-destructible (Bedrock) voxel — spec FR-010/Edge Cases |
| `IA_Place` | Digital (pressed) | Right mouse button | Line trace from camera → `VoxelWorld::TryPlaceBlock(HitLocation, HitNormal, Player.SelectedBlockType)` | No-op if trace misses, target cell is occupied, or placement would overlap player collision — spec FR-009/FR-010 |
| `IA_CycleBlockType` | Digital (pressed) | `Q` (or mouse wheel — implementer's choice, not spec-constrained) | Advances `Player.SelectedBlockType` to the next entry in the placeable-type cycle | Pure client-side state change, no world mutation (spec FR-012) |

## Pre/post-conditions for the two mutating actions

**`TryBreakBlock`**

- Pre: a line trace from the camera along its forward vector, within max reach, hits a chunk's
  collision mesh.
- Post (success): the hit voxel becomes `Air`; its chunk and any face-adjacent neighbor chunk are
  marked dirty and re-meshed asynchronously; function returns `true`.
- Post (no-op): no trace hit, hit voxel already `Air` (shouldn't happen given mesh-visible faces
  imply solid voxels, but still defensively checked), or hit voxel is non-destructible (Bedrock)
  → world state unchanged; function returns `false`. No error/crash in any case (spec Edge
  Cases).

**`TryPlaceBlock`**

- Pre: a line trace hits a chunk's collision mesh; the target cell (one voxel outward along the
  hit normal) is in-bounds and currently `Air`; placing there would not overlap the player's
  collision volume.
- Post (success): the target voxel becomes `Player.SelectedBlockType`; its chunk and any
  face-adjacent neighbor chunk are marked dirty and re-meshed asynchronously; function returns
  `true`.
- Post (no-op): any pre-condition fails → world state unchanged; function returns `false`. No
  error/crash in any case.
