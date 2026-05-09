#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SpiritlessGameInstance.h"
#include "SpiritlessSaveGame.generated.h"

UCLASS()
class PROJECTSPIRITLESS_API USpiritlessSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY() EDifficulty SavedDifficulty = EDifficulty::Medium;
};
