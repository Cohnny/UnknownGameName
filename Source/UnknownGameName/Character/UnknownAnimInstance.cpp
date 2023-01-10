// Fill out your copyright notice in the Description page of Project Settings.


#include "UnknownAnimInstance.h"
#include "UnknownCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "UnknownGameName/Weapon/Weapon.h"

void UUnknownAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	UnknownCharacter = Cast<AUnknownCharacter>(TryGetPawnOwner());
}

void UUnknownAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (UnknownCharacter == nullptr)
	{
		UnknownCharacter = Cast<AUnknownCharacter>(TryGetPawnOwner());
	}
	if (UnknownCharacter == nullptr)
	{
		return;
	}

	FVector Velocity = UnknownCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = UnknownCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = UnknownCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = UnknownCharacter->IsWeaponEquipped();
	EquippedWeapon = UnknownCharacter->GetEquippedWeapon();
	bIsCrouched = UnknownCharacter->bIsCrouched;
	bAiming = UnknownCharacter->IsAiming();
	TurningInPlace = UnknownCharacter->GetTurningInPlace();

	// Offset Yaw for strafing
	FRotator AimRotation = UnknownCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(UnknownCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = UnknownCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = UnknownCharacter->GetAO_Yaw();
	AO_Pitch = UnknownCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && UnknownCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		UnknownCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}
}