#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpiritHUDWidget.generated.h"

class APlayerCharacter;

UCLASS(Blueprintable, BlueprintType)
class PROJECTSPIRITLESS_API USpiritHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	// Current orbs being carried
	UFUNCTION(BlueprintPure, Category = "Spirits")
	int32 GetSpiritCount() const;

	// Total orbs banked at deposit points
	UFUNCTION(BlueprintPure, Category = "Spirits")
	int32 GetDepositedCount() const;

private:
	APlayerCharacter* CachedPlayer = nullptr;
};
