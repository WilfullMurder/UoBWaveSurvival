// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupActor.generated.h"

class USphereComponent;
class APowerupActor;


UCLASS()
class SURVIVAL_API APickupActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickupActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* Collision;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PickupActor")
		TArray<TSubclassOf<APowerupActor>> PowerupClass;

	APowerupActor* PowerupInstance;

public:	

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

		
};
