#include "PlayerCharacter.h"
#include "HeroAnimInstance.h"
#include "EnemyCharacter.h"
#include "SpiritDepositPoint.h"
#include "SpiritHUDWidget.h"
#include "PlayerHealthBarWidget.h"
#include "Blueprint/UserWidget.h"

#include "CineCameraComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraShakeBase.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollection.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AnimSequences/PaperZDAnimSequence.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength       = 1700.f;
	CameraBoom->bDoCollisionTest      = false;
	CameraBoom->bInheritPitch         = false;
	CameraBoom->bInheritRoll          = false;
	CameraBoom->bInheritYaw           = false;
	CameraBoom->SetRelativeRotation(FRotator(-7.f, -90.f, 0.f));
	CameraBoom->TargetOffset          = FVector(0.f, 0.f, 50.f);

	SideCamera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("SideCamera"));
	SideCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	SideCamera->bUsePawnControlRotation = false;
	SideCamera->CurrentAperture        = 1.4f;
	SideCamera->CurrentFocalLength     = 50.f;
	SideCamera->FocusSettings.FocusMethod         = ECameraFocusMethod::Manual;
	SideCamera->FocusSettings.ManualFocusDistance = 1500.f;

	GetCharacterMovement()->SetPlaneConstraintEnabled(true);
	GetCharacterMovement()->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);
	GetCharacterMovement()->GravityScale               = 1.4f;
	GetCharacterMovement()->MaxWalkSpeed               = MaxWalkSpeed;
	GetCharacterMovement()->JumpZVelocity              = 1100.f;
	GetCharacterMovement()->AirControl                 = 0.85f;
	GetCharacterMovement()->FallingLateralFriction     = 0.2f;
	GetCharacterMovement()->GroundFriction             = 5.f;
	GetCharacterMovement()->MaxAcceleration            = 2000.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 800.f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	AmbientAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudio"));
	AmbientAudioComponent->SetupAttachment(RootComponent);
	AmbientAudioComponent->bAutoActivate = false;
	AmbientAudioComponent->SetUISound(true);

	MusicAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicAudio"));
	MusicAudioComponent->SetupAttachment(RootComponent);
	MusicAudioComponent->bAutoActivate = false;
	MusicAudioComponent->SetUISound(true);

	CombatMusicAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("CombatMusicAudio"));
	CombatMusicAudioComponent->SetupAttachment(RootComponent);
	CombatMusicAudioComponent->bAutoActivate = false;
	CombatMusicAudioComponent->SetUISound(true);

	WeatherAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("WeatherAudio"));
	WeatherAudioComponent->SetupAttachment(RootComponent);
	WeatherAudioComponent->bAutoActivate = false;
	WeatherAudioComponent->SetUISound(true);
}

