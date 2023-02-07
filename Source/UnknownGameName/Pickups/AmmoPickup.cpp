// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "UnknownGameName/Character/UnknownCharacter.h"
#include "UnknownGameName/UnknownComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	AUnknownCharacter* UnknownCharacter = Cast<AUnknownCharacter>(OtherActor);
	if (UnknownCharacter)
	{
		UCombatComponent* Combat = UnknownCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	Destroy();
}
