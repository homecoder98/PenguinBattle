#include "CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PenguinBattle/Weapon/WeaponTypes.h"
#include "PenguinBattle/Character/PenguinCharacter.h"
#include "PenguinBattle/HUD/PenguinHUD.h"
#include "TimerManager.h"
#include "PenguinBattle/PlayerController/PenguinPlayerController.h"
#include "PenguinBattle/Weapon/Projectile.h"
#include "PenguinBattle/Weapon/Shotgun.h"
#include "PenguinBattle/Weapon/Weapon.h"
#include "Sound/SoundCue.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		
		if (Character->GetFollowCamera())
		{
			DefaulFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaulFOV;
		}
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
			Grenades = StartingGrenade;
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0 ,MaxCarriedAmmo);
		UpdateAmmoValue();
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	ReloadEmptyWeapon();
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	if (!EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun) return true;
	if (bLocallyReloading) return false;
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<APenguinPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	bool bJumpToShotgunEnd =
		CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon != nullptr &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CarriedAmmo == 0;
	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire() && EquippedWeapon)
	{
		bCanFire = false;
		
		if (EquippedWeapon)
		{
			CrosshairShootingFactor = .75f;
			switch (EquippedWeapon->FireType)
			{
			case EFireType::UFT_Projectile:
				FireProjectileWeapon();
				break;
			case EFireType::UFT_HitScan:
				FireHitScanWeapon();
				break;
			case EFireType::UFT_Shothun:
				FireShotgun();
				break;
			}
		}
		StartFireTimer();
	}
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority()) LocalFire(HitTarget); // 없으면 서버는 두번 쏨
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority()) LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}
}

void UCombatComponent::FireShotgun()
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun && Character)
	{
		TArray<FVector_NetQuantize> HitTargets;
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets);
		if (!Character->HasAuthority()) ShotgunLocalFire(HitTargets);
		ServerShotgunFire(HitTargets, EquippedWeapon->FireDelay);
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (Character && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{ // Shotgun
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun == nullptr || Character == nullptr) return;
	if (CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
		CombatState = ECombatState::ECS_Unoccupied;
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	LocalFire(HitTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	MulticastShotgunFire(TraceHitTargets);
}


bool UCombatComponent::ServerShotgunFire_Validate(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	if (EquippedWeapon)
	{
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	ShotgunLocalFire(TraceHitTargets);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	
	DropEquippedWeapon();
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(WeaponToEquip);
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound();
	ReloadEmptyWeapon();

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = false;
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon == nullptr) return;
	bool bUsePistolSocket =
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachinGun;
	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");
	
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	
	Controller = Controller == nullptr ? Cast<APenguinPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::PlayEquipWeaponSound()
{
	if (Character && EquippedWeapon && EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			EquippedWeapon->GetActorLocation()
			);
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::Reload()
{
	if (EquippedWeapon->IsFull()) return;
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
	if (!Character->HasAuthority() && Character->IsLocallyControlled())
	{
		HandleReload();
	}
}

void UCombatComponent::ServerReload_Implementation()//클라가 호출시 서버에서 실행,서버가 호출시 서버만 실행
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoominMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoominMag, AmountCarried);
		return FMath::Clamp(RoominMag, 0, Least);
	}
	return int32();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	bLocallyReloading = false;
	if (Character->HasAuthority())
	{
	}
	CombatState = ECombatState::ECS_Unoccupied;
	if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		// UpdateShotgunAmmoValue();
	}
	else
	{
		UpdateAmmoValue();
	}
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::HandleReload()
{
	if (Character)
	{
		Character->PlayReloadMontage();
	}
}


void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
	}
	UpdateShotgunAmmoValue();
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance && Character->GetReloadMontage())
	{
		AnimInstance->Montage_Play(Character->GetReloadMontage());
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UCombatComponent::UpdateShotgunAmmoValue()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()] ; 
	}
	Controller = Controller == nullptr ? Cast<APenguinPlayerController>(Character->Controller) : Controller;
	EquippedWeapon->AddAmmo(1);
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
		Controller->SetHUDWeaponAmmo(EquippedWeapon->GetAmmo());
	}
	bCanFire = true;
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0)
	{
		// Jump to ShotgunEnd Section
		JumpToShotgunEnd();
	}
}

void UCombatComponent::UpdateAmmoValue()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()] ; 
	}
	
	Controller = Controller == nullptr ? Cast<APenguinPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	
	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		if (Character && !Character->IsLocallyControlled()) HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_ThrowingGrenade:
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlayThrowGrenadeMontage();
			AttachActorToLeftHand(EquippedWeapon);
			ShowAttachedGrenade(true);
		}
		break;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		PlayEquipWeaponSound();
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2d ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2d CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0.f),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);
	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility
		);
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairInterface>())
		{
			HUDPackage.CrosshairColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
		
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
		}
		// else
		// {
		// 	HitTarget = TraceHitResult.ImpactPoint;
		// DrawDebugSphere(
		// 		GetWorld(),
		// 		TraceHitResult.ImpactPoint,
		// 		12.f,
		// 		12.f,
		// 		FColor::Red
		// 	);
		// }
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;
	Controller = Controller == nullptr ? Cast<APenguinPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<APenguinHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairBottom;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}
			// Calculate crosshair spread
			// [0, 600] -> [0, 1]
			FVector2D WalkSpreadRange(0.f, Character->GetCharacterMovement()->GetMaxSpeed());
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			CrosshairVelocityFector = FMath::GetMappedRangeValueClamped(WalkSpreadRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.2f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}
			
			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);
			
			HUDPackage.CrosshairSpread =
				0.5f +
				CrosshairVelocityFector +
				CrosshairInAirFactor -
				CrosshairAimFactor +
				CrosshairShootingFactor;
			
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (Grenades == 0) return;
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	if (Character && !Character->HasAuthority())
	{
		ServerThrowGrenade();
	}
	if (Character && Character->HasAuthority())
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades == 0) return;
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
	}
	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	UpdateHUDGrenades();
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);
	if (Character && Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
	Controller = Controller == nullptr ? Cast<APenguinPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDGrenades(Grenades);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && GrenadeClass && Character->GetAttachedGrenade())
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		UWorld* World = GetWorld();
		if (World)
		{
			World->SpawnActor<AProjectile>(
				GrenadeClass,
				StartingLocation,
				ToTarget.Rotation(),
				SpawnParams
			);
		}
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;
	
	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaulFOV, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	bAiming = bIsAiming;
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	if (Character->IsLocallyControlled()) bAimButtonPressed = bIsAiming;
}

void UCombatComponent::OnRep_Aiming()
{
	if (Character && Character->IsLocallyControlled())
	{
		bAiming = bAimButtonPressed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachinGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperRifleAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Grenade, StartingGrenade);
}