// ─────────────────────────────────────────────────────────────────────────────
// Begin Play
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
		CM->StartCameraFade(1.f, 0.f, 1.25f, FLinearColor::Black, false, false);

	if (AmbientSound)
	{
		AmbientAudioComponent->SetSound(AmbientSound);
		AmbientAudioComponent->SetVolumeMultiplier(AmbientVolume);
		AmbientAudioComponent->Play();
	}

	if (MusicSound)
	{
		MusicAudioComponent->SetSound(MusicSound);
		MusicAudioComponent->SetVolumeMultiplier(MusicVolume);
		MusicAudioComponent->Play();
	}

	if (CombatMusicSound)
		CombatMusicAudioComponent->SetSound(CombatMusicSound); // played on demand by UpdateCombatMusic

	if (WeatherSound)
	{
		WeatherAudioComponent->SetSound(WeatherSound);
		WeatherAudioComponent->SetVolumeMultiplier(WeatherVolume);
		WeatherAudioComponent->Play();
	}

	// Poll every half-second for enemy proximity to drive combat music crossfade
	GetWorldTimerManager().SetTimer(CombatMusicCheckHandle, this,
		&APlayerCharacter::UpdateCombatMusic, 0.5f, true);

	if (UPrimitiveComponent* RenderComp = GetAnimationComponent()->GetRenderComponent())
	{
		RenderComp->SetRenderCustomDepth(true);
		RenderComp->CustomDepthStencilValue = 1;
	}

	if (!DeathCamera)
	{
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("DeathCamera"), Tagged);
		if (Tagged.Num() > 0)
			DeathCamera = Cast<ACameraActor>(Tagged[0]);
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Sub->AddMappingContext(DefaultMappingContext, 0);
		}

		if (SpiritHUDClass)
		{
			USpiritHUDWidget* HUD = CreateWidget<USpiritHUDWidget>(PC, SpiritHUDClass);
			if (HUD) HUD->AddToViewport();
		}

		if (HealthBarClass)
		{
			HealthBarWidget = CreateWidget<UPlayerHealthBarWidget>(PC, HealthBarClass);
			if (HealthBarWidget)
			{
				HealthBarWidget->AddToViewport();
				HealthBarWidget->RefreshOrbs(CurrentHealth, MaxHealth);
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UCharacterMovementComponent* CM = GetCharacterMovement();
	MoveSpeed  = FMath::Abs(CM->Velocity.X);
	bIsMoving  = MoveSpeed > 10.f;

	// ── Jump phases ───────────────────────────────────────────────────────────
	const bool bAirborne = CM->IsFalling();
	bIsJumping    = bAirborne && CM->Velocity.Z > 0.f;     // legacy compat
	bIsFalling    = bAirborne && CM->Velocity.Z <= 0.f;    // legacy compat
	bIsJumpRising  = bAirborne && CM->Velocity.Z > 250.f;   // JUMP / JUMP-START
	bIsJumpApex    = bAirborne && CM->Velocity.Z <= 250.f && CM->Velocity.Z > -200.f; // JUMP-TRANSITION
	bIsJumpFalling = bAirborne && CM->Velocity.Z <= -200.f; // JUMP-FALL

	// ── Grounded / coyote ─────────────────────────────────────────────────────
	const bool bIsGrounded = !bAirborne;
	if (bIsGrounded)
	{
		if (!bWasGrounded)
		{
			bJustLanded = true;
			GetWorldTimerManager().SetTimer(LandingTimerHandle, this,
				&APlayerCharacter::ClearJustLanded, 0.18f, false);
		}
		bWasGrounded        = true;
		CoyoteTimeRemaining = 0.f;
	}
	else if (bWasGrounded)
	{
		if (CM->Velocity.Z <= 0.f)
			CoyoteTimeRemaining = CoyoteTime;
		bWasGrounded = false;
	}
	else if (CoyoteTimeRemaining > 0.f)
	{
		CoyoteTimeRemaining = FMath::Max(0.f, CoyoteTimeRemaining - DeltaTime);
	}

	// Facing is driven by input only (Move()) — never by velocity — so knockback never flips the sprite.

	// ── Wall slide ────────────────────────────────────────────────────────────
	if (bAirborne && CM->Velocity.Z < -30.f && !bIsWallJumping)
	{
		const FVector Loc   = GetActorLocation();
		const float   Range = GetCapsuleComponent()->GetScaledCapsuleRadius() + WallTraceDistance;
		FCollisionQueryParams SlideParams;
		SlideParams.AddIgnoredActor(this);
		FHitResult SlideHit;

		const bool bWallOnRight = GetWorld()->LineTraceSingleByChannel(SlideHit, Loc, Loc + FVector( Range, 0, 0), ECC_WorldStatic, SlideParams);
		const bool bWallOnLeft  = !bWallOnRight && GetWorld()->LineTraceSingleByChannel(SlideHit, Loc, Loc + FVector(-Range, 0, 0), ECC_WorldStatic, SlideParams);

		bIsWallSliding = bWallOnRight || bWallOnLeft;

		if (bIsWallSliding)
		{
			// Face away from the wall so the slide sprite renders correctly
			const bool bWantRight = bWallOnLeft; // wall on left → face right (and vice versa)
			if (bWantRight != bIsFacingRight)
			{
				bIsFacingRight = bWantRight;
				SetActorRotation(FRotator(0.f, bIsFacingRight ? 0.f : 180.f, 0.f));
			}
		}
	}
	else
	{
		bIsWallSliding = false;
	}


	// ── Footsteps ─────────────────────────────────────────────────────────────
	if (FootstepSound && bIsGrounded && bIsMoving && !bIsAttacking && !bIsDashing && !bIsDead && !bIsDefending && !bIsHealing)
	{
		FootstepAccumulator += DeltaTime;
		const float ScaledInterval = FootstepInterval * (550.f / FMath::Max(MoveSpeed, 1.f));
		if (FootstepAccumulator >= ScaledInterval)
		{
			FootstepAccumulator = 0.f;
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), FootstepSound,
				GetActorLocation(), FRotator::ZeroRotator, FootstepVolume);
		}
	}
	else
	{
		FootstepAccumulator = 0.f;
	}

	// ── Walk trail ────────────────────────────────────────────────────────────
	if (WalkTrailFX && bIsGrounded && bIsMoving && !bIsDead)
	{
		TrailAccumulator += DeltaTime;
		if (TrailAccumulator >= TrailInterval)
		{
			TrailAccumulator = 0.f;
			const FVector FacingDir = bIsFacingRight ? FVector(1, 0, 0) : FVector(-1, 0, 0);
			const FVector Feet      = GetActorLocation() - FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			const FVector TrailPos  = Feet - FacingDir * 20.f;
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WalkTrailFX, TrailPos, FRotator::ZeroRotator, FVector(1.2f));
		}
	}
	else
	{
		TrailAccumulator = 0.f;
	}

	// ── I-frame flicker ───────────────────────────────────────────────────────
	if (UPrimitiveComponent* RC = GetAnimationComponent()->GetRenderComponent())
	{
		if (bIsInvincible)
		{
			IFrameFlickerAccum += DeltaTime;
			RC->SetVisibility(FMath::Fmod(IFrameFlickerAccum, 0.1f) < 0.05f);
		}
		else if (!bIsDashing && !bDashSpriteHidden)
		{
			IFrameFlickerAccum = 0.f;
			RC->SetVisibility(true);
		}
	}

	UpdateCameraOcclusion(DeltaTime);

	if (UHeroAnimInstance* Anim = GetHeroAnim())
		Anim->SyncFromCharacter(this);

	if (PlayerMPC)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(
			GetWorld(), PlayerMPC, FName("PlayerWorldPos"),
			FLinearColor(GetActorLocation()));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Input
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(IA_Move,    ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		EIC->BindAction(IA_Move,    ETriggerEvent::Completed, this, &APlayerCharacter::Move);
		EIC->BindAction(IA_Jump,    ETriggerEvent::Started,   this, &ACharacter::Jump);
		EIC->BindAction(IA_Jump,    ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EIC->BindAction(IA_Attack,  ETriggerEvent::Started,   this, &APlayerCharacter::StartAttack);
		EIC->BindAction(IA_Dash,    ETriggerEvent::Started,   this, &APlayerCharacter::StartDash);

		if (IA_Defend)
		{
			EIC->BindAction(IA_Defend, ETriggerEvent::Started,   this, &APlayerCharacter::StartDefend);
			EIC->BindAction(IA_Defend, ETriggerEvent::Completed, this, &APlayerCharacter::StopDefend);
		}
		if (IA_Heal)
			EIC->BindAction(IA_Heal,      ETriggerEvent::Started, this, &APlayerCharacter::StartHeal);
		if (IA_Interact)
			EIC->BindAction(IA_Interact,  ETriggerEvent::Started, this, &APlayerCharacter::Interact);
	}

	PlayerInputComponent->BindKey(EKeys::AnyKey, IE_Pressed, this, &APlayerCharacter::RestartLevel);
}

