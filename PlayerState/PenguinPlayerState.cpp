#include "PenguinPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "PenguinBattle/Character/PenguinCharacter.h"
#include "PenguinBattle/PlayerController/PenguinPlayerController.h"

void APenguinPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(APenguinPlayerState, Defeats);
	DOREPLIFETIME(APenguinPlayerState, Team);
}

void APenguinPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character == nullptr ? Cast<APenguinCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<APenguinPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void APenguinPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	Character = Character == nullptr ? Cast<APenguinCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<APenguinPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void APenguinPlayerState::OnRep_Defeats()
{
	Character = Character == nullptr ? Cast<APenguinCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<APenguinPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void APenguinPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	Character = Character == nullptr ? Cast<APenguinCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<APenguinPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void APenguinPlayerState::OnRep_Team()
{
	APenguinCharacter* PCharacter = Cast<APenguinCharacter>(GetPawn());
	if (PCharacter)
	{
		PCharacter->SetTeamColor(Team);
	}
}

void APenguinPlayerState::SetTeam(ETeam TeamToSet)
{
	Team = TeamToSet;
	APenguinCharacter* PCharacter = Cast<APenguinCharacter>(GetPawn());
	if (PCharacter)
	{
		PCharacter->SetTeamColor(Team);
	}
}
