// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WFCLogic.h"
#include "WFCTileSet.generated.h"

USTRUCT(BlueprintType)
struct FTileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere)
	int32 North;

	UPROPERTY(EditAnywhere)
	int32 East;

	UPROPERTY(EditAnywhere)
	int32 South;

	UPROPERTY(EditAnywhere)
	int32 West;
};

/**
 * 
 */
UCLASS()
class GAMETECHBOX_API UWFCTileSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<FTileData> Tiles;
};
