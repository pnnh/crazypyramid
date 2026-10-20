<!--
Sync Impact Report
Version change: [TEMPLATE] → 1.0.0
Modified principles: N/A (initial ratification; template placeholders replaced)
Added sections:
  - Core Principles (5): Learning-First Pragmatic Iteration; Playable Vertical Slices;
    Single-Player Scope Discipline (YAGNI); Performance-Aware Voxel Architecture;
    Tool-Assisted Workflow
  - Technical Constraints
  - Development Workflow
  - Governance
Removed sections: none (first ratification from template)
Templates requiring follow-up: none — plan/spec/tasks templates reference principles
  generically and need no structural change for this ratification.
Follow-up TODOs: none
-->

# CrazyPyramid Voxel Sandbox Constitution

## Core Principles

### I. Learning-First, Pragmatic Iteration

This is a personal, learning-driven project. Its purpose is to understand the core
mechanics of a voxel engine — chunked data storage, procedural mesh generation, noise-based
terrain generation, and block add/remove editing — by building them, not by wiring up a
black box. Hand-written C++ implementations of the Chunk/Mesh systems MUST be preferred over
integrating a heavyweight third-party voxel plugin, even when the plugin would be faster to
adopt, because the implementation IS the learning outcome. This does not excuse sloppy code:
all hand-written systems MUST remain clear, maintainable, and handle realistic error/edge
cases (e.g., out-of-bounds chunk access, malformed save data) rather than assuming happy-path
input only.

### II. Playable Vertical Slices

Every Speckit feature or milestone MUST end in something runnable and observable inside the
Unreal Editor via Play-In-Editor (PIE) — not merely compiling code with no visible behavior.
When scoping a feature, prefer the smallest slice that produces a visible, interactive result
("I can see it, I can touch it") over a larger slice that leaves the loop incomplete.
Incremental depth and polish are added in subsequent features once the end-to-end loop works.

### III. Single-Player Scope Discipline (YAGNI)

The MVP and all early-phase features target a single local player only. Multiplayer network
replication, server authority, and anti-cheat validation are explicitly out of scope until a
feature spec says otherwise. Abstractions for not-yet-planned systems (mob AI, crafting,
day/night cycles, inventory, etc.) MUST NOT be pre-built "just in case" — design them only
when a concrete feature spec requires them. Building for a hypothetical future need, instead
of the current spec, is a constitution violation, not a style preference.

### IV. Performance-Aware Voxel Architecture

Voxel data MUST be organized by chunk. Chunk mesh (re)generation MUST NOT block the game
thread for a noticeable duration — use async tasks or frame-sliced work instead of synchronous
full remeshes on the main thread. Chunk load/unload MUST be driven by a dynamic radius around
the player (streaming), not a statically baked world. The MVP may validate on a small/finite
world, but data structures and APIs MUST be designed so that scaling to a larger streamed
world later does not require a rewrite of the chunk/mesh contracts — only tuning of radii and
thresholds.

### V. Tool-Assisted Workflow

Development uses Claude Code paired with the locally running Unreal MCP server (`unreal-mcp`,
port 8000) for editor automation — asset creation, actor placement, Blueprint and material
adjustments, and similar editor-side operations. Core voxel engine logic (chunk data
structures, meshing algorithms, terrain generation) MUST be hand-written C++ maintained under
`Source/`, since MCP tools automate the editor, not engine-logic authorship. Every feature
MUST progress through the standard Speckit flow: `constitution` → `specify` → `clarify`
(optional) → `plan` → `tasks` → `analyze` (optional) → `implement`.

## Technical Constraints

- **Engine version**: locked to Unreal Engine 5.8. Upgrading engine versions requires a
  constitution amendment noting the reason and any migration risk.
- **Primary language**: C++ for all core voxel systems, under the `MYPROJECT_API` module.
  Blueprint is permitted only for level tuning, UI, and quick prototyping — not for core
  chunk/mesh/terrain-generation logic.
- **No third-party voxel plugins**: plugins such as "Voxel Plugin" are explicitly excluded
  for the core engine (see Principle I). General-purpose engine plugins already enabled in
  this project (ModelingToolsEditorMode, ModelContextProtocol, AllToolsets) remain in scope
  for editor tooling, not as a voxel-engine substitute.
- **Mesh generation**: use the engine-native `ProceduralMeshComponent` by default. Migrating
  to an alternative (e.g., `RealtimeMeshComponent`) requires measured performance data
  demonstrating `ProceduralMeshComponent` is an actual bottleneck — it MUST NOT be swapped out
  preemptively.

## Development Workflow

- This is a solo project: no pull-request review gate is required.
- Each completed, milestone-level feature MUST end in a meaningful git commit (not a vague
  checkpoint commit) so the project history reflects real progress steps.
- Verification is primarily manual PIE testing in the Unreal Editor for MVP and early
  features. Automated test coverage (e.g., via `AutomationTestToolset`) is not required yet,
  and MAY be introduced later for performance regressions or systems too complex to verify
  by hand.

## Governance

- This constitution supersedes ad hoc practice for this project. Any feature plan or task
  list that conflicts with a principle here must either change or justify the deviation
  explicitly in that feature's plan.
- Amendments are made by re-running `/speckit-constitution`. Versioning follows semantic
  versioning: MAJOR for incompatible principle removals/redefinitions, MINOR for new
  principles/sections or materially expanded guidance, PATCH for wording/clarification only.
- Because this is a personal project, amendments do not require external approval, but each
  amendment MUST record its reason in the Sync Impact Report at the top of this file.

**Version**: 1.0.0 | **Ratified**: 2026-10-20 | **Last Amended**: 2026-10-20
