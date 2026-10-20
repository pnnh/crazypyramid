// T005/T006: Plain (non-UObject) mesh-buffer types and the pure chunk-meshing function.
// Deliberately has no engine-actor/UObject dependency so it is safe to call off the game thread.
#pragma once

#include "CoreMinimal.h"
#include "Voxel/VoxelTypes.h"

struct FVoxelMeshData
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FColor> VertexColors;
	TArray<FVector2D> UVs;

	bool IsEmpty() const { return Vertices.Num() == 0; }
};

// A chunk's own voxel data plus a one-voxel-thick border slice copied from each horizontal
// neighbor (empty if that neighbor doesn't exist), so meshing can resolve face visibility at
// chunk edges without reaching into another chunk/actor.
struct FVoxelChunkMeshInput
{
	TArray<EVoxelBlockType> Voxels;       // SizeX*SizeY*SizeZ, indexed x + y*SizeX + z*SizeX*SizeY
	TArray<EVoxelBlockType> NeighborMinX; // this chunk's -X neighbor's voxels at its x=SizeX-1 face, indexed y + z*SizeY
	TArray<EVoxelBlockType> NeighborMaxX; // this chunk's +X neighbor's voxels at its x=0 face, indexed y + z*SizeY
	TArray<EVoxelBlockType> NeighborMinY; // this chunk's -Y neighbor's voxels at its y=SizeY-1 face, indexed x + z*SizeX
	TArray<EVoxelBlockType> NeighborMaxY; // this chunk's +Y neighbor's voxels at its y=0 face, indexed x + z*SizeX

	int32 SizeX = 16;
	int32 SizeY = 16;
	int32 SizeZ = 64;
	float VoxelSize = 100.f;
};

namespace VoxelMeshBuilder
{
	// Naive per-face-culling meshing: emits one quad per solid-voxel face whose neighbor is Air
	// or outside the world. No UObject access - safe to run on a background task.
	FVoxelMeshData BuildChunkMeshData(const FVoxelChunkMeshInput& Input);
}
