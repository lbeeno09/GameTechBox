// Fill out your copyright notice in the Description page of Project Settings.

#include "WFCMazeGeneratorActor.h"
#include "Engine/StaticMeshActor.h"

// Sets default values
AWFCMazeGeneratorActor::AWFCMazeGeneratorActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	GridWidth = 10;
	GridHeight = 10;
	TileVisualScale = 1.0f;
	FallbackTileIndex = 0;

	bGenerationSuccessful = false;
}

// Called when the game starts or when spawned
void AWFCMazeGeneratorActor::BeginPlay()
{
	Super::BeginPlay();

	//GenerateMaze();
}

int32 AWFCMazeGeneratorActor::GetCellIndex(int32 X, int32 Y) const
{
	if(X < 0 || X >= GridWidth || Y < 0 || Y >= GridHeight)
	{
		return INDEX_NONE;
	}

	return Y * GridWidth + X;
}

void AWFCMazeGeneratorActor::GenerateMaze()
{
	UE_LOG(LogTemp, Log, TEXT("===== Starting WFC Maze Generation ====="));
	ClearGeneratedMaze();

	if(GridWidth <= 0 || GridHeight <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Grid dimensions must be positive (Width: %d, Height %d)."), GridWidth, GridHeight);
		return;
	}
	if(AvailableTiles.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No AvailableTiles defined! Please add tile definitions in Blueprint."));
		return;
	}

	InitializeGrid();
	RunWFC();
	SpawnVisualTiles();
	UE_LOG(LogTemp, Log, TEXT("===== WFC Maze Generation Complete ====="));
}

void AWFCMazeGeneratorActor::ClearGeneratedMaze()
{
	UE_LOG(LogTemp, Log, TEXT("Clearing previously generated maze..."));

	for(UStaticMeshComponent *MeshComp : SpawnedMazeTiles)
	{
		if(IsValid(MeshComp))
		{
			MeshComp->UnregisterComponent();
			MeshComp->DestroyComponent();
		}
	}
	SpawnedMazeTiles.Empty();

	UE_LOG(LogTemp, Log, TEXT("Finished clearing maze."));
}

void AWFCMazeGeneratorActor::InitializeGrid()
{
	GridCells.Empty();
	GridCells.SetNum(GridWidth * GridHeight);

	for(int32 i = 0; i < GridCells.Num(); i++)
	{
		GridCells[i] = FWFCCell();

		for(int32 TileIndex = 0; TileIndex < AvailableTiles.Num(); TileIndex++)
		{
			GridCells[i].PossibleTileIndices.Add(TileIndex);
		}

#ifdef UE_BUILD_DEBUG
		if(i < 5)
		{
			UE_LOG(LogTemp, Log, TEXT("Cell %d initialized with %d possibilities. IsCollapsed: %s"), i, GridCells[i].PossibleTileIndices.Num(), (GridCells[i].IsCollapsed() ? TEXT("True") : TEXT("False")));
		}
#endif

	}
	UE_LOG(LogTemp, Log, TEXT("Grid Initialized with all possibilities. AvailableTiles cound: %d"), AvailableTiles.Num());
}

