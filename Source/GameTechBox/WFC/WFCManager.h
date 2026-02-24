// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WFCTileSet.h"
#include "WFCLogic.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "WFCManager.generated.h"

UCLASS()
class GAMETECHBOX_API AWFCManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AWFCManager();

	UPROPERTY(EditAnywhere, Category="WFC Settings")
	int32 GridWidth = 10;

	UPROPERTY(EditAnywhere, Category = "WFC Settings")
	int32 GridHeight = 10;

	UPROPERTY(EditAnywhere, Category = "WFC Settings")
	float TileSize = 400.0f;

	UPROPERTY(EditAnywhere, Category = "WFC Settings")
	UWFCTileSet* TileSetAsset;

	UFUNCTION(CallInEditor, Category="WFC Actions")
	void GenerateMap();

protected:
	TArray<FWFCNode> Grid;
	TArray<FTileRule> Palette;

	UPROPERTY()
	TArray<UHierarchicalInstancedStaticMeshComponent*> HISMCComponents;

	void InitializeGrid();
	bool Observe();
	void Propagate(int32 StartIdx);

	int32 GetIdx(int32 X, int32 Y) const;
	bool HasValidConnection(int32 MyTileIdx, uint64 NeighborPossibilities, int32 Direction);
	void SetupComponents();
};
