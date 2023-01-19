// Fill out your copyright notice in the Description page of Project Settings.


#include "UnknownPlayerState.h"
#include "UnknownGameName/Character/UnknownCharacter.h"
#include "UnknownGameName/PlayerController/UnknownPlayerController.h"

void AUnknownPlayerState::AddToScore(float ScoreAmount)
{
	Score += ScoreAmount;
	Character = Character == nullptr ? Cast<AUnknownCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AUnknownPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(Score);
		}
	}
}

void AUnknownPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	
	Character = Character == nullptr ? Cast<AUnknownCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AUnknownPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(Score);
		}
	}
}