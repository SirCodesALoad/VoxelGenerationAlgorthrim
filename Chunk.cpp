#include "Chunk.h"
#include "FastNoiseLite.h"
#include "ProceduralMeshComponent.h"
#include "../../../../../../../Program Files/Unreal Engine/UE_5.2/Engine/Plugins/Media/BlackmagicMedia/Source/ThirdParty/Build/Include/DeckLinkAPI_h.h"
#include "Engine/StaticMeshActor.h"

// Sets default values
AChunk::AChunk()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
	Noise = new FastNoiseLite();

	Blocks.SetNum(Size * Size * Size);

	Mesh->SetCastShadow(true);
	
}

void AChunk::BeginPlay()
{
	Noise->SetFrequency(NoiseFrequency);
	Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	Noise->SetFractalType(FastNoiseLite::FractalType_FBm);
	Noise->SetSeed(rand());

	if (GenerationType == EGenerationType::Surface)
	{
		GenerateSurfaceBlocks();
		SpawnTrees();
	}
	else
	{
		GenerateUndergroundBlocks();
	}

	GenerateMesh();

	ApplyMesh();
}

void AChunk::GenerateSurfaceBlocks()
{
	const FVector Location = GetActorLocation();

	for (int x = 0; x < Size; ++x)
	{
		for (int y = 0; y < Size; ++y)
		{
			const float XPos = x + (Location.X / 100);
			const float YPos = y + (Location.Y / 100);

			const int Height = FMath::Clamp(FMath::RoundToInt((Noise->GetNoise(XPos, YPos) + 1) * Size / 2), Size * 25 / 100, Size);
			for (int z = 0; z < Size; z++)
			{
				if(z < Height - 3)
				{
					Blocks[GetBlockIndex(x,y,z)] = EBlock::Stone;
				}
				else if (z < Height - 1)
				{
					Blocks[GetBlockIndex(x,y,z)] = EBlock::Dirt;
				}
				else if (z == Height - 1 )
				{
					Blocks[GetBlockIndex(x,y,z)] = EBlock::Grass;
				}
				else
				{
					Blocks[GetBlockIndex(x,y,z)] = EBlock::Air;
				}
			}
		}
	}
}

void AChunk::GenerateUndergroundBlocks()
{
	const FVector Position = GetActorLocation();

	for (int x = 0; x < Size; ++x)
	{
		for (int y = 0; y < Size; ++y)
		{
			for (int z = 0; z < Size; ++z)
			{
				const float NoiseValue = Noise->GetNoise(x + Position.X, y + Position.Y, z + Position.Z);

				if (NoiseValue >= 0)
				{
					Blocks[GetBlockIndex(x, y, z)] = EBlock::Air;
				}
				else
				{
					Blocks[GetBlockIndex(x, y, z)] = EBlock::Stone;
				}
			}
		}
	}
}