void AWFCMazeGeneratorActor::RunWFC()
{
	bGenerationSuccessful = false;

	const int32 MaxLoopIterations = GridWidth * GridHeight * AvailableTiles.Num() * 2;
	int32 loopCount = 0;
	while(loopCount < MaxLoopIterations)
	{
		loopCount++;

		int32 CellX, CellY;
		FWFCCell *LowestEntropyCell = FindLowestEntropyCell(CellX, CellY);

		if(!LowestEntropyCell)
		{
			UE_LOG(LogTemp, Log, TEXT("No more non-collapsed cells! WFC complete."));
			break;
		}
		if(LowestEntropyCell->GetEntropy() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Contradiction: Cell (%d, %d) has 0 possibilities. Backtracking needed. Current iteration will simply stop."), CellX, CellY);
			// Right now it will stop
			break;
		}

		TArray<int32> PossibilitiesArray = LowestEntropyCell->PossibleTileIndices.Array();
		int32 ChosenTileIndex = PossibilitiesArray[FMath::RandRange(0, PossibilitiesArray.Num() - 1)];
		CollapseCell(CellX, CellY, ChosenTileIndex);

		Propagate(CellX, CellY);

		bool AllCollapsed = true;
		for(const FWFCCell &Cell : GridCells)
		{
			if(!Cell.IsCollapsed())
			{
				AllCollapsed = false;
				break;
			}
		}
		if(AllCollapsed)
		{
			UE_LOG(LogTemp, Log, TEXT("All cells collapsed. WFC Successful."));
			bGenerationSuccessful = true;
			break;
		}
	}

	if(!bGenerationSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("WFC failed to collapse all cells (Timeout or other issues) Max iterations reached: %d."), loopCount);
	}
}

FWFCCell *AWFCMazeGeneratorActor::FindLowestEntropyCell(int32 &OutX, int32 &OutY)
{
	FWFCCell *LowestEntropyCell = nullptr;
	int32 MinEntropy = MAX_int32;

	TArray<FIntPoint> Candidates;
	for(int32 Y = 0; Y < GridHeight; Y++)
	{
		for(int32 X = 0; X < GridWidth; X++)
		{
			int32 Index = GetCellIndex(X, Y);
			if(Index == INDEX_NONE)
			{
				continue;
			}

			FWFCCell &Cell = GridCells[Index];
			if(Cell.IsCollapsed())
			{
				continue;
			}

			int32 CurrentEntropy = Cell.GetEntropy();
			if(CurrentEntropy < MinEntropy)
			{
				MinEntropy = CurrentEntropy;
				Candidates.Empty();
				Candidates.Add(FIntPoint(X, Y));
			}
			else if(CurrentEntropy == MinEntropy)
			{
				Candidates.Add(FIntPoint(X, Y));
			}
		}
	}

	if(Candidates.Num() > 0)
	{
		FIntPoint ChosenPoint = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		OutX = ChosenPoint.X;
		OutY = ChosenPoint.Y;

		return &GridCells[GetCellIndex(OutX, OutY)];
	}

	return nullptr;
}

void AWFCMazeGeneratorActor::CollapseCell(int32 X, int32 Y, int32 ChosenTileIndex)
{
	int32 Index = GetCellIndex(X, Y);
	if(Index == INDEX_NONE)
	{
		return;
	}

	FWFCCell &Cell = GridCells[Index];
	Cell.CollapsedTileIndex = ChosenTileIndex;
	Cell.PossibleTileIndices.Empty();
	Cell.PossibleTileIndices.Add(ChosenTileIndex);
	UE_LOG(LogTemp, Log, TEXT("Collapsed Cell (%d, %d) to Tile Index %d."), X, Y, ChosenTileIndex);

	PropagationQueue.AddUnique(FIntPoint(X, Y));
}

void AWFCMazeGeneratorActor::Propagate(int32 StartX, int32 StartY)
{
	PropagationQueue.AddUnique(FIntPoint(StartX, StartY));
	while(PropagationQueue.Num() > 0)
	{
		FIntPoint CurrentPoint = PropagationQueue.Pop(false);
		for(int32 Direction = 0; Direction < 4; Direction++)
		{
			int32 NeighborX = CurrentPoint.X;
			int32 NeighborY = CurrentPoint.Y;
			switch(Direction)
			{
				case 0:
					NeighborY--;
					break;
				case 1:
					NeighborX++;
					break;
				case 2:
					NeighborY++;
					break;
				case 3:
					NeighborX--;
					break;
			}

			int32 NeighborIdx = GetCellIndex(NeighborX, NeighborY);
			if(NeighborIdx == INDEX_NONE)
			{
				continue;
			}

			FWFCCell &NeighborCell = GridCells[NeighborIdx];
			if(NeighborCell.IsCollapsed())
			{
				continue;
			}

			if(!EnforceConstraints(NeighborX, NeighborY))
			{
				continue;
			}

			PropagationQueue.AddUnique(FIntPoint(NeighborX, NeighborY));
		}
	}
}

