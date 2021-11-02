// Fill out your copyright notice in the Description page of Project Settings.


#include "ExplosiveAsset.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AExplosiveAsset::AExplosiveAsset()
{


	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	MeshComponent->SetSimulatePhysics(true);
	//Set to physics body. Allows radial component to apply affects (eg. other nearby barrel explodes)
	MeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComponent;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnHealthChanged.AddDynamic(this, &AExplosiveAsset::OnHealthChanged);

	RadialForceComponent = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComponent"));
	RadialForceComponent->SetupAttachment(MeshComponent);
	RadialForceComponent->Radius = 250.0f;
	RadialForceComponent->bImpulseVelChange = true;
	RadialForceComponent->bAutoActivate = false; //Prevents compnent from tick. Only use FireImpules().
	RadialForceComponent->bIgnoreOwningActor = true; // Ignore Self

	ExplosionImpulse = 400.0f;

	bReplicates = true;
	SetReplicates(true);
	SetReplicateMovement(true);


}


void AExplosiveAsset::OnHealthChanged(UHealthComponent* OwningHealthComp, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{

	if (bExploded)
	{
		return;
	}

	if (Health <= 0.0f)
	{
		//Dead, Explode
		bExploded = true;
		OnRep_Exploded();

		//Apply z-axis boost
		FVector BoostIntensity = FVector::UpVector * ExplosionImpulse;
		MeshComponent->AddImpulse(BoostIntensity, NAME_None, true);

		//Apply radial force to nearby physics actors
		RadialForceComponent->FireImpulse();

		//@@TODO:
		//Apply Radial Damage

		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		UGameplayStatics::ApplyRadialDamage(GetWorld(), ExplosionDamage, GetActorLocation(), ExplosionDamageRadius, CurrentDamageType, IgnoredActors, this, nullptr, true, ECC_Visibility);

	}
}

void AExplosiveAsset::OnRep_Exploded()
{
	// VFX
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	// Override material with Exploded version
	MeshComponent->SetMaterial(0, ExplodedMaterial);

}

void AExplosiveAsset::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AExplosiveAsset, bExploded);
}