// ─────────────────────────────────────────────────────────────────────────────
// Movement
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (bIsDead || bIsAttacking || bIsAirAttacking || bIsHealing || bIsDefendBroken || bIsDefending) return;
	const float Axis = Value.Get<float>();
	AddMovementInput(FVector(1.f, 0.f, 0.f), Axis);
	UpdateFacingDirection(Axis);
}

void APlayerCharacter::UpdateFacingDirection(float MoveAxisValue)
{
	if (FMath::IsNearlyZero(MoveAxisValue)) return;
	if (bIsWallSliding) return; // wall slide sets facing directly
	const bool bWantsRight = MoveAxisValue > 0.f;
	if (bWantsRight == bIsFacingRight) return;
	bIsFacingRight = bWantsRight;
	SetActorRotation(FRotator(0.f, bIsFacingRight ? 0.f : 180.f, 0.f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Jump — variable height + coyote + wall jump
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::Jump()
{
	if (bIsDead || bIsHealing || bIsDefendBroken) return;
	if (bIsDefending) { StopDefend(); }

	if (GetCharacterMovement()->IsFalling() && CoyoteTimeRemaining > 0.f)
	{
		CoyoteTimeRemaining = 0.f;
		bWasGrounded        = false;
		LaunchCharacter(FVector(0.f, 0.f, GetCharacterMovement()->JumpZVelocity), false, true);
		if (JumpSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, GetActorLocation(), JumpVolume);
		OnJumped();
		return;
	}

	if (GetCharacterMovement()->IsFalling() && bCanWallJump && IsNearWall())
	{
		bCanWallJump   = false;
		bIsWallJumping = true;
		bIsWallSliding = false;
		GetWorldTimerManager().SetTimer(WallJumpAnimHandle, this, &APlayerCharacter::EndWallJump, 0.25f, false);

		// Detect which side the wall is on and kick away from it
		const FVector Loc   = GetActorLocation();
		const float   Range = GetCapsuleComponent()->GetScaledCapsuleRadius() + WallTraceDistance;
		FCollisionQueryParams WallParams;
		WallParams.AddIgnoredActor(this);
		FHitResult WallHit;

		float WallNormalX = 0.f;
		if (GetWorld()->LineTraceSingleByChannel(WallHit, Loc, Loc + FVector(Range, 0, 0), ECC_WorldStatic, WallParams))
			WallNormalX = -1.f; // wall on right → push left
		else if (GetWorld()->LineTraceSingleByChannel(WallHit, Loc, Loc + FVector(-Range, 0, 0), ECC_WorldStatic, WallParams))
			WallNormalX =  1.f; // wall on left  → push right

		LaunchCharacter(FVector(WallNormalX * WallJumpXVelocity, 0.f, WallJumpZVelocity), true, true);

		if (JumpSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, GetActorLocation(), JumpVolume);
		if (WallJumpFX)
		{
			const FVector Feet = GetActorLocation() - FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WallJumpFX, Feet, FRotator::ZeroRotator, FVector(2.5f));
		}
		return;
	}

	// Only play when actually grounded — prevents sound spam from mashing jump in the air
	if (!GetCharacterMovement()->IsFalling())
	{
		if (JumpSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, GetActorLocation(), JumpVolume);
	}
	Super::Jump();
}

bool APlayerCharacter::IsNearWall() const
{
	const FVector Loc     = GetActorLocation();
	const float   Range   = GetCapsuleComponent()->GetScaledCapsuleRadius() + WallTraceDistance;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, Loc, Loc + FVector( Range, 0, 0), ECC_WorldStatic, Params)
	    || GetWorld()->LineTraceSingleByChannel(Hit, Loc, Loc + FVector(-Range, 0, 0), ECC_WorldStatic, Params);
}

void APlayerCharacter::StopJumping()
{
	Super::StopJumping();
	const float CutoffVelocity = 420.f;
	if (GetCharacterMovement()->IsFalling() && GetVelocity().Z > CutoffVelocity)
		GetCharacterMovement()->Velocity.Z = CutoffVelocity;
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	bJustLanded  = true;
	bWasGrounded = true;
	bCanWallJump = true;
	CoyoteTimeRemaining = 0.f;

	// Cancel air attack on landing
	if (bIsAirAttacking)
	{
		GetWorldTimerManager().ClearTimer(AirAttackHandle);
		GetWorldTimerManager().ClearTimer(AirAttackHitHandle);
		bIsAirAttacking = false;
	}

	if (LandSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), LandSound, GetActorLocation(), LandVolume);
	GetWorldTimerManager().SetTimer(LandingTimerHandle, this, &APlayerCharacter::ClearJustLanded, 0.15f, false);
}

