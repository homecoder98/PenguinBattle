#include "PenguinAnimInstance.h"
#include "PenguinCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "PenguinBattle/Weapon/Weapon.h"

void UPenguinAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PenguinCharacter = Cast<APenguinCharacter>(TryGetPawnOwner());
}

void UPenguinAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (PenguinCharacter == nullptr)
	{
		PenguinCharacter = Cast<APenguinCharacter>(TryGetPawnOwner());
	}
	if (PenguinCharacter == nullptr) return;
	
	FVector Velocity = PenguinCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = PenguinCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = PenguinCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = PenguinCharacter->IsWeaponEquipped();
	EquippedWeapon = PenguinCharacter->GetEquippedWeapon();
	bIsCrouched = PenguinCharacter->bIsCrouched;
	bAiming = PenguinCharacter->IsAiming();
	TurningInPlace = PenguinCharacter->GetTurningInPlace();
	bRotateRootBone = PenguinCharacter->ShouldRotateRootBone();
	bElimmed = PenguinCharacter->IsElimmed();

	// Offset Yaw for Strafing
	const FRotator AimRotation = PenguinCharacter->GetBaseAimRotation();//요게 자동 리플리케이션이라 yawoffset,lean 자동 복제됨
	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(PenguinCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);//-180~180전환시 지글없애려 0~180로 변환
	YawOffset = DeltaRotation.Yaw;

	// CharacterRotationLastFrame = CharacterRotation;
	// CharacterRotation = PenguinCharacter->GetActorRotation();
	// const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotationLastFrame, CharacterRotation);
	// const float Target = Delta.Yaw / DeltaTime;
	// const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	// Lean = FMath::Clamp(Interp, -90., 90.f);

	// Aim Offset
	AO_Yaw = PenguinCharacter->GetAO_Yaw();
	AO_Pitch = PenguinCharacter->GetAO_Pitch();

	// UE_LOG(LogTemp,Warning,TEXT("yaw : %f, pitch : %f"),AO_Yaw, AO_Pitch);
	
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && PenguinCharacter->GetMesh())
	{
		// 왼손 FABRIK
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		PenguinCharacter->GetMesh()->TransformToBoneSpace(FName("Arm__R_2"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (PenguinCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			// 현재 보는 방향으로 총구 확인
			// FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Arm__R_2"), ERelativeTransformSpace::RTS_World);
			FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), RTS_World);
			FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::Y));
			// FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(PenguinCharacter->GetActorLocation(), PenguinCharacter->GetHitTarget());

			DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f,FColor::Red);
			DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), PenguinCharacter->GetHitTarget(), FColor::Orange);
		}
	}
	
	
	bUseFABRIK = PenguinCharacter->GetCombatState() != ECombatState::ECS_Unoccupied;
	if (PenguinCharacter->IsLocallyControlled())
	{
		bUseFABRIK = !PenguinCharacter->IsLocallyReloading();
	}
	bUseOffsets = PenguinCharacter->GetCombatState() != ECombatState::ECS_Unoccupied && !PenguinCharacter->GetDisableGameplay();
	bTransformRightHand = PenguinCharacter->GetCombatState() != ECombatState::ECS_Unoccupied && !PenguinCharacter->GetDisableGameplay();
}
