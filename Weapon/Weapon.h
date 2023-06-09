#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};
UENUM(BlueprintType)
enum class EFireType : uint8
{
	UFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	UFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	UFT_Shothun UMETA(DisplayName = "Shothun Weapon"),
	UFT_MAX UMETA(DisplayName = "DefaultMAX"),
};

UCLASS()
class PENGUINBATTLE_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void OnRep_Owner() override;
	void SetHUDAmmo();
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	void Dropped();
	void AddAmmo(int32 AmmoToAdd);
	

	/*
	 *  Trace end with scatter
	 */
	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false;

	UPROPERTY()
	class APenguinCharacter* PenguinOwnerCharacter;
	UPROPERTY()
	class APenguinPlayerController* PenguinOwnerController;
	
	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
	
	UPROPERTY(EditAnywhere, Category="Weapon Scatter")
	bool bUseScatter = false;

	FVector TraceEndWithScatter(const FVector& HitTarget);
	
	/*
	 *  Textures for the weapon crosshairs
	 */
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	UTexture2D* CrosshairCenter;

	UPROPERTY(EditAnywhere, Category="Crosshairs")
	UTexture2D* CrosshairLeft;

	UPROPERTY(EditAnywhere, Category="Crosshairs")
	UTexture2D* CrosshairRight;

	UPROPERTY(EditAnywhere, Category="Crosshairs")
	UTexture2D* CrosshairTop;

	UPROPERTY(EditAnywhere, Category="Crosshairs")
	UTexture2D* CrosshairBottom;

	/*
	 *  Damage
	 */

	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;
	
	/*
	 * Zoomed FOV while aiming
	 */
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	/*
	 * Automatic fire
	 */
	UPROPERTY(EditAnywhere, Category="Combat")
	float FireDelay = .1f;

	UPROPERTY(EditAnywhere, Category="Combat")
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	/*
	 *  Enable or disable custom depth
	 */
	void EnableCustomDepth(bool bEnable);

	UPROPERTY(EditAnywhere)
	EFireType FireType;
	
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
	
	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState,VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<class ACasing> CasingClass;

	/*
	 *  Ammo
	 */
	UPROPERTY(EditAnywhere)
	int32 Ammo;

	void SpendRound();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);
	
	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	// The number of unprocessed server requests for ammo
	// Incremented in SpendRound, decremented in ClientUpdateAmmo
	int32 Sequence = 0;

public:
	bool IsFull();

private:
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
	
public:
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	bool IsEmpty();
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotamage() const { return HeadShotDamage; }
};