void APlayerCharacter::ClearJustLanded()
{
	bJustLanded = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Dash — pre-vanish frames → invisible → reappear frames
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::StartDash()
{
	if (bIsDead || bIsDashing || bDashOnCooldown || bIsDefending || bIsHealing) return;

	bIsDashing      = true;
	bDashOnCooldown = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	const FVector Dir = bIsFacingRight ? FVector(1, 0, 0) : FVector(-1, 0, 0);
	LaunchCharacter(Dir * DashSpeed, true, false);

	GetCharacterMovement()->AirControl    = 0.f;
	GetCharacterMovement()->GroundFriction = 0.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 0.f;

	GetWorldTimerManager().ClearTimer(DashHideHandle);
	GetWorldTimerManager().ClearTimer(DashShowHandle);

	// Show pre-vanish frames, then hide (teleport effect), then show landing frames
	GetWorldTimerManager().SetTimer(DashHideHandle, this, &APlayerCharacter::HideSpriteForDash, DashVanishDelay, false);
	GetWorldTimerManager().SetTimer(DashTimerHandle,    this, &APlayerCharacter::OnDashEnd,         DashDuration, false);
	GetWorldTimerManager().SetTimer(DashCooldownHandle, this, &APlayerCharacter::ResetDashCooldown, DashCooldown, false);
}

void APlayerCharacter::OnDashEnd()
{
	bIsDashing = false;
	GetCharacterMovement()->AirControl                 = 0.85f;
	GetCharacterMovement()->GroundFriction             = 5.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 800.f;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void APlayerCharacter::ResetDashCooldown()
{
	bDashOnCooldown = false;
}

void APlayerCharacter::HideSpriteForDash()
{
	bDashSpriteHidden = true;
	if (UPrimitiveComponent* RC = GetAnimationComponent()->GetRenderComponent())
		RC->SetVisibility(false);

	if (DashFX)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DashFX, GetActorLocation(), FRotator::ZeroRotator, FVector(1.5f));

	if (DashOutSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashOutSound, GetActorLocation(), DashOutVolume);

	GetWorldTimerManager().SetTimer(DashShowHandle, this, &APlayerCharacter::ShowSpriteAfterDash, DashInvisibleDuration, false);
}

void APlayerCharacter::ShowSpriteAfterDash()
{
	bDashSpriteHidden = false;
	if (UPrimitiveComponent* RC = GetAnimationComponent()->GetRenderComponent())
		RC->SetVisibility(true);

	if (DashInSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashInSound, GetActorLocation(), DashInVolume);
}

// ─────────────────────────────────────────────────────────────────────────────
// Combat — 4-step combo
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::StartAttack()
{
	if (bIsDead || bIsDefending || bIsDefendBroken || bIsHealing) return;

	// Air attack takes priority when airborne
	if (GetCharacterMovement()->IsFalling() && !bIsAirAttacking && !bIsAttacking)
	{
		StartAirAttack();
		return;
	}

	if (!bIsAttacking && !bIsAirAttacking)
	{
		ComboStep    = 1;
		bIsAttacking = true;
		bComboQueued = false;
		PlayAttackStep(1);
	}
	else if (bIsAttacking && ComboStep < 4)
	{
		bComboQueued = true;
	}
}

void APlayerCharacter::PlayAttackStep(int32 Step)
{
	UPaperZDAnimSequence* Seq = nullptr;
	if      (Step == 1) Seq = AttackSequence1;
	else if (Step == 2) Seq = AttackSequence2;
	else if (Step == 3) Seq = AttackSequence3;
	else if (Step == 4) Seq = AttackSequence4;
	if (!Seq) { ResetCombo(); return; }

	JumpToAttackState(Step);

	if (AttackSwingSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), AttackSwingSound, GetActorLocation(), AttackSwingVolume);

	const float Duration = (Step == 1) ? Attack1Duration
	                     : (Step == 2) ? Attack2Duration
	                     : (Step == 3) ? Attack3Duration
	                                   : Attack4Duration;

	GetWorldTimerManager().SetTimer(AttackTimerHandle, this,
		&APlayerCharacter::OnAttackFinished, Duration, false);
	GetWorldTimerManager().SetTimer(HitTimerHandle, this,
		&APlayerCharacter::OnAttackHitWindowOpen, Duration * HitWindowFraction, false);
}

