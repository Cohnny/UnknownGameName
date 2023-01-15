// Fill out your copyright notice in the Description page of Project Settings.


#include "UnknownGameMode.h"
#include "UnknownGameName/Character/UnknownCharacter.h"
#include "UnknownGameName/PlayerController/UnknownPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

void AUnknownGameMode::PlayerEliminated(AUnknownCharacter* ElimmedCharacter, AUnknownPlayerController* VictimController, AUnknownPlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void AUnknownGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		TArray<AActor*> Players;

		// only really need to do this once, as the spawn points do not move (in this game) - but relies on knowing when the match has actually started (e.g. cant use BeginPlay)
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		UGameplayStatics::GetAllActorsOfClass(this, AUnknownCharacter::StaticClass(), Players);
		AActor* PlayerStart = GetSpawnPointWithLargestMinimumDistance(PlayerStarts, Players);

		if (PlayerStart)
		{
			RestartPlayerAtPlayerStart(ElimmedController, PlayerStart);
		}
		else
		{
			// no respawn for you!  (needs error handling)
		}
	}
}

AActor* AUnknownGameMode::GetSpawnPointWithLargestMinimumDistance(TArray<AActor*> PlayerStarts, TArray<AActor*> Players) const
{
	if (PlayerStarts.Num() == 0 || Players.Num() == 0) { return nullptr; }

	float LargestMinimumDistance = -1 * std::numeric_limits<float>::infinity();
	AActor* Result = nullptr;

	for (AActor* PlayerStart : PlayerStarts)
	{
		const float MinimumDistance = GetMinimumDistance(PlayerStart, Players);

		if (MinimumDistance > LargestMinimumDistance)
		{
			LargestMinimumDistance = MinimumDistance;
			Result = PlayerStart;
		}
	}

	return Result;
}

float AUnknownGameMode::GetMinimumDistance(const AActor* PlayerStart, const TArray<AActor*> Players) const
{
	float MinimumDistanceSquared = std::numeric_limits<float>::infinity();

	for (AActor* Player : Players)
	{
		const float DistanceSquared = FVector::DistSquared(PlayerStart->GetActorLocation(), Player->GetActorLocation());
		MinimumDistanceSquared = FMath::Min(MinimumDistanceSquared, DistanceSquared);
	}

	return FMath::Sqrt(MinimumDistanceSquared);
}
