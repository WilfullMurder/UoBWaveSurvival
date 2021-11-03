// Fill out your copyright notice in the Description page of Project Settings.


#include "SImpleAIBot.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "HealthComponent.h"
#include "SurvivalGameCharacter.h"
#include "Components/SphereComponent.h"
#include "Sound/SoundCue.h"
#include "EngineUtils.h"

static int32 DebugBotDrawing = 0;
FAutoConsoleVariableRef CVARDebugBotDrawing(
	TEXT("COOP.DebugTrackerBot"),
	DebugBotDrawing,
	TEXT("Draw Debug Lines for TrackerBot"),
	ECVF_Cheat);

// Sets default values
ASImpleAIBot::ASImpleAIBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	MeshComponent->SetCanEverAffectNavigation(false);
	MeshComponent->SetSimulatePhysics(true);
	RootComponent = MeshComponent;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthCompnent"));

	HealthComponent->TeamNum = 1;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereComponent->SetSphereRadius(200.0f);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComponent->SetupAttachment(RootComponent);

	bUseVelocityChange = false;
	MovementForce = 1000;
	RequiredDistanceToTarget = 100;

	ExplosionDamage = 60;
	ExplosionRadius = 350;

	SelfHarmInterval = 0.25f;
}

// Called when the game starts or when spawned
void ASImpleAIBot::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		//Initial Move to
		NextPathPoint = GetNextPathPoint();

		//Update Power Level by Nearby bots
		FTimerHandle TH_PowerLevelCheck;
		GetWorldTimerManager().SetTimer(TH_PowerLevelCheck, this, &ASImpleAIBot::OnCheckNearbyBots, 1.0f, true);
	}
	HealthComponent->OnHealthChanged.AddDynamic(this, &ASImpleAIBot::OnHealthChanged);

	
}



void ASImpleAIBot::OnHealthChanged(UHealthComponent* OwningHealthComp, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (MatInst == nullptr)
	{
		MatInst = MeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComponent->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	if (Health <= 0.0f)
	{
		SelfDestruct();
	}
}

FVector ASImpleAIBot::GetNextPathPoint()
{
	AActor* BestTarget = nullptr;
	float DistanceNearestTarget = FLT_MAX;

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* TestPawn = *It;
		if (TestPawn == nullptr || UHealthComponent::IsFriendly(TestPawn, this))
		{
			continue;
		}

		UHealthComponent* TestPawnHealthComponent = Cast<UHealthComponent>(TestPawn->GetComponentByClass(UHealthComponent::StaticClass()));
		if (TestPawnHealthComponent && TestPawnHealthComponent->GetHealth() > 0.0f)
		{
			float Distance = ((TestPawn->GetActorLocation() - GetActorLocation()).Size());

			if (Distance < DistanceNearestTarget)
			{
				BestTarget = TestPawn;
				DistanceNearestTarget = Distance;
			}
		}
	}

	if (BestTarget)
	{
		UNavigationPath* NavigationPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), BestTarget);

		GetWorldTimerManager().ClearTimer(TH_RefreshPath);
		GetWorldTimerManager().SetTimer(TH_RefreshPath, this, &ASImpleAIBot::RefreshPath, 5.0f, false);

		if (NavigationPath && NavigationPath->PathPoints.Num() > 1)
		{
			// Return next point in the path
			return NavigationPath->PathPoints[1];
		}


	}

	// Failed to find path
	return GetActorLocation();
}

void ASImpleAIBot::SelfDestruct()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;

	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	}
	if (ExplodeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
	}
	

	

	MeshComponent->SetVisibility(false, true);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (HasAuthority())
	{
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		float ActualDamage = ExplosionDamage + (ExplosionDamage * PowerLevel);

		UGameplayStatics::ApplyRadialDamage(this, ActualDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		if (DebugBotDrawing)
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Black, false, 2.0f, 0, 1.0f);
		}

		SetLifeSpan(2.0f);

	}

}

void ASImpleAIBot::SelfHarm()
{
	UGameplayStatics::ApplyDamage(this, 20.0f, GetInstigatorController(), this, nullptr);
}

void ASImpleAIBot::OnCheckNearbyBots()
{

	const float Radius = 600;

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(Radius);

	FCollisionObjectQueryParams QueryParams;

	QueryParams.AddObjectTypesToQuery(ECC_Pawn);
	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollisionShape);

	if (DebugBotDrawing)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 1.0f);
	}

	int32 TotalBots = 0;

	for (FOverlapResult Result : Overlaps)
	{
		ASImpleAIBot* Bot = Cast<ASImpleAIBot>(Result.GetActor());

		if (Bot && Bot != this)
		{
			TotalBots++;
		}
	}

	const int32 PowerLevel_Max = 4;

	PowerLevel = FMath::Clamp(TotalBots, 0, PowerLevel_Max);

	if (MatInst == nullptr)
	{
		MatInst = MeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComponent->GetMaterial(0));
	}
	if (MatInst)
	{
		float Aplha = PowerLevel / (float)PowerLevel_Max;

		MatInst->SetScalarParameterValue("PowerLevelAlpha", Aplha);
	}

	if (DebugBotDrawing)
	{
		FVector StringLoc = GetActorLocation();
		StringLoc.Z += 20.0f;
		DrawDebugString(GetWorld(), StringLoc, FString::FromInt(PowerLevel), this, FColor::White, 1.0f, true);
	}
}

void ASImpleAIBot::RefreshPath()
{
	NextPathPoint = GetNextPathPoint();
}

// Called every frame
void ASImpleAIBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && !bExploded)
	{
		float TargetDistance = (GetActorLocation() - NextPathPoint).Size();

		if (TargetDistance <= RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint();

			if (DebugBotDrawing)
			{
				FVector StringLoc = GetActorLocation();
				StringLoc.Z += 20.0f;
				DrawDebugString(GetWorld(), StringLoc, "Path point reached!");
			}
			
		}
		else
		{
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();

			ForceDirection *= MovementForce;

			MeshComponent->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

			if (DebugBotDrawing)
			{
				DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Yellow, false, 0.0f, 0, 1.0f);
			}

		}

		if (DebugBotDrawing)
		{
			DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 0.0f, 1.0f);
		}
	}

}

void ASImpleAIBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (!bStartedSelfDestruction && !bExploded)
	{
		ASurvivalGameCharacter* PlayerPawn = Cast<ASurvivalGameCharacter>(OtherActor);
		if (PlayerPawn && !UHealthComponent::IsFriendly(OtherActor, this))
		{
			if (HasAuthority())
			{
				GetWorldTimerManager().SetTimer(TH_SelfHarm, this, &ASImpleAIBot::SelfHarm, SelfHarmInterval, true, 0.0f);
			}
		}

	}
	bStartedSelfDestruction = true;

	UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);


}


