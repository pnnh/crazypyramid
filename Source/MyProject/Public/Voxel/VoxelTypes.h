// T004: Block type enum and per-type rules (color, destructibility, placeable set).
#pragma once

#include "CoreMinimal.h"

enum class EVoxelBlockType : uint8
{
	Air     = 0,
	Grass   = 1,
	Dirt    = 2,
	Stone   = 3,
	Bedrock = 4
};

// Collapses the mesh builder's 6 physical face directions down to just these 2 for the purpose
// of atlas lookup: only Grass distinguishes Top from everything else (see contracts/
// block-atlas-mapping.md in 002-first-person-visuals).
enum class EVoxelFaceDirection : uint8
{
	Top  = 0,
	Side = 1
};

namespace VoxelBlockRules
{
	bool IsSolid(EVoxelBlockType BlockType);
	bool IsDestructible(EVoxelBlockType BlockType);
	FColor GetBaseColor(EVoxelBlockType BlockType);

	// Ordered list of types the player can cycle through and place. Air/Bedrock are excluded.
	TArray<EVoxelBlockType> GetPlaceableBlockTypes();

	// Total number of cells in T_BlockAtlas (GrassTop, GrassSide, Dirt, Stone, Bedrock).
	extern const int32 AtlasTotalCells;

	// Index (left-to-right) of this block type/face's cell in T_BlockAtlas. BlockType must be
	// solid (never Air) - callers only invoke this for faces already known to be solid.
	int32 GetAtlasCellIndex(EVoxelBlockType BlockType, EVoxelFaceDirection FaceDirection);
}