void APlayerCharacter::OnAttackFinished()
{
	OnAttackHitWindowClose();

	if (bComboQueued && ComboStep < 4)
	{
		bComboQueued = false;
		ComboStep++;
		PlayAttackStep(ComboStep);
	}
	else
	{
		ResetCombo();
	}
}

void APlayerCharacter::ResetCombo()
{
	ComboStep         = 0;
	bIsAttacking      = false;
	bComboQueued      = false;
	bAttackWindowOpen = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Air attack
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::StartAirAttack()
{
	if (!AirAttackSequence) return;
	bIsAirAttacking = true;

	JumpToAttackState(0); // 0 = air attack — wire this in ABP to play AirAttackSequence

	if (AttackSwingSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), AttackSwingSound, GetActorLocation(), AttackSwingVolume);

	GetWorldTimerManager().SetTimer(AirAttackHitHandle, this,
		&APlayerCharacter::OnAirAttackHit, AirAttackDuration * HitWindowFraction, false);
	GetWorldTimerManager().SetTimer(AirAttackHandle, this,
		&APlayerCharacter::EndAirAttack, AirAttackDuration, false);
}

void APlayerCharacter::OnAirAttackHit()
{
	const FVector FacingDir = bIsFacingRight ? FVector(1, 0, 0) : FVector(-1, 0, 0);
	const float   CapsuleR  = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float   CapsuleH  = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Origin    = GetActorLocation() + FacingDir * CapsuleR;
	const FVector End       = Origin + FacingDir * AttackRange;
	const FVector HalfBox   = FVector(AttackRange * 0.5f, 200.f, CapsuleH * 0.75f);

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	GetWorld()->SweepMultiByChannel(Hits, Origin, End, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeBox(HalfBox), Params);

	TSet<AEnemyCharacter*> HitThisSwing;
	bool bLandedHit = false;
	for (const FHitResult& Hit : Hits)
	{
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Hit.GetActor());
		if (!Enemy || HitThisSwing.Contains(Enemy)) continue;
		HitThisSwing.Add(Enemy);
		UGameplayStatics::ApplyDamage(Enemy, AttackDamage * AirAttackDamageMultiplier,
			GetController(), this, UDamageType::StaticClass());
		bLandedHit = true;
	}

	if (bLandedHit)
	{
		StartHitStop();
		if (AttackHitSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), AttackHitSound, GetActorLocation(), AttackHitVolume);
		if (AttackHitShake)
		{
			if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
				CM->StartCameraShake(AttackHitShake, 1.8f);
		}
	}
}

void APlayerCharacter::EndAirAttack()
{
	bIsAirAttacking = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Attack hit detection — ground combo
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::OnAttackHitWindowOpen()
{
	bAttackWindowOpen = true;

	const FVector FacingDir = bIsFacingRight ? FVector(1, 0, 0) : FVector(-1, 0, 0);
	const float   CapsuleR  = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float   CapsuleH  = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Origin    = GetActorLocation() + FacingDir * CapsuleR;
	const FVector End       = Origin + FacingDir * AttackRange;
	const FVector HalfBox   = FVector(AttackRange * 0.5f, 200.f, CapsuleH * 0.75f);

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	GetWorld()->SweepMultiByChannel(Hits, Origin, End, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeBox(HalfBox), Params);

	TSet<AEnemyCharacter*> HitThisSwing;
	bool bLandedHit = false;
	for (const FHitResult& Hit : Hits)
	{
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Hit.GetActor());
		if (!Enemy || HitThisSwing.Contains(Enemy)) continue;
		HitThisSwing.Add(Enemy);

		const float DmgMult = (bIsDashing    ? DashAttackDamageMultiplier : 1.f)
		                    * (ComboStep == 3 ? Attack3DamageMultiplier    : 1.f)
		                    * (ComboStep == 4 ? Attack4DamageMultiplier    : 1.f);
		UGameplayStatics::ApplyDamage(Enemy, AttackDamage * DmgMult,
			GetController(), this, UDamageType::StaticClass());
		bLandedHit = true;
	}

	if (bLandedHit)
	{
		StartHitStop();
		if (AttackHitSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), AttackHitSound, GetActorLocation(), AttackHitVolume);
		if (AttackHitShake)
		{
			if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
				CM->StartCameraShake(AttackHitShake, 1.8f);
		}
	}
}

