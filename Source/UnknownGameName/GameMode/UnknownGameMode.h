// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "UnknownGameMode.generated.h"

/**
 * 
 */
UCLASS()
class UNKNOWNGAMENAME_API AUnknownGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual void PlayerEliminated(class AUnknownCharacter* ElimmedCharacter, class AUnknownPlayerController* VictimController, AUnknownPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);

private:
	AActor* GetSpawnPointWithLargestMinimumDistance(TArray<AActor*> PlayerStarts, TArray<AActor*> Players) const;
	float GetMinimumDistance(const AActor* PlayerStart, const TArray<AActor*> Players) const;
};