//Greedy meshing, from 3rd party project.
void AChunk::GenerateMesh()
{
	// Sweep over each axis (X, Y, Z)
	for (int Axis = 0; Axis < 3; ++Axis)
	{
		// 2 Perpendicular axis
		const int Axis1 = (Axis + 1) % 3;
		const int Axis2 = (Axis + 2) % 3;

		const int MainAxisLimit = Size;
		const int Axis1Limit = Size;
		const int Axis2Limit = Size;

		auto DeltaAxis1 = FIntVector::ZeroValue;
		auto DeltaAxis2 = FIntVector::ZeroValue;

		auto ChunkItr = FIntVector::ZeroValue;
		auto AxisMask = FIntVector::ZeroValue;

		AxisMask[Axis] = 1;

		TArray<FMask> Mask;
		Mask.SetNum(Axis1Limit * Axis2Limit);

		// Check each slice of the chunk
		for (ChunkItr[Axis] = -1; ChunkItr[Axis] < MainAxisLimit;)
		{
			int N = 0;

			// Compute Mask
			for (ChunkItr[Axis2] = 0; ChunkItr[Axis2] < Axis2Limit; ++ChunkItr[Axis2])
			{
				for (ChunkItr[Axis1] = 0; ChunkItr[Axis1] < Axis1Limit; ++ChunkItr[Axis1])
				{
					const EBlock CurrentBlock = GetBlock(ChunkItr);
					const EBlock CompareBlock = GetBlock(ChunkItr + AxisMask);

					const bool CurrentBlockOpaque = CurrentBlock != EBlock::Air;
					const bool CompareBlockOpaque = CompareBlock != EBlock::Air;

					if (CurrentBlockOpaque == CompareBlockOpaque)
					{
						Mask[N++] = FMask{EBlock::Null, 0};
					}
					else if (CurrentBlockOpaque)
					{
						Mask[N++] = FMask{CurrentBlock, 1};
					}
					else
					{
						Mask[N++] = FMask{CompareBlock, -1};
					}
				}
			}

			++ChunkItr[Axis];
			N = 0;

			// Generate Mesh From Mask
			for (int j = 0; j < Axis2Limit; ++j)
			{
				for (int i = 0; i < Axis1Limit;)
				{
					if (Mask[N].Normal != 0)
					{
						const auto CurrentMask = Mask[N];
						ChunkItr[Axis1] = i;
						ChunkItr[Axis2] = j;

						int Width;

						for (Width = 1; i + Width < Axis1Limit && CompareMask(Mask[N + Width], CurrentMask); ++Width)
						{
						}

						int Height;
						bool Done = false;

						for (Height = 1; j + Height < Axis2Limit; ++Height)
						{
							for (int k = 0; k < Width; ++k)
							{
								if (CompareMask(Mask[N + k + Height * Axis1Limit], CurrentMask)) continue;

								Done = true;
								break;
							}

							if (Done) break;
						}

						DeltaAxis1[Axis1] = Width;
						DeltaAxis2[Axis2] = Height;

						CreateQuad(
							CurrentMask, AxisMask, Width, Height,
							ChunkItr,
							ChunkItr + DeltaAxis1,
							ChunkItr + DeltaAxis2,
							ChunkItr + DeltaAxis1 + DeltaAxis2
						);

						DeltaAxis1 = FIntVector::ZeroValue;
						DeltaAxis2 = FIntVector::ZeroValue;

						for (int l = 0; l < Height; ++l)
						{
							for (int k = 0; k < Width; ++k)
							{
								Mask[N + k + l * Axis1Limit] = FMask{EBlock::Null, 0};
							}
						}

						i += Width;
						N += Width;
					}
					else
					{
						i++;
						N++;
					}
				}
			}
		}
	}
}

void AChunk::ApplyMesh() const
{
	Mesh->SetMaterial(0,Material);
	Mesh->CreateMeshSection(
		0,
		MeshData.Vertices,
		MeshData.Triangles,
		MeshData.Normals,
		MeshData.UV0,
		MeshData.Colors,
		TArray<FProcMeshTangent>(),
		true
	);
}

void AChunk::SpawnTrees() const
{
	// Really should be aware of other chunks.
	const FVector Location = GetActorLocation();
	FVector Position;
	for (int x = 0; x < Size; ++x)
	{
		for (int y = 0; y < Size; ++y)
		{
			for (int z = 0; z < Size; ++z)
			{
				Position = FVector(x,y,z);

				if (CheckIfGrass(Position))
				{
					FVector TreePosition;
					bool isValidTree = true;
					for (int tX = x; tX >= x + 3 && isValidTree; tX++)
					{
						for (int tY = y; tY >= y + 3 && isValidTree; tY++)
						{
							for (int tZ = z + 1; tZ >= z + 5 && isValidTree; tZ++)
							{
								TreePosition = FVector(tX,tY,tZ);
								if (!CheckIfAir(TreePosition))
								{
									isValidTree = false;
								}
							}
						}
					}
					if (isValidTree)
					{
						const float XPos = (x * 100 + Location.X);
						const float YPos = (y * 100 + Location.Y);
						const float zPos = ((z + 1) * 100 + Location.Z);

						TreePosition = FVector(XPos,YPos,zPos);
						
						AStaticMeshActor* TreeMesh = GetWorld()->SpawnActor<AStaticMeshActor>(TreePosition, FRotator(0, 0, 0));
						TreeMesh->GetStaticMeshComponent()->SetStaticMesh(TreeModel);
						x += 4;
						y += 4;
						break;
					}
								
				}
			}
		}
	}
	
}

bool AChunk::CheckIfAir(FVector& Pos) const
{
	if (Pos.X >= Size || Pos.Y >= Size || Pos.Z >= Size || Pos.X < 0 || Pos.Y < 0 || Pos.Z < 0)
	{
		return true;
	}
    return Blocks[GetBlockIndex(Pos.X, Pos.Y, Pos.Z)] == EBlock::Air;
}

