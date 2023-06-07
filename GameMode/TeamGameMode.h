#pragma once

#include "CoreMinimal.h"
#include "PenguinGameMode.h"
#include "TeamGameMode.generated.h"

UCLASS()
class PENGUINBATTLE_API ATeamGameMode : public APenguinGameMode
{
	GENERATED_BODY()
	
public:
	ATeamGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;
	virtual void PlayerEliminated(class APenguinCharacter* ElimmedCharacter,
		class APenguinPlayerController* VictimController,
		APenguinPlayerController* AttackerController);
	
protected:
	virtual void HandleMatchHasStarted() override;
};
