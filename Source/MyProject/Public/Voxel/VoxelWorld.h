// T009: Chunk grid owner - world generation and the break/place API the player calls into.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Voxel/VoxelTypes.h"
#include "Voxel/VoxelChunk.h"
#include "VoxelWorld.generated.h"

class UMaterialInterface;

USTRUCT(BlueprintType)
struct FVoxelWorldGenerationParams
{
	GENERATED_BODY()

	// Column height (in voxels) at noise sample 0.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BaseHeight = 24;

	// How many voxels of height variation the noise can add/subtract.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Amplitude = 10.f;

	// Noise sample-coordinate scale; smaller = broader hills, larger = choppier terrain.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Frequency = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DirtLayerThickness = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BedrockLayerThickness = 1;
};

UCLASS()
class MYPROJECT_API AVoxelWorld : public AActor
{
	GENERATED_BODY()

public:
	AVoxelWorld();

	UPROPERTY(EditAnywhere, Category = "Voxel World")
	int32 WorldSizeChunksX = 8;

	UPROPERTY(EditAnywhere, Category = "Voxel World")
	int32 WorldSizeChunksY = 8;

	// Fixed seed so terrain is reproducible run-to-run (spec Assumptions).
	UPROPERTY(EditAnywhere, Category = "Voxel World")
	int32 Seed = 12345;

	UPROPERTY(EditAnywhere, Category = "Voxel World")
	FVoxelWorldGenerationParams GenerationParams;

	UPROPERTY(EditAnywhere, Category = "Voxel World")
	TObjectPtr<UMaterialInterface> ChunkMaterial;

	// Attempts to remove the voxel at WorldLocation, approached from HitNormal. No-op (returns
	// false) if nothing is there, the target is out of the world, or the block is indestructible.
	bool TryBreakBlock(const FVector& WorldLocation, const FVector& HitNormal);

	// Attempts to place NewType in the cell just outside WorldLocation along HitNormal. No-op
	// (returns false) if the target is occupied, out of the world, or would overlap RequestingActor.
	bool TryPlaceBlock(const FVector& WorldLocation, const FVector& HitNormal, EVoxelBlockType NewType, const AActor* RequestingActor);

protected:
	virtual void BeginPlay() override;

private:
	TMap<FVoxelChunkCoordinate, TObjectPtr<AVoxelChunk>> Chunks;

	void GenerateWorld();
	AVoxelChunk* CreateChunk(const FVoxelChunkCoordinate& Coord);
	AVoxelChunk* GetOrNullChunk(const FVoxelChunkCoordinate& Coord) const;

	bool WorldLocationToVoxel(const FVector& WorldLocation, FVoxelChunkCoordinate& OutChunk, int32& OutLocalX, int32& OutLocalY, int32& OutLocalZ) const;
	FVector VoxelToWorldLocationCenter(const FVoxelChunkCoordinate& Chunk, int32 LocalX, int32 LocalY, int32 LocalZ) const;

	void RemeshChunk(AVoxelChunk* Chunk);
	void RemeshChunkAndNeighborsIfBoundary(const FVoxelChunkCoordinate& Coord, int32 LocalX, int32 LocalY);

	bool DoesPlaceOverlapPlayer(const FVector& VoxelCenter, const AActor* RequestingActor) const;
};
