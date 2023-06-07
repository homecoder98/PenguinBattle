#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "PenguinGameState.generated.h"

UCLASS()
class PENGUINBATTLE_API APenguinGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class APenguinPlayerState* ScoringPlayer);
	
	UPROPERTY(Replicated)
	TArray<class APenguinPlayerState*> TopScoringPlayers;

	/*
	 *  Teams
	 */
	void RedTeamScores();
	void BlueTeamScores();
	
	TArray<APenguinPlayerState*> RedTeam;
	TArray<APenguinPlayerState*> BlueTeam;

	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	int32 RedTeamScore = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	int32 BlueTeamScore = 0.f;

	UFUNCTION()
	void OnRep_RedTeamScore();

	UFUNCTION()
	void OnRep_BlueTEamScore();
	
private:
	float TopScore = 0.f;
};
