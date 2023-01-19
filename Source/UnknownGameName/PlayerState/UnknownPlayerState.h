// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "UnknownPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class UNKNOWNGAMENAME_API AUnknownPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	virtual void OnRep_Score() override;
	void AddToScore(float ScoreAmount);

private:
	class AUnknownCharacter* Character;
	class AUnknownPlayerController* Controller;
};
