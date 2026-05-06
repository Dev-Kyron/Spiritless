#pragma once

#include "CoreMinimal.h"
#include "PaperZDCharacter.h"
#include "EnemyHealthBarWidget.h"
#include "EnemyCharacter.generated.h"

class APlayerCharacter;
class UWidgetComponent;
class UNiagaraSystem;

UCLASS(Blueprintable, BlueprintType)
class PROJECTSPIRITLESS_API AEnemyCharacter : public APaperZDCharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	// ── Identity ──────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FText EnemyName = FText::FromString("Enemy");

	// ── Stats ─────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float CurrentHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float AttackDamage = 15.f;

	// ── Movement ──────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ChaseSpeed = 350.f;

	// ── Melee attack ──────────────────────────────────────────────────────────
	// Distance to stop and punch
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee")
	float AttackRange = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Melee")
	float AttackCooldown = 1.2f;

	// ── Dash attack ───────────────────────────────────────────────────────────
	// Distance band that triggers the dash-in attack (beyond melee, within this)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|DashAttack")
	float DashAttackRange = 750.f;

	// Burst speed of the dash lunge
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|DashAttack")
	float DashAttackSpeed = 1400.f;

	// Window after launch during which the hit is checked
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|DashAttack")
	float DashAttackDuration = 0.28f;

	// Generous hit radius after the dash lands
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|DashAttack")
	float DashAttackHitRadius = 200.f;

	// Damage multiplier vs base AttackDamage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|DashAttack")
	float DashAttackDamageMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|DashAttack")
	float DashAttackCooldown = 7.f;

	// Minimum distance before dash is allowed (gap-closer only, not spam)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|DashAttack")
	float DashAttackMinRange = 400.f;

	// Delay between the impact hit and the follow-up slice hit (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|DashAttack")
	float DashAttackSliceDelay = 0.35f;

	// Damage multiplier for the slice hit (vs base AttackDamage)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|DashAttack")
	float DashAttackSliceDamageMultiplier = 1.0f;

	// ── Combat feel ───────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Feel")
	float KnockbackForce = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Feel")
	float StaggerDuration = 0.25f;

	// ── Detection ─────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DetectionRange = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AggroDropMultiplier = 1.5f;

	// ── State (ABP readable) ──────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsChasing = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDashAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsStaggered = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsHurt = false;

	// How long the hurt anim flash plays
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Feel")
	float HurtAnimDuration = 0.25f;

	// ── AI ────────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol")
	bool bPatrolEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Patrol")
	float PatrolDistance = 300.f;

	// ── Drops ─────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
	TSubclassOf<AActor> SpiritPickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
	int32 SpiritDropCount = 1;

	// ── Audio ─────────────────────────────────────────────────────────────────
	// Assign sound assets in BP_Enemy1 Details
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* FootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float FootstepInterval = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float FootstepVolume = 0.015f;

	// Distance at which footsteps become fully silent (linear falloff from 0 to this distance)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "100.0"))
	float FootstepMaxHearDistance = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* MeleeAttackSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MeleeAttackVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DashAttackSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DashAttackVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* TakeDamageSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float TakeDamageVolume = 1.0f;

	// ── VFX ───────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* HitFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* WalkTrailFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float TrailInterval = 0.07f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DeathSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DeathVolume = 1.0f;

	// ── Health Bar ────────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HealthBarComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UEnemyHealthBarWidget> HealthBarWidgetClass;

	// How long after death before the sprite vanishes (seconds) — tune to match anim length
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DeathSpriteHideDelay = 0.45f;

	// Delay before spirit orbs appear after death
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
	float SpiritSpawnDelay = 0.6f;

private:
	void Die();
	void HideDeathSprite();
	void SpawnSpiritDrops();

	// Melee
	void PerformMeleeAttack();
	void OnMeleeHit1();
	void OnMeleeHit2();
	void ResetMeleeCooldown();

	// Dash attack
	void PerformDashAttack();
	void OnDashAttackLand();
	void OnDashAttackSlice();
	void ResetDashAttackCooldown();

	void UpdateFacing(float XDir);
	void RefreshHealthBar();

	bool  bMeleeOnCooldown      = false;
	bool  bDashAttackOnCooldown = false;
	bool  bStaggerImmune        = false;
	bool  bIsFacingRight        = true;
	bool  bIsAggro              = false;
	bool  bPatrollingRight      = true;
	float TrailAccumulator      = 0.f;
	FVector PatrolOrigin;

	APlayerCharacter*      CachedPlayer   = nullptr;
	UEnemyHealthBarWidget* HealthBarWidget = nullptr;

	FTimerHandle MeleeCooldownHandle;
	FTimerHandle MeleeHit1Handle;
	FTimerHandle MeleeHit2Handle;
	FTimerHandle DashAttackLandHandle;
	FTimerHandle DashAttackSliceHandle;
	FTimerHandle DashAttackCooldownHandle;
	FTimerHandle FootstepTimerHandle;
	FTimerHandle DeathSpriteHideHandle;
	FTimerHandle StaggerHandle;
	FTimerHandle StaggerImmuneHandle;
	FTimerHandle HurtAnimHandle;
	FTimerHandle SpiritSpawnHandle;

	void PlayFootstep();
	void EndStagger();
	void EndHurt();
};
