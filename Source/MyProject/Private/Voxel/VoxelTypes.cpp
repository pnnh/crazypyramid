#include "Voxel/VoxelTypes.h"

bool VoxelBlockRules::IsSolid(EVoxelBlockType BlockType)
{
	return BlockType != EVoxelBlockType::Air;
}

bool VoxelBlockRules::IsDestructible(EVoxelBlockType BlockType)
{
	switch (BlockType)
	{
	case EVoxelBlockType::Bedrock:
	case EVoxelBlockType::Air:
		return false;
	default:
		return true;
	}
}

FColor VoxelBlockRules::GetBaseColor(EVoxelBlockType BlockType)
{
	switch (BlockType)
	{
	case EVoxelBlockType::Grass:
		return FColor(86, 148, 62);
	case EVoxelBlockType::Dirt:
		return FColor(107, 74, 48);
	case EVoxelBlockType::Stone:
		return FColor(120, 120, 120);
	case EVoxelBlockType::Bedrock:
		return FColor(40, 40, 40);
	case EVoxelBlockType::Air:
	default:
		return FColor::Magenta; // should never actually be rendered; stands out if it ever is
	}
}

TArray<EVoxelBlockType> VoxelBlockRules::GetPlaceableBlockTypes()
{
	return { EVoxelBlockType::Grass, EVoxelBlockType::Dirt, EVoxelBlockType::Stone };
}
