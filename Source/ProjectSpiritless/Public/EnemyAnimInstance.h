#pragma once

#include "CoreMinimal.h"
#include "PaperZDAnimInstance.h"
#include "EnemyAnimInstance.generated.h"

class AEnemyCharacter;

UCLASS(Blueprintable, BlueprintType)
class PROJECTSPIRITLESS_API UEnemyAnimInstance : public UPaperZDAnimInstance
{
	GENERATED_BODY()

public:
	// Is the enemy walking toward the player
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsWalking = false;

	// Melee attack swing
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsAttacking = false;

	// Dash lunge in progress
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDashAttacking = false;

	// Dead
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	// Called every tick from EnemyCharacter
	void SyncFromEnemy(AEnemyCharacter* Enemy);
};
