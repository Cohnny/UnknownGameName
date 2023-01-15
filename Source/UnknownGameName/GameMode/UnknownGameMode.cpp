// Fill out your copyright notice in the Description page of Project Settings.


#include "UnknownGameMode.h"
#include "UnknownGameName/Character/UnknownCharacter.h"
#include "UnknownGameName/PlayerController/UnknownPlayerController.h"

void AUnknownGameMode::PlayerEliminated(AUnknownCharacter* ElimmedCharacter, AUnknownPlayerController* VictimController, AUnknownPlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}
