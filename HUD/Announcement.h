#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Announcement.generated.h"

UCLASS()
class PENGUINBATTLE_API UAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget="WarmupTime"))
	class UTextBlock* WarmupTime;

	UPROPERTY(meta=(BindWidget="WarmupTime"))
	UTextBlock* AnnouncementText;
    
	UPROPERTY(meta=(BindWidget="WarmupTime"))
	UTextBlock* InfoText;
};
