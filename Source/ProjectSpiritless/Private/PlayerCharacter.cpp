#include "PlayerCharacter.h"
#include "HeroAnimInstance.h"

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
#include "DrawDebugHelpers.h"
#include "Sound/SoundBase.h"
#include "AnimSequences/PaperZDAnimSequence.h"

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength       = 1500.f;
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
	GetCharacterMovement()->GroundFriction             = 10.f;
	GetCharacterMovement()->MaxAcceleration            = 2500.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Begin Play
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// Fade in from black on spawn
	if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
		CM->StartCameraFade(1.f, 0.f, 1.25f, FLinearColor::Black, false, false);

	if (UPrimitiveComponent* RenderComp = GetAnimationComponent()->GetRenderComponent())
	{
		RenderComp->SetRenderCustomDepth(true);
		RenderComp->CustomDepthStencilValue = 1;
	}

	// Auto-find the death camera by tag "DeathCamera" if not manually assigned
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
	bIsJumping = CM->IsFalling() && CM->Velocity.Z > 0.f;
	bIsFalling = CM->IsFalling() && CM->Velocity.Z <= 0.f;

	const bool bIsGrounded = !CM->IsFalling();
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

	if (!FMath::IsNearlyZero(CM->Velocity.X))
		UpdateFacingDirection(CM->Velocity.X);

	// ── Footsteps ─────────────────────────────────────────────────────────────
	if (FootstepSound && bIsGrounded && bIsMoving && !bIsAttacking && !bIsDashing && !bIsDead)
	{
		FootstepAccumulator += DeltaTime;
		const float ScaledInterval = FootstepInterval * (550.f / FMath::Max(MoveSpeed, 1.f));
		if (FootstepAccumulator >= ScaledInterval)
		{
			FootstepAccumulator = 0.f;
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), FootstepSound,
				GetActorLocation(), FRotator::ZeroRotator, 0.6f);
		}
	}
	else
	{
		FootstepAccumulator = 0.f;
	}

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
		EIC->BindAction(IA_Move,   ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		EIC->BindAction(IA_Move,   ETriggerEvent::Completed, this, &APlayerCharacter::Move);
		EIC->BindAction(IA_Jump,   ETriggerEvent::Started,   this, &ACharacter::Jump);
		EIC->BindAction(IA_Jump,   ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EIC->BindAction(IA_Attack, ETriggerEvent::Started,   this, &APlayerCharacter::StartAttack);
		EIC->BindAction(IA_Dash,   ETriggerEvent::Started,   this, &APlayerCharacter::StartDash);
	}

	// Any key restarts the level — gated by bWaitingForDeathInput so it only fires on death screen
	PlayerInputComponent->BindKey(EKeys::AnyKey, IE_Pressed, this, &APlayerCharacter::RestartLevel);
}

// ─────────────────────────────────────────────────────────────────────────────
// Movement
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (bIsDead || bIsAttacking) return;
	AddMovementInput(FVector(1.f, 0.f, 0.f), Value.Get<float>());
}

void APlayerCharacter::UpdateFacingDirection(float MoveAxisValue)
{
	if (FMath::IsNearlyZero(MoveAxisValue)) return;
	const bool bWantsRight = MoveAxisValue > 0.f;
	if (bWantsRight == bIsFacingRight) return;
	bIsFacingRight = bWantsRight;
	SetActorRotation(FRotator(0.f, bIsFacingRight ? 0.f : 180.f, 0.f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Jump — variable height + coyote time
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::Jump()
{
	if (bIsDead || bIsAttacking) return;

	if (GetCharacterMovement()->IsFalling() && CoyoteTimeRemaining > 0.f)
	{
		CoyoteTimeRemaining = 0.f;
		bWasGrounded        = false;
		LaunchCharacter(FVector(0.f, 0.f, GetCharacterMovement()->JumpZVelocity), false, true);
		if (JumpSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, GetActorLocation());
		OnJumped();
		return;
	}

	if (JumpSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, GetActorLocation());
	Super::Jump();
}

void APlayerCharacter::StopJumping()
{
	Super::StopJumping();

	const float CutoffVelocity = 420.f;
	if (GetCharacterMovement()->IsFalling() && GetVelocity().Z > CutoffVelocity)
		GetCharacterMovement()->Velocity.Z = CutoffVelocity;
}

// ─────────────────────────────────────────────────────────────────────────────
// Dash
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	bJustLanded  = true;
	bWasGrounded = true;
	CoyoteTimeRemaining = 0.f;
	if (LandSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), LandSound, GetActorLocation());
	GetWorldTimerManager().SetTimer(LandingTimerHandle, this, &APlayerCharacter::ClearJustLanded, 0.15f, false);
}

void APlayerCharacter::ClearJustLanded()
{
	bJustLanded = false;
}

void APlayerCharacter::StartDash()
{
	if (bIsDead || bIsDashing || bDashOnCooldown) return;

	bIsDashing      = true;
	bDashOnCooldown = true;

	const FVector Dir = bIsFacingRight ? FVector(1, 0, 0) : FVector(-1, 0, 0);
	LaunchCharacter(Dir * DashSpeed, true, false);

	if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
		CM->StartCameraFade(0.08f, 0.f, 0.1f, FLinearColor::White, false, false);

	GetCharacterMovement()->AirControl = 0.f;

	// Reset any in-flight hide/show timers (re-dash while invisible)
	GetWorldTimerManager().ClearTimer(DashHideHandle);
	GetWorldTimerManager().ClearTimer(DashShowHandle);

	// Sprite vanishes 0.05s after dash, reappears 0.45s after that
	GetWorldTimerManager().SetTimer(DashHideHandle, this, &APlayerCharacter::HideSpriteForDash, 0.05f, false);

	GetWorldTimerManager().SetTimer(DashTimerHandle,    this, &APlayerCharacter::OnDashEnd,         DashDuration, false);
	GetWorldTimerManager().SetTimer(DashCooldownHandle, this, &APlayerCharacter::ResetDashCooldown, DashCooldown, false);
}

void APlayerCharacter::OnDashEnd()
{
	bIsDashing = false;
	GetCharacterMovement()->AirControl = 0.85f;
}

void APlayerCharacter::ResetDashCooldown()
{
	bDashOnCooldown = false;
}

void APlayerCharacter::HideSpriteForDash()
{
	if (UPrimitiveComponent* RC = GetAnimationComponent()->GetRenderComponent())
		RC->SetVisibility(false);

	if (DashOutSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashOutSound, GetActorLocation());

	GetWorldTimerManager().SetTimer(DashShowHandle, this, &APlayerCharacter::ShowSpriteAfterDash, 0.45f, false);
}

void APlayerCharacter::ShowSpriteAfterDash()
{
	if (UPrimitiveComponent* RC = GetAnimationComponent()->GetRenderComponent())
		RC->SetVisibility(true);

	if (DashInSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashInSound, GetActorLocation());
}

// ─────────────────────────────────────────────────────────────────────────────
// Combat — combo system
// ─────────────────────────────────────────────────────────────────────────────

void APlayerCharacter::StartAttack()
{
	if (bIsDead) return;

	if (!bIsAttacking)
	{
		ComboStep    = 1;
		bIsAttacking = true;
		bComboQueued = false;
		PlayAttackStep(1);
	}
	else if (bAttackWindowOpen && ComboStep == 1)
	{
		bComboQueued = true;
	}
}

void APlayerCharacter::PlayAttackStep(int32 Step)
{
	UPaperZDAnimSequence* Seq = (Step == 1) ? AttackSequence1 : AttackSequence2;
	if (!Seq) { ResetCombo(); return; }

	JumpToAttackState(Step);

	if (AttackSwingSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), AttackSwingSound, GetActorLocation());

	const float Duration = (Step == 1) ? Attack1Duration : Attack2Duration;

	GetWorldTimerManager().SetTimer(AttackTimerHandle, this,
		&APlayerCharacter::OnAttackFinished, Duration, false);
	GetWorldTimerManager().SetTimer(HitTimerHandle, this,
		&APlayerCharacter::OnAttackHitWindowOpen, Duration * HitWindowFraction, false);
}

