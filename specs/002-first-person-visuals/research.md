# Phase 0 Research: First-Person Visuals

No `[NEEDS CLARIFICATION]` markers remained after `/speckit-specify` (both spec-level
ambiguities — grass side texture style, bare hand vs. held tool — were resolved with the user
during that step). This document records the engineering decisions made while filling in the
Technical Context and project structure, since each had more than one reasonable option.

## Bedrock gets a 5th atlas cell instead of a vertex-color fallback

- **Decision**: Generate a 5th small atlas texture for Bedrock (a simple dark stone-like tile),
  giving every `EVoxelBlockType` a uniform atlas-UV code path. No per-block "use vertex color
  instead of texture" branch in the material or mesh builder.
- **Rationale**: The spec's Assumptions explicitly *allow* keeping Bedrock flat-colored but do
  not *require* it. Generating one more small Pixellab texture is strictly less total complexity
  than building and maintaining a second rendering path (a vertex-color/atlas blend switch in
  the material graph, plus matching logic in the mesh builder to decide which path a given face
  uses). One lookup table, one shader path, no conditional — simpler to write and to reason
  about later (Constitution Principle I).
- **Alternatives considered**: Keep 001's vertex-color pipeline alive just for Bedrock, blended
  via a repurposed vertex-color alpha channel as a 0/1 switch — rejected as more total moving
  parts for no real benefit, since Bedrock is rarely seen and costs nothing extra to texture.

## Atlas layout and texture import settings

- **Decision**: A single `T_BlockAtlas` texture, laid out as a simple N-cell horizontal strip
  (5 cells: Grass Top, Grass Side, Dirt, Stone, Bedrock), each cell a small square pixel-art
  tile (exact pixel size decided at implementation time based on what Pixellab produces
  cleanly, e.g. 32×32 or 64×64 per cell — not load-bearing for the architecture). Imported with
  **no mipmaps** (`MipGenSettings = TMGS_NoMipmaps`) and **Nearest/Point filtering**, texture
  group set for UI/2D (uncompressed or low-artifact compression) rather than the default
  photographic compression.
- **Rationale**: Pixel-art texture atlases are well known to "bleed" between adjacent cells when
  mipmapping or bilinear filtering samples across a cell boundary at a distance or shallow
  angle — exactly the voxel-cube-at-a-distance case this feature renders constantly. Disabling
  mipmaps and using point filtering is the standard, simple fix and avoids needing inter-cell
  padding math for a 5-cell atlas this small.
- **Alternatives considered**: Add padding/border pixels around each cell to tolerate bilinear
  filtering and keep mipmaps (better for distant LOD aliasing) — more correct for a large game
  but unnecessary complexity for 5 small cells in a learning-scoped MVP-adjacent feature; can be
  revisited if aliasing at a distance actually becomes a visible problem.

## UV emission in the mesh builder

- **Decision**: Extend `VoxelBlockRules` with a lookup `GetAtlasCellIndex(EVoxelBlockType,
  EVoxelFaceDirection) -> int32` (Grass differentiates Top vs. the other 5 directions, which all
  map to "Side"; every other type returns the same cell regardless of direction). In
  `VoxelMeshBuilder::BuildChunkMeshData`, each emitted face corner gets a real UV computed from
  that cell index and the atlas's fixed cell count, replacing the previous hardcoded `(0,0)`.
  Per-vertex color is still emitted (harmless, simply unused by the new material path) so no
  existing mesh-data shape (`FVoxelMeshData`) needs to change.
- **Rationale**: Keeps the exact same single-draw-call-per-chunk structure from 001 (one
  `ProceduralMeshComponent` mesh section, one material) — only the UV values and the material's
  BaseColor source change. No new per-face material/mesh-section split is introduced.
- **Alternatives considered**: One mesh section per block type (so each could use a distinct,
  simple, non-atlas material) — would multiply draw calls per chunk for no visual benefit over a
  single atlas-sampling material; rejected on the same "don't add complexity the project doesn't
  need yet" grounds as greedy meshing was deferred in 001.

## First-person arm: static mesh + code-driven sway, no skeleton

- **Decision**: The arm is a plain `UStaticMeshComponent` (not skeletal), generated as a static
  mesh in Blender MCP, exported as FBX, and imported via `unreal-mcp`'s `AssetTools`. It is
  attached under `AVoxelCharacter`'s existing `FirstPersonCamera` (so it inherits head look
  naturally) with a fixed relative offset placing it in the lower-right of the view, bare-handed
  per the resolved clarification. Idle "breathing/sway" motion (the spec's only required motion)
  is produced by a small per-frame sine-wave offset applied to the component's relative
  location/rotation in `AVoxelCharacter::Tick` — no animation asset, no skeleton, no Control Rig.
- **Rationale**: The spec explicitly waives skeletal animation ("不需要骨骼绑定的复杂动画"). A
  static mesh avoids the entire rig/skin-weight/retarget-to-UE-skeleton pipeline, which is a
  disproportionate amount of new complexity for "the camera isn't just a floating head anymore."
  A two-line sine function is the simplest thing that satisfies the actual requirement.
- **Alternatives considered**: Skeletal mesh with a simple idle animation clip — more
  "authentic" FPS-arm setup, but requires rigging/skinning in Blender and an Unreal skeleton to
  retarget onto; explicitly out of proportion to what the spec asks for. Can be revisited later
  if a future feature wants actual swing/tool-use animations tied to break/place actions (also
  explicitly out of scope this round per spec Assumptions).

## Why the arm never interferes with the aim trace (no new code needed)

- **Decision**: No new "ignore this component" logic is written for the arm mesh.
- **Rationale**: `AVoxelCharacter::TraceForTargetVoxel` already calls
  `FCollisionQueryParams::AddIgnoredActor(this)` before every break/place line trace (from 001).
  `AddIgnoredActor` ignores collision from *every component owned by that actor*, not just a
  named one. Since the new arm `UStaticMeshComponent` is attached under the same
  `AVoxelCharacter`, it is automatically covered — satisfying spec FR-006 for free, as a direct
  consequence of 001's existing trace setup rather than anything new in this feature.
- **Alternatives considered**: Explicitly set the arm mesh's collision to `NoCollision` as
  defense-in-depth — worth doing anyway (the arm never needs to physically collide with
  anything), and is included in tasks as a one-line safeguard, but it's a belt-and-braces
  addition, not what actually satisfies FR-006.

## Tooling split: Pixellab for textures, Blender for the mesh

- **Decision**: Pixellab MCP generates the four textured cells' pixel art (Grass Top, Grass
  Side, Dirt, Stone) plus the Bedrock tile; Blender MCP generates/exports the arm static mesh.
  `unreal-mcp`'s `TextureTools`/`AssetTools` import both into the project; `MaterialTools` wires
  the atlas texture into `M_VoxelChunk`'s graph.
- **Rationale**: Matches the tool to the medium — Pixellab is purpose-built for pixel-art 2D
  tiles/textures (exactly this feature's texture need), while Blender is the right tool for an
  actual 3D mesh (the arm). This also directly follows through on the "Note for later features"
  left in 001's research.md, which anticipated exactly this split.
- **Alternatives considered**: Modeling individual textured blocks in Blender instead of using a
  2D atlas — rejected during the pre-spec discussion with the user as architecturally wrong for
  a voxel game (would turn procedurally-meshed cubes into placed pre-made meshes, defeating the
  point of the chunk mesher); recorded here again because it's the single most important
  reasoning the next implementer should not re-litigate.
