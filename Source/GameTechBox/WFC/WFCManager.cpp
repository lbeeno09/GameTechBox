// Fill out your copyright notice in the Description page of Project Settings.

#include "WFCManager.h"

// Sets default values
AWFCManager::AWFCManager()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

void AWFCManager::SetupComponents()
{
	if(!TileSetAsset)
	{
		return;
	}

	// Ensure only 1 HISMC for every tile
	int32 TargetCount = TileSetAsset->Tiles.Num();
	for(auto Comp : HISMCComponents)
	{
		if(Comp)
		{
			Comp->ClearInstances();
		}
	}

	for(int32 i = HISMCComponents.Num(); i < TargetCount; i++)
	{
		FName CompName = FName(*FString::Printf(TEXT("TileRenderer_%d"), i));
		UHierarchicalInstancedStaticMeshComponent* NewComp = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, CompName);

		NewComp->RegisterComponent();
		NewComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		HISMCComponents.Add(NewComp);
	}

	// Sync mesh from data asset
	for(int32 i = 0; i < TargetCount; i++)
	{
		if(HISMCComponents[i])
		{
			HISMCComponents[i]->SetStaticMesh(TileSetAsset->Tiles[i].Mesh);
		}
	}
}

void AWFCManager::InitializeGrid()
{
	Grid.Empty();
	Grid.SetNum(GridWidth * GridHeight);

	// All tiles mask is on
	uint64 InitialMask = (1ULL << Palette.Num()) - 1;
	for(auto& Node : Grid)
	{
		Node.Possibilities = InitialMask;
		Node.bCollapsed = false;
	}
}

void AWFCManager::GenerateMap()
{
	// Prep
	if(!TileSetAsset)
	{
		return;
	}

	SetupComponents();
	Palette.Empty();
	for(int32 i = 0; i < TileSetAsset->Tiles.Num(); i++)
	{
		FTileRule R;
		R.TileID = i;
		R.EdgesIDs[0] = TileSetAsset->Tiles[i].North;
		R.EdgesIDs[1] = TileSetAsset->Tiles[i].East;
		R.EdgesIDs[2] = TileSetAsset->Tiles[i].South;
		R.EdgesIDs[3] = TileSetAsset->Tiles[i].West;
		Palette.Add(R);
	}

	// Setup
	InitializeGrid();

	// Main Loop
	int32 Safety = 0;
	int32 MaxIter = GridWidth * GridHeight;
	while(Observe() && Safety < MaxIter)
	{
		Safety++;
	}

	// Final Rendering
	for(int32 i = 0; i < Grid.Num(); i++)
	{
		if(Grid[i].bCollapsed)
		{
			int32 TileIdx = Grid[i].FinalTileIndex;
			if(HISMCComponents.IsValidIndex(TileIdx))
			{
				int32 X = i % GridWidth;
				int32 Y = i / GridWidth;
				FTransform Transform(FVector(X * TileSize, Y * TileSize, 0));
				HISMCComponents[TileIdx]->AddInstance(Transform);
			}
		}
	}
}


bool AWFCManager::Observe()
{
	int32 LowEntropyIdx = -1;
	int32 MinEntropy = 999;
	// Find cell with fewest choices left
	for(int32 i = 0; i < Grid.Num(); i++)
	{
		if(!Grid[i].bCollapsed && Grid[i].GetEntropy() < MinEntropy)
		{
			MinEntropy = Grid[i].GetEntropy();
			LowEntropyIdx = i;
		}
	}
	if(LowEntropyIdx == -1 || MinEntropy == 0)
	{
		return false;
	}

	TArray<int32> PossibleIndices;
	for(int32 t = 0; t < Palette.Num(); t++)
	{
		if((Grid[LowEntropyIdx].Possibilities >> t) & 1)
		{
			PossibleIndices.Add(t);
		}
	}

	int32 Chosen = PossibleIndices[FMath::RandRange(0, PossibleIndices.Num() - 1)];
	Grid[LowEntropyIdx].Possibilities = (1ULL << Chosen);
	Grid[LowEntropyIdx].bCollapsed = true;
	Grid[LowEntropyIdx].FinalTileIndex = Chosen;

	Propagate(LowEntropyIdx);
	
	return true;
}

void AWFCManager::Propagate(int32 StartIdx)
{
	TArray<int32> Stack;
	Stack.Push(StartIdx);

	// N, E, S, W
	int32 DX[] = { 1, 0, -1, 0 };
	int32 DY[] = { 0, 1, 0, -1 };

	while(Stack.Num() > 0)
	{
		int32 CurrentIdx = Stack.Pop();
		for(int32 d = 0; d < 4; d++)
		{
			int32 NX = (CurrentIdx % GridWidth) + DX[d];
			int32 NY = (CurrentIdx / GridWidth) + DX[d];

			if(0 <= NX && NX < GridWidth && 0 <= NY && NY < GridHeight)
			{
				int32 NeighborIdx = GetIdx(NX, NY);
				FWFCNode& Neighbor = Grid[NeighborIdx];
				if(Neighbor.bCollapsed)
				{
					continue;
				}

				uint64 NewMask = 0;
				// Check all tile allowed in the neighbor
				for(int32 t = 0; t < Palette.Num(); t++)
				{
					if((Neighbor.Possibilities >> t) & 1)
					{
						if(HasValidConnection(t, Grid[CurrentIdx].Possibilities, d))
						{
							NewMask |= (1ULL << t);
						}
					}
				}

				if(NewMask != Neighbor.Possibilities)
				{
					Neighbor.Possibilities = NewMask;
					Stack.Push(NeighborIdx);
				}
			}
		}
	}
}

int32 AWFCManager::GetIdx(int32 X, int32 Y) const
{
	if(X < 0 || X >= GridWidth || Y < 0 || Y >= GridHeight)
	{
		return -1;
	}

	return Y * GridWidth + X;
}

bool AWFCManager::HasValidConnection(int32 MyTileIdx, uint64 NeighborPossibilities, int32 Direction)
{
	// Reminder: 0: N(+X), 1: E(+Y), 2: S(-X), 3: W(-Y)
	int32 OppositeDir = (Direction + 2) % 4;
	int32 MyEdge = Palette[MyTileIdx].EdgesIDs[Direction];
	for(int32 t = 0; t < Palette.Num(); t++)
	{
		if((NeighborPossibilities >> t) & 1)
		{
			int32 NeighborEdge = Palette[t].EdgesIDs[OppositeDir];
			if(MyEdge == NeighborEdge)
			{
				return true;
			}
		}
	}

	return false;
}
