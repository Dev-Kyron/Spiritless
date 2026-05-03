#include "EnemyAnimInstance.h"
#include "EnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UEnemyAnimInstance::SyncFromEnemy(AEnemyCharacter* Enemy)
{
	if (!Enemy) return;

	const float Speed = FMath::Abs(Enemy->GetCharacterMovement()->Velocity.X);

	bIsWalking      = Enemy->bIsChasing && Speed > 10.f && !Enemy->bIsAttacking && !Enemy->bIsDashAttacking;
	bIsAttacking    = Enemy->bIsAttacking;
	bIsDashAttacking = Enemy->bIsDashAttacking;
	bIsDead         = Enemy->bIsDead;
}
