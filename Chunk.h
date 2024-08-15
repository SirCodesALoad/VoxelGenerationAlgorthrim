#pragma once

#include "CoreMinimal.h"
#include "ChunkMeshData.h"
#include "GameFramework/Actor.h"
#include "Enums.h"
#include "Engine/LevelStreamingVolume.h"
#include "Chunk.generated.h"

enum class EBlock;
enum class EDirectionl;
class FastNoiseLite;
class UProceduralMeshComponent;

UCLASS()
class AChunk : public AActor
{
	GENERATED_BODY()

	struct FMask
	{
		EBlock Block;
		int Normal;
	};
	
public:	
	// Sets default values for this actor's properties
	AChunk();

	UPROPERTY(EditAnywhere, Category="Chunk")
	int Size = 32;

	UPROPERTY(EditAnywhere, Category="Chunk")
	int Scale = 1;

	UPROPERTY(EditAnywhere, Category="Chunk")
	float NoiseFrequency = 0.03f;

	TObjectPtr<UMaterialInterface> Material;
	EGenerationType GenerationType = EGenerationType::Surface;
	TObjectPtr<UStaticMesh> TreeModel;
	
protected:
	virtual void BeginPlay() override;
	virtual void ModifyVoxelData(const FIntVector Position, const EBlock Block);

private:	
	TObjectPtr<UProceduralMeshComponent> Mesh;
	TObjectPtr<FastNoiseLite> Noise;

	TArray<EBlock> Blocks;
	FChunkMeshData MeshData;
	int VertexCount = 0;

	const FVector BlockVertexData[8] = {
		FVector(100, 100, 100),
		FVector(100, 0, 100),
		FVector(100, 0, 0),
		FVector(100, 100, 0),
		FVector(0, 0, 100),
		FVector(0, 100, 100),
		FVector(0, 100, 0),
		FVector(0, 0, 0)
	};

	const int BlockTriangleData[24] = {
		0, 1, 2, 3, //foward
		5, 0, 3, 6, // Right
		4, 5, 6, 7, // Back
		1, 4, 7, 2, // Left
		5, 4, 1, 0, // Up
		3, 2, 7, 6 // Down
	};

	// Populates block area according to the height map provided by the noise, Based on if it's on the surface or not.
	void GenerateSurfaceBlocks();
	void GenerateUndergroundBlocks();


	void GenerateMesh(); // Populate vertex data creating the Mesh

	void ApplyMesh() const; // Take vertex and index data and pass it to the Mesh component, where rendering happens.

	void SpawnTrees() const;
	
	bool CheckIfAir(FVector& Pos) const; // Helper method checks if a position contains an transparent or opaque block.
	bool CheckIfGrass(FVector& Pos) const; // Helper method checks if a position contains a grass block.


	
	void CreateFace(EDirection Direction, FVector& Pos);

	TArray<FVector> GetFaceVertices(EDirection Direction, FVector& Pos); // helper method gets the verties for a given face.

	FVector GetPositionInDirection(EDirection Direction, const FVector& Pos);
	
	void CreateQuad(FMask Mask, FIntVector AxisMask, int Width, int Height, FIntVector V1, FIntVector V2, FIntVector V3, FIntVector V4);
	
	int GetBlockIndex(int X, int Y, int Z) const;
	
	EBlock GetBlock(FIntVector Index) const;
	
	bool CompareMask(FMask M1, FMask M2) const;
	
	FColor GetColor(EBlock Block, FVector Normal) const;
	
};
