#pragma once

#include "CoreMinimal.h"
#include "PaperZDCharacter.h"
#include "InputActionValue.h"
#include "PaperZDAnimationComponent.h"
#include "PlayerCharacter.generated.h"

class UAudioComponent;
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
class UNiagaraSystem;
class USpiritHUDWidget;
class UPlayerHealthBarWidget;
class ASpiritDepositPoint;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UMaterialParameterCollection* PlayerMPC;

	// Opacity occluding meshes fade to (0=hidden, 1=fully opaque). Materials use Masked blend mode
	// with a scalar "Opacity" parameter wired to OpacityMask — Nanite-compatible.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Occlusion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OcclusionFadeOpacity = 0.0f;

	// Lerp speed for the fade in / fade out
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Occlusion", meta = (ClampMin = "0.1"))
	float OcclusionFadeSpeed = 8.f;

	// Name of the scalar parameter in occluding materials that controls opacity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Occlusion")
	FName OcclusionOpacityParam = FName("Opacity");

	// Sphere radius for the camera-to-player sweep (larger catches wider objects)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Occlusion", meta = (ClampMin = "1.0"))
	float OcclusionSweepRadius = 12.f;

	// How quickly the camera trails behind horizontal movement (higher = snappier)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0"))
	float CameraLagSpeed = 7.0f;

	// Camera pivot shifts upward by this scale * Z velocity, keeping fast upward attacks on-screen
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraVerticalLeadScale = 0.07f;

	// Max extra upward pivot shift (units) — caps how far the camera leads above the player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraVerticalLeadMax = 160.0f;

	// Interpolation speed for the vertical lead adjustment
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0"))
	float CameraVerticalLeadSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	ACameraActor* DeathCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float DeathFadeToBlackTime = 0.7f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Defend;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Heal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Interact;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Esc;

	// ── Attack animations ─────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animations")
	UPaperZDAnimSequence* AttackSequence1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animations")
	UPaperZDAnimSequence* AttackSequence2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animations")
	UPaperZDAnimSequence* AttackSequence3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animations")
	UPaperZDAnimSequence* AttackSequence4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animations")
	UPaperZDAnimSequence* AirAttackSequence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animations")
	UPaperZDAnimSequence* HealSequence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float Attack1Duration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float Attack2Duration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float Attack3Duration = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float Attack4Duration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float AirAttackDuration = 0.5f;

	// Fraction of the attack animation at which the hit box opens (attacks 1-3)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float HitWindowFraction = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float Attack3DamageMultiplier = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float Attack4DamageMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animations")
	float AirAttackDamageMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashAttackDamageMultiplier = 1.5f;

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

	// ── Wall Jump ─────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallJump")
	float WallJumpZVelocity = 1100.f;

	// Horizontal kick away from the wall — forces an outward arc
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallJump")
	float WallJumpXVelocity = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallJump")
	float WallTraceDistance = 65.f;

	// ── Dash ──────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashSpeed = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashCooldown = 3.0f;

	// Frames of the dash anim to show before vanishing (at 15fps, 3 frames ≈ 0.2s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashVanishDelay = 0.2f;

	// How long the character stays invisible mid-dash before reappearing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashInvisibleDuration = 0.25f;

	// ── Defend ────────────────────────────────────────────────────────────────
	// Fraction of incoming damage blocked (1.0 = full block)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Defend")
	float DefendDamageReduction = 1.0f;

	// Block breaks after a random number of hits in this range
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Defend")
	int32 DefendBreakMinHits = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Defend")
	int32 DefendBreakMaxHits = 7;

	// Passive backwards drift while holding block
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Defend")
	float DefendPushbackSpeed = 100.f;

	// How long the broken-guard stagger lasts
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Defend")
	float DefendBreakStaggerTime = 0.6f;

	// ── Heal ──────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Heal")
	float HealPercent = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Heal")
	float HealCooldown = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Heal")
	float HealDuration = 1.2f;

	// ── VFX ───────────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* DashFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* WallJumpFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* DeathMistFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* DamageFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* WalkTrailFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float TrailInterval = 0.07f;

	// ── Movement state ────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "State")
	float MoveSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsMoving = false;

	// Legacy — kept for backwards compat with ABP
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsJumping = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsFalling = false;

	// Granular jump phases for the full flipbook set
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsJumpRising = false;   // JUMP / JUMP-START

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsJumpApex = false;     // JUMP-TRANSITION

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsJumpFalling = false;  // JUMP-FALL

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDashing = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bCanWallJump = true;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsWallSliding = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsWallJumping = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bJustLanded = false;

	// ── Combat state ──────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 ComboStep = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsFacingRight = true;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsAirAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDefending = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDefendBroken = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsHealing = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bHealOnCooldown = false;

	// ── Invincibility / feedback ───────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsInvincible = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsHurt = false;

	// ── Spirits ───────────────────────────────────────────────────────────────
	// Spirits currently carried (lost on death if desired)
	UPROPERTY(BlueprintReadOnly, Category = "Spirits")
	int32 SpiritCount = 0;

	// Spirits banked at a deposit point — persistent total
	UPROPERTY(BlueprintReadOnly, Category = "Spirits")
	int32 DepositedSpiritCount = 0;

	// Assign WBP_SpiritHUD in BP_Hero Details panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spirits")
	TSubclassOf<USpiritHUDWidget> SpiritHUDClass;

	// Assign WBP_PlayerHealthBar in BP_Hero Details panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UPlayerHealthBarWidget> HealthBarClass;

	// Assign WDG_Tips_04 here — shown only on New Game, dismissed by A/D press
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> TipsWidgetClass;

	// Assign the pause menu widget class here
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuClass;

	// Called by the pause menu Resume button
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResumePauseMenu();

	// Called by the pause menu Quit button (also usable from Blueprint)
	UFUNCTION(BlueprintCallable, Category = "UI")
	void GoToMainMenu();

	// ── Camera effects ────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TSubclassOf<UCameraShakeBase> AttackHitShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TSubclassOf<UCameraShakeBase> PlayerHitShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TSubclassOf<UCameraShakeBase> DeathShake;

	// ── Ambient: Environment ──────────────────────────────────────────────────
	// General background noise — wind, cave drips, forest, etc.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sound|Ambient")
	UAudioComponent* AmbientAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Ambient")
	USoundBase* AmbientSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Ambient", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AmbientVolume = 0.4f;

	// ── Ambient: Music ────────────────────────────────────────────────────────
	// Looping background music track
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sound|Music")
	UAudioComponent* MusicAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Music")
	USoundBase* MusicSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Music", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MusicVolume = 0.7f;

	// ── Ambient: Combat Music ─────────────────────────────────────────────────
	// Auto-crossfades in when an enemy is within CombatMusicBlendRadius
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sound|Music")
	UAudioComponent* CombatMusicAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Music")
	USoundBase* CombatMusicSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Music", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float CombatMusicVolume = 0.8f;

	// Distance at which the combat track fades in (unreal units)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Music", meta = (ClampMin = "100.0"))
	float CombatMusicBlendRadius = 1500.f;

	// Seconds to crossfade between music and combat music
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Music", meta = (ClampMin = "0.1"))
	float CombatMusicFadeTime = 2.0f;

	// ── Ambient: Weather ──────────────────────────────────────────────────────
	// Looping weather layer — rain, wind, thunder, etc.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sound|Ambient")
	UAudioComponent* WeatherAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Ambient")
	USoundBase* WeatherSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Ambient", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float WeatherVolume = 0.5f;

	// ── Sounds ───────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DashOutSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DashOutVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DashInSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DashInVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* JumpSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float JumpVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* LandSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float LandVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* AttackSwingSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AttackSwingVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* AttackHitSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AttackHitVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* PlayerHurtSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float PlayerHurtVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DeathSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DeathVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* FootstepSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float FootstepInterval = 0.32f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float FootstepVolume = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DefendSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DefendVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DefendHitSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DefendHitVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DefendBreakSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DefendBreakVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* HealSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float HealVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* SpiritPickupSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SpiritPickupVolume = 1.0f;

	// ── Combat feel ───────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Feel")
	float HitStopDuration = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Feel")
	float HitStopTimeDilation = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Feel")
	float IFrameDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Feel")
	float KnockbackForce = 450.f;

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

	// Implemented in ABP — jumps the anim state machine to the given combo step
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

	// Hit stop
	void StartHitStop();
	void EndHitStop();

	// I-frames
	void EndIFrames();

	// Hurt / wall anim
	void EndHurt();
	void EndWallJump();

	// Air attack
	void StartAirAttack();
	void EndAirAttack();
	void OnAirAttackHit();

	// Defend
	void StartDefend();
	void StopDefend();
	void BreakDefend();
	void EndDefendBreak();

	// Heal
	void StartHeal();
	void EndHeal();
	void ResetHealCooldown();

	// Interact
	void Interact();

	// ESC — toggle pause menu open/closed
	void TogglePauseMenu();

	// Tips overlay — shown on New Game, cleared on first A/D press
	UPROPERTY()
	UUserWidget* TipsWidget = nullptr;
	void DismissTips();

	UPROPERTY()
	UUserWidget* PauseMenuWidget = nullptr;

	// Throw
	bool  IsNearWall() const;
	float GetClosestEnemyDistance() const;

	void UpdateCombatMusic();
	void UpdateCameraOcclusion(float DeltaTime);
	void RefreshHealthBar();

	UPlayerHealthBarWidget* HealthBarWidget = nullptr;
	bool  bCombatMusicActive  = false;

	// Per-component DMIs and their current faded opacity
	TMap<UPrimitiveComponent*, TArray<UMaterialInstanceDynamic*>> OccluderDMIs;
	TMap<UPrimitiveComponent*, float>                             OccluderOpacity;
	bool  bDashSpriteHidden   = false;
	bool  bAttackWindowOpen   = false;
	bool  bComboQueued        = false;
	bool  bWasGrounded        = false;
	bool  bDashOnCooldown     = false;
	bool  bWaitingForDeathInput = false;
	int32 DefendHitsRemaining = 0;
	float CoyoteTimeRemaining = 0.f;
	float FootstepAccumulator = 0.f;
	float TrailAccumulator      = 0.f;
	float IFrameFlickerAccum    = 0.f;

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
	FTimerHandle HitStopHandle;
	FTimerHandle IFrameHandle;
	FTimerHandle HurtTimerHandle;
	FTimerHandle WallJumpAnimHandle;
	FTimerHandle AirAttackHandle;
	FTimerHandle CombatMusicCheckHandle;
	FTimerHandle AirAttackHitHandle;
	FTimerHandle DefendBreakHandle;
	FTimerHandle HealHandle;
	FTimerHandle HealCooldownHandle;

	UHeroAnimInstance* GetHeroAnim() const;
};
