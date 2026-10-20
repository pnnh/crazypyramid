#include "Voxel/VoxelWorld.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// Floor division that behaves correctly for negative A (unlike plain integer '/').
	int32 FloorDiv(int32 A, int32 B)
	{
		const int32 Q = A / B;
		const int32 R = A % B;
		return (R != 0 && ((R < 0) != (B < 0))) ? Q - 1 : Q;
	}

	EVoxelBlockType ComputeColumnBlockType(int32 Z, int32 ColumnHeight, const FVoxelWorldGenerationParams& Params)
	{
		if (Z < Params.BedrockLayerThickness)
		{
			return EVoxelBlockType::Bedrock;
		}
		if (Z > ColumnHeight)
		{
			return EVoxelBlockType::Air;
		}
		if (Z == ColumnHeight)
		{
			return EVoxelBlockType::Grass;
		}
		if (Z > ColumnHeight - Params.DirtLayerThickness)
		{
			return EVoxelBlockType::Dirt;
		}
		return EVoxelBlockType::Stone;
	}
}

AVoxelWorld::AVoxelWorld()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AVoxelWorld::BeginPlay()
{
	Super::BeginPlay();
	GenerateWorld();
}

void AVoxelWorld::GenerateWorld()
{
	Chunks.Empty();

	for (int32 ChunkY = 0; ChunkY < WorldSizeChunksY; ++ChunkY)
	{
		for (int32 ChunkX = 0; ChunkX < WorldSizeChunksX; ++ChunkX)
		{
			CreateChunk(FVoxelChunkCoordinate(ChunkX, ChunkY));
		}
	}

	// Keep generated terrain within [BedrockLayerThickness+DirtLayerThickness+2, SizeZ-2] so every
	// column has room for a full Bedrock+Stone+Dirt+Grass stack without touching the world's
	// vertical limits, regardless of the noise sample.
	const int32 MinSafeHeight = GenerationParams.BedrockLayerThickness + GenerationParams.DirtLayerThickness + 2;
	const int32 MaxSafeHeight = AVoxelChunk::SizeZ - 2;

	for (const TPair<FVoxelChunkCoordinate, TObjectPtr<AVoxelChunk>>& Entry : Chunks)
	{
		AVoxelChunk* Chunk = Entry.Value;
		const FVoxelChunkCoordinate Coord = Entry.Key;

		for (int32 LocalY = 0; LocalY < AVoxelChunk::SizeY; ++LocalY)
		{
			for (int32 LocalX = 0; LocalX < AVoxelChunk::SizeX; ++LocalX)
			{
				const int32 GlobalX = Coord.X * AVoxelChunk::SizeX + LocalX;
				const int32 GlobalY = Coord.Y * AVoxelChunk::SizeY + LocalY;

				const float NoiseSample = FMath::PerlinNoise2D(FVector2D(
					(GlobalX + Seed) * GenerationParams.Frequency,
					(GlobalY + Seed) * GenerationParams.Frequency));

				const int32 ColumnHeight = FMath::Clamp(
					GenerationParams.BaseHeight + FMath::RoundToInt(NoiseSample * GenerationParams.Amplitude),
					MinSafeHeight, MaxSafeHeight);

				for (int32 Z = 0; Z < AVoxelChunk::SizeZ; ++Z)
				{
					Chunk->SetVoxel(LocalX, LocalY, Z, ComputeColumnBlockType(Z, ColumnHeight, GenerationParams));
				}
			}
		}
	}

	for (const TPair<FVoxelChunkCoordinate, TObjectPtr<AVoxelChunk>>& Entry : Chunks)
	{
		RemeshChunk(Entry.Value);
	}
}

