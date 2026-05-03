#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * C++ base for WBP_EnemyHealthBar.
 * Create WBP_EnemyHealthBar in Blueprint (parent = EnemyHealthBarWidget).
 * Name the ProgressBar widget exactly "HealthBar" and the text block exactly "NameText".
 */
UCLASS()
class PROJECTSPIRITLESS_API UEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// These names MUST match the widget names inside WBP_EnemyHealthBar exactly
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* NameText;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetHealthPercent(float Percent);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetEnemyName(const FText& InName);
};