void APlayerCharacter::OnAttackHitWindowClose()
{
	bAttackWindowOpen = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Defend — hold to block, random break threshold, passive pushback
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::StartDefend()
{
	if (bIsDead || bIsAttacking || bIsAirAttacking || bIsHealing || bIsDefendBroken) return;

	bIsDefending        = true;
	DefendHitsRemaining = FMath::RandRange(DefendBreakMinHits, DefendBreakMaxHits);
}

void APlayerCharacter::StopDefend()
{
	if (!bIsDefending) return;
	bIsDefending = false;
}

void APlayerCharacter::BreakDefend()
{
	bIsDefending    = false;
	bIsDefendBroken = true;

	if (DefendBreakSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DefendBreakSound, GetActorLocation(), DefendBreakVolume);

	if (PlayerHitShake)
	{
		if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
			CM->StartCameraShake(PlayerHitShake, 1.2f);
	}

	GetWorldTimerManager().SetTimer(DefendBreakHandle, this,
		&APlayerCharacter::EndDefendBreak, DefendBreakStaggerTime, false);
}

void APlayerCharacter::EndDefendBreak()
{
	bIsDefendBroken = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Heal — Q press, 15% HP restore, plays healing anim, locks movement
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::StartHeal()
{
	if (bIsDead || bIsAttacking || bIsAirAttacking || bIsDefending ||
		bIsHealing || bHealOnCooldown) return;

	bIsHealing      = true;
	bHealOnCooldown = true;

	GetCharacterMovement()->StopMovementImmediately();

	const float HealAmount = MaxHealth * HealPercent;
	CurrentHealth = FMath::Min(CurrentHealth + HealAmount, MaxHealth);
	RefreshHealthBar();

	if (HealSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HealSound, GetActorLocation(), HealVolume);

	GetWorldTimerManager().SetTimer(HealHandle,        this, &APlayerCharacter::EndHeal,          HealDuration, false);
	GetWorldTimerManager().SetTimer(HealCooldownHandle,this, &APlayerCharacter::ResetHealCooldown, HealCooldown, false);
}

void APlayerCharacter::EndHeal()
{
	bIsHealing = false;
}

void APlayerCharacter::ResetHealCooldown()
{
	bHealOnCooldown = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Interact — find nearest deposit point in range and deposit
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::Interact()
{
	if (bIsDead) return;

	TArray<AActor*> Points;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpiritDepositPoint::StaticClass(), Points);

	ASpiritDepositPoint* Nearest  = nullptr;
	float                NearestD = TNumericLimits<float>::Max();

	for (AActor* Actor : Points)
	{
		ASpiritDepositPoint* Deposit = Cast<ASpiritDepositPoint>(Actor);
		if (!Deposit || !Deposit->bPlayerInRange) continue;
		const float Dist = static_cast<float>(FVector::Dist(GetActorLocation(), Deposit->GetActorLocation()));
		if (Dist < NearestD) { NearestD = Dist; Nearest = Deposit; }
	}

	if (Nearest)
		Nearest->TryDeposit(this);
}

// ─────────────────────────────────────────────────────────────────────────────
// Hit stop
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::StartHitStop()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), HitStopTimeDilation);
	GetWorldTimerManager().SetTimer(HitStopHandle, this, &APlayerCharacter::EndHitStop,
		HitStopDuration * HitStopTimeDilation, false);
}

void APlayerCharacter::EndHitStop()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// State clearers
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::EndIFrames()   { bIsInvincible  = false; }
void APlayerCharacter::EndHurt()      { bIsHurt        = false; }
void APlayerCharacter::EndWallJump()  { bIsWallJumping = false; }

// ─────────────────────────────────────────────────────────────────────────────
// Health / Death
// ─────────────────────────────────────────────────────────────────────────────

float APlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.f;
	if (DamageCauser == this) return 0.f;
	if (bIsInvincible) return 0.f;

	// Block absorbs damage; decrement the hit counter and potentially break the block
	if (bIsDefending && !bIsDefendBroken)
	{
		if (DefendSound)
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), DefendSound, GetActorLocation(), DefendVolume);
		if (DefendHitSound)
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), DefendHitSound, GetActorLocation(), DefendHitVolume);

		DefendHitsRemaining--;
		if (DefendHitsRemaining <= 0)
			BreakDefend();

		// Push back from the hit
		const FVector PushDir = bIsFacingRight ? FVector(-1.f, 0.f, 0.f) : FVector(1.f, 0.f, 0.f);
		LaunchCharacter(PushDir * DefendPushbackSpeed, false, false);

		const float Absorbed = DamageAmount * DefendDamageReduction;
		return Absorbed;
	}

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - Applied, 0.f, MaxHealth);
	RefreshHealthBar();

	bIsInvincible = true;
	GetWorldTimerManager().SetTimer(IFrameHandle, this, &APlayerCharacter::EndIFrames, IFrameDuration, false);

	bIsHurt = true;
	GetWorldTimerManager().SetTimer(HurtTimerHandle, this, &APlayerCharacter::EndHurt, 0.35f, false);

	if (DamageCauser)
	{
		const FVector KnockDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
		LaunchCharacter(FVector(KnockDir.X * KnockbackForce, 0.f, KnockbackForce * 0.15f), true, true);
	}

	if (PlayerHurtSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), PlayerHurtSound, GetActorLocation(), PlayerHurtVolume);

	if (DamageFX)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DamageFX, GetActorLocation(), FRotator::ZeroRotator, FVector(2.5f));

	if (PlayerHitShake)
	{
		if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
			CM->StartCameraShake(PlayerHitShake, 0.7f);
	}

	if (CurrentHealth <= 0.f)
		Die();

	return Applied;
}

void APlayerCharacter::Die()
{
	if (bIsDead) return;

	bIsDead         = true;
	bIsDefending    = false;
	bIsDefendBroken = false;
	bIsHealing      = false;
	bIsAirAttacking = false;
	ResetCombo();

	GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeed;

	GetWorldTimerManager().ClearTimer(CombatMusicCheckHandle);
	AmbientAudioComponent->FadeOut(1.5f, 0.f);
	MusicAudioComponent->FadeOut(1.5f, 0.f);
	CombatMusicAudioComponent->FadeOut(1.5f, 0.f);
	WeatherAudioComponent->FadeOut(1.5f, 0.f);

	if (DeathSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, GetActorLocation(), DeathVolume);

	if (DeathMistFX)
	{
		const FVector Feet = GetActorLocation() - FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathMistFX, Feet, FRotator::ZeroRotator, FVector(3.f));
	}

	if (DeathShake)
	{
		if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
			CM->StartCameraShake(DeathShake, 1.f);
	}

	bDashSpriteHidden = false;
	GetWorldTimerManager().ClearTimer(DashHideHandle);
	GetWorldTimerManager().ClearTimer(DashShowHandle);
	GetWorldTimerManager().ClearTimer(AirAttackHandle);
	GetWorldTimerManager().ClearTimer(AirAttackHitHandle);
	GetWorldTimerManager().ClearTimer(DefendBreakHandle);
	GetWorldTimerManager().ClearTimer(HealHandle);

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		DisableInput(PC);

	if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
		CM->StartCameraFade(0.f, 1.f, DeathFadeToBlackTime, FLinearColor::Black, false, true);

	GetWorldTimerManager().SetTimer(DeathSpriteFadeHandle, this,
		&APlayerCharacter::OnDeathSpriteHide, DeathFadeToBlackTime * 0.5f, false);

	GetWorldTimerManager().SetTimer(DeathFadeHandle, this,
		&APlayerCharacter::OnDeathFadeComplete, DeathFadeToBlackTime, false);

	GetWorldTimerManager().SetTimer(DeathInputEnableHandle, this,
		&APlayerCharacter::OnDeathInputReady, DeathFadeToBlackTime + DeathFadeFromBlackTime + 0.5f, false);
}

// ─────────────────────────────────────────────────────────────────────────────
// Death sequence
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::OnDeathSpriteHide()
{
	if (UPrimitiveComponent* RC = GetAnimationComponent()->GetRenderComponent())
		RC->SetVisibility(false);
}

void APlayerCharacter::OnDeathFadeComplete()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	if (!DeathCamera)
	{
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("DeathCamera"), Tagged);
		if (Tagged.Num() > 0)
			DeathCamera = Cast<ACameraActor>(Tagged[0]);
	}

	if (DeathCamera)
		PC->SetViewTarget(DeathCamera);

	if (APlayerCameraManager* CM = PC->PlayerCameraManager)
		CM->StartCameraFade(1.f, 0.f, DeathFadeFromBlackTime, FLinearColor::Black, false, false);
}

void APlayerCharacter::OnDeathInputReady()
{
	bWaitingForDeathInput = true;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		EnableInput(PC);
}

