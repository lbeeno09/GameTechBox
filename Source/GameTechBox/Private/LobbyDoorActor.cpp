// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyDoorActor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

// Sets default values
ALobbyDoorActor::ALobbyDoorActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	RootComponent = DoorMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Game/StarterContent/Architecture/Wall_Door_400x400'"));
	if(MeshAsset.Succeeded())
	{
		DoorMesh->SetStaticMesh(MeshAsset.Object);
		DoorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		DoorMesh->SetWorldScale3D(FVector(0.5f, 1.0f, 0.5f));
	}

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	CollisionBox->SetCollisionProfileName(TEXT("Trigger"));
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ALobbyDoorActor::OnOverlapBegin);

	MapNameText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MapNameText"));
	MapNameText->SetupAttachment(RootComponent);
	MapNameText->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	MapNameText->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	MapNameText->SetHorizontalAlignment(EHTA_Center);
	MapNameText->SetVerticalAlignment(EVRTA_TextCenter);
	MapNameText->SetWorldSize(50.0f);
	MapNameText->SetTextRenderColor(FColor::Black);

	TargetMapName = FName(TEXT("LobbyMap"));
}

// Called when the game starts or when spawned
void ALobbyDoorActor::BeginPlay()
{
	Super::BeginPlay();

	if(MapNameText)
	{
		MapNameText->SetText(FText::FromName(TargetMapName));
	}
}

#if WITH_EDITOR
void ALobbyDoorActor::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(PropertyChangedEvent.Property == nullptr || PropertyChangedEvent.Property->GetFName() != GET_MEMBER_NAME_CHECKED(ALobbyDoorActor, TargetMapName))
	{
		return;
	}
	if(!MapNameText)
	{
		return;
	}

	MapNameText->SetText(FText::FromName(TargetMapName));
}
#endif

void ALobbyDoorActor::OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	if(!OtherActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Overlap detected! OtherActor is NULL."));
	}

	// Check if collision was player
	APlayerController *PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if(!PlayerController || OtherActor != PlayerController->GetPawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("Overlap detected, but not by the local player's Pawn. OtherActor: %s"), *OtherActor->GetName());
	}

	UE_LOG(LogTemp, Log, TEXT("Local player's Pawn overlapped. Attempting to open map: %s"), *TargetMapName.ToString());
	if(TargetMapName.IsNone())
	{
		UE_LOG(LogTemp, Log, TEXT("TargetMapName is not set for this door!"));
	}


	UGameplayStatics::OpenLevel(this, TargetMapName, true);
}
