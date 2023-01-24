// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "UnknownGameState.generated.h"

/**
 * 
 */
UCLASS()
class UNKNOWNGAMENAME_API AUnknownGameState : public AGameState
{
	GENERATED_BODY()
public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class AUnknownPlayerState* ScoringPlayer);

	UPROPERTY(Replicated)
	TArray<AUnknownPlayerState*> TopScoringPlayers;

private:

	float TopScore = 0.f;
};
