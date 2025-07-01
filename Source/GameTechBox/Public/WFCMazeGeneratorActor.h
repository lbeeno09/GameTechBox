// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "WFCMazeGeneratorActor.generated.h"

UENUM(BlueprintType)
enum class EWFCSocketType : uint8
{
	None UMETA(DisplayName = "None"),
	Path UMETA(DisplayName = "Path"),
	Wall UMETA(DisplayName = "Wall"),
};

UENUM(BlueprintType)
enum class ETileShape : uint8
{
	Generic UMETA(DisplayName = "Generic"),
	Straight UMETA(DisplayName = "Straight"),
	TJunction UMETA(DisplayName = "TJunction"),
	Corner UMETA(DisplayName = "Corner"),
	Cross UMETA(DisplayName = "Cross"),
	Wall UMETA(DisplayName = "Wall"),
	Open UMETA(DisplayName = "Open"),
};

USTRUCT(BlueprintType)
struct FWFCTileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC Tile")
	FString TileName; // use for debugging

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC Tile")
	TSoftObjectPtr<UStaticMesh> TileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC Tile")
	TArray<EWFCSocketType> Sockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC Tile")
	ETileShape TileShape;

	FWFCTileData()
	{
		// top, right, bottom, left
		Sockets.SetNum(4);
		for(int32 i = 0; i < Sockets.Num(); i++)
		{
			Sockets[i] = EWFCSocketType::Wall;
		}
	}
};

USTRUCT()
struct FWFCCell
{
	GENERATED_BODY()

	TSet<int32> PossibleTileIndices;
	int32 CollapsedTileIndex;

	int32 GetEntropy() const { return PossibleTileIndices.Num(); }

	FWFCCell() : CollapsedTileIndex(-1) {}

	bool IsCollapsed() const { return CollapsedTileIndex != -1; }
};

UCLASS()
class GAMETECHBOX_API AWFCMazeGeneratorActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWFCMazeGeneratorActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Grid Dim
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC Settings")
	int32 GridWidth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC Settings")
	int32 GridHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WFC Settings")
	TArray<FWFCTileData> AvailableTiles;

	TArray<FWFCCell> GridCells; // index = y * GridWidth + x

	// Core WFC Functions
	UFUNCTION(CallInEditor, Category = "WFC Generation")
	void GenerateMaze();

	UFUNCTION(CallInEditor, Category = "WFC Generation")
	void ClearMaze();

protected:
	int32 GetCellIndex(int32 X, int32 Y) const;

	// init function
	void InitializeGrid();

	// main loop
	void RunWFC();

	// step 1: find cell with lowest entropy (if not yet collapsed)
	FWFCCell *FindLowestEntropyCell(int32 &OutX, int32 &OutY);

	// step 2: collapse chosen cell to a single tile
	void CollapseCell(int32 X, int32 Y, int32 ChosenTileIndex);

	// step 3: propagate changes to neighbors
	void Propagate(int32 StartX, int32 StartY);

	// Queue for cells that need resolving
	TArray<FIntPoint> PropagationQueue;

	bool AreSocketsCompatible(EWFCSocketType Socket1, EWFCSocketType Socket2) const;

	bool EnforceConstraints(int32 CellX, int32 CellY);

	// visuals
	void SpawnVisualTiles();

	UPROPERTY()
	TArray<UStaticMeshComponent *> SpawnedMazeTiles;
};
