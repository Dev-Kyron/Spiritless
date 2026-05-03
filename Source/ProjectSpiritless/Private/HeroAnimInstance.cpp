#include "HeroAnimInstance.h"
#include "PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UHeroAnimInstance::SyncFromCharacter(APlayerCharacter* Character)
{
	if (!Character) return;

	const UCharacterMovementComponent* CM = Character->GetCharacterMovement();

	MoveSpeed  = FMath::Abs(CM->Velocity.X);
	bIsMoving  = MoveSpeed > 10.f;
	bIsJumping = CM->IsFalling() && CM->Velocity.Z > 0.f;
	bIsFalling = CM->IsFalling() && CM->Velocity.Z <= 0.f;

	bIsAttacking = Character->bIsAttacking;
	ComboIndex   = Character->ComboStep;
	bIsDead      = Character->bIsDead;
	bJustLanded  = Character->bJustLanded;
}
