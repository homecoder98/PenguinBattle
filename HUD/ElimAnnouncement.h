#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncement.generated.h"

UCLASS()
class PENGUINBATTLE_API UElimAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetElimAnnouncementText(FString AttackerName, FString VictimName);
	
	UPROPERTY(meta=(BindWidget))
	class UHorizontalBox* AnnouncementBox;

	UPROPERTY(meta=(BindWidget))
	class UTextBlock* AnnouncementText;
};
