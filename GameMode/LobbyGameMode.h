#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

UCLASS()
class PENGUINBATTLE_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
};
