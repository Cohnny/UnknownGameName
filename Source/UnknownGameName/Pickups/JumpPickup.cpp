#include "JumpPickup.h"
#include "UnknownGameName/Character/UnknownCharacter.h"
#include "UnknownGameName/UnknownComponents/BuffComponent.h"

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	AUnknownCharacter* UnknownCharacter = Cast<AUnknownCharacter>(OtherActor);
	if (UnknownCharacter)
	{
		UBuffComponent* Buff = UnknownCharacter->GetBuff();
		if (Buff)
		{
			Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
		}
	}
	Destroy();
}