void APlayerCharacter::RestartLevel()
{
	if (!bWaitingForDeathInput) return;
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// Camera occlusion fade
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::UpdateCameraOcclusion(float DeltaTime)
{
	const FVector CamLoc    = SideCamera->GetComponentLocation();
	const FVector PlayerLoc = GetActorLocation();

	// Sphere sweep from camera to player — catches wide objects line traces would miss
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bReturnFaceIndex = false;
	GetWorld()->SweepMultiByChannel(Hits, CamLoc, PlayerLoc, FQuat::Identity,
		ECC_Visibility, FCollisionShape::MakeSphere(OcclusionSweepRadius), Params);

	// Collect unique primitive components blocking the view this frame
	TSet<UPrimitiveComponent*> ThisFrameOccluders;
	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* Prim = Hit.GetComponent();
		if (!Prim || Prim->IsA<USkinnedMeshComponent>()) continue; // skip skeletal (player, enemies)
		ThisFrameOccluders.Add(Prim);
	}

	// Fade in new occluders — create DMIs on first encounter
	for (UPrimitiveComponent* Prim : ThisFrameOccluders)
	{
		if (!OccluderDMIs.Contains(Prim))
		{
			TArray<UMaterialInstanceDynamic*> DMIs;
			for (int32 i = 0; i < Prim->GetNumMaterials(); ++i)
			{
				UMaterialInstanceDynamic* DMI = Prim->CreateAndSetMaterialInstanceDynamic(i);
				if (DMI) DMIs.Add(DMI);
			}
			OccluderDMIs.Add(Prim, DMIs);
			OccluderOpacity.Add(Prim, 1.f); // start at full, let lerp do the work
		}

		// Lerp opacity toward the target fade value
		float& CurrentOpacity = OccluderOpacity[Prim];
		CurrentOpacity = FMath::FInterpTo(CurrentOpacity, OcclusionFadeOpacity, DeltaTime, OcclusionFadeSpeed);
		for (UMaterialInstanceDynamic* DMI : OccluderDMIs[Prim])
			if (DMI) DMI->SetScalarParameterValue(OcclusionOpacityParam, CurrentOpacity);
	}

	// Fade out components that are no longer occluding
	TArray<UPrimitiveComponent*> ToRemove;
	for (auto& Pair : OccluderOpacity)
	{
		UPrimitiveComponent* Prim = Pair.Key;
		if (ThisFrameOccluders.Contains(Prim)) continue;

		float& CurrentOpacity = Pair.Value;
		CurrentOpacity = FMath::FInterpTo(CurrentOpacity, 1.f, DeltaTime, OcclusionFadeSpeed);
		for (UMaterialInstanceDynamic* DMI : OccluderDMIs[Prim])
			if (DMI) DMI->SetScalarParameterValue(OcclusionOpacityParam, CurrentOpacity);

		// Once fully restored, stop tracking
		if (FMath::IsNearlyEqual(CurrentOpacity, 1.f, 0.01f))
			ToRemove.Add(Prim);
	}
	for (UPrimitiveComponent* Prim : ToRemove)
	{
		OccluderDMIs.Remove(Prim);
		OccluderOpacity.Remove(Prim);
	}
}

UHeroAnimInstance* APlayerCharacter::GetHeroAnim() const
{
	return Cast<UHeroAnimInstance>(GetAnimationComponent()->GetAnimInstance());
}

// ─────────────────────────────────────────────────────────────────────────────
// Combat music crossfade
// ─────────────────────────────────────────────────────────────────────────────

float APlayerCharacter::GetClosestEnemyDistance() const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyCharacter::StaticClass(), Enemies);

	float Nearest = TNumericLimits<float>::Max();
	for (AActor* Actor : Enemies)
	{
		const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Actor);
		if (!Enemy || Enemy->bIsDead) continue;
		const float Dist = static_cast<float>(FVector::Dist(GetActorLocation(), Enemy->GetActorLocation()));
		if (Dist < Nearest) Nearest = Dist;
	}
	return Nearest;
}

void APlayerCharacter::RefreshHealthBar()
{
	if (HealthBarWidget)
		HealthBarWidget->RefreshOrbs(CurrentHealth, MaxHealth);
}

void APlayerCharacter::UpdateCombatMusic()
{
	if (bIsDead || !CombatMusicSound) return;

	const float NearestDist = GetClosestEnemyDistance();
	const bool  bShouldBeActive = NearestDist <= CombatMusicBlendRadius;

	if (bShouldBeActive && !bCombatMusicActive)
	{
		bCombatMusicActive = true;
		MusicAudioComponent->FadeOut(CombatMusicFadeTime, 0.f);
		CombatMusicAudioComponent->FadeIn(CombatMusicFadeTime, CombatMusicVolume);
	}
	else if (!bShouldBeActive && bCombatMusicActive)
	{
		bCombatMusicActive = false;
		CombatMusicAudioComponent->FadeOut(CombatMusicFadeTime, 0.f);
		if (MusicSound)
			MusicAudioComponent->FadeIn(CombatMusicFadeTime, MusicVolume);
	}
}
