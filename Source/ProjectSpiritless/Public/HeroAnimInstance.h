#pragma once

#include "CoreMinimal.h"
#include "PaperZDAnimInstance.h"
#include "HeroAnimInstance.generated.h"

class APlayerCharacter;

UCLASS(Blueprintable, BlueprintType)
class PROJECTSPIRITLESS_API UHeroAnimInstance : public UPaperZDAnimInstance
{
	GENERATED_BODY()

public:
	// ── Movement ──────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	float MoveSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsMoving = false;

	// Legacy bools — kept so existing ABP transitions still compile
	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsJumping = false;

	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsFalling = false;

	// Granular jump phases — use these for the full flipbook set
	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsJumpRising = false;   // JUMP / JUMP-START  (Z vel > 250)

	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsJumpApex = false;     // JUMP-TRANSITION    (-200 < Z vel <= 250)

	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsJumpFalling = false;  // JUMP-FALL          (Z vel <= -200)

	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsDashing = false;

	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsWallSliding = false;

	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bIsWallJumping = false;

	UPROPERTY(BlueprintReadOnly, Category = "State|Movement")
	bool bJustLanded = false;

	// ── Combat ────────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "State|Combat")
	bool bIsAttacking = false;

	// 0 = air attack, 1–4 = ground combo step
	UPROPERTY(BlueprintReadOnly, Category = "State|Combat")
	int32 ComboIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "State|Combat")
	bool bIsAirAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "State|Combat")
	bool bIsDefending = false;

	UPROPERTY(BlueprintReadOnly, Category = "State|Combat")
	bool bIsDefendBroken = false;

	UPROPERTY(BlueprintReadOnly, Category = "State|Combat")
	bool bIsHealing = false;

	UPROPERTY(BlueprintReadOnly, Category = "State|Combat")
	bool bIsHurt = false;

	// ── Death ─────────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	// Called every tick by APlayerCharacter to push state in
	void SyncFromCharacter(APlayerCharacter* Character);
};
