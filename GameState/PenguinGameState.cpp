#include "PenguinGameState.h"
#include "PenguinBattle/PlayerState/PenguinPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "PenguinBattle/PlayerController/PenguinPlayerController.h"

void APenguinGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APenguinGameState, TopScoringPlayers);
	DOREPLIFETIME(APenguinGameState, RedTeamScore);
	DOREPLIFETIME(APenguinGameState, BlueTeamScore);
}

void APenguinGameState::UpdateTopScore(APenguinPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void APenguinGameState::RedTeamScores()
{
	++RedTeamScore;

	APenguinPlayerController* PPlayer = Cast<APenguinPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PPlayer)
	{
		PPlayer->SetHUDRedTeamScore(RedTeamScore);
	}
}

void APenguinGameState::BlueTeamScores()
{
	++BlueTeamScore;
	
	APenguinPlayerController* PPlayer = Cast<APenguinPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PPlayer)
	{
		PPlayer->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void APenguinGameState::OnRep_RedTeamScore()
{
	APenguinPlayerController* PPlayer = Cast<APenguinPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PPlayer)
	{
		PPlayer->SetHUDRedTeamScore(RedTeamScore);
	}
}

void APenguinGameState::OnRep_BlueTEamScore()
{
	APenguinPlayerController* PPlayer = Cast<APenguinPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PPlayer)
	{
		PPlayer->SetHUDBlueTeamScore(BlueTeamScore);
	}
}
