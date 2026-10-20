#include "Voxel/VoxelMeshBuilder.h"

namespace
{
	struct FVoxelFace
	{
		FIntVector Direction;
		FVector Normal;
		FVector CornerOffsets[4]; // unit-cube-local corners, listed so (0,1,2)+(0,2,3) winds outward
		EVoxelFaceDirection AtlasFaceDirection; // which atlas lookup this physical face uses
	};

	// Quad UV for each corner above, in the same (0,1,2,3) order - a fixed CCW (0,0)->(1,0)->(1,1)->(0,1)
	// mapping used for every face; only Grass's atlas-cell choice differs between Top and Side.
	const FVector2D GQuadCornerUVs[4] = { FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1) };

	const FVoxelFace GVoxelFaces[6] = {
		{ FIntVector(1, 0, 0),  FVector(1.f, 0.f, 0.f),  { FVector(1, 0, 0), FVector(1, 0, 1), FVector(1, 1, 1), FVector(1, 1, 0) }, EVoxelFaceDirection::Side },
		{ FIntVector(-1, 0, 0), FVector(-1.f, 0.f, 0.f), { FVector(0, 0, 0), FVector(0, 1, 0), FVector(0, 1, 1), FVector(0, 0, 1) }, EVoxelFaceDirection::Side },
		{ FIntVector(0, 1, 0),  FVector(0.f, 1.f, 0.f),  { FVector(0, 1, 0), FVector(1, 1, 0), FVector(1, 1, 1), FVector(0, 1, 1) }, EVoxelFaceDirection::Side },
		{ FIntVector(0, -1, 0), FVector(0.f, -1.f, 0.f), { FVector(0, 0, 0), FVector(0, 0, 1), FVector(1, 0, 1), FVector(1, 0, 0) }, EVoxelFaceDirection::Side },
		{ FIntVector(0, 0, 1),  FVector(0.f, 0.f, 1.f),  { FVector(0, 0, 1), FVector(1, 0, 1), FVector(1, 1, 1), FVector(0, 1, 1) }, EVoxelFaceDirection::Top },
		{ FIntVector(0, 0, -1), FVector(0.f, 0.f, -1.f), { FVector(0, 0, 0), FVector(0, 1, 0), FVector(1, 1, 0), FVector(1, 0, 0) }, EVoxelFaceDirection::Side },
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

					const int32 CellIndex = VoxelBlockRules::GetAtlasCellIndex(BlockType, Face.AtlasFaceDirection);
					const float CellWidth = 1.f / static_cast<float>(VoxelBlockRules::AtlasTotalCells);
					const float UMin = CellIndex * CellWidth;
					const float UMax = UMin + CellWidth;

					const int32 BaseIndex = MeshData.Vertices.Num();
					const FVector VoxelOrigin(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z));
					for (int32 Corner = 0; Corner < 4; ++Corner)
					{
						MeshData.Vertices.Add((VoxelOrigin + Face.CornerOffsets[Corner]) * Input.VoxelSize);
						MeshData.Normals.Add(Face.Normal);
						MeshData.VertexColors.Add(Color); // unused by the atlas material path; kept for parity with FVoxelMeshData's shape

						const FVector2D& QuadUV = GQuadCornerUVs[Corner];
						MeshData.UVs.Add(FVector2D(FMath::Lerp(UMin, UMax, QuadUV.X), QuadUV.Y));
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
