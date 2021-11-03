// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupActor.h"
#include "PowerupActor.h"
#include "SurvivalGameCharacter.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
APickupActor::APickupActor()
{
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphereComponent"));
	Collision->SetSphereRadius(75.0f);
	RootComponent = Collision;

	SetReplicates(true);

}

// Called when the game starts or when spawned
void APickupActor::BeginPlay()
{
	Super::BeginPlay();

	for (int i = 0; i < PowerupClass.Num(); i++)
	{
		if (!PowerupClass[i])
		{
			UE_LOG(LogTemp, Warning, TEXT("PowerupClass does not exist. Pleas rectify in editor!"));
			return;
		}
	}

	if (HasAuthority())
	{

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		int RandIndex = FMath::RandRange(0, PowerupClass.Num() - 1);

		PowerupInstance = GetWorld()->SpawnActor<APowerupActor>(PowerupClass[RandIndex], GetTransform(), SpawnParams);
	}

	
}


void APickupActor::NotifyActorBeginOverlap(AActor* OtherActor)
 {
	Super::NotifyActorBeginOverlap(OtherActor);

	if (HasAuthority() && PowerupInstance)
	{
		ASurvivalGameCharacter* Player = Cast<ASurvivalGameCharacter>(OtherActor);
		if (Player && !Player->bIsEnemy)
		{
			PowerupInstance->ActivatePowerup(OtherActor);
			PowerupInstance = nullptr;
		}
	}
}

