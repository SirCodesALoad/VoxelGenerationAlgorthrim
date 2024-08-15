// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Enums.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"

#include "ChunkWorld.generated.h"

class AChunk;

UCLASS()
class AChunkWorld final : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditInstanceOnly, Category="World")
	TSubclassOf<AChunk> ChunkType;
	
	UPROPERTY(EditInstanceOnly, Category="World")
	int DrawDistance = 5;
	
	UPROPERTY(EditInstanceOnly, Category="Chunk")
	int Size = 32;

	UPROPERTY(EditInstanceOnly, Category="Height Map")
	float NoiseFrequency = 0.03f;

	UPROPERTY(EditInstanceOnly, Category="Chunk")
	TObjectPtr<UMaterialInterface> Material;
	UPROPERTY(EditInstanceOnly, Category="Chunk")
	TObjectPtr<UStaticMesh> TreeModel;

	UPROPERTY(EditInstanceOnly, Category="Chunk")
	ANavMeshBoundsVolume* NavMeshBounds;

	UPROPERTY(EditInstanceOnly, Category="Chunk")
	ARecastNavMesh* RecastNavMesh;

	
	// Sets default values for this actor's properties
	AChunkWorld();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void UpdateNavMesh();

private:
	int ChunkCount;
	
	void Generate3DWorld();
};