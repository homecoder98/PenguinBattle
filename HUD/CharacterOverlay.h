#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

UCLASS()
class PENGUINBATTLE_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta=(BindWidget))
	class UTextBlock* HealthText;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* ScoreAmount;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* DefeatsAmount;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* BlueTeamScore;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* RedTeamScore;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* ScoreSpacerText;

	UPROPERTY(meta=(BindWidget))
	class UImage* HighPingImage;

	UPROPERTY(meta=(BindWidgetAnim), Transient)
	class UWidgetAnimation* HighPingAnimation;
	
	UPROPERTY(meta=(BindWidget))
	UTextBlock* AmmoAmount;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* CarriedAmmoAmount;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* MatchCountdownText;
};
