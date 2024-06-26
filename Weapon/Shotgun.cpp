#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "PenguinBattle/Character/PenguinCharacter.h"
#include "PenguinBattle/PenguinComponents/LagCompensationComponent.h"
#include "PenguinBattle/PlayerController/PenguinPlayerController.h"
#include "Sound/SoundCue.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector());
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		// Maps hit character to number of times hit
		TMap<APenguinCharacter*, uint32> HitMap;
		TMap<APenguinCharacter*, uint32> HeadShotHitMap;
		
		for (FVector_NetQuantize HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);
			APenguinCharacter* PenguinCharacter = Cast<APenguinCharacter>(FireHit.GetActor());
			if (PenguinCharacter)
			{
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("Head");
				// if (HitMap.Contains(PenguinCharacter)) HitMap[PenguinCharacter]++;
				// else HitMap.Emplace(PenguinCharacter, 1);

				if (bHeadShot)
				{
					if (HeadShotHitMap.Contains(PenguinCharacter)) HeadShotHitMap[PenguinCharacter]++;
					else HeadShotHitMap.Emplace(PenguinCharacter, 1);
				}
				else
				{
					if (HitMap.Contains(PenguinCharacter)) HitMap[PenguinCharacter]++;
					else HitMap.Emplace(PenguinCharacter, 1);
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
						FireHit.ImpactPoint,
						.5f,
						FMath::FRandRange(-.5f, .5f)
					);
				}
			}
		}
		TArray<APenguinCharacter*> HitCharacters;
		TMap<APenguinCharacter*, float> DamageMap;
		
		// Calculate body shot damage by multiplying times hit x Damage - store in DamageMap
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key)
			{
				DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);

				HitCharacters.AddUnique(HitPair.Key);
			}
		}

		// Calculate head shot damage by multiplying times hit x HeadShotDamage - store in DamageMap
		for (auto HeadShotHitPair : HeadShotHitMap)
		{
			if (HeadShotHitPair.Key)
			{
				if (DamageMap.Contains(HeadShotHitPair.Key)) DamageMap[HeadShotHitPair.Key] += HeadShotHitPair.Value * HeadShotDamage;
				else DamageMap.Emplace(HeadShotHitPair.Key, HeadShotHitPair.Value * HeadShotDamage);

				HitCharacters.AddUnique(HeadShotHitPair.Key);
			}
		}

		// Loop through DamageMap to get total damage for each character
		for (auto DamagePair : DamageMap)
		{
			if (DamagePair.Key && InstigatorController)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						DamagePair.Key, // Character that was hit
						DamagePair.Value, // Damage calculated in the two for loops above
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}
		
		if (!HasAuthority() && bUseServerSideRewind && OwnerPawn->IsLocallyControlled())
		{
			PenguinOwnerCharacter = PenguinOwnerCharacter == nullptr ? Cast<APenguinCharacter>(OwnerPawn) : PenguinOwnerCharacter;
			PenguinOwnerController = PenguinOwnerController == nullptr ? Cast<APenguinPlayerController>(InstigatorController) : PenguinOwnerController;
			if (PenguinOwnerController && PenguinOwnerCharacter && PenguinOwnerCharacter->GetLagCompensation())
			{
				PenguinOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters,
					Start,
					HitTargets,
					PenguinOwnerController->GetServerTime() - PenguinOwnerController->SingleTripTime
				);
			}
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return;
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();
	
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12.f, FColor::Orange, false, 3.f);
	
	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec; // Random Point
		HitTargets.Add(EndLoc);
		// FVector ToEndLoc = FVector(TraceStart + EndLoc * TRACE_LENGTH / ToEndLoc.Size());
		// DrawDebugSphere(GetWorld(), EndLoc, 3.f, 12.f, FColor::Blue, false, 3.f);
	}
}
