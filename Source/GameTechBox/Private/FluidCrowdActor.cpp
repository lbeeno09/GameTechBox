// Fill out your copyright notice in the Description page of Project Settings.

#include "FluidCrowdActor.h

// Sets default values
AFluidCrowdActor::AFluidCrowdActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GridSize = 41;
	dx = 2 / (GridSize - 1);
	TimeStep = 25;
	dt = 0.025;
	WaveSpeed = 1;
}

// Called when the game starts or when spawned
void AFluidCrowdActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFluidCrowdActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

