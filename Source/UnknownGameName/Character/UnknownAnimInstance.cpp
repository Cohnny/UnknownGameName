// Fill out your copyright notice in the Description page of Project Settings.


#include "UnknownAnimInstance.h"
#include "UnknownCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

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
	bIsCrouched = UnknownCharacter->bIsCrouched;
	bAiming = UnknownCharacter->IsAiming();

	// Offset Yaw for strafing
	FRotator AimRotation = UnknownCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(UnknownCharacter->GetVelocity());
	YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;


}