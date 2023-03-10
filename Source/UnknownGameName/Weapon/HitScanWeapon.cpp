// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "UnknownGameName/Character/UnknownCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "DrawDebugHelpers.h"
#include "EntitySystem/MovieSceneEntitySystemRunner.h"
#include "UnknownGameName/PlayerController/UnknownPlayerController.h"
#include "UnknownGameName/UnknownComponents/LagCompensationComponent.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		AUnknownCharacter* UnknownCharacter = Cast<AUnknownCharacter>(FireHit.GetActor());
		if (UnknownCharacter && InstigatorController)
		{
			if (HasAuthority() && !bUseServerSideRewind)
			{
				UGameplayStatics::ApplyDamage(
					UnknownCharacter,
					Damage,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
			if (!HasAuthority() && bUseServerSideRewind)
			{
				UnknownOwnerCharacter = UnknownOwnerCharacter == nullptr ? Cast<AUnknownCharacter>(OwnerPawn) : UnknownOwnerCharacter;
				UnknownOwnerController = UnknownOwnerController == nullptr ? Cast<AUnknownPlayerController>(InstigatorController) : UnknownOwnerController;
				if (UnknownOwnerController && UnknownOwnerCharacter && UnknownOwnerCharacter->GetLagCompensation())
				{
					UnknownOwnerCharacter->GetLagCompensation()->ServerScoreRequest(
						UnknownCharacter,
						Start,
						HitTarget,
						UnknownOwnerController->GetServerTime() - UnknownOwnerController->SingleTripTime,
						this
					);
				}
			}
		}
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint
			);
		}
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlash,
				SocketTransform
			);
		}
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				FireSound,
				GetActorLocation()
			);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;
		
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility
		);
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}

		DrawDebugSphere(GetWorld(), BeamEnd, 16.f, 12, FColor::Orange, true);
		
		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}