void APlayerCharacter::OnAttackFinished()
{
	OnAttackHitWindowClose();

	if (ComboStep == 1 && bComboQueued)
	{
		bComboQueued = false;
		ComboStep    = 2;
		PlayAttackStep(2);
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
// Attack hit detection
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

#if WITH_EDITOR
	DrawDebugBox(GetWorld(), (Origin + End) * 0.5f, HalfBox,
		Hits.Num() > 0 ? FColor::Red : FColor::Green, false, 0.5f);
#endif

	bool bLandedHit = false;
	for (const FHitResult& Hit : Hits)
	{
		if (AActor* Actor = Hit.GetActor())
		{
			UGameplayStatics::ApplyDamage(Actor, AttackDamage,
				GetController(), this, UDamageType::StaticClass());
			bLandedHit = true;
		}
	}

	if (bLandedHit)
	{
		if (AttackHitSound)  UGameplayStatics::PlaySoundAtLocation(GetWorld(), AttackHitSound, GetActorLocation());
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
// Health / Death
// ─────────────────────────────────────────────────────────────────────────────

float APlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.f;

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - Applied, 0.f, MaxHealth);

	if (PlayerHurtSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), PlayerHurtSound, GetActorLocation());

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

	bIsDead = true;
	ResetCombo();

	if (DeathSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, GetActorLocation());

	if (DeathShake)
	{
		if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
			CM->StartCameraShake(DeathShake, 1.f);
	}

	// Cancel any dash visibility timers
	GetWorldTimerManager().ClearTimer(DashHideHandle);
	GetWorldTimerManager().ClearTimer(DashShowHandle);

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		DisableInput(PC);

	// 1. Fade to black — hold it when done so the screen stays black for the cam switch
	if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
		CM->StartCameraFade(0.f, 1.f, DeathFadeToBlackTime, FLinearColor::Black, false, true);

	// 2. Halfway through fade: hide the sprite (looks like it fizzles into the black)
	GetWorldTimerManager().SetTimer(DeathSpriteFadeHandle, this,
		&APlayerCharacter::OnDeathSpriteHide, DeathFadeToBlackTime * 0.5f, false);

	// 3. Once fully black: switch cam instantly and fade back in
	GetWorldTimerManager().SetTimer(DeathFadeHandle, this,
		&APlayerCharacter::OnDeathFadeComplete, DeathFadeToBlackTime, false);

	// 4. After fade-in settles: accept any key to restart
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

	// Instant camera cut while screen is black — no visible pop
	if (DeathCamera)
		PC->SetViewTarget(DeathCamera);

	// Fade from black to reveal the death cam
	if (APlayerCameraManager* CM = PC->PlayerCameraManager)
		CM->StartCameraFade(1.f, 0.f, DeathFadeFromBlackTime, FLinearColor::Black, false, false);
}

void APlayerCharacter::OnDeathInputReady()
{
	bWaitingForDeathInput = true;

	// Re-enable input so AnyKey binding can fire
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

UHeroAnimInstance* APlayerCharacter::GetHeroAnim() const
{
	return Cast<UHeroAnimInstance>(GetAnimationComponent()->GetAnimInstance());
}
