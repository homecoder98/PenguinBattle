#include "PenguinPlayerController.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PenguinBattle/Character/PenguinCharacter.h"
#include "PenguinBattle/GameMode/PenguinGameMode.h"
#include "PenguinBattle/GameState/PenguinGameState.h"
#include "PenguinBattle/HUD/Announcement.h"
#include "PenguinBattle/HUD/CharacterOverlay.h"
#include "PenguinBattle/HUD/PenguinHUD.h"
#include "PenguinBattle/HUD/ReturnToMainMenu.h"
#include "PenguinBattle/PenguinComponents/CombatComponent.h"
#include "PenguinBattle/PenguinTypes/Announcement.h"
#include "PenguinBattle/PlayerState/PenguinPlayerState.h"

void APenguinPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	PenguinHUD = Cast<APenguinHUD>(GetHUD());
	ServerCheckMatchState();
}

void APenguinPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APenguinPlayerController, MatchState);
	DOREPLIFETIME(APenguinPlayerController, bShowTeamScores);
}

void APenguinPlayerController::HideTeamScore()
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->BlueTeamScore &&
		PenguinHUD->CharacterOverlay->RedTeamScore &&
		PenguinHUD->CharacterOverlay->ScoreSpacerText;

	if (bHUDValid)
	{
		PenguinHUD->CharacterOverlay->BlueTeamScore->SetText(FText());
		PenguinHUD->CharacterOverlay->RedTeamScore->SetText(FText());
		PenguinHUD->CharacterOverlay->ScoreSpacerText->SetText(FText());
	}
}

void APenguinPlayerController::InitTeamScore()
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->BlueTeamScore &&
		PenguinHUD->CharacterOverlay->RedTeamScore &&
		PenguinHUD->CharacterOverlay->ScoreSpacerText;

	if (bHUDValid)
	{
		FString Zero("0");
		FString Spacer("|");
		PenguinHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(Zero));
		PenguinHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(Zero));
		PenguinHUD->CharacterOverlay->ScoreSpacerText->SetText(FText::FromString(Spacer));
	}
}

void APenguinPlayerController::SetHUDRedTeamScore(int32 RedScore)
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->RedTeamScore;

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), RedScore);
		PenguinHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void APenguinPlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->BlueTeamScore;

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), BlueScore);
		PenguinHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void APenguinPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetHUDTime();
	CheckTimeSync(DeltaTime);
	CheckPing(DeltaTime);
	PollInit();
}

void APenguinPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		PenguinGameMode = PenguinGameMode == nullptr ? Cast<APenguinGameMode>(UGameplayStatics::GetGameMode(this)) : PenguinGameMode;
		if (PenguinGameMode)
		{
			SecondsLeft = FMath::CeilToInt(PenguinGameMode->GetCountdownTime() + PenguinGameMode->GetCountdownTime());
		}
	}
	
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

void APenguinPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (PenguinHUD && PenguinHUD->CharacterOverlay)
		{
			CharacterOverlay = PenguinHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);

				APenguinCharacter* PenguinCharacter = Cast<APenguinCharacter>(GetPawn());
				if (PenguinCharacter && PenguinCharacter->GetCombat())
				{
					SetHUDGrenades(PenguinCharacter->GetCombat()->GetGrenades());
				}
			}
		}
	}
}

void APenguinPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void APenguinPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr) return;
	// TODO show the return to Main Menu widget
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

void APenguinPlayerController::OnRep_ShowTeamScores()
{
	if (bShowTeamScores)
	{
		InitTeamScore();
	}
	else
	{
		HideTeamScore();
	}
}