bool AWFCMazeGeneratorActor::EnforceConstraints(int32 CellX, int32 CellY)
{
	int32 CellIndex = GetCellIndex(CellX, CellY);
	if(CellIndex == INDEX_NONE)
	{
		return false;
	}

	FWFCCell &CurrentCell = GridCells[CellIndex];
	if(CurrentCell.IsCollapsed())
	{
		return false;
	}

	TSet<int32> NewPossibilities;

	const bool bOnTopEdge = (CellY == 0);
	const bool bOnBottomEdge = (CellY == GridHeight - 1);
	const bool bOnLeftEdge = (CellX == 0);
	const bool bOnRightEdge = (CellX == GridWidth - 1);

	const bool bIsCorner = (bOnTopEdge && bOnLeftEdge) || (bOnTopEdge && bOnRightEdge) || (bOnBottomEdge && bOnLeftEdge) || (bOnBottomEdge && bOnRightEdge);
	const bool bIsOnAnyEdge = bOnTopEdge || bOnBottomEdge || bOnLeftEdge || bOnRightEdge;


	UE_LOG(LogTemp, Log, TEXT("Enforcing constraints for Cell (%d, %d0. Initial possibilities: %d"), CellX, CellY, CurrentCell.PossibleTileIndices.Num());
	for(int32 PossibleTileIdx : CurrentCell.PossibleTileIndices)
	{
		bool bIsStillPossible = true;
		const FWFCTileData &CurrentTileDef = AvailableTiles[PossibleTileIdx];

		UE_LOG(LogTemp, Log, TEXT("    Checking Tile '%s' (Idx %d, Shape: %s) for Cell (%d, %d). Sockets: [T:%s, R:%s, B:%s, L:%s]"),
			*CurrentTileDef.TileName, PossibleTileIdx, *UEnum::GetValueAsString(CurrentTileDef.TileShape), CellX, CellY,
			*UEnum::GetValueAsString(CurrentTileDef.Sockets[0]), *UEnum::GetValueAsString(CurrentTileDef.Sockets[1]),
			*UEnum::GetValueAsString(CurrentTileDef.Sockets[2]), *UEnum::GetValueAsString(CurrentTileDef.Sockets[3]));
		// 1. Check Neighbors
		for(int32 Direction = 0; Direction < 4; Direction++)
		{
			int32 NeighborX = CellX;
			int32 NeighborY = CellY;
			int32 OppositeDirection = -1;

			switch(Direction)
			{
			case 0:
				NeighborY--;
				OppositeDirection = 2;
				break;
			case 1:
				NeighborX++;
				OppositeDirection = 3;
				break;
			case 2:
				NeighborY++;
				OppositeDirection = 0;
				break;
			case 3:
				NeighborX--;
				OppositeDirection = 1;
				break;
			}

			int32 NeighborIdx = GetCellIndex(NeighborX, NeighborY);
			if(NeighborIdx != INDEX_NONE)
			{
				const FWFCCell &NeighborCell = GridCells[NeighborIdx];
				bool bFoundCompatibleNeighborTile = false;

				for(int32 NeighborPossibleTileIdx : NeighborCell.PossibleTileIndices)
				{
					const FWFCTileData &NeighborTileDef = AvailableTiles[NeighborPossibleTileIdx];
					if(AreSocketsCompatible(CurrentTileDef.Sockets[Direction], NeighborTileDef.Sockets[OppositeDirection]))
					{
						bFoundCompatibleNeighborTile = true;
						break;
					}
				}

				if(!bFoundCompatibleNeighborTile)
				{
					bIsStillPossible = false;
					UE_LOG(LogTemp, Log, TEXT("    Tile '%s' for Cell (%d, %d) removed: Incompatible with Neighbor (%d, %d) in Direction %d."),
						*CurrentTileDef.TileName, CellX, CellY, NeighborX, NeighborY, Direction);
					break;
				}
			}
		}

		if(!bIsStillPossible)
		{
			continue;
		}

		//2: Apply Edge/Corner Rules
		if(bIsOnAnyEdge)
		{
			UE_LOG(LogTemp, Log, TEXT("    Cell (%d, %d) is on an edge. Applying boundary rules for Tile '%s'."), CellX, CellY, *CurrentTileDef.TileName);

			if(bIsCorner)
			{
				if(CurrentTileDef.TileShape != ETileShape::Corner)
				{
					bIsStillPossible = false;
					UE_LOG(LogTemp, Warning, TEXT("    Tile '%s' for Cell (%d, %d) removed: Not a Corner shape for a corner cell"), *CurrentTileDef.TileName, CellX, CellY);
				}
				else
				{
					// brute force all combination
					bool bCorrectCornerOrientation = false;
					if(bOnTopEdge && bOnLeftEdge)
					{
						if(CurrentTileDef.Sockets[0] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Wall)
						{
							bCorrectCornerOrientation = true;
						}
					}
					else if(bOnTopEdge && bOnRightEdge)
					{
						if(CurrentTileDef.Sockets[0] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Path)
						{
							bCorrectCornerOrientation = true;
						}
					}
					else if(bOnBottomEdge && bOnLeftEdge)
					{
						if(CurrentTileDef.Sockets[0] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Wall)
						{
							bCorrectCornerOrientation = true;
						}
					}
					else if(bOnBottomEdge && bOnRightEdge)
					{
						if(CurrentTileDef.Sockets[0] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Wall)
						{
							bCorrectCornerOrientation = true;
						}
					}

					if(!bCorrectCornerOrientation)
					{
						bIsStillPossible = false;
						UE_LOG(LogTemp, Warning, TEXT("    Tile '%s' for Cell(%d, %d) removed: Incorrect Corner orientation for this specific corner."), *CurrentTileDef.TileName, CellX, CellY);
					}
				}
			}
			else // Edge cells
			{
				if(!(CurrentTileDef.TileShape == ETileShape::Straight || CurrentTileDef.TileShape == ETileShape::TJunction))
				{
					bIsStillPossible = false;
					UE_LOG(LogTemp, Warning, TEXT("    Tile '%s' for Cell (%d, %d) removed: Not Straight/TJunction for a non-corner edge cell"), *CurrentTileDef.TileName, CellX, CellY);

				}
				else
				{
					bool bCorrectEdgeOrientation = false;

					if(CurrentTileDef.TileShape == ETileShape::Straight)
					{
						if(bOnTopEdge &&
							CurrentTileDef.Sockets[0] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Path)
						{
							bCorrectEdgeOrientation = true;
						}
						else if(bOnBottomEdge &&
							CurrentTileDef.Sockets[0] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Path)
						{
							bCorrectEdgeOrientation = true;
						}
						else if(bOnLeftEdge &&
							CurrentTileDef.Sockets[0] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Wall)
						{
							bCorrectEdgeOrientation = true;
						}
						else if(bOnRightEdge &&
							CurrentTileDef.Sockets[0] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Wall)
						{
							bCorrectEdgeOrientation = true;
						}
					}
					else if(CurrentTileDef.TileShape == ETileShape::TJunction)
					{
						if(bOnTopEdge &&
							CurrentTileDef.Sockets[0] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Path)
						{
							bCorrectEdgeOrientation = true;
						}
						else if(bOnBottomEdge &&
							CurrentTileDef.Sockets[0] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Path)
						{
							bCorrectEdgeOrientation = true;
						}
						else if(bOnLeftEdge &&
							CurrentTileDef.Sockets[0] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Wall)
						{
							bCorrectEdgeOrientation = true;
						}
						else if(bOnRightEdge &&
							CurrentTileDef.Sockets[0] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[1] == EWFCSocketType::Wall &&
							CurrentTileDef.Sockets[2] == EWFCSocketType::Path &&
							CurrentTileDef.Sockets[3] == EWFCSocketType::Path)
						{
							bCorrectEdgeOrientation = true;
						}
					}
					else
					{
						bCorrectEdgeOrientation = false;
					}

					if(!bCorrectEdgeOrientation)
					{
						bIsStillPossible = false;
						UE_LOG(LogTemp, Warning, TEXT("    Tile '%s' for Cell(%d, %d) removed: Incorrect Straight/TJunctioni orientation for this specific edge."), *CurrentTileDef.TileName, CellX, CellY);
					}
				}
			}
		}

		if(bIsStillPossible)
		{
			NewPossibilities.Add(PossibleTileIdx);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Enforcing constraints for Cell (%d, %d). Final possibilities: %d"), CellX, CellY, NewPossibilities.Num());

	bool bChanged = NewPossibilities.Num() != CurrentCell.PossibleTileIndices.Num();
	CurrentCell.PossibleTileIndices = NewPossibilities;

	if(CurrentCell.GetEntropy() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("EnforceConstraints led to contradiction for Cell (%d, %d)! No possible tiles remain."), CellX, CellY);
	}

	return bChanged;
}

bool AWFCMazeGeneratorActor::AreSocketsCompatible(EWFCSocketType Socket1, EWFCSocketType Socket2) const
{
	return Socket1 == Socket2 && Socket1 != EWFCSocketType::None;
}

void AWFCMazeGeneratorActor::SpawnVisualTiles()
{
	float TileSize = 100.0f;
	for(int32 Y = 0; Y < GridHeight; Y++)
	{
		for(int32 X = 0; X < GridWidth; X++)
		{
			int32 Index = GetCellIndex(X, Y);
			if(Index == INDEX_NONE)
			{
				continue;
			}

			const FWFCCell &Cell = GridCells[Index];
			int32 TileToSpawnIndex = FallbackTileIndex;
			
			if(Cell.IsCollapsed())
			{
				TileToSpawnIndex = Cell.CollapsedTileIndex;
			}
			else
			{
				if(Cell.GetEntropy() == 0)
				{
					UE_LOG(LogTemp, Error, TEXT("Cell (%d, %d) has 0 possibilities (contradiction) and will use Fallback Tile."), X, Y);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Cell (%d, %d) remains uncollapsed with %d possibilities and will use Fallback Tile."), X, Y, Cell.GetEntropy());
				}
			}

			if(!AvailableTiles.IsValidIndex(TileToSpawnIndex))
			{
				UE_LOG(LogTemp, Error, TEXT("Tile index %d for cell (%d,%d) is out of bounds for AvailableTiles! Check Blueprint config and FallbackTileIndex."), TileToSpawnIndex, X, Y);
				continue;
			}

			const FWFCTileData &TileData = AvailableTiles[TileToSpawnIndex];
			if(!TileData.TileMesh.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("Mesh for Tile '%s' (Index %d) at (%d,%d) is not valid/loaded. Check Blueprint config."), *TileData.TileName, TileToSpawnIndex, X, Y);
				continue;
			}

			UStaticMeshComponent *NewMesh = NewObject<UStaticMeshComponent>(this);
			NewMesh->SetStaticMesh(TileData.TileMesh.LoadSynchronous());

			NewMesh->SetWorldLocation(GetActorLocation() + FVector(X * TileSize, Y * TileSize, 0.0f));
			NewMesh->SetWorldScale3D(FVector(TileVisualScale));

			NewMesh->SetupAttachment(GetRootComponent());
			NewMesh->RegisterComponent();

			SpawnedMazeTiles.Add(NewMesh);
		}
	}
}