AVoxelChunk* AVoxelWorld::CreateChunk(const FVoxelChunkCoordinate& Coord)
{
	const FVector SpawnLocation = GetActorLocation() + FVector(
		Coord.X * AVoxelChunk::SizeX * AVoxelChunk::VoxelSize,
		Coord.Y * AVoxelChunk::SizeY * AVoxelChunk::VoxelSize,
		0.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	AVoxelChunk* NewChunk = GetWorld()->SpawnActor<AVoxelChunk>(AVoxelChunk::StaticClass(), FTransform(SpawnLocation), SpawnParams);
	if (NewChunk)
	{
		NewChunk->InitializeChunk(Coord, ChunkMaterial);
		Chunks.Add(Coord, NewChunk);
	}
	return NewChunk;
}

AVoxelChunk* AVoxelWorld::GetOrNullChunk(const FVoxelChunkCoordinate& Coord) const
{
	if (const TObjectPtr<AVoxelChunk>* Found = Chunks.Find(Coord))
	{
		return *Found;
	}
	return nullptr;
}

void AVoxelWorld::RemeshChunk(AVoxelChunk* Chunk)
{
	if (!Chunk)
	{
		return;
	}

	const FVoxelChunkCoordinate Coord = Chunk->GetChunkCoordinate();

	TArray<EVoxelBlockType> NeighborMinX, NeighborMaxX, NeighborMinY, NeighborMaxY;
	if (AVoxelChunk* Neighbor = GetOrNullChunk(FVoxelChunkCoordinate(Coord.X - 1, Coord.Y)))
	{
		NeighborMinX = Neighbor->GetBorderSliceMaxX();
	}
	if (AVoxelChunk* Neighbor = GetOrNullChunk(FVoxelChunkCoordinate(Coord.X + 1, Coord.Y)))
	{
		NeighborMaxX = Neighbor->GetBorderSliceMinX();
	}
	if (AVoxelChunk* Neighbor = GetOrNullChunk(FVoxelChunkCoordinate(Coord.X, Coord.Y - 1)))
	{
		NeighborMinY = Neighbor->GetBorderSliceMaxY();
	}
	if (AVoxelChunk* Neighbor = GetOrNullChunk(FVoxelChunkCoordinate(Coord.X, Coord.Y + 1)))
	{
		NeighborMaxY = Neighbor->GetBorderSliceMinY();
	}

	Chunk->RequestRemesh(NeighborMinX, NeighborMaxX, NeighborMinY, NeighborMaxY);
}

void AVoxelWorld::RemeshChunkAndNeighborsIfBoundary(const FVoxelChunkCoordinate& Coord, int32 LocalX, int32 LocalY)
{
	if (AVoxelChunk* Chunk = GetOrNullChunk(Coord))
	{
		RemeshChunk(Chunk);
	}

	// The edited voxel's own chunk is always remeshed above. If it sits on a chunk face, the
	// neighbor sharing that face also depends on it and must be remeshed too.
	if (LocalX == 0)
	{
		if (AVoxelChunk* Neighbor = GetOrNullChunk(FVoxelChunkCoordinate(Coord.X - 1, Coord.Y)))
		{
			RemeshChunk(Neighbor);
		}
	}
	else if (LocalX == AVoxelChunk::SizeX - 1)
	{
		if (AVoxelChunk* Neighbor = GetOrNullChunk(FVoxelChunkCoordinate(Coord.X + 1, Coord.Y)))
		{
			RemeshChunk(Neighbor);
		}
	}

	if (LocalY == 0)
	{
		if (AVoxelChunk* Neighbor = GetOrNullChunk(FVoxelChunkCoordinate(Coord.X, Coord.Y - 1)))
		{
			RemeshChunk(Neighbor);
		}
	}
	else if (LocalY == AVoxelChunk::SizeY - 1)
	{
		if (AVoxelChunk* Neighbor = GetOrNullChunk(FVoxelChunkCoordinate(Coord.X, Coord.Y + 1)))
		{
			RemeshChunk(Neighbor);
		}
	}
}

bool AVoxelWorld::WorldLocationToVoxel(const FVector& WorldLocation, FVoxelChunkCoordinate& OutChunk, int32& OutLocalX, int32& OutLocalY, int32& OutLocalZ) const
{
	const FVector Local = WorldLocation - GetActorLocation();
	const int32 GlobalX = FMath::FloorToInt(Local.X / AVoxelChunk::VoxelSize);
	const int32 GlobalY = FMath::FloorToInt(Local.Y / AVoxelChunk::VoxelSize);
	const int32 GlobalZ = FMath::FloorToInt(Local.Z / AVoxelChunk::VoxelSize);

	if (GlobalZ < 0 || GlobalZ >= AVoxelChunk::SizeZ)
	{
		return false; // above or below the world entirely
	}

	const int32 ChunkX = FloorDiv(GlobalX, AVoxelChunk::SizeX);
	const int32 ChunkY = FloorDiv(GlobalY, AVoxelChunk::SizeY);

	if (ChunkX < 0 || ChunkX >= WorldSizeChunksX || ChunkY < 0 || ChunkY >= WorldSizeChunksY)
	{
		return false; // outside the finite world's horizontal bounds
	}

	OutChunk = FVoxelChunkCoordinate(ChunkX, ChunkY);
	OutLocalX = GlobalX - ChunkX * AVoxelChunk::SizeX;
	OutLocalY = GlobalY - ChunkY * AVoxelChunk::SizeY;
	OutLocalZ = GlobalZ;
	return true;
}

FVector AVoxelWorld::VoxelToWorldLocationCenter(const FVoxelChunkCoordinate& Chunk, int32 LocalX, int32 LocalY, int32 LocalZ) const
{
	const float WorldX = (Chunk.X * AVoxelChunk::SizeX + LocalX + 0.5f) * AVoxelChunk::VoxelSize;
	const float WorldY = (Chunk.Y * AVoxelChunk::SizeY + LocalY + 0.5f) * AVoxelChunk::VoxelSize;
	const float WorldZ = (LocalZ + 0.5f) * AVoxelChunk::VoxelSize;
	return GetActorLocation() + FVector(WorldX, WorldY, WorldZ);
}

bool AVoxelWorld::TryBreakBlock(const FVector& WorldLocation, const FVector& HitNormal)
{
	// Nudge half a voxel against the hit normal to land inside the voxel being broken, not the
	// one it's touching on the other side of the hit face.
	const FVector TargetPoint = WorldLocation - HitNormal * (AVoxelChunk::VoxelSize * 0.5f);

	FVoxelChunkCoordinate Coord;
	int32 LocalX, LocalY, LocalZ;
	if (!WorldLocationToVoxel(TargetPoint, Coord, LocalX, LocalY, LocalZ))
	{
		return false;
	}

	AVoxelChunk* Chunk = GetOrNullChunk(Coord);
	if (!Chunk)
	{
		return false;
	}

	const EVoxelBlockType CurrentType = Chunk->GetVoxel(LocalX, LocalY, LocalZ);
	if (!VoxelBlockRules::IsSolid(CurrentType) || !VoxelBlockRules::IsDestructible(CurrentType))
	{
		return false;
	}

	Chunk->SetVoxel(LocalX, LocalY, LocalZ, EVoxelBlockType::Air);
	RemeshChunkAndNeighborsIfBoundary(Coord, LocalX, LocalY);
	return true;
}

bool AVoxelWorld::TryPlaceBlock(const FVector& WorldLocation, const FVector& HitNormal, EVoxelBlockType NewType, const AActor* RequestingActor)
{
	// Nudge half a voxel along the hit normal to land in the empty cell just outside the hit face.
	const FVector TargetPoint = WorldLocation + HitNormal * (AVoxelChunk::VoxelSize * 0.5f);

	FVoxelChunkCoordinate Coord;
	int32 LocalX, LocalY, LocalZ;
	if (!WorldLocationToVoxel(TargetPoint, Coord, LocalX, LocalY, LocalZ))
	{
		return false;
	}

	AVoxelChunk* Chunk = GetOrNullChunk(Coord);
	if (!Chunk)
	{
		return false;
	}

	if (VoxelBlockRules::IsSolid(Chunk->GetVoxel(LocalX, LocalY, LocalZ)))
	{
		return false; // target cell already occupied
	}

	const FVector VoxelCenter = VoxelToWorldLocationCenter(Coord, LocalX, LocalY, LocalZ);
	if (DoesPlaceOverlapPlayer(VoxelCenter, RequestingActor))
	{
		return false;
	}

	Chunk->SetVoxel(LocalX, LocalY, LocalZ, NewType);
	RemeshChunkAndNeighborsIfBoundary(Coord, LocalX, LocalY);
	return true;
}

bool AVoxelWorld::DoesPlaceOverlapPlayer(const FVector& VoxelCenter, const AActor* RequestingActor) const
{
	if (!RequestingActor)
	{
		return false;
	}

	const float HalfVoxel = AVoxelChunk::VoxelSize * 0.5f;
	const FBox VoxelBox(VoxelCenter - FVector(HalfVoxel), VoxelCenter + FVector(HalfVoxel));
	const FBox ActorBox = RequestingActor->GetComponentsBoundingBox(true);
	return VoxelBox.Intersect(ActorBox);
}