FString APenguinPlayerController::GetInfoText(const TArray<APenguinPlayerState*>& Players)
{
	APenguinPlayerState* PenguinPlayerState = GetPlayerState<APenguinPlayerState>();
	if (PenguinPlayerState == nullptr) return FString();
	FString InfoTextString;
	if (Players.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (Players.Num() == 1 && Players[0] == PenguinPlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	else if (Players.Num() == 1)
	{
		InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *Players[0]->GetPlayerName());
	}
	else if (Players.Num() > 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
		InfoTextString.Append(FString("\n"));
		for (auto TiedPlayer : Players)
		{
			InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
		}
	}

	return InfoTextString;
}

FString APenguinPlayerController::GetTeamsInfoText(APenguinGameState* PenguinGameState)
{
	if (PenguinGameState == nullptr) return FString();
	FString InfoTextString;

	const int32 RedTeamScore = PenguinGameState->RedTeamScore;
	const int32 BlueTeamScore = PenguinGameState->BlueTeamScore;

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore)
	{
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::TeamsTiedForTheWin);
		InfoTextString.Append(Announcement::RedTeam);
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(Announcement::BlueTeam);
		InfoTextString.Append(TEXT("\n"));
	}
	else if (RedTeamScore > BlueTeamScore)
	{
		InfoTextString = Announcement::RedTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
	}
	else if (BlueTeamScore > RedTeamScore)
	{
		InfoTextString = Announcement::BlueTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::BlueTeam, BlueTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d\n"), *Announcement::RedTeam, RedTeamScore));
	}

	return InfoTextString;
}

void APenguinPlayerController::CheckPing(float DeltaTime)
{
	if (HasAuthority()) return;
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState)
		{
			// UE_LOG(LogTemp, Warning, TEXT("PlayerState->GetPing() * 4 : %d"), PlayerState->GetPing() * 4);
			if (PlayerState->GetPingInMilliseconds() * 4 > HighPingThreshold) // ping is compressed; it's actually ping / 4
				{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
				}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighPingAnimationPlaying =
		PenguinHUD && PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->HighPingAnimation &&
		PenguinHUD->CharacterOverlay->IsAnimationPlaying(PenguinHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void APenguinPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void APenguinPlayerController::HighPingWarning()
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;
	
	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->HighPingImage &&
		PenguinHUD->CharacterOverlay->HighPingAnimation;
	
	if (bHUDValid)
	{
		PenguinHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		PenguinHUD->CharacterOverlay->PlayAnimation(PenguinHUD->CharacterOverlay->HighPingAnimation,0.f, 5);
	}
}

void APenguinPlayerController::StopHighPingWarning()
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;
	
	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->HighPingImage &&
		PenguinHUD->CharacterOverlay->HighPingAnimation;
	
	if (bHUDValid)
	{
		PenguinHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (PenguinHUD->CharacterOverlay->IsAnimationPlaying(PenguinHUD->CharacterOverlay->HighPingAnimation))
		{
			PenguinHUD->CharacterOverlay->StopAnimation(PenguinHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void APenguinPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Cooldown, float Match, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	LevelStartingTime = StartingTime;
	CooldownTime = Cooldown;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	
	if (PenguinHUD && MatchState == MatchState::WaitingToStart)
	{
		PenguinHUD->AddAnnouncement();
	}
}

void APenguinPlayerController::ServerCheckMatchState_Implementation()
{
	APenguinGameMode* GameMode = Cast<APenguinGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, CooldownTime,MatchTime, LevelStartingTime);
	}
}

void APenguinPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr) return;
	InputComponent->BindAction("Quit", IE_Pressed, this, &APenguinPlayerController::ShowReturnToMainMenu);
}

void APenguinPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;
	
	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->HealthBar &&
		PenguinHUD->CharacterOverlay->HealthText;
	
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		PenguinHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		const FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		PenguinHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
		bInitializeCharacterOverlay = true;
	}
}

void APenguinPlayerController::SetHUDScore(float Score)
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->ScoreAmount;

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		PenguinHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		HUDScore = Score;
		bInitializeCharacterOverlay = true;
	}
}

void APenguinPlayerController::SetHUDDefeats(int32 Defeats)
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->DefeatsAmount;

	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		PenguinHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		HUDDefeats = Defeats;
		bInitializeCharacterOverlay = true;
	}
}

void APenguinPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->AmmoAmount;

	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		PenguinHUD->CharacterOverlay->AmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void APenguinPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->CarriedAmmoAmount;

	if (bHUDValid)
	{
		FString CarriedText = FString::Printf(TEXT("%d"), Ammo);
		PenguinHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedText));
	}
}

void APenguinPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->MatchCountdownText;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			PenguinHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		PenguinHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void APenguinPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->Announcement &&
		PenguinHUD->Announcement->WarmupTime;

	if (bHUDValid)
	{

		if (CountdownTime < 0.f)
		{
			PenguinHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		PenguinHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void APenguinPlayerController::SetHUDGrenades(int32 Grenades)
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;

	bool bHUDValid = PenguinHUD &&
		PenguinHUD->CharacterOverlay &&
		PenguinHUD->CharacterOverlay->GrenadeText;

	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		PenguinHUD->CharacterOverlay->GrenadeText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		HUDGrenades = Grenades;
	}
}

void APenguinPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	APenguinCharacter* PenguinCharacter = Cast<APenguinCharacter>(InPawn);
	if (PenguinCharacter)
	{
		SetHUDHealth(PenguinCharacter->GetHealth(), PenguinCharacter->GetMaxHealth());
	}
}

void APenguinPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void APenguinPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float APenguinPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void APenguinPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void APenguinPlayerController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
	MatchState = State;
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamsMatch);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void APenguinPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void APenguinPlayerController::HandleMatchHasStarted(bool bTeamsMatch)
{
	if (HasAuthority()) bShowTeamScores = bTeamsMatch;
	
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;
	if (PenguinHUD)
	{
		if (PenguinHUD->CharacterOverlay == nullptr) PenguinHUD->AddCharacterOverlay(); //바로 시작하려고
		if (PenguinHUD->Announcement)
		{
			PenguinHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
		if (!HasAuthority()) return;
		
		if (bTeamsMatch)
		{
			InitTeamScore();
		}
		else
		{
			HideTeamScore();
		}
	}
}

void APenguinPlayerController::HandleCooldown()
{
	PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;
	if (PenguinHUD)
	{
		PenguinHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = PenguinHUD->Announcement &&
			PenguinHUD->Announcement->AnnouncementText &&
			PenguinHUD->Announcement->InfoText;
		
		if (bHUDValid)
		{
			PenguinHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText = Announcement::NewMatchStartsIn;
			PenguinHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			APenguinGameState* PenguinGameState = Cast<APenguinGameState>(UGameplayStatics::GetGameState(this));
			APenguinPlayerState* PenguinPlayerState = GetPlayerState<APenguinPlayerState>();
			if (PenguinGameState && PenguinPlayerState)
			{
				TArray<APenguinPlayerState*> TopPlayers = PenguinGameState->TopScoringPlayers;
				FString InfoTextString = bShowTeamScores ? GetTeamsInfoText(PenguinGameState) : GetInfoText(TopPlayers);
				PenguinHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	APenguinCharacter* PenguinCharacter = Cast<APenguinCharacter>(GetPawn());
	if (PenguinCharacter && PenguinCharacter->GetCombat())
	{
		PenguinCharacter->bDisableGameplay = true;
		PenguinCharacter->GetCombat()->FireButtonPressed(false);
	}
}

void APenguinPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
}

void APenguinPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		PenguinHUD = PenguinHUD ==nullptr ? Cast<APenguinHUD>(GetHUD()) : PenguinHUD;
		if (PenguinHUD)
		{
			if (Attacker == Self && Victim != Self)
			{
				PenguinHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
				return;
			}
			if (Victim == Self && Attacker != Self)
			{
				PenguinHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "You");
				return;
			}
			if (Attacker == Victim && Attacker == Self)
			{
				PenguinHUD->AddElimAnnouncement("You", "yourself");
				return;
			}
			if (Attacker == Victim && Attacker != Self)
			{
				PenguinHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "themselves");
				return;
			}
			PenguinHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
	}
}
