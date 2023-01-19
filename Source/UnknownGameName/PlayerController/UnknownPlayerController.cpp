// Fill out your copyright notice in the Description page of Project Settings.


#include "UnknownPlayerController.h"
#include "UnknownGameName/HUD/UnknownHUD.h"
#include "UnknownGameName/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UnknownGameName/Character/UnknownCharacter.h"

void AUnknownPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UnknownHUD = Cast<AUnknownHUD>(GetHUD());
}

void AUnknownPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AUnknownCharacter* UnknownCharacter = Cast<AUnknownCharacter>(InPawn);
	if (UnknownCharacter)
	{
		SetHUDHealth(UnknownCharacter->GetHealth(), UnknownCharacter->GetMaxHealth());
	}
}

void AUnknownPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->HealthBar &&
		UnknownHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		UnknownHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		UnknownHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void AUnknownPlayerController::SetHUDScore(float Score)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		UnknownHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void AUnknownPlayerController::SetHUDDefeats(int32 Defeats)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		UnknownHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}