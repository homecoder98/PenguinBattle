 #include "Weapon.h"
#include "Casing.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "PenguinBattle/Character/PenguinCharacter.h"
#include "PenguinBattle/PenguinComponents/CombatComponent.h"
#include "PenguinBattle/PlayerController/PenguinPlayerController.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(WeaponMesh);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(WeaponMesh);
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
	
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		PenguinOwnerCharacter = nullptr;
		PenguinOwnerController = nullptr;
	}
	else
	{
		SetHUDAmmo();
	}
}

void AWeapon::SetHUDAmmo()
{
	PenguinOwnerCharacter = PenguinOwnerCharacter == nullptr ? Cast<APenguinCharacter>(GetOwner()) : PenguinOwnerCharacter;
	if (PenguinOwnerCharacter)
	{
		PenguinOwnerController = PenguinOwnerController ==nullptr ? Cast<APenguinPlayerController>(PenguinOwnerCharacter->Controller) : PenguinOwnerController;
		if (PenguinOwnerController)
		{
			PenguinOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APenguinCharacter* PenguinCharacter = Cast<APenguinCharacter>(OtherActor);
	if (PenguinCharacter && PickupWidget)
	{
		PenguinCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APenguinCharacter* PenguinCharacter = Cast<APenguinCharacter>(OtherActor);
	if (PenguinCharacter && PickupWidget)
	{
		PenguinCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (WeaponType == EWeaponType::EWT_SubmachinGun)
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
		EnableCustomDepth(false);
		PenguinOwnerCharacter = PenguinOwnerCharacter == nullptr ? Cast<APenguinCharacter>(GetOwner()) : PenguinOwnerCharacter;
		if (PenguinOwnerCharacter)
		{
			PenguinOwnerController = PenguinOwnerController == nullptr ? Cast<APenguinPlayerController>(PenguinOwnerCharacter->Controller) : PenguinOwnerController;
			if (PenguinOwnerController && HasAuthority() && !PenguinOwnerController->HighPingDelegate.IsBound())
			{
				PenguinOwnerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
			}
		}
		break;
	case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
		break;
	}
}

 void AWeapon::OnRep_Ammo()
 {
	PenguinOwnerCharacter = PenguinOwnerCharacter == nullptr ? Cast<APenguinCharacter>(GetOwner()) : PenguinOwnerCharacter;
	if (PenguinOwnerCharacter && PenguinOwnerCharacter->GetCombat() && IsFull())
	{
		PenguinOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
	SetHUDAmmo();
 }

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		++Sequence;
	}
}

 void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
 {
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo();
 }

 void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
	ClientUpdateAmmo(AmmoToAdd);
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	// PenguinOwnerCharacter = PenguinOwnerCharacter == nullptr ? Cast<APenguinCharacter>(GetOuter()) : PenguinOwnerCharacter;
	// if (PenguinOwnerCharacter && PenguinOwnerCharacter->GetCombat() && IsFull())
	// {
	// 	//샷건 장전 끝 처리
	// }
}
bool AWeapon::IsFull()
{
	return Ammo == MagCapacity;
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (WeaponType == EWeaponType::EWT_SubmachinGun)
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
		EnableCustomDepth(false);
		PenguinOwnerCharacter = PenguinOwnerCharacter == nullptr ? Cast<APenguinCharacter>(GetOwner()) : PenguinOwnerCharacter;
		if (PenguinOwnerCharacter)
		{
			PenguinOwnerController = PenguinOwnerController == nullptr ? Cast<APenguinPlayerController>(PenguinOwnerCharacter->Controller) : PenguinOwnerController;
			if (PenguinOwnerController && HasAuthority() && !PenguinOwnerController->HighPingDelegate.IsBound())
			{
				PenguinOwnerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
			}
		}
		break;
	case EWeaponState::EWS_Dropped:
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
		break;
	}
}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = GetWeaponMesh()->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(GetWeaponMesh());
	
			FActorSpawnParameters SpawnParameters;
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}
	SpendRound();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	PenguinOwnerCharacter = nullptr;
	PenguinOwnerController = nullptr;
	
	PenguinOwnerCharacter = PenguinOwnerCharacter == nullptr ? Cast<APenguinCharacter>(GetOwner()) : PenguinOwnerCharacter;
	if (PenguinOwnerCharacter)
	{
		PenguinOwnerController = PenguinOwnerController == nullptr ? Cast<APenguinPlayerController>(PenguinOwnerCharacter->Controller) : PenguinOwnerController;
		if (PenguinOwnerController && HasAuthority() && !PenguinOwnerController->HighPingDelegate.IsBound())
		{
			PenguinOwnerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
}

 void AWeapon::OnPingTooHigh(bool bPingTooHigh)
 {
	bUseServerSideRewind = !bPingTooHigh;
 }

 FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget)
 {
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return FVector();
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();
	
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec; // Random Point
	const FVector ToEndLoc = EndLoc - TraceStart;

	// DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12.f, FColor::Red, true);
	// DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Blue, true);
	
	// DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + EndLoc * TRACE_LENGTH / ToEndLoc.Size()), FColor::Orange, true);

	return FVector(TraceStart + EndLoc * TRACE_LENGTH / ToEndLoc.Size()); // From MuzzleFlash To Sphere Random Point Vector
 }
