// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "UnknownGameMode.generated.h"

namespace MatchState
{
	extern UNKNOWNGAMENAME_API const FName Cooldown; // Match duration has been reached. Display winner and begin cooldown timer.
}

/**
 * 
 */
UCLASS()
class UNKNOWNGAMENAME_API AUnknownGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	AUnknownGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class AUnknownCharacter* ElimmedCharacter, class AUnknownPlayerController* VictimController, AUnknownPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	AActor* GetSpawnPointWithLargestMinimumDistance(TArray<AActor*> PlayerStarts, TArray<AActor*> Players) const;
	float GetMinimumDistance(const AActor* PlayerStart, const TArray<AActor*> Players) const;

	float CountdownTime = 0.f;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
