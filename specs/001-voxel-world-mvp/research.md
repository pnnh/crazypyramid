# Phase 0 Research: Voxel World MVP

No `[NEEDS CLARIFICATION]` markers remained in the Technical Context after drafting it (the two
spec-level ambiguities were already resolved with the user during `/speckit-specify`: key-cycle
block selection, gravity/collision character movement). This document instead records the
concrete engineering decisions made while filling in the Technical Context, since each one had
more than one reasonable option.

## Chunk & world dimensions

- **Decision**: Chunk = 16×16×64 voxels. World = 8×8 chunks (128×128×64 voxels total). Voxel
  size = 100 Unreal units (1 m).
- **Rationale**: 16×16 is the conventional horizontal chunk footprint in voxel-game tutorials
  and keeps per-edit remesh work small (max 16×16×64 = 16,384 voxels to re-scan). 64 vertical
  layers is enough room for noise-driven hills plus 3 visible layers + 1 bedrock layer without
  wasting memory on empty headroom. 100 uu/voxel (1 m) matches the default UE character capsule
  scale (~88 uu tall crouch-to-stand range fits within 1–2 blocks), so the stock first-person
  pawn needs no rescaling. 8×8 chunks gives a world a few hundred meters across — walkable in a
  few minutes on foot, matching the spec's "explorable in a few minutes" assumption — while
  keeping one-shot startup generation (64 chunks) cheap.
- **Alternatives considered**: 32×32×32 chunks (more common in some engines) — rejected because
  it mixes horizontal and vertical granularity awkwardly for a terrain-with-depth-layers design.
  Truly unbounded/streamed world — explicitly out of scope per spec (deferred feature).

## Terrain generation

- **Decision**: Per-column height = `BaseHeight + FMath::PerlinNoise2D(FVector2D(worldX, worldY) * Frequency) * Amplitude`, using the engine's built-in Perlin noise (no plugin). A fixed constant `Seed` offsets the sample coordinates so results are deterministic run-to-run (spec Assumption: reproducible terrain).
- **Rationale**: `FMath::PerlinNoise2D` ships in Unreal's core `Math` library — zero extra
  dependency, satisfies Constitution's "no third-party voxel plugin" constraint, and is more
  than sufficient fidelity for "visible hills, not flat ground."
- **Alternatives considered**: FastNoise/FastNoise2 plugin — more noise types (Simplex, ridged,
  etc.) but an unnecessary dependency for a single height-noise MVP; can be revisited later if a
  richer biome feature needs it.

## Block types & vertical layering

- **Decision**: `EVoxelBlockType { Air, Grass, Dirt, Stone, Bedrock }`. Per-column rule: topmost
  solid voxel = Grass; next ~3 voxels down = Dirt; everything below down to z = 1 = Stone; z = 0
  (world floor) = Bedrock, hardcoded non-destructible. Each type's base color and
  "IsDestructible" flag live in one small `constexpr`/`switch`-based lookup in `VoxelTypes.h/.cpp`.
- **Rationale**: Matches the spec's "at least 3 visually distinct layers + indestructible floor"
  requirement (FR-002, FR-003) with the least code. A `switch`/lookup table (rather than a
  DataTable asset) keeps the logic hand-written and inspectable per Constitution Principle I; a
  DataTable is an easy, low-risk upgrade later if the block-type count grows enough to justify
  data-driven authoring — not needed for 5 types.
- **Alternatives considered**: DataTable-driven block type registry — deferred (YAGNI, Principle
  III); only worth it once a feature actually needs non-programmer-editable block data.

## Mesh generation algorithm

- **Decision**: Naive per-face culling meshing — for every solid voxel, emit a quad for each of
  its 6 faces whose neighbor (in-chunk or in the adjacent chunk) is Air or out-of-world. One
  `UProceduralMeshComponent` mesh section per chunk, block color encoded as per-vertex color,
  read by a single simple master material (no texture, no per-block-type material/mesh-section).
