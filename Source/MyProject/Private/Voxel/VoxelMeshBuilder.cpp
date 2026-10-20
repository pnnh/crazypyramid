#include "Voxel/VoxelMeshBuilder.h"

namespace
{
	struct FVoxelFace
	{
		FIntVector Direction;
		FVector Normal;
		FVector CornerOffsets[4]; // unit-cube-local corners, listed so (0,1,2)+(0,2,3) winds outward
	};

	const FVoxelFace GVoxelFaces[6] = {
		{ FIntVector(1, 0, 0),  FVector(1.f, 0.f, 0.f),  { FVector(1, 0, 0), FVector(1, 0, 1), FVector(1, 1, 1), FVector(1, 1, 0) } },
		{ FIntVector(-1, 0, 0), FVector(-1.f, 0.f, 0.f), { FVector(0, 0, 0), FVector(0, 1, 0), FVector(0, 1, 1), FVector(0, 0, 1) } },
		{ FIntVector(0, 1, 0),  FVector(0.f, 1.f, 0.f),  { FVector(0, 1, 0), FVector(1, 1, 0), FVector(1, 1, 1), FVector(0, 1, 1) } },
		{ FIntVector(0, -1, 0), FVector(0.f, -1.f, 0.f), { FVector(0, 0, 0), FVector(0, 0, 1), FVector(1, 0, 1), FVector(1, 0, 0) } },
		{ FIntVector(0, 0, 1),  FVector(0.f, 0.f, 1.f),  { FVector(0, 0, 1), FVector(1, 0, 1), FVector(1, 1, 1), FVector(0, 1, 1) } },
		{ FIntVector(0, 0, -1), FVector(0.f, 0.f, -1.f), { FVector(0, 0, 0), FVector(0, 1, 0), FVector(1, 1, 0), FVector(1, 0, 0) } },
	};

	EVoxelBlockType GetBlockSafe(const FVoxelChunkMeshInput& Input, int32 X, int32 Y, int32 Z)
	{
		if (Z < 0 || Z >= Input.SizeZ)
		{
			// No vertical neighbor chunks in this MVP (one chunk tall) - above/below the world is Air.
			return EVoxelBlockType::Air;
		}

		if (X < 0)
		{
			const int32 Index = Y + Z * Input.SizeY;
			return Input.NeighborMinX.IsValidIndex(Index) ? Input.NeighborMinX[Index] : EVoxelBlockType::Air;
		}
		if (X >= Input.SizeX)
		{
			const int32 Index = Y + Z * Input.SizeY;
			return Input.NeighborMaxX.IsValidIndex(Index) ? Input.NeighborMaxX[Index] : EVoxelBlockType::Air;
		}
		if (Y < 0)
		{
			const int32 Index = X + Z * Input.SizeX;
			return Input.NeighborMinY.IsValidIndex(Index) ? Input.NeighborMinY[Index] : EVoxelBlockType::Air;
		}
		if (Y >= Input.SizeY)
		{
			const int32 Index = X + Z * Input.SizeX;
			return Input.NeighborMaxY.IsValidIndex(Index) ? Input.NeighborMaxY[Index] : EVoxelBlockType::Air;
		}

		return Input.Voxels[X + Y * Input.SizeX + Z * Input.SizeX * Input.SizeY];
	}
}

FVoxelMeshData VoxelMeshBuilder::BuildChunkMeshData(const FVoxelChunkMeshInput& Input)
{
	FVoxelMeshData MeshData;

	for (int32 Z = 0; Z < Input.SizeZ; ++Z)
	{
		for (int32 Y = 0; Y < Input.SizeY; ++Y)
		{
			for (int32 X = 0; X < Input.SizeX; ++X)
			{
				const EVoxelBlockType BlockType = Input.Voxels[X + Y * Input.SizeX + Z * Input.SizeX * Input.SizeY];
				if (!VoxelBlockRules::IsSolid(BlockType))
				{
					continue;
				}

				const FColor Color = VoxelBlockRules::GetBaseColor(BlockType);

				for (const FVoxelFace& Face : GVoxelFaces)
				{
					const EVoxelBlockType NeighborType = GetBlockSafe(Input, X + Face.Direction.X, Y + Face.Direction.Y, Z + Face.Direction.Z);
					if (VoxelBlockRules::IsSolid(NeighborType))
					{
						continue; // hidden face between two solid voxels - never emitted, which is what avoids z-fighting
					}

					const int32 BaseIndex = MeshData.Vertices.Num();
					const FVector VoxelOrigin(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z));
					for (int32 Corner = 0; Corner < 4; ++Corner)
					{
						MeshData.Vertices.Add((VoxelOrigin + Face.CornerOffsets[Corner]) * Input.VoxelSize);
						MeshData.Normals.Add(Face.Normal);
						MeshData.VertexColors.Add(Color);
						MeshData.UVs.Add(FVector2D(0.f, 0.f)); // unused: material reads vertex color, not UV
					}

					MeshData.Triangles.Add(BaseIndex + 0);
					MeshData.Triangles.Add(BaseIndex + 1);
					MeshData.Triangles.Add(BaseIndex + 2);
					MeshData.Triangles.Add(BaseIndex + 0);
					MeshData.Triangles.Add(BaseIndex + 2);
					MeshData.Triangles.Add(BaseIndex + 3);
				}
			}
		}
	}

	return MeshData;
}
