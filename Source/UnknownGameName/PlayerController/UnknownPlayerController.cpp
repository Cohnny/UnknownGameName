// Fill out your copyright notice in the Description page of Project Settings.


#include "UnknownPlayerController.h"

#include "Components/Image.h"
#include "UnknownGameName/HUD/UnknownHUD.h"
#include "UnknownGameName/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UnknownGameName/Character/UnknownCharacter.h"
#include "Net/UnrealNetwork.h"
#include "UnknownGameName/GameMode/UnknownGameMode.h"
#include "UnknownGameName/PlayerState/UnknownPlayerState.h"
#include "UnknownGameName/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "UnknownGameName/UnknownComponents/CombatComponent.h"
#include "UnknownGameName/GameState/UnknownGameState.h"
#include "Components/Image.h"

void AUnknownPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ServerCheckMatchState();

	UnknownHUD = Cast<AUnknownHUD>(GetHUD());
	ServerCheckMatchState();
}

void AUnknownPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUnknownPlayerController, MatchState);
}


void AUnknownPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
	CheckPing(DeltaTime);
	ShowPing(DeltaTime);
}

void AUnknownPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState)
		{
			if (PlayerState->GetPingInMilliseconds() > HighPingThreshold)
			{
				HighPingWaning();
				PingAnimationRunningTime = 0.f;
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighPingAnimationPlaying =
		UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->HighPingAnimation &&
		UnknownHUD->CharacterOverlay->IsAnimationPlaying(UnknownHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void AUnknownPlayerController::ShowPing(float DeltaTime)
{
	PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
	if (PlayerState)
	{
		float CurrentPing = PlayerState->GetPingInMilliseconds();
		if (UnknownHUD && UnknownHUD->CharacterOverlay && UnknownHUD->CharacterOverlay->PingAmountText)
		{
			FString PingText = FString::Printf(TEXT("%d ms"), FMath::FloorToInt(CurrentPing));
			UnknownHUD->CharacterOverlay->PingAmountText->SetText(FText::FromString(PingText));
		}
	}
}

void AUnknownPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void AUnknownPlayerController::HighPingWaning()
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->HighPingImage &&
		UnknownHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		UnknownHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		UnknownHUD->CharacterOverlay->PlayAnimation(
			UnknownHUD->CharacterOverlay->HighPingAnimation,
			0.f,
			5
			);
	}
}

void AUnknownPlayerController::StopHighPingWarning()
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->HighPingImage &&
		UnknownHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		UnknownHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (UnknownHUD->CharacterOverlay->IsAnimationPlaying(UnknownHUD->CharacterOverlay->HighPingAnimation))
		{
			UnknownHUD->CharacterOverlay->StopAnimation(UnknownHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void AUnknownPlayerController::ServerCheckMatchState_Implementation()
{
	AUnknownGameMode* GameMode = Cast<AUnknownGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void AUnknownPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if (UnknownHUD && MatchState == MatchState::WaitingToStart)
	{
		UnknownHUD->AddAnnouncement();
	}
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
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void AUnknownPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->ShieldBar &&
		UnknownHUD->CharacterOverlay->ShieldText;
	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		UnknownHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		UnknownHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
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
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
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
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void AUnknownPlayerController::SetHUDWeaponType(FText WeaponType)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->WeaponTypeText;
	if (bHUDValid)
	{
		UnknownHUD->CharacterOverlay->WeaponTypeText->SetText(WeaponType);
	}
}

void AUnknownPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		UnknownHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void AUnknownPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		UnknownHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void AUnknownPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			UnknownHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		bool bLastThirtySeconds = Minutes == FMath::FloorToInt(0.f) && Seconds <= FMath::FloorToInt(30.f);
		if (bLastThirtySeconds)
		{
			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);

			FTimerHandle TimerHandle;

			FSlateColor RedColor = FSlateColor(FLinearColor::Red);

			UnknownHUD->CharacterOverlay->MatchCountdownText->SetColorAndOpacity(RedColor);
			UnknownHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));

			GetWorld()->GetTimerManager().SetTimer(TimerHandle, [&]()
				{
					UnknownHUD->CharacterOverlay->MatchCountdownText->SetVisibility(ESlateVisibility::Hidden);
				}, .5f, false);
			UnknownHUD->CharacterOverlay->MatchCountdownText->SetVisibility(ESlateVisibility::Visible);

		}
		else
		{
			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);

			FSlateColor WhiteColor = FSlateColor(FLinearColor::White);

			UnknownHUD->CharacterOverlay->MatchCountdownText->SetColorAndOpacity(WhiteColor);

			UnknownHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
		}
	}
}

void AUnknownPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->Announcement &&
		UnknownHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			UnknownHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		UnknownHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void AUnknownPlayerController::SetHUDGrenades(int32 Grenades)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->GrenadesText;
	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		UnknownHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void AUnknownPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		UnknownGameMode = UnknownGameMode == nullptr ? Cast<AUnknownGameMode>(UGameplayStatics::GetGameMode(this)) : UnknownGameMode;
		if (UnknownGameMode)
		{
			SecondsLeft = FMath::CeilToInt(UnknownGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CountdownInt = SecondsLeft;
}

void AUnknownPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (UnknownHUD && UnknownHUD->CharacterOverlay)
		{
			CharacterOverlay = UnknownHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
				
				AUnknownCharacter* UnknownCharacter = Cast<AUnknownCharacter>(GetPawn());
				if (UnknownCharacter && UnknownCharacter->GetCombat())
				{
					if (bInitializeGrenades) SetHUDGrenades(UnknownCharacter->GetCombat()->GetGrenades());
				}
			}
		}
	}
}

void AUnknownPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AUnknownPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float AUnknownPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AUnknownPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AUnknownPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AUnknownPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AUnknownPlayerController::HandleMatchHasStarted()
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	if (UnknownHUD)
	{
		if (UnknownHUD->CharacterOverlay == nullptr)
		{
			UnknownHUD->AddCharacterOverlay();
		}
		if (UnknownHUD->Announcement)
		{
			UnknownHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AUnknownPlayerController::HandleCooldown()
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	if (UnknownHUD)
	{
		UnknownHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = UnknownHUD->Announcement &&
			UnknownHUD->Announcement->AnnouncementText &&
			UnknownHUD->Announcement->InfoText;
		if (bHUDValid)
		{
			UnknownHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New match starts in:");
			UnknownHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
			
			AUnknownGameState* UnknownGameState = Cast<AUnknownGameState>(UGameplayStatics::GetGameState(this));
			AUnknownPlayerState* UnknownPlayerState = GetPlayerState<AUnknownPlayerState>();
			if (UnknownGameState && UnknownPlayerState)
			{
				TArray<AUnknownPlayerState*> TopPlayers = UnknownGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("NO ONE WON.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == UnknownPlayerState)
				{
					InfoTextString = FString("YOU WON THE MATCH!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("PLAYERS TIED FOR THE MATCH:\n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}

				UnknownHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	AUnknownCharacter* UnknownCharacter = Cast<AUnknownCharacter>(GetPawn());
	if (UnknownCharacter && UnknownCharacter->GetCombat())
	{
		UnknownCharacter->bDisableGameplay = true;
		UnknownCharacter->GetCombat()->FireButtonPressed(false);
	}
}

void AUnknownPlayerController::SetElimText(FString InText)
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->ElimText;
	if (bHUDValid)
	{
		if (InText.IsEmpty())
		{
			FString TextToDisplay = FString::Printf(TEXT("You killed yourself"));
			UnknownHUD->CharacterOverlay->ElimText->SetText(FText::FromString(TextToDisplay));
		}
		else
		{
			FString TextToDisplay = FString::Printf(TEXT("You were killed by \r %s"), *InText);
			UnknownHUD->CharacterOverlay->ElimText->SetText(FText::FromString(TextToDisplay));
		}
		UnknownHUD->CharacterOverlay->ElimText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void AUnknownPlayerController::ClearElimText()
{
	UnknownHUD = UnknownHUD == nullptr ? Cast<AUnknownHUD>(GetHUD()) : UnknownHUD;
	bool bHUDValid = UnknownHUD &&
		UnknownHUD->CharacterOverlay &&
		UnknownHUD->CharacterOverlay->ElimText;
	if (bHUDValid && HasAuthority())
	{
		UnknownHUD->CharacterOverlay->ElimText->SetText(FText());
		UnknownHUD->CharacterOverlay->ElimText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (bHUDValid)
	{
		UnknownHUD->CharacterOverlay->ElimText->SetText(FText());
		UnknownHUD->CharacterOverlay->ElimText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