- **Rationale**: Simplest correct implementation that already satisfies the spec's "no holes, no
  missing faces, no z-fighting" requirement — overlapping/internal faces between two solid
  voxels are never emitted, which is what actually prevents z-fighting. Vertex-color shading
  avoids needing N materials or a texture atlas, matching the spec's explicit "no art assets
  needed" assumption, and keeps each chunk a single draw call.
- **Alternatives considered**: Greedy meshing (merging coplanar faces into larger quads) —
  meaningfully reduces triangle count but adds real algorithmic complexity; explicitly deferred
  as a *future* performance feature once the naive version is measured and found insufficient
  (Constitution Technical Constraints: no premature optimization without measured need).

## Async remesh strategy

- **Decision**: A pure function `BuildChunkMeshData(const FVoxelChunkData& Voxels, ...) ->
  FVoxelMeshData` (plain vertex/index/color/normal arrays, no `UObject` access) is launched via
  `AsyncTask` (Task Graph) from the game thread. On completion, a game-thread continuation calls
  `UProceduralMeshComponent::CreateMeshSection`/`UpdateMeshSection` with the returned buffers.
  Both the one-time initial world build (64 chunks) and every single-chunk edit rebuild go
  through this same path.
- **Rationale**: Satisfies Constitution Principle IV ("remesh MUST NOT block the game thread for
  a noticeable duration") by construction rather than by assuming the data is "small enough" —
  the triangulation work (the only non-trivial cost) never runs on the game thread, for either
  the big one-time build or a single edit. Because it's the same function used in the same way
  for both the one-time initial build and per-edit incremental rebuilds, there is no separate
  "sync path" to keep consistent with the async one.
- **Alternatives considered**: Synchronous rebuild on the game thread, justified by "the chunk is
  small so it won't be noticeable" — rejected because it would satisfy the spec's qualitative
  bar today but violate the constitution's architectural requirement, and because measuring
  "noticeable" subjectively per machine is not a stable foundation to build later features on.

## Break/place targeting

- **Decision**: Use a standard `LineTraceSingleByChannel` (visibility channel) from the player
  camera along its forward vector, out to a fixed max reach (500 uu = 5 m), against chunk
  `UProceduralMeshComponent` collision (complex-as-simple mesh collision, enabled on the
  generated mesh section). The hit `ImpactPoint` nudged slightly *against* `ImpactNormal`
  resolves to the targeted (breakable) voxel; nudged slightly *along* `ImpactNormal` resolves to
  the placement voxel coordinate.
- **Rationale**: Reuses Unreal's existing physics/collision system instead of hand-rolling a
  voxel DDA/ray-march, which is simpler, already handles the "aiming at nothing" edge case
  (trace just returns no hit), and is precise enough at 1 m voxel scale for this MVP.
- **Alternatives considered**: Custom voxel ray-marching (stepping cell-by-cell along the ray) —
  more "from scratch" but unnecessary complexity when the engine's own collision query already
  gives an exact hit point/normal; may be revisited only if a future feature needs sub-voxel or
  non-cube-shaped targeting.

## Chunk-boundary edit propagation

- **Decision**: When an edited voxel lies on a chunk face (local coordinate 0 or `Size-1` on any
  axis), the neighboring chunk(s) sharing that face are also marked dirty and re-queued through
  the same async remesh path, in addition to the edited chunk itself.
- **Rationale**: Required for spec Edge Case "挖除/放置在区块交界处" — a chunk's visible faces at
  its border depend on its neighbor's voxel data, so only rebuilding the edited chunk would leave
  the neighbor's border face stale (a hole or a z-fighting double face).
- **Alternatives considered**: Always keep a 1-voxel "ghost border" duplicated from the neighbor
  inside each chunk's data to avoid cross-chunk lookups during meshing — a valid optimization,
  but adds bookkeeping (keeping ghost borders in sync) not justified until profiling shows direct
  neighbor lookups during meshing are a bottleneck.

## Editor/content tooling split

- **Decision**: All new editor-only content (level, material, Enhanced Input assets, actor
  placement) is created/edited through the local `unreal-mcp` tools (`AssetTools`,
  `MaterialTools`, `SceneTools`, etc.). Blender MCP is available but **not used** for this
  feature, since the MVP's visuals are vertex-color-only (spec Assumptions: no texture/art asset
  required).
