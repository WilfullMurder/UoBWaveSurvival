// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PowerupActor.generated.h"


class UStaticMeshComponent;

UCLASS()
class SURVIVAL_API APowerupActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APowerupActor();

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Powerups")
		UStaticMeshComponent* Mesh;


	// Time between powerup ticks
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powerups")
		float PowerupInterval;

	// Total applications of Powerup effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powerups")
		int32 TotalNrOfTicks;

	FTimerHandle TH_PowerupTick;

	// Total number of ticks applied
	int32 TicksProcessed;

	UFUNCTION()
		void OnTickPowerup();

	// Tracks state of Powerup
	UPROPERTY(ReplicatedUsing = OnRep_PowerupActive)
		bool bIsPowerupActive;

	UFUNCTION()
		void OnRep_PowerupActive();

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
		void OnPowerupStateChanged(bool bNewIsActive);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ActivatePowerup(AActor* ActivateFor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
		void OnActivated(AActor* ActiveFor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
		void OnPowerupTicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
		void OnExpired();

};
