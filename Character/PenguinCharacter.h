#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "PenguinBattle/PenguinTypes/TurningInPlace.h"
#include "PenguinBattle/PenguinTypes/CombatState.h"
#include "PenguinBattle/Interfaces/InteractWithCrosshairInterface.h"
#include "PenguinCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

UCLASS()
class PENGUINBATTLE_API APenguinCharacter : public ACharacter, public IInteractWithCrosshairInterface
{
	GENERATED_BODY()

public:
	APenguinCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayThrowGrenadeMontage();
	
	void Elim(bool bPlayerLeftGame);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);
	
	virtual void OnRep_ReplicatedMovement() override;
	virtual void Destroyed() override;

	void UpdateHUDHealth();
	
	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();
	
	FOnLeftGame OnLeftGame;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedTheLead();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();

	UFUNCTION()
	void SetTeamColor(ETeam Team);
	
protected:
	virtual void BeginPlay() override;
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void ReloadButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void CalculateAO_Pitch();
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();
	void PlayElimMontage();
	void GrenadeButtonPressed();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		class AController* InstigatorController, AActor* DamageCauser);
	
	// Poll for any relevant classes and initialize our HUD
	void PollInit();
	void RotateInPlace(float DeltaTime);
	
	/*
	 *  Grenade
	 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	/*
	 * Hit boxes used for server-side rewind
	 */
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Head;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Pelvis;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Spine_0;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Spine_1;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Spine_2;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Neck;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Arm_L_0;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Arm_R_0;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Arm_L_1;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Arm_R_1;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* Leg_L;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Leg_R;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Foot_L;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* Foot_R;

public:
	UPROPERTY()
	TMap<FName, UBoxComponent*> HitCollisionBoxes;

private:
	UPROPERTY(VisibleAnywhere, Category="Camera")
	class USpringArmComponent* CameraBoom;
	
	UPROPERTY(VisibleAnywhere, Category="Camera")
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	/*
	 *  Components
	 */
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;
	
	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* LagCompensation;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	/*
	 *  Aim Offset
	 */
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);
	
	/*
	 * Montages
	 */

	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* HitReactMontage;

    UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ThrowGrenadeMontage;
	
	/*
	 * To Remove Character when too close
	 */
	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	/*
	 * For Remove RootBone Gitter to Simulated proxy
	 */
	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	float CalculateSpeed();

	/*
	 * Player Health
	 */
	UPROPERTY(EditAnywhere, Category="Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category="Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	UPROPERTY()
	class APenguinPlayerController* PenguinPlayerController;

	/*
	 * Elim
	 */
	bool bElimmed = false;

	FTimerHandle ElimTimer;

	void ElimTimerFinished();

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	UPROPERTY()
	class APenguinPlayerState* PenguinPlayerState;

	bool bLeftGame =false;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;

	UPROPERTY()
	class UNiagaraComponent* CrownComponent;

	UPROPERTY()
	class APenguinGameMode* PenguinGameMode;

	/*
	 * Dissolve Effect
	 */
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category="Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category="Elim")
	UMaterialInstance* DissolveMaterialInstance;
	
public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	ECombatState GetCombatState() const;
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	bool IsLocallyReloading();
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
};
