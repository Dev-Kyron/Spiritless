#include "EnemyCharacter.h"
#include "EnemyAnimInstance.h"
#include "PlayerCharacter.h"
#include "SpiritPickup.h"
#include "SpiritlessGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

AEnemyCharacter::AEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(22.f, 52.f);

	GetCharacterMovement()->SetPlaneConstraintEnabled(true);
	GetCharacterMovement()->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);
	GetCharacterMovement()->GravityScale              = 1.4f;
	GetCharacterMovement()->MaxWalkSpeed              = 350.f;
	GetCharacterMovement()->GroundFriction            = 8.f;
	GetCharacterMovement()->MaxAcceleration           = 1200.f;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	bUseControllerRotationYaw = false;

	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBarComponent->SetupAttachment(RootComponent);
	HealthBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawSize(FVector2D(220.f, 55.f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Begin Play
// ─────────────────────────────────────────────────────────────────────────────

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Scale stats by difficulty before setting CurrentHealth
	if (USpiritlessGameInstance* GI = Cast<USpiritlessGameInstance>(GetGameInstance()))
	{
		MaxHealth    *= GI->GetEnemyHealthMultiplier();
		AttackDamage *= GI->GetEnemyDamageMultiplier();
	}

	CurrentHealth = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;

	CachedPlayer  = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	PatrolOrigin  = GetActorLocation();

	if (HealthBarWidgetClass)
	{
		HealthBarComponent->SetWidgetClass(HealthBarWidgetClass);
		HealthBarComponent->InitWidget();
		HealthBarWidget = Cast<UEnemyHealthBarWidget>(HealthBarComponent->GetWidget());
		if (HealthBarWidget)
		{
			HealthBarWidget->SetEnemyName(EnemyName);
			HealthBarWidget->SetHealthPercent(1.f);
		}
	}

	// Looping footstep timer — plays only when actually chasing
	GetWorldTimerManager().SetTimer(FootstepTimerHandle, this,
		&AEnemyCharacter::PlayFootstep, FootstepInterval, true);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick
// ─────────────────────────────────────────────────────────────────────────────

void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Always sync anim — covers idle, death, and all states
	if (UEnemyAnimInstance* Anim = Cast<UEnemyAnimInstance>(GetAnimationComponent()->GetAnimInstance()))
		Anim->SyncFromEnemy(this);

	// ── Walk trail ────────────────────────────────────────────────────────────
	const bool bIsMovingOnGround = !GetCharacterMovement()->IsFalling()
	                             && GetCharacterMovement()->Velocity.SizeSquared2D() > 100.f;
	if (WalkTrailFX && bIsMovingOnGround && !bIsDead)
	{
		TrailAccumulator += DeltaTime;
		if (TrailAccumulator >= TrailInterval)
		{
			TrailAccumulator = 0.f;
			const FVector MoveDir  = GetCharacterMovement()->Velocity.GetSafeNormal2D();
			const FVector Feet     = GetActorLocation() - FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			const FVector TrailPos = Feet - MoveDir * 20.f;
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WalkTrailFX, TrailPos, FRotator::ZeroRotator, FVector(1.2f));
		}
	}
	else
	{
		TrailAccumulator = 0.f;
	}

	if (bIsDead || !CachedPlayer || CachedPlayer->bIsDead)
	{
		bIsChasing = false;
		return;
	}

	const FVector MyLoc     = GetActorLocation();
	const FVector PlayerLoc = CachedPlayer->GetActorLocation();
	const float   Dist      = FVector::Dist(MyLoc, PlayerLoc);

	// Aggro hysteresis — engage at DetectionRange, drop only at DetectionRange * AggroDropMultiplier
	if (Dist <= DetectionRange)
		bIsAggro = true;
	else if (Dist > DetectionRange * AggroDropMultiplier)
		bIsAggro = false;

	if (!bIsAggro)
	{
		bIsChasing = false;

		// Patrol back and forth around origin
		if (bPatrolEnabled && !bIsStaggered)
		{
			const float TargetX = PatrolOrigin.X + (bPatrollingRight ? PatrolDistance : -PatrolDistance);
			const float DiffX   = TargetX - MyLoc.X;

			if (FMath::Abs(DiffX) < 10.f)
			{
				bPatrollingRight = !bPatrollingRight;
				GetCharacterMovement()->StopMovementImmediately();
			}
			else
			{
				const float Dir = DiffX > 0.f ? 1.f : -1.f;
				AddMovementInput(FVector(Dir, 0.f, 0.f), 0.5f);
				UpdateFacing(Dir);
			}
		}
		return;
	}

	bIsChasing = true;

	// Don't interfere with mid-dash movement
	if (bIsDashAttacking) return;

	if (Dist <= AttackRange)
	{
		// Close enough — stop and melee
		GetCharacterMovement()->StopMovementImmediately();
		PerformMeleeAttack();
	}
	else if (Dist <= DashAttackRange && Dist >= DashAttackMinRange && !bDashAttackOnCooldown)
	{
		// Gap-closer range — only dash when player is far enough to warrant the lunge
		PerformDashAttack();
	}
	else
	{
		// Outside dash range or dash on cooldown — walk toward player in XY
		FVector Dir = PlayerLoc - MyLoc;
		Dir.Z = 0.f;
		Dir   = Dir.GetSafeNormal();

		AddMovementInput(Dir, 1.f);

		if (!FMath::IsNearlyZero(Dir.X))
			UpdateFacing(Dir.X);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Damage
// ─────────────────────────────────────────────────────────────────────────────

float AEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.f;

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - Applied, 0.f, MaxHealth);

	// Knockback — push away from attacker
	if (DamageCauser)
	{
		const FVector KnockDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
		LaunchCharacter(FVector(KnockDir.X * KnockbackForce, 0.f, KnockbackForce * 0.2f), true, false);
	}

	// Stagger — only apply if not already staggered or immune (prevents infinite chain-stun)
	if (!bIsStaggered && !bStaggerImmune)
	{
		bIsStaggered = true;
		GetWorldTimerManager().SetTimer(StaggerHandle, this, &AEnemyCharacter::EndStagger, StaggerDuration, false);
	}
	// Hurt flash always resets so every hit registers visually
	bIsHurt = true;
	GetWorldTimerManager().ClearTimer(HurtAnimHandle);
	GetWorldTimerManager().SetTimer(HurtAnimHandle, this, &AEnemyCharacter::EndHurt, HurtAnimDuration, false);

	if (TakeDamageSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), TakeDamageSound, GetActorLocation(), TakeDamageVolume);

	if (HitFX)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitFX, GetActorLocation(), FRotator::ZeroRotator, FVector(2.5f));

	RefreshHealthBar();

	if (CurrentHealth <= 0.f)
		Die();

	return Applied;
}

// ─────────────────────────────────────────────────────────────────────────────
// Melee attack
// ─────────────────────────────────────────────────────────────────────────────

void AEnemyCharacter::PerformMeleeAttack()
{
	if (bMeleeOnCooldown || bIsStaggered || !CachedPlayer) return;

	bMeleeOnCooldown = true;
	bIsAttacking     = true;

	if (MeleeAttackSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), MeleeAttackSound, GetActorLocation(), MeleeAttackVolume);

	// Hit 1 — frame 3 of 14 @ 9fps
	GetWorldTimerManager().SetTimer(MeleeHit1Handle, this,
		&AEnemyCharacter::OnMeleeHit1, 0.33f, false);

	// Hit 2 — frame 7 of 14 @ 9fps
	GetWorldTimerManager().SetTimer(MeleeHit2Handle, this,
		&AEnemyCharacter::OnMeleeHit2, 0.78f, false);

	GetWorldTimerManager().SetTimer(MeleeCooldownHandle, this,
		&AEnemyCharacter::ResetMeleeCooldown, AttackCooldown, false);
}

void AEnemyCharacter::DoMeleeHitCheck()
{
	if (!CachedPlayer || bIsDead) return;
	const float Dist = FVector::Dist(GetActorLocation(), CachedPlayer->GetActorLocation());
	if (Dist <= AttackRange * 1.2f)
		UGameplayStatics::ApplyDamage(CachedPlayer, AttackDamage * 0.5f,
			GetController(), this, UDamageType::StaticClass());
}

