// Fill out your copyright notice in the Description page of Project Settings.


#include "SurvivalGameModeBase.h"
#include "SurvivalGameStateBase.h"
#include "SurvivalPlayerState.h"
#include "HealthComponent.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"


ASurvivalGameModeBase::ASurvivalGameModeBase()
{
	TimeBetweenWaves = 2.0f;

	GameStateClass = ASurvivalGameStateBase::StaticClass();
	PlayerStateClass = ASurvivalPlayerState::StaticClass();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;
}

void ASurvivalGameModeBase::StartPlay()
{
	Super::StartPlay();
	

	PrepareForNextWave();
}

void ASurvivalGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckWaveState();
	CheckAnyPlayerAlive();
}

void ASurvivalGameModeBase::SpawnBotTimerElapsed()
{
	SpawnNewBot();

	NumBotsToSpawn--;

	if (NumBotsToSpawn <= 0)
	{
		EndWave();
	}
}

void ASurvivalGameModeBase::StartWave()
{
	WaveCount++;

	NumBotsToSpawn = 2 * WaveCount;

	GetWorldTimerManager().SetTimer(TH_BotSpawner, this, &ASurvivalGameModeBase::SpawnBotTimerElapsed, 1.0f, true, 0.0f);

	SetWaveState(EWaveState::WaveInProgress);
}

void ASurvivalGameModeBase::EndWave()
{
	GetWorldTimerManager().ClearTimer(TH_BotSpawner);

	SetWaveState(EWaveState::WaitingToComplete);
}

void ASurvivalGameModeBase::PrepareForNextWave()
{
	GetWorldTimerManager().SetTimer(TH_NextWaveStart, this, &ASurvivalGameModeBase::StartWave, TimeBetweenWaves, false);

	SetWaveState(EWaveState::WaitingToStart);

	RestartDeadPlayers();
}

void ASurvivalGameModeBase::CheckWaveState()
{
	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TH_NextWaveStart);

	if (NumBotsToSpawn > 0 || bIsPreparingForWave)
	{
		return;
	}

	bool bAnyBotAlive = false;

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* TestPawn = *It;
		if (TestPawn == nullptr || TestPawn->IsPlayerControlled())
		{
			continue;
		}

		UHealthComponent* HealthComp = Cast<UHealthComponent>(TestPawn->GetComponentByClass(UHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0.0f)
		{
			bAnyBotAlive = true;
			break;
		}

	}

	if (!bAnyBotAlive)
	{
		SetWaveState(EWaveState::WaveComplete);

		PrepareForNextWave();
	}
}

void ASurvivalGameModeBase::CheckAnyPlayerAlive()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			APawn* Pawn = PC->GetPawn();
			UHealthComponent* HealthComp = Cast<UHealthComponent>(Pawn->GetComponentByClass(UHealthComponent::StaticClass()));
			if (ensure(HealthComp) && HealthComp->GetHealth() > 0.0f)
			{
				return;
			}
		}
	}

	GameOver();
}

void ASurvivalGameModeBase::GameOver()
{
	EndWave();

	SetWaveState(EWaveState::GameOver);

	UE_LOG(LogTemp, Log, TEXT("GAME OVER! Players Died"));
}

void ASurvivalGameModeBase::SetWaveState(EWaveState NewState)
{
	ASurvivalGameStateBase* GS = GetGameState<ASurvivalGameStateBase>();
	if (ensureAlways(GS))
	{
		GS->SetWaveState(NewState);
	}
}

void ASurvivalGameModeBase::RestartDeadPlayers()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn() == nullptr)
		{
			RestartPlayer(PC);
		}
	}
}
