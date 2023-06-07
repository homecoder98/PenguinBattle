#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PenguinBattle/PenguinTypes/TurningInPlace.h"
#include "PenguinAnimInstance.generated.h"

UCLASS()
class PENGUINBATTLE_API UPenguinAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	class APenguinCharacter* PenguinCharacter;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	float Speed;
	
	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bIsAccelerating;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bWeaponEquipped;

	class AWeapon* EquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bIsCrouched;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bAiming;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	float YawOffset;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	float Lean;

	FRotator CharacterRotationLastFrame;
	FRotator CharacterRotation;
	FRotator DeltaRotation;

	/*
	 *  Aim Offset
	 */
	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	float AO_Yaw;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	float AO_Pitch;
	
	/*
	 *  FABRIK
	 */
	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	FTransform LeftHandTransform;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	FRotator RightHandRotation;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bLocallyControlled;
	
	/*
	 *  Turning In Place
	 */
	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	ETurningInPlace TurningInPlace;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bRotateRootBone;

	
	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bElimmed;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bUseFABRIK;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bUseOffsets;

	UPROPERTY(BlueprintReadOnly, Category="Character", meta=(AllowPrivateAccess="true"))
	bool bTransformRightHand;
};
