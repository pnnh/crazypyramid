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

namespace VoxelBlockRules
{
	bool IsSolid(EVoxelBlockType BlockType);
	bool IsDestructible(EVoxelBlockType BlockType);
	FColor GetBaseColor(EVoxelBlockType BlockType);

	// Ordered list of types the player can cycle through and place. Air/Bedrock are excluded.
	TArray<EVoxelBlockType> GetPlaceableBlockTypes();
}
