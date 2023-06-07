#include "PenguinGameMode.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "PenguinBattle/Character/PenguinCharacter.h"
#include "PenguinBattle/GameState/PenguinGameState.h"
#include "PenguinBattle/PlayerController/PenguinPlayerController.h"
#include "PenguinBattle/PlayerState/PenguinPlayerState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

APenguinGameMode::APenguinGameMode()
{
	bDelayedStart = true;
}

void APenguinGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void APenguinGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator();It;++It)
	{
		APenguinPlayerController* PenguinPlayer = Cast<APenguinPlayerController>(*It);
		if (PenguinPlayer)
		{
			PenguinPlayer->OnMatchStateSet(MatchState, bTeamsMath);
		}
	}
}

void APenguinGameMode::PlayerEliminated(APenguinCharacter* ElimmedCharacter, APenguinPlayerController* VictimController,
                                        APenguinPlayerController* AttackerController)
{
	APenguinPlayerState* AttackerPlayerState = AttackerController ? Cast<APenguinPlayerState>(AttackerController->PlayerState) : nullptr;
	APenguinPlayerState* VictimPlayerState = VictimController ? Cast<APenguinPlayerState>(VictimController->PlayerState) : nullptr;

	APenguinGameState* PenguinGameState = GetGameState<APenguinGameState>();
	
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && PenguinGameState)
	{
		TArray<APenguinPlayerState*> PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : PenguinGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.Add(LeadPlayer);
		}

		if (PenguinGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			APenguinCharacter* Leader = Cast<APenguinCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainedTheLead();
			}
		}
		
		AttackerPlayerState->AddToScore(1.f);
		PenguinGameState->UpdateTopScore(AttackerPlayerState);

		for (int32 i=0;i < PlayersCurrentlyInTheLead.Num();i++)
		{
			if (!PenguinGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				APenguinCharacter* Loser = Cast<APenguinCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser)
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}
	
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim(false);
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APenguinPlayerController* PenguinPlayer = Cast<APenguinPlayerController>(*It);
		if (PenguinPlayer && AttackerPlayerState && VictimPlayerState)
		{
			PenguinPlayer->BroadcastElim(AttackerPlayerState, VictimPlayerState);
		}
	}
}

void APenguinGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Restart();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void APenguinGameMode::BeginPlay()
{
	Super::BeginPlay();
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void APenguinGameMode::PlayerLeftGame(APenguinPlayerState* PlayerLeaving)
{
	// TODO call Elim, passing in true for bLeftGmae
	if (PlayerLeaving == nullptr) return;
	APenguinGameState* PenguinGameState = GetGameState<APenguinGameState>();
	if (PenguinGameState && PenguinGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		PenguinGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}
	APenguinCharacter* CharacterLeaving = Cast<APenguinCharacter>(PlayerLeaving->GetPawn());
	if (CharacterLeaving)
	{
		CharacterLeaving->Elim(true);
	}
}

float APenguinGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}
