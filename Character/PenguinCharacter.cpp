#include "PenguinCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "PenguinBattle/PenguinBattle.h"
#include "PenguinBattle/GameMode/PenguinGameMode.h"
#include "PenguinBattle/GameState/PenguinGameState.h"
#include "PenguinBattle/PenguinComponents/BuffComponent.h"
#include "PenguinBattle/PenguinCOmponents/CombatComponent.h"
#include "PenguinBattle/PenguinComponents/LagCompensationComponent.h"
#include "PenguinBattle/PlayerController/PenguinPlayerController.h"
#include "PenguinBattle/PlayerState/PenguinPlayerState.h"
#include "PenguinBattle/Weapon/Weapon.h"

APenguinCharacter::APenguinCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);
	
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));
	
	/*
	 *  Hit boxes for server-side rewind
	 */
	Head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	Head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("Head"), Head);

	Pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("Pelvis"));
	Pelvis->SetupAttachment(GetMesh(), FName("Pelvis"));
	HitCollisionBoxes.Add(FName("Pelvis"), Pelvis);

	Spine_0 = CreateDefaultSubobject<UBoxComponent>(TEXT("Spine_0"));
	Spine_0->SetupAttachment(GetMesh(), FName("Spine_0"));
	HitCollisionBoxes.Add(FName("Spine_0"), Spine_0);

	Spine_1 = CreateDefaultSubobject<UBoxComponent>(TEXT("Spine_1"));
	Spine_1->SetupAttachment(GetMesh(), FName("Spine_1"));
	HitCollisionBoxes.Add(FName("Spine_1"), Spine_1);

	Spine_2 = CreateDefaultSubobject<UBoxComponent>(TEXT("Spine_2"));
	Spine_2->SetupAttachment(GetMesh(), FName("Spine_2"));
	HitCollisionBoxes.Add(FName("Spine_2"), Spine_2);

	Neck = CreateDefaultSubobject<UBoxComponent>(TEXT("Neck"));
	Neck->SetupAttachment(GetMesh(), FName("Neck"));
	HitCollisionBoxes.Add(FName("Neck"), Neck);

	Arm_L_0 = CreateDefaultSubobject<UBoxComponent>(TEXT("Arm_L_0"));
	Arm_L_0->SetupAttachment(GetMesh(), FName("Arm_L_0"));
	HitCollisionBoxes.Add(FName("Arm_L_0"), Arm_L_0);

	Arm_R_0 = CreateDefaultSubobject<UBoxComponent>(TEXT("Arm_R_0"));
	Arm_R_0->SetupAttachment(GetMesh(), FName("Arm_R_0"));
	HitCollisionBoxes.Add(FName("Arm_R_0"), Arm_R_0);

	Arm_L_1 = CreateDefaultSubobject<UBoxComponent>(TEXT("Arm_L_1"));
	Arm_L_1->SetupAttachment(GetMesh(), FName("Arm_L_1"));
	HitCollisionBoxes.Add(FName("Arm_L_1"), Arm_L_1);

	Arm_R_1 = CreateDefaultSubobject<UBoxComponent>(TEXT("Arm_R_1"));
	Arm_R_1->SetupAttachment(GetMesh(), FName("Arm_R_1"));
	HitCollisionBoxes.Add(FName("Arm_R_1"), Arm_R_1);

	Leg_L = CreateDefaultSubobject<UBoxComponent>(TEXT("Leg_L"));
	Leg_L->SetupAttachment(GetMesh(), FName("Leg_L"));
	HitCollisionBoxes.Add(FName("Leg_L"), Leg_L);

	Leg_R = CreateDefaultSubobject<UBoxComponent>(TEXT("Leg_R"));
	Leg_R->SetupAttachment(GetMesh(), FName("Leg_R"));
	HitCollisionBoxes.Add(FName("Leg_R"), Leg_R);

	Foot_L = CreateDefaultSubobject<UBoxComponent>(TEXT("Foot_L"));
	Foot_L->SetupAttachment(GetMesh(), FName("Foot_L"));
	HitCollisionBoxes.Add(FName("Foot_L"), Foot_L);

	Foot_R = CreateDefaultSubobject<UBoxComponent>(TEXT("Foot_R"));
	Foot_R->SetupAttachment(GetMesh(), FName("Foot_R"));
	HitCollisionBoxes.Add(FName("Foot_R"), Foot_R);

	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void APenguinCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APenguinCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(APenguinCharacter, Health);
	DOREPLIFETIME(APenguinCharacter, bDisableGameplay);
}

void APenguinCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
	}
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<APenguinPlayerController>(Controller);
		}
	}
}

void APenguinCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleHip") : FName("RifleAim");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void APenguinCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void APenguinCharacter::PlayElimMontage()
{
	if (Combat == nullptr) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void APenguinCharacter::PlayThrowGrenadeMontage()
{
	if (Combat == nullptr) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void APenguinCharacter::GrenadeButtonPressed()
{
	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void APenguinCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
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
			case EWeaponType::EWT_RocketLauncher:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_Pistol:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_SubmachinGun:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_Shotgun:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_SniperRifle:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_GrenadeLauncher:
				SectionName = FName("Rifle");
				break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void APenguinCharacter::Elim(bool bPlayerLeftGame)
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	
	MulticastElim(bPlayerLeftGame);
}

void APenguinCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;
	if (PenguinPlayerController)
	{
		PenguinPlayerController->SetHUDWeaponAmmo(0);
	}
	bElimmed = true;
	PlayElimMontage();

	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
	
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&APenguinCharacter::ElimTimerFinished,
		ElimDelay
	);
	
	// Start Dissolve Effect
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Disable Character Movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
	if (PenguinPlayerController)
	{
		DisableInput(PenguinPlayerController);
	}

	// Disabled Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APenguinCharacter::ElimTimerFinished()
{
	PenguinGameMode = PenguinGameMode == nullptr ? GetWorld()->GetAuthGameMode<APenguinGameMode>() : PenguinGameMode;
	if (PenguinGameMode && !bLeftGame)
	{
		PenguinGameMode->RequestRespawn(this, Controller);
	}
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void APenguinCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                      AController* InstigatorController, AActor* DamageCauser)
{
	PenguinGameMode = PenguinGameMode == nullptr ? GetWorld()->GetAuthGameMode<APenguinGameMode>() : PenguinGameMode;
	if (bElimmed || PenguinGameMode == nullptr) return;
	Damage = PenguinGameMode->CalculateDamage(InstigatorController, Controller, Damage);
	
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();

	// When die
	if (Health == 0.f)
	{
		PenguinGameMode = PenguinGameMode == nullptr ? GetWorld()->GetAuthGameMode<APenguinGameMode>() : PenguinGameMode;
		
		if (PenguinGameMode)
		{
			PenguinPlayerController = PenguinPlayerController == nullptr ? Cast<APenguinPlayerController>(Controller) : PenguinPlayerController;
			APenguinPlayerController* AttackerController = Cast<APenguinPlayerController>(InstigatorController);
			PenguinGameMode->PlayerEliminated(this, PenguinPlayerController, AttackerController);
		}
	}
	
}

void APenguinCharacter::UpdateHUDHealth()
{
	PenguinPlayerController = PenguinPlayerController == nullptr ? Cast<APenguinPlayerController>(Controller) : PenguinPlayerController;
	if (PenguinPlayerController)
	{
		PenguinPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void APenguinCharacter::PollInit()
{
	if (PenguinPlayerState == nullptr)
	{
		PenguinPlayerState = GetPlayerState<APenguinPlayerState>();
		if (PenguinPlayerState)
		{
			PenguinPlayerState->AddToScore(0.f);
			PenguinPlayerState->AddToDefeats(0);
			SetTeamColor(PenguinPlayerState->GetTeam());

			APenguinGameState* PenguinGameState = Cast<APenguinGameState>(UGameplayStatics::GetGameState(this));
			if (PenguinGameState && PenguinGameState->TopScoringPlayers.Contains(PenguinPlayerState))
			{
				MulticastGainedTheLead();
			}
		}
	}
}

void APenguinCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
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

void APenguinCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void APenguinCharacter::Destroyed()
{
	Super::Destroyed();

	PenguinGameMode = PenguinGameMode == nullptr ? GetWorld()->GetAuthGameMode<APenguinGameMode>() : PenguinGameMode;
	bool bMatchNotInProgress = PenguinGameMode && PenguinGameMode->GetMatchState() != MatchState::InProgress;
	
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}

	bool bHideSniperScope = IsLocallyControlled() &&
		Combat->bAiming && Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}
}

void APenguinCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem == nullptr) return;
	if (CrownComponent == nullptr)
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetCapsuleComponent(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 110.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
	if (CrownComponent)
	{
		CrownComponent->Activate();
	}
}

void APenguinCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

void APenguinCharacter::SetTeamColor(ETeam Team)
{
	if (GetMesh() == nullptr) return;
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		switch (Team)
		{
			case ETeam::ET_RedTeam:
				DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Red"), 10.f);
				break;
			case ETeam::ET_BlueTeam:
				DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Red"), 0.f);
				break;
			case ETeam::ET_NoTeam:
				DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Red"), 1.f);
				break;
		}
		
	}
}

void APenguinCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &APenguinCharacter::ReceiveDamage);
	}
	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void APenguinCharacter::Tick(float DeltaTime) 
{
	Super::Tick(DeltaTime);
	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
}

void APenguinCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this,&APenguinCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this,&APenguinCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this,&APenguinCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this,&APenguinCharacter::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &APenguinCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &APenguinCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &APenguinCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &APenguinCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &APenguinCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &APenguinCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &APenguinCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &APenguinCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &APenguinCharacter::GrenadeButtonPressed);
}

void APenguinCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void APenguinCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void APenguinCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void APenguinCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void APenguinCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;
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

void APenguinCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void APenguinCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->Reload();
	}
}

void APenguinCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void APenguinCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void APenguinCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	if (Speed == 0.f && !bIsInAir) // standing still, not jumping
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);

		AO_Yaw = DeltaAimRotation.Yaw;

		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
			bUseControllerRotationYaw = false;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir) // running, or jumping
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void APenguinCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch && !IsLocallyControlled())
	{
		FVector2d InRange(270.f, 360.f);
		FVector2d OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
}

void APenguinCharacter::TurnInPlace(float DeltaTime)
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

void APenguinCharacter::SimProxiesTurn() // 루트본 리플리케이션시 틱보다 느려 지터 현상 없애려고.컨트롤 안 하는 애들
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
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

void APenguinCharacter::Jump()
{
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void APenguinCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void APenguinCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

//클라이언트가 실행시 서버 머신에서 실행
void APenguinCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void APenguinCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
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

float APenguinCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return  Velocity.Size();
}

void APenguinCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void APenguinCharacter::ServerLeaveGame_Implementation()
{
	PenguinGameMode = PenguinGameMode == nullptr ? GetWorld()->GetAuthGameMode<APenguinGameMode>() : PenguinGameMode;
	PenguinPlayerState = PenguinPlayerState == nullptr ? GetPlayerState<APenguinPlayerState>() : PenguinPlayerState;
	if (PenguinGameMode && PenguinPlayerState)
	{
		PenguinGameMode->PlayerLeftGame(PenguinPlayerState);
	}
}

void APenguinCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void APenguinCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &APenguinCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void APenguinCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())//서버일때
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool APenguinCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool APenguinCharacter::IsAiming()
{
	return (Combat && Combat->bAiming); 
}

AWeapon* APenguinCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector APenguinCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ECombatState APenguinCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

bool APenguinCharacter::IsLocallyReloading()
{
	if (Combat == nullptr) return false;
	return Combat->bLocallyReloading;
}

void APenguinCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{//클라이언트일때 호출
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}
