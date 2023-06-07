#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PenguinGameMode.generated.h"

namespace MatchState
{
	// Match duration has been reached.Display winner and begin cooldown timer.
	extern PENGUINBATTLE_API const FName Cooldown;
}
UCLASS()
class PENGUINBATTLE_API APenguinGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	APenguinGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class APenguinCharacter* ElimmedCharacter,
		class APenguinPlayerController* VictimController,
		APenguinPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);
	void PlayerLeftGame(class APenguinPlayerState* PlayerLeaving);
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;
	
	float LevelStartingTime = 0.f;

	bool bTeamsMath = false;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
	
private:
	float CountdownTime = 0.f;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