- **Rationale**: Matches Constitution Principle V (MCP automates the editor; hand-written C++
  owns engine logic) and the spec's explicit art-asset waiver. Recording this explicitly avoids
  a future implementer reaching for Blender MCP "just because it's available" when nothing in
  this feature needs a modeled mesh or baked texture.
- **Note for later features**: if a future feature wants a modeled player character, hand tool,
  or a real texture atlas, Blender MCP generating the asset and `unreal-mcp`'s `AssetTools`
  importing it is the expected path — no architecture change needed to adopt it then.

## Lessons learned during implementation (post-hoc)

Two non-obvious UE 5.8 gotchas cost most of the debugging time after `/speckit-implement` and
are worth recording so a future feature doesn't re-discover them the hard way.

- **`UInputMappingContext`'s real data lives in `DefaultKeyMappings.Mappings`, not the top-level
  `Mappings` property.** Newer Enhanced Input (with the Player Mappable Key / User Settings
  profile system) keeps the actually-evaluated key→action bindings in a nested
  `DefaultKeyMappings` struct. The legacy top-level `Mappings` property still exists, is still
  readable/writable, and *looks* completely normal when round-tripped through generic property
  tooling — but the runtime input system never reads it. Symptom: `HasMappingContext()` reports
  the context as active, the `EnhancedInputComponent` binds the action, yet
  `GetActionValue()` is always zero no matter what's pressed. The only way this surfaced was
  opening the asset in the actual Mapping Context editor UI, where the real ("Default Key
  Mappings") section visibly had zero entries. Lesson: for this asset type, trust the native
  editor UI over a generic property reader when something's not triggering — and write key
  mappings to `DefaultKeyMappings.Mappings`.
- **`AddMappingContext` must happen in `PossessedBy`, not `BeginPlay`.** For a GameMode-spawned
  default pawn, `BeginPlay` runs before `Possess()`, so `GetController()` is still null at that
  point and the mapping context silently never gets added — every binding looks correct but no
  key ever does anything.
- **Voxel/blocky collision needs `bUseFlatBaseForFloorChecks = true` on
  `CharacterMovementComponent`.** With `bUseComplexAsSimpleCollision` on the chunk's
  `ProceduralMeshComponent` (triangle-mesh collision, no separate simple shape), the default
  rounded-capsule-bottom floor sweep failed to reliably detect "standing on the ground" against
  our flat, grid-aligned voxel tops — the character stayed in `MOVE_Falling` indefinitely
  (heavy, sluggish air-control on movement input, and jump silently blocked since it never saw
  `IsMovingOnGround() == true`). The flat-based floor check is the standard fix for blocky/voxel
  ground and resolved both symptoms at once.
- **A Blueprint's CDO can pin a property value independent of its native C++ parent's
  constructor default.** After fixing `bUseFlatBaseForFloorChecks` in `AVoxelCharacter`'s C++
  constructor and even fully restarting the editor, `BP_VoxelCharacter`'s CDO still reported the
  old value — the Blueprint had its own serialized default that didn't reconcile with the new
  native default. Setting the property directly on the Blueprint CDO (and recompiling the
  Blueprint) was what actually took effect. Lesson: when a C++ default change doesn't seem to
  "stick" for a Blueprint subclass, check (and if needed, directly set) the same property on the
  Blueprint's own CDO rather than assuming the native constructor alone controls it.
- **With `r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange=True` (physically-based
  lighting units active, inherited from this project's engine config), a `DirectionalLight`
  intensity of ~6 (a value that looked plausible copied from the template level) is drastically
  under-lit — real sunlight is tens of thousands of lux. This made every surface, including the
  grass top faces, render as a dark, almost-black green, which looked exactly like a missing-face
  rendering bug but wasn't one. Confirmed by temporarily spiking the intensity to 50 and watching
  lit faces jump to correctly bright green. Settled on 18 as a reasonably bright, non-blown-out
  "sunny day" value for this scene. Deep, fully enclosed pits still render dark even at this
  intensity — that part is expected (no GI/bounce lighting, see "Editor/content tooling split"
  above), not a bug.
