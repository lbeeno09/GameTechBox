// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidCrowdActor.generated.h"

UCLASS()
class GAMETECHBOX_API AFluidCrowdActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFluidCrowdActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


protected:
	int GridSize;
	double dx;
	int TimeStep;
	double dt;
	double WaveSpeed;
};
