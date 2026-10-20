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

// T_BlockAtlas layout: a single left-to-right strip, cell order fixed by
// specs/002-first-person-visuals/contracts/block-atlas-mapping.md.
namespace
{
	constexpr int32 AtlasCell_GrassTop  = 0;
	constexpr int32 AtlasCell_GrassSide = 1;
	constexpr int32 AtlasCell_Dirt      = 2;
	constexpr int32 AtlasCell_Stone     = 3;
	constexpr int32 AtlasCell_Bedrock   = 4;
}

const int32 VoxelBlockRules::AtlasTotalCells = 5;

int32 VoxelBlockRules::GetAtlasCellIndex(EVoxelBlockType BlockType, EVoxelFaceDirection FaceDirection)
{
	switch (BlockType)
	{
	case EVoxelBlockType::Grass:
		return (FaceDirection == EVoxelFaceDirection::Top) ? AtlasCell_GrassTop : AtlasCell_GrassSide;
	case EVoxelBlockType::Dirt:
		return AtlasCell_Dirt;
	case EVoxelBlockType::Stone:
		return AtlasCell_Stone;
	case EVoxelBlockType::Bedrock:
		return AtlasCell_Bedrock;
	case EVoxelBlockType::Air:
	default:
		checkf(false, TEXT("GetAtlasCellIndex called with a non-solid BlockType"));
		return AtlasCell_Stone;
	}
}
