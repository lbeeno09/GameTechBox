// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Actor.h"
#include "LobbyDoorActor.generated.h"

UCLASS()
class GAMETECHBOX_API ALobbyDoorActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALobbyDoorActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
	FName TargetMapName;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent *CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UTextRenderComponent *MapNameText;
};
