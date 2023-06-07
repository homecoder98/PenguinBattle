#include "TeamGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "PenguinBattle/GameState/PenguinGameState.h"
#include "PenguinBattle/PenguinTypes/Team.h"
#include "PenguinBattle/PlayerController/PenguinPlayerController.h"
#include "PenguinBattle/PlayerState/PenguinPlayerState.h"

ATeamGameMode::ATeamGameMode()
{
	bTeamsMath = true;
}

void ATeamGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	APenguinGameState* BGameState = Cast<APenguinGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		APenguinPlayerState* BPState = NewPlayer->GetPlayerState<APenguinPlayerState>();
		if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam)
		{
			if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
			{
				BGameState->RedTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_RedTeam);
			}
			else
			{
				BGameState->BlueTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}

void ATeamGameMode::Logout(AController* Exiting)
{
	APenguinGameState* BGameState = Cast<APenguinGameState>(UGameplayStatics::GetGameState(this));
	APenguinPlayerState* BPState = Exiting->GetPlayerState<APenguinPlayerState>();
	if (BGameState && BPState)
	{
		if (BGameState->RedTeam.Contains(BPState))
		{
			BGameState->RedTeam.Remove(BPState);
		}
		if (BGameState->BlueTeam.Contains(BPState))
		{
			BGameState->BlueTeam.Remove(BPState);
		}
	}
}

void ATeamGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	APenguinGameState* BGameState = Cast<APenguinGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		for (auto PState : BGameState->PlayerArray)
		{
			APenguinPlayerState* BPState = Cast<APenguinPlayerState>(PState.Get());
			if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
				{
					BGameState->RedTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					BGameState->BlueTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}

float ATeamGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	APenguinPlayerState* AttackerState = Attacker->GetPlayerState<APenguinPlayerState>();
	APenguinPlayerState* VictimState = Victim->GetPlayerState<APenguinPlayerState>();
	if (AttackerState == nullptr || VictimState == nullptr) return BaseDamage;
	if (VictimState == AttackerState)
	{
		return BaseDamage;
	}
	if (AttackerState->GetTeam() == VictimState->GetTeam())
	{
		return 0.f;
	}
	return BaseDamage;
}

void ATeamGameMode::PlayerEliminated(APenguinCharacter* ElimmedCharacter, APenguinPlayerController* VictimController,
	APenguinPlayerController* AttackerController)
{
	Super::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);
	APenguinGameState* BGameState = Cast<APenguinGameState>(UGameplayStatics::GetGameState(this));
	APenguinPlayerState* AttackerPlayerState = AttackerController ? Cast<APenguinPlayerState>(AttackerController->PlayerState) : nullptr;
	if (BGameState && AttackerPlayerState)
	{
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BGameState->BlueTeamScores();
		}
		if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BGameState->RedTeamScores();
		}
	}
}
