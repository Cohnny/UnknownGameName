// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerName(APawn* InPawn)
{
	APlayerState* UnknownPlayerState = InPawn->GetPlayerState();
	if (!ensure(UnknownPlayerState != nullptr))
	{
		return;
	}

	FString PlayerName = UnknownPlayerState->GetPlayerName();
	FString DisplayName = FString::Printf(TEXT("%s"), *PlayerName);
	SetDisplayText(DisplayName);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}