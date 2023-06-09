#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PenguinBattle/Character/PenguinCharacter.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	APenguinCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<APenguinCharacter* , uint32> HeadShots;

	UPROPERTY()
	TMap<APenguinCharacter* , uint32> BodyShots;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PENGUINBATTLE_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend class APenguinCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, FColor Color);

	/*
	 *  HitScan
	 */
	FServerSideRewindResult  ServerSideRewind(
		class APenguinCharacter* HitCharacter, 
		const FVector_NetQuantize& TraceStart, 
		const FVector_NetQuantize& HitLocation, 
		float HitTime
	);
	
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		APenguinCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime,
		class AWeapon* DamageCauser
	);
	
protected:
	virtual void BeginPlay() override;
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame,
		const FFramePackage& YoungerFrame, float HitTime);
	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package, 
		APenguinCharacter* HitCharacter, 
		const FVector_NetQuantize& TraceStart, 
		const FVector_NetQuantize& HitLocation);
	void CacheBoxPositions(APenguinCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(APenguinCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(APenguinCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(APenguinCharacter* HitCharacter,
		ECollisionEnabled::Type CollisionEnabled);
	void SaveFramePackage();

	/*
	 *  Shotgun
	 */
	FFramePackage GetFrameToCheck(APenguinCharacter* HitCharacter, float HitTime);

public:
	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<APenguinCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart, 
		const TArray<FVector_NetQuantize>& HitLocation,
		float HitTime
	);

	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(
		const TArray<APenguinCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime
	);
protected:
	FShotgunServerSideRewindResult ShotgunConfirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations
	);

	/*
	 *  Projectile
	 */
public:
	FServerSideRewindResult ProjectileServerSideRewind(
		APenguinCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);
	
	FServerSideRewindResult ProjectileConfirmHit(
		const FFramePackage& Package,
		APenguinCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);
	
	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(
		APenguinCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);
	
private:
	UPROPERTY()
	APenguinCharacter* Character;

	UPROPERTY()
	class APenguinPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;
};
