#pragma once

#include "CoreMinimal.h"
#include "PaperZDCharacter.h"
#include "InputActionValue.h"
#include "PaperZDAnimationComponent.h"
#include "PlayerCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class UCineCameraComponent;
class USpringArmComponent;
class UHeroAnimInstance;
class UPaperZDAnimSequence;
class UMaterialParameterCollection;
class UCameraShakeBase;
class ACameraActor;
class USoundBase;

UCLASS(Blueprintable, BlueprintType)
class PROJECTSPIRITLESS_API APlayerCharacter : public APaperZDCharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Jump() override;
	virtual void StopJumping() override;
	virtual void Landed(const FHitResult& Hit) override;

	// ── Camera ────────────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCineCameraComponent* SideCamera;

	// Assign MPC_Player in BP_Hero Details
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UMaterialParameterCollection* PlayerMPC;

	// Place a CineCameraActor in the level and assign here — view cuts to it on death
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	ACameraActor* DeathCamera;

	// Time to fade TO black after death (sprite hides halfway through)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float DeathFadeToBlackTime = 0.7f;

	// Time to fade FROM black to reveal the death cam
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float DeathFadeFromBlackTime = 1.2f;

	// ── Input ─────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Move;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Jump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Dash;

	// ── Attack animations ─────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animations")
	UPaperZDAnimSequence* AttackSequence1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animations")
	UPaperZDAnimSequence* AttackSequence2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float Attack1Duration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float Attack2Duration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float HitWindowFraction = 0.4f;

	// ── Stats ─────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float AttackDamage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float AttackRange = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxWalkSpeed = 550.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CoyoteTime = 0.12f;

	// ── Dash ──────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashSpeed = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashCooldown = 0.25f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDashing = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bJustLanded = false;

	// ── Movement state ────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "State")
	float MoveSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsJumping = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsFalling = false;

	// ── Combat state ──────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 ComboStep = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsFacingRight = true;

	// ── Camera effects ────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TSubclassOf<UCameraShakeBase> AttackHitShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TSubclassOf<UCameraShakeBase> PlayerHitShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TSubclassOf<UCameraShakeBase> DeathShake;

	// ── Sounds ───────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DashOutSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DashInSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* JumpSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* LandSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* AttackSwingSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* AttackHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* PlayerHurtSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DeathSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* FootstepSound;

	// Time between footstep sounds (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float FootstepInterval = 0.32f;

	// ── Combat functions ──────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnAttackHitWindowOpen();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnAttackHitWindowClose();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartDash();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnAttackFinished();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void JumpToAttackState(int32 Step);

private:
	void Move(const FInputActionValue& Value);
	void PlayAttackStep(int32 Step);
	void ResetCombo();
	void Die();
	void UpdateFacingDirection(float MoveAxisValue);
	void OnDashEnd();
	void ResetDashCooldown();
	void ClearJustLanded();
	void HideSpriteForDash();
	void ShowSpriteAfterDash();

	void OnDeathSpriteHide();
	void OnDeathFadeComplete();
	void OnDeathInputReady();
	void RestartLevel();

	bool  bAttackWindowOpen     = false;
	bool  bComboQueued          = false;
	bool  bWasGrounded          = false;
	bool  bDashOnCooldown       = false;
	bool  bWaitingForDeathInput = false;
	float CoyoteTimeRemaining   = 0.f;
	float FootstepAccumulator   = 0.f;

	FTimerHandle AttackTimerHandle;
	FTimerHandle HitTimerHandle;
	FTimerHandle DashTimerHandle;
	FTimerHandle DashCooldownHandle;
	FTimerHandle LandingTimerHandle;
	FTimerHandle DashHideHandle;
	FTimerHandle DashShowHandle;
	FTimerHandle DeathInputEnableHandle;
	FTimerHandle DeathSpriteFadeHandle;
	FTimerHandle DeathFadeHandle;

	UHeroAnimInstance* GetHeroAnim() const;
};
