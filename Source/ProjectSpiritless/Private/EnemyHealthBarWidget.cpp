#include "EnemyHealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UEnemyHealthBarWidget::SetHealthPercent(float Percent)
{
	if (HealthBar)
		HealthBar->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
}

void UEnemyHealthBarWidget::SetEnemyName(const FText& InName)
{
	if (NameText)
		NameText->SetText(InName);
}
