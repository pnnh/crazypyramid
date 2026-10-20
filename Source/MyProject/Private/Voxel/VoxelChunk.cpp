#include "Voxel/VoxelChunk.h"
#include "ProceduralMeshComponent.h"
#include "Async/Async.h"
#include "Materials/MaterialInterface.h"

AVoxelChunk::AVoxelChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->bUseComplexAsSimpleCollision = true; // let character movement sweep against the generated triangles directly
	MeshComponent->SetMobility(EComponentMobility::Static);

	Voxels.SetNumZeroed(SizeX * SizeY * SizeZ); // zero-initialized == EVoxelBlockType::Air
}

void AVoxelChunk::InitializeChunk(const FVoxelChunkCoordinate& InCoordinate, UMaterialInterface* ChunkMaterial)
{
	Coordinate = InCoordinate;
	if (ChunkMaterial)
	{
		MeshComponent->SetMaterial(0, ChunkMaterial);
	}
}

bool AVoxelChunk::IsLocalInBounds(int32 X, int32 Y, int32 Z)
{
	return X >= 0 && X < SizeX && Y >= 0 && Y < SizeY && Z >= 0 && Z < SizeZ;
}

int32 AVoxelChunk::VoxelIndex(int32 X, int32 Y, int32 Z)
{
	return X + Y * SizeX + Z * SizeX * SizeY;
}

EVoxelBlockType AVoxelChunk::GetVoxel(int32 LocalX, int32 LocalY, int32 LocalZ) const
{
	if (!IsLocalInBounds(LocalX, LocalY, LocalZ))
	{
		return EVoxelBlockType::Air;
	}
	return Voxels[VoxelIndex(LocalX, LocalY, LocalZ)];
}

bool AVoxelChunk::SetVoxel(int32 LocalX, int32 LocalY, int32 LocalZ, EVoxelBlockType NewType)
{
	if (!IsLocalInBounds(LocalX, LocalY, LocalZ))
	{
		return false;
	}
	Voxels[VoxelIndex(LocalX, LocalY, LocalZ)] = NewType;
	return true;
}

TArray<EVoxelBlockType> AVoxelChunk::GetBorderSliceMinX() const
{
	TArray<EVoxelBlockType> Slice;
	Slice.SetNumUninitialized(SizeY * SizeZ);
	for (int32 Z = 0; Z < SizeZ; ++Z)
	{
		for (int32 Y = 0; Y < SizeY; ++Y)
		{
			Slice[Y + Z * SizeY] = GetVoxel(0, Y, Z);
		}
	}
	return Slice;
}

TArray<EVoxelBlockType> AVoxelChunk::GetBorderSliceMaxX() const
{
	TArray<EVoxelBlockType> Slice;
	Slice.SetNumUninitialized(SizeY * SizeZ);
	for (int32 Z = 0; Z < SizeZ; ++Z)
	{
		for (int32 Y = 0; Y < SizeY; ++Y)
		{
			Slice[Y + Z * SizeY] = GetVoxel(SizeX - 1, Y, Z);
		}
	}
	return Slice;
}

TArray<EVoxelBlockType> AVoxelChunk::GetBorderSliceMinY() const
{
	TArray<EVoxelBlockType> Slice;
	Slice.SetNumUninitialized(SizeX * SizeZ);
	for (int32 Z = 0; Z < SizeZ; ++Z)
	{
		for (int32 X = 0; X < SizeX; ++X)
		{
			Slice[X + Z * SizeX] = GetVoxel(X, 0, Z);
		}
	}
	return Slice;
}

TArray<EVoxelBlockType> AVoxelChunk::GetBorderSliceMaxY() const
{
	TArray<EVoxelBlockType> Slice;
	Slice.SetNumUninitialized(SizeX * SizeZ);
	for (int32 Z = 0; Z < SizeZ; ++Z)
	{
		for (int32 X = 0; X < SizeX; ++X)
		{
			Slice[X + Z * SizeX] = GetVoxel(X, SizeY - 1, Z);
		}
	}
	return Slice;
}

void AVoxelChunk::RequestRemesh(const TArray<EVoxelBlockType>& NeighborMinX, const TArray<EVoxelBlockType>& NeighborMaxX,
	const TArray<EVoxelBlockType>& NeighborMinY, const TArray<EVoxelBlockType>& NeighborMaxY)
{
	++RemeshGenerationCounter;
	const uint32 ThisGeneration = RemeshGenerationCounter;

	FVoxelChunkMeshInput Input;
	Input.Voxels = Voxels;
	Input.NeighborMinX = NeighborMinX;
	Input.NeighborMaxX = NeighborMaxX;
	Input.NeighborMinY = NeighborMinY;
	Input.NeighborMaxY = NeighborMaxY;
	Input.SizeX = SizeX;
	Input.SizeY = SizeY;
	Input.SizeZ = SizeZ;
	Input.VoxelSize = VoxelSize;

	const TWeakObjectPtr<AVoxelChunk> WeakThis(this);

	// Triangulation runs on a background Task Graph worker; only the final ApplyMeshData call
	// (CreateMeshSection) happens on the game thread, so neither the initial world build nor a
	// single edit's remesh blocks the game thread doing the actual mesh-building work.
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, Input = MoveTemp(Input), ThisGeneration]()
	{
		FVoxelMeshData MeshData = VoxelMeshBuilder::BuildChunkMeshData(Input);

		AsyncTask(ENamedThreads::GameThread, [WeakThis, MeshData = MoveTemp(MeshData), ThisGeneration]()
		{
			if (AVoxelChunk* Chunk = WeakThis.Get())
			{
				if (Chunk->RemeshGenerationCounter == ThisGeneration)
				{
					Chunk->ApplyMeshData(MeshData);
				}
				// else: a newer edit already requested another remesh - discard this stale result.
			}
		});
	});
}

void AVoxelChunk::ApplyMeshData(const FVoxelMeshData& MeshData)
{
	if (MeshData.IsEmpty())
	{
		MeshComponent->ClearMeshSection(0);
		return;
	}

	static const TArray<FProcMeshTangent> EmptyTangents;
	MeshComponent->CreateMeshSection(
		0,
		MeshData.Vertices,
		MeshData.Triangles,
		MeshData.Normals,
		MeshData.UVs,
		MeshData.VertexColors,
		EmptyTangents,
		/*bCreateCollision=*/ true);
}
