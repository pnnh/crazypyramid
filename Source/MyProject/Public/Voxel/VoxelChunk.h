// T007/T008: Per-chunk voxel storage + rendering/collision, with async remeshing.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Voxel/VoxelTypes.h"
#include "Voxel/VoxelMeshBuilder.h"
#include "VoxelChunk.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

// Plain (non-reflected) chunk-grid coordinate - internal bookkeeping only, never Blueprint-exposed.
struct FVoxelChunkCoordinate
{
	int32 X = 0;
	int32 Y = 0;

	FVoxelChunkCoordinate() = default;
	FVoxelChunkCoordinate(int32 InX, int32 InY) : X(InX), Y(InY) {}

	bool operator==(const FVoxelChunkCoordinate& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}
};

FORCEINLINE uint32 GetTypeHash(const FVoxelChunkCoordinate& Coord)
{
	return HashCombine(::GetTypeHash(Coord.X), ::GetTypeHash(Coord.Y));
}

UCLASS()
class MYPROJECT_API AVoxelChunk : public AActor
{
	GENERATED_BODY()

public:
	AVoxelChunk();

	static constexpr int32 SizeX = 16;
	static constexpr int32 SizeY = 16;
	static constexpr int32 SizeZ = 64;
	static constexpr float VoxelSize = 100.f;

	void InitializeChunk(const FVoxelChunkCoordinate& InCoordinate, UMaterialInterface* ChunkMaterial);

	EVoxelBlockType GetVoxel(int32 LocalX, int32 LocalY, int32 LocalZ) const;
	bool SetVoxel(int32 LocalX, int32 LocalY, int32 LocalZ, EVoxelBlockType NewType);

	// Kicks off an async rebuild of this chunk's mesh. NeighborMin/Max arrays are border slices
	// gathered by AVoxelWorld from this chunk's horizontal neighbors (empty if none exists).
	void RequestRemesh(const TArray<EVoxelBlockType>& NeighborMinX, const TArray<EVoxelBlockType>& NeighborMaxX,
		const TArray<EVoxelBlockType>& NeighborMinY, const TArray<EVoxelBlockType>& NeighborMaxY);

	TArray<EVoxelBlockType> GetBorderSliceMinX() const; // this chunk's x=0 face, indexed y + z*SizeY
	TArray<EVoxelBlockType> GetBorderSliceMaxX() const; // this chunk's x=SizeX-1 face, indexed y + z*SizeY
	TArray<EVoxelBlockType> GetBorderSliceMinY() const; // this chunk's y=0 face, indexed x + z*SizeX
	TArray<EVoxelBlockType> GetBorderSliceMaxY() const; // this chunk's y=SizeY-1 face, indexed x + z*SizeX

	FVoxelChunkCoordinate GetChunkCoordinate() const { return Coordinate; }

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProceduralMeshComponent> MeshComponent;

	FVoxelChunkCoordinate Coordinate;
	TArray<EVoxelBlockType> Voxels;

	// Bumped on every RequestRemesh so a stale async result (superseded by a newer edit before
	// the first one finished) is discarded instead of being applied out of order.
	uint32 RemeshGenerationCounter = 0;

	static bool IsLocalInBounds(int32 X, int32 Y, int32 Z);
	static int32 VoxelIndex(int32 X, int32 Y, int32 Z);

	void ApplyMeshData(const FVoxelMeshData& MeshData);
};
