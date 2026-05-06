#include "SpiritHUDWidget.h"
#include "PlayerCharacter.h"
#include "Kismet/GameplayStatics.h"

void USpiritHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CachedPlayer = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
}

int32 USpiritHUDWidget::GetSpiritCount() const
{
	return CachedPlayer ? CachedPlayer->SpiritCount : 0;
}

int32 USpiritHUDWidget::GetDepositedCount() const
{
	return CachedPlayer ? CachedPlayer->DepositedSpiritCount : 0;
}
