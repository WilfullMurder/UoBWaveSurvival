// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "ExplosiveAsset.generated.h"


class UHealthComponent;
class UStaticMeshComponent;
class URadialForceComponent;
class UParticleSystem;


UCLASS()
class SURVIVAL_API AExplosiveAsset : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AExplosiveAsset();

protected:



	UPROPERTY(VisibleAnywhere, Category = "Components")
		UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
		UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
		URadialForceComponent* RadialForceComponent;

	UFUNCTION()
		void OnHealthChanged(UHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType,
			class AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY(ReplicatedUsing = OnRep_Exploded)
		bool bExploded;


	UFUNCTION()
		void OnRep_Exploded();

	//Impulse applied to the barrel mesh when it explodes
	UPROPERTY(EditDefaultsOnly, Category = "FX")
		float ExplosionImpulse;

	//VFX to play when health <= zero
	UPROPERTY(EditDefaultsOnly, Category = "FX")
		UParticleSystem* ExplosionEffect;

	//Material replacing original mesh once exploded
	UPROPERTY(EditDefaultsOnly, Category = "FX")
		UMaterialInterface* ExplodedMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
		float ExplosionDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
		float ExplosionDamageRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
		TSubclassOf<class UDamageType> CurrentDamageType;


};