bool AChunk::CheckIfGrass(FVector& Pos) const
{
	if (Pos.X >= Size || Pos.Y >= Size || Pos.Z >= Size || Pos.X < 0 || Pos.Y < 0 || Pos.Z < 0)
	{
		return false;
	}
	return Blocks[GetBlockIndex(Pos.X, Pos.Y, Pos.Z)] == EBlock::Grass;
}

void AChunk::CreateFace(EDirection Direction, FVector& Pos)
{
	MeshData.Vertices.Append(GetFaceVertices(Direction, Pos));
	MeshData.UV0.Append({
		FVector2D(1,1),
		FVector2D(1,0),
		FVector2d(0,0),
		FVector2D(0,1)
	});
	MeshData.Triangles.Append({
		VertexCount + 3,
		VertexCount + 2,
		VertexCount,
		VertexCount + 2,
		VertexCount + 1,
		VertexCount
	});
	VertexCount += 4;
}

TArray<FVector> AChunk::GetFaceVertices(EDirection Direction, FVector& Pos)
{
	TArray<FVector> Vertices;
	int i = 0;
	do {
		Vertices.Add(BlockVertexData[BlockTriangleData[i + static_cast<int>(Direction) * 4]] * Scale + Pos);
		++i;
	}
	while (i < 4);
    
	return Vertices;
}

FVector AChunk::GetPositionInDirection(EDirection Direction, const FVector& Pos)
{
	FVector NewPos = Pos;
	switch (Direction)
	{
		case EDirection::Forward: NewPos += FVector::ForwardVector;		
		case EDirection::Right: NewPos += FVector::RightVector;
		case EDirection::Back: NewPos += FVector::BackwardVector;
		case EDirection::Left: NewPos += FVector::LeftVector;
		case EDirection::Up: NewPos += FVector::UpVector;
		case EDirection::Down: NewPos += FVector::DownVector;
	}
    return NewPos;
}

void AChunk::CreateQuad(
	const FMask Mask,
	const FIntVector AxisMask,
	const int Width,
	const int Height,
	const FIntVector V1,
	const FIntVector V2,
	const FIntVector V3,
	const FIntVector V4
)
{
	const auto Normal = FVector(AxisMask * Mask.Normal);
	const auto Color = FColor(GetColor(Mask.Block, Normal));

	MeshData.Vertices.Append({
		FVector(V1) * 100,
		FVector(V2) * 100,
		FVector(V3) * 100,
		FVector(V4) * 100
	});

	MeshData.Triangles.Append({
		VertexCount,
		VertexCount + 2 + Mask.Normal,
		VertexCount + 2 - Mask.Normal,
		VertexCount + 3,
		VertexCount + 1 - Mask.Normal,
		VertexCount + 1 + Mask.Normal
	});

	MeshData.Normals.Append({
		Normal,
		Normal,
		Normal,
		Normal
	});

	MeshData.Colors.Append({
		Color,
		Color,
		Color,
		Color
	});

	if (Normal.X == 1 || Normal.X == -1)
	{
		MeshData.UV0.Append({
			FVector2D(Width, Height),
			FVector2D(0, Height),
			FVector2D(Width, 0),
			FVector2D(0, 0),
		});
	}
	else
	{
		MeshData.UV0.Append({
			FVector2D(Height, Width),
			FVector2D(Height, 0),
			FVector2D(0, Width),
			FVector2D(0, 0),
		});
	}

	VertexCount += 4;
}

void AChunk::ModifyVoxelData(const FIntVector Position, const EBlock Block)
{
	const int Index = GetBlockIndex(Position.X, Position.Y, Position.Z);
	
	Blocks[Index] = Block;
}

int AChunk::GetBlockIndex(const int X, const int Y, const int Z) const
{
	return Z * Size * Size + Y * Size + X;
}

EBlock AChunk::GetBlock(const FIntVector Index) const
{
	if (Index.X >= Size || Index.Y >= Size || Index.Z >= Size || Index.X < 0 || Index.Y < 0 || Index.Z < 0)
		return EBlock::Air;
	return Blocks[GetBlockIndex(Index.X, Index.Y, Index.Z)];
}

bool AChunk::CompareMask(const FMask M1, const FMask M2) const
{
	return M1.Block == M2.Block && M1.Normal == M2.Normal;
}

FColor AChunk::GetColor(const EBlock Block, const FVector Normal) const
{
	switch (Block) {
	case EBlock::Grass:
		{
			if (Normal == FVector::UpVector) return FColor::Green;
			return FColor::Black;
		}
	case EBlock::Dirt: return FColor::Black;
;
	case EBlock::Stone: return FColor::White;
	default: return FColor::Transparent;
	}
}