void AEnemyCharacter::OnMeleeHit1() { DoMeleeHitCheck(); }
void AEnemyCharacter::OnMeleeHit2() { DoMeleeHitCheck(); }

void AEnemyCharacter::ResetMeleeCooldown()
{
	bMeleeOnCooldown = false;
	bIsAttacking     = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Dash attack — lunge toward player, hit if close on arrival
// ─────────────────────────────────────────────────────────────────────────────

void AEnemyCharacter::PerformDashAttack()
{
	if (bDashAttackOnCooldown || bIsDashAttacking || bIsStaggered || !CachedPlayer) return;

	bIsDashAttacking      = true;
	bDashAttackOnCooldown = true;

	if (DashAttackSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashAttackSound, GetActorLocation(), DashAttackVolume);

	// Face the player before lunging
	FVector Dir = CachedPlayer->GetActorLocation() - GetActorLocation();
	Dir.Z = 0.f;
	Dir   = Dir.GetSafeNormal();

	if (!FMath::IsNearlyZero(Dir.X))
		UpdateFacing(Dir.X);

	// Instant burst — overrides XY velocity, preserves Z so gravity still works
	LaunchCharacter(Dir * DashAttackSpeed, true, false);

	GetWorldTimerManager().SetTimer(DashAttackLandHandle,    this, &AEnemyCharacter::OnDashAttackLand,        DashAttackDuration, false);
	GetWorldTimerManager().SetTimer(DashAttackCooldownHandle, this, &AEnemyCharacter::ResetDashAttackCooldown, DashAttackCooldown, false);
}

void AEnemyCharacter::OnDashAttackLand()
{
	bIsDashAttacking = false;

	if (!CachedPlayer) return;

	// Hit 1 — impact: damage on landing if player is close
	const float Dist = FVector::Dist(GetActorLocation(), CachedPlayer->GetActorLocation());
	if (Dist <= DashAttackHitRadius)
	{
		UGameplayStatics::ApplyDamage(CachedPlayer, AttackDamage * DashAttackDamageMultiplier,
			GetController(), this, UDamageType::StaticClass());
	}

	// Hit 2 — slice: follow-up hit a moment later
	GetWorldTimerManager().SetTimer(DashAttackSliceHandle, this,
		&AEnemyCharacter::OnDashAttackSlice, DashAttackSliceDelay, false);
}

void AEnemyCharacter::OnDashAttackSlice()
{
	if (!CachedPlayer || bIsDead) return;

	// Slice only connects if the player is still nearby
	const float Dist = FVector::Dist(GetActorLocation(), CachedPlayer->GetActorLocation());
	if (Dist <= DashAttackHitRadius * 1.3f)   // slightly wider window for the follow-up
	{
		UGameplayStatics::ApplyDamage(CachedPlayer, AttackDamage * DashAttackSliceDamageMultiplier,
			GetController(), this, UDamageType::StaticClass());
	}
}

void AEnemyCharacter::ResetDashAttackCooldown()
{
	bDashAttackOnCooldown = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Death
// ─────────────────────────────────────────────────────────────────────────────

void AEnemyCharacter::Die()
{
	if (bIsDead) return;

	bIsDead          = true;
	bIsChasing       = false;
	bIsDashAttacking = false;

	GetWorldTimerManager().ClearTimer(MeleeCooldownHandle);
	GetWorldTimerManager().ClearTimer(MeleeHit1Handle);
	GetWorldTimerManager().ClearTimer(MeleeHit2Handle);
	GetWorldTimerManager().ClearTimer(DashAttackLandHandle);
	GetWorldTimerManager().ClearTimer(DashAttackSliceHandle);
	GetWorldTimerManager().ClearTimer(DashAttackCooldownHandle);
	GetWorldTimerManager().ClearTimer(FootstepTimerHandle);
	GetWorldTimerManager().ClearTimer(StaggerHandle);
	GetWorldTimerManager().ClearTimer(StaggerImmuneHandle);
	GetWorldTimerManager().ClearTimer(HurtAnimHandle);

	const FVector DeathLoc = GetActorLocation();
	if (DeathSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, DeathLoc, DeathVolume);

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HealthBarComponent->SetVisibility(false);

	// Hide the sprite after the death animation finishes, then destroy
	GetWorldTimerManager().SetTimer(DeathSpriteHideHandle, this,
		&AEnemyCharacter::HideDeathSprite, DeathSpriteHideDelay, false);

	// Scatter spirit pickups after a short delay
	if (SpiritPickupClass)
		GetWorldTimerManager().SetTimer(SpiritSpawnHandle, this, &AEnemyCharacter::SpawnSpiritDrops, SpiritSpawnDelay, false);

	SetLifeSpan(DeathSpriteHideDelay + 1.5f);
}

void AEnemyCharacter::HideDeathSprite()
{
	if (UPrimitiveComponent* RC = GetAnimationComponent()->GetRenderComponent())
		RC->SetVisibility(false);
}

void AEnemyCharacter::SpawnSpiritDrops()
{
	if (!SpiritPickupClass) return;
	const FVector DeathLoc = GetActorLocation();
	const float   SpawnY   = CachedPlayer ? CachedPlayer->GetActorLocation().Y : DeathLoc.Y;
	for (int32 i = 0; i < SpiritDropCount; ++i)
	{
		const FVector DropOffset = FVector(FMath::RandRange(-30.f, 30.f), 0.f, FMath::RandRange(5.f, 25.f));
		const FVector SpawnLoc   = FVector(DeathLoc.X + DropOffset.X, SpawnY, DeathLoc.Z + DropOffset.Z);
		GetWorld()->SpawnActor<ASpiritPickup>(SpiritPickupClass, SpawnLoc, FRotator::ZeroRotator);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

void AEnemyCharacter::EndStagger()
{
	bIsStaggered   = false;
	bStaggerImmune = true;
	// Brief immune window so the enemy can respond before being staggered again
	GetWorldTimerManager().SetTimer(StaggerImmuneHandle,
		[this]() { bStaggerImmune = false; }, StaggerDuration * 2.f, false);
}

void AEnemyCharacter::EndHurt()
{
	bIsHurt = false;
}

void AEnemyCharacter::PlayFootstep()
{
	if (bIsDead || !FootstepSound) return;
	const bool bIsMoving = GetCharacterMovement()->Velocity.SizeSquared2D() > 100.f;
	if (!bIsMoving) return;

	float Volume = FootstepVolume;
	if (CachedPlayer)
	{
		const float Dist = FVector::Dist(GetActorLocation(), CachedPlayer->GetActorLocation());
		Volume = FootstepVolume * FMath::Clamp(1.f - (Dist / FootstepMaxHearDistance), 0.f, 1.f);
	}
	if (Volume > 0.f)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FootstepSound, GetActorLocation(), Volume);
}

void AEnemyCharacter::UpdateFacing(float XDir)
{
	const bool bWantsRight = XDir > 0.f;
	if (bWantsRight == bIsFacingRight) return;
	bIsFacingRight = bWantsRight;
	SetActorRotation(FRotator(0.f, bIsFacingRight ? 0.f : 180.f, 0.f));
}

void AEnemyCharacter::RefreshHealthBar()
{
	if (HealthBarWidget)
		HealthBarWidget->SetHealthPercent(CurrentHealth / MaxHealth);
}
