#pragma once

#include "CoreMinimal.h"
#include "PaperZDAnimInstance.h"
#include "HeroAnimInstance.generated.h"

class APlayerCharacter;

/**
 * C++ base for ABP_Hero. Reparent ABP_Hero to this class in the editor
 * (Class Settings -> Parent Class -> HeroAnimInstance).
 * The ABP state machine reads these properties directly instead of
 * going through the Hero Blueprint variable.
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTSPIRITLESS_API UHeroAnimInstance : public UPaperZDAnimInstance
{
	GENERATED_BODY()

public:
	// ── Movement ──────────────────────────────────────────────────────────
	// Horizontal speed (X axis only for 2.5D)
	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	float MoveSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsMoving = false;

	// Velocity.Z > 0 while airborne
	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsJumping = false;

	// Velocity.Z <= 0 while airborne
	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsFalling = false;

	// ── Combat ────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "State|Combat")
	bool bIsAttacking = false;

	// 0 = none, 1 = Attack1, 2 = Attack2
	UPROPERTY(BlueprintReadOnly, Category = "State|Combat")
	int32 ComboIndex = 0;

	// ── Dead ──────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bJustLanded = false;

	// Called every tick by APlayerCharacter to push state in
	void SyncFromCharacter(APlayerCharacter* Character);
};
