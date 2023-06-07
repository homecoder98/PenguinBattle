#include "AmmoPickup.h"
#include "PenguinBattle/Character/PenguinCharacter.h"
#include "PenguinBattle/PenguinComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	APenguinCharacter* PenguinCharacter = Cast<APenguinCharacter>(OtherActor);
	if (PenguinCharacter)
	{
		UCombatComponent* Combat = PenguinCharacter->GetCombat();
		if (Combat)
		{
			UE_LOG(LogTemp,Warning, TEXT("On ssssssss"));
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	Destroy();
}
