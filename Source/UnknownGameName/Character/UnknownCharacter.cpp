// Fill out your copyright notice in the Description page of Project Settings.


#include "UnknownCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "UnknownGameName/Weapon/Weapon.h"
#include "UnknownGameName/UnknownComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "UnknownAnimInstance.h"
#include "UnknownGameName/UnknownGameName.h"
#include "UnknownGameName/PlayerController/UnknownPlayerController.h"
#include "UnknownGameName/GameMode/UnknownGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "UnknownGameName/PlayerState/UnknownPlayerState.h"
#include "UnknownGameName/Weapon/WeaponTypes.h"

// Sets default values
AUnknownCharacter::AUnknownCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void AUnknownCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AUnknownCharacter, OverlappingWeapon,COND_OwnerOnly);
	DOREPLIFETIME(AUnknownCharacter, Health);
	DOREPLIFETIME(AUnknownCharacter, bDisableGameplay);
}

void AUnknownCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void AUnknownCharacter::Elim(APlayerController* AttackerController)
{
	FString AttackerName = FString();
	AUnknownPlayerController* AttackerUnknownController = Cast<AUnknownPlayerController>(AttackerController);
	if (AttackerUnknownController)
	{
		AUnknownPlayerState* AttackerUnknownPlayerState = Cast<AUnknownPlayerState>(AttackerUnknownController->PlayerState);
		if (AttackerUnknownPlayerState)
		{
			AttackerName = AttackerUnknownPlayerState->GetPlayerName();
		}
	}
	MulticastElim(AttackerName);

	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastElim(AttackerName);
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&AUnknownCharacter::ElimTimerFinished,
		ElimDelay
	);
}

void AUnknownCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	AUnknownGameMode* UnknownGameMode = Cast<AUnknownGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = UnknownGameMode && UnknownGameMode->GetMatchState() != MatchState::InProgress;

	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void AUnknownCharacter::MulticastElim_Implementation(const FString& AttackerName)
{
	if (UnknownPlayerController)
	{
		DisableInput(UnknownPlayerController);
		UnknownPlayerController->SetElimText(AttackerName);

		UnknownPlayerController->SetHUDWeaponAmmo(0);
		UnknownPlayerController->SetHUDWeaponType(FText::FromString(""));
	}
	bElimmed = true;
	PlayElimMontage();

	// Start dissolve effect
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Disable character movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn elim bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
}

void AUnknownCharacter::ElimTimerFinished()
{
	AUnknownGameMode* UnknownGameMode = GetWorld()->GetAuthGameMode<AUnknownGameMode>();
	if (UnknownGameMode)
	{
		UnknownGameMode->RequestRespawn(this, Controller);
		if (AUnknownPlayerController* UnknownController = Cast<AUnknownPlayerController>(Controller))
		{
			UnknownController->ClearElimText();
		}
	}
}

void AUnknownCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AUnknownCharacter::ReceiveDamage);
	}
	if (AUnknownPlayerController* UnknownController = Cast<AUnknownPlayerController>(Controller))
	{
		UnknownController->ClearElimText();
	}
}

void AUnknownCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
}

void AUnknownCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

void AUnknownCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AUnknownCharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &AUnknownCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AUnknownCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &AUnknownCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AUnknownCharacter::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &AUnknownCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AUnknownCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AUnknownCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &AUnknownCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AUnknownCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AUnknownCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AUnknownCharacter::ReloadButtonPressed);


	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AUnknownCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

void AUnknownCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AUnknownCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		}

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AUnknownCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void AUnknownCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AUnknownCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();
	if (Health == 0.f)
	{
		AUnknownGameMode* UnknownGameMode = GetWorld()->GetAuthGameMode<AUnknownGameMode>();
		if (UnknownGameMode)
		{
			UnknownPlayerController = UnknownPlayerController == nullptr ? Cast<AUnknownPlayerController>(Controller) : UnknownPlayerController;
			AUnknownPlayerController* AttackerController = Cast<AUnknownPlayerController>(InstigatorController);
			UnknownGameMode->PlayerEliminated(this, UnknownPlayerController, AttackerController);
		}
	}
	
}

void AUnknownCharacter::MoveForward(float Value)
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void AUnknownCharacter::MoveRight(float Value)
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void AUnknownCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void AUnknownCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void AUnknownCharacter::EquipButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void AUnknownCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AUnknownCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		Combat->Reload();
	}
}

void AUnknownCharacter::AimButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AUnknownCharacter::AimButtonReleased()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

float AUnknownCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void AUnknownCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // Standing still, not jumping
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);

	}
	if (Speed > 0.f || bIsInAir) // Running or jumping
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void AUnknownCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// Map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void AUnknownCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void AUnknownCharacter::Jump()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void AUnknownCharacter::FireButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void AUnknownCharacter::FireButtonReleased()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void AUnknownCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;

	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void AUnknownCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void AUnknownCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void AUnknownCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}

void AUnknownCharacter::UpdateHUDHealth()
{
	UnknownPlayerController = UnknownPlayerController == nullptr ? Cast<AUnknownPlayerController>(Controller) : UnknownPlayerController;
	if (UnknownPlayerController)
	{
		UnknownPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void AUnknownCharacter::PollInit()
{
	if (UnknownPlayerState == nullptr)
	{
		UnknownPlayerState = GetPlayerState<AUnknownPlayerState>();
		if (UnknownPlayerState)
		{
			UnknownPlayerState->AddToScore(0.f);
			UnknownPlayerState->AddToDefeats(0);
		}
	}
}

void AUnknownCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void AUnknownCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AUnknownCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void AUnknownCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void AUnknownCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool AUnknownCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool AUnknownCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon* AUnknownCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector AUnknownCharacter::GetHitTarget() const
{
	if (Combat == nullptr)
	{
		return FVector();
	}
	return Combat->HitTarget;
}

ECombatState AUnknownCharacter::GetCombatState() const
{
	if (Combat == nullptr)
	{
		return ECombatState::ECS_MAX;
	}
	return Combat->CombatState;
}
