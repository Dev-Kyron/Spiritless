#include "HeroAnimInstance.h"
#include "PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UHeroAnimInstance::SyncFromCharacter(APlayerCharacter* Character)
{
	if (!Character) return;

	const UCharacterMovementComponent* CM = Character->GetCharacterMovement();

	// Movement
	MoveSpeed      = FMath::Abs(CM->Velocity.X);
	bIsMoving      = MoveSpeed > 10.f;
	bIsDashing     = Character->bIsDashing;
	bIsWallSliding = Character->bIsWallSliding;
	bIsWallJumping = Character->bIsWallJumping;
	bJustLanded    = Character->bJustLanded;

	// Jump — legacy
	bIsJumping = CM->IsFalling() && CM->Velocity.Z > 0.f;
	bIsFalling = CM->IsFalling() && CM->Velocity.Z <= 0.f;

	// Jump — granular phases
	bIsJumpRising  = Character->bIsJumpRising;
	bIsJumpApex    = Character->bIsJumpApex;
	bIsJumpFalling = Character->bIsJumpFalling;

	// Combat
	bIsAttacking      = Character->bIsAttacking;
	ComboIndex        = Character->ComboStep;
	bIsAirAttacking   = Character->bIsAirAttacking;
	bIsDefending      = Character->bIsDefending;
	bIsDefendBroken   = Character->bIsDefendBroken;
	bIsHealing = Character->bIsHealing;
	bIsHurt    = Character->bIsHurt;

	// Death
	bIsDead = Character->bIsDead;
}
