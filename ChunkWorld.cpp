// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkWorld.h"

#include "Chunk.h"
#include "NavigationSystem.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"

// Sets default values
AChunkWorld::AChunkWorld()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	ChunkCount = 0;
}

// Called when the game starts or when spawned
void AChunkWorld::BeginPlay()
{
	Super::BeginPlay();
	
	Generate3DWorld();

	UE_LOG(LogTemp, Warning, TEXT("%d Chunks Created"), ChunkCount);

	GetWorld()->OnWorldBeginPlay.AddUObject(this, &AChunkWorld::UpdateNavMesh);
	
}

void AChunkWorld::UpdateNavMesh()
{
	if (NavMeshBounds)
	{
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

		RecastNavMesh->RebuildAll();
		NavSys->OnNavigationBoundsUpdated(NavMeshBounds);
	}
}

void AChunkWorld::Generate3DWorld()
{
	for (int x = -DrawDistance; x <= DrawDistance; x++)
	{
		for (int y = -DrawDistance; y <= DrawDistance; ++y)
		{
			for (int z = 0; z <= DrawDistance; ++z)
			{
				EGenerationType GenerationType = EGenerationType::Surface;
				if (z > 0)
				{
					GenerationType = EGenerationType::Underground;
					//Rather than do this we could have Create a AUndergroundChunk class which inherits from AChunk
				}
				FTransform Transform = FTransform(
						FRotator::ZeroRotator,
						FVector(x * Size * 100, y * Size * 100, -(z * Size * 100)),
						FVector::OneVector
					);
				
				const auto Chunk = GetWorld()->SpawnActorDeferred<AChunk>(
					ChunkType,
					Transform,
					this
				);

				Chunk->NoiseFrequency = NoiseFrequency;
				Chunk->Size = Size;
				Chunk->Material = Material;
				Chunk->GenerationType = GenerationType;
				Chunk->TreeModel = TreeModel;

				UGameplayStatics::FinishSpawningActor(Chunk, Transform);

				ChunkCount++;
			}
		}
	}
}
