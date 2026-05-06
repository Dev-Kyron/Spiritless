#include "PlayerHealthBarWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

// ─────────────────────────────────────────────────────────────────────────────
// Construct
// ─────────────────────────────────────────────────────────────────────────────

void UPlayerHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshDecorations();

	// Build bar at full health on first show
	RefreshOrbs(static_cast<float>(MaxOrbs), static_cast<float>(MaxOrbs));
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void UPlayerHealthBarWidget::RefreshOrbs(float CurrentHP, float MaxHP)
{
	if (!OrbContainer) return;

	const float Ratio       = (MaxHP > 0.f) ? FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f) : 0.f;
	const int32 FilledOrbs  = FMath::RoundToInt(Ratio * MaxOrbs);
	const bool  bIsLow      = Ratio <= LowHealthThreshold;

	RebuildOrbContainer(FilledOrbs, bIsLow);
}

void UPlayerHealthBarWidget::RefreshDecorations()
{
	auto SetTex = [](UImage* Img, UTexture2D* Tex)
	{
		if (Img && Tex) Img->SetBrushFromTexture(Tex, /*bMatchSize=*/true);
	};

	SetTex(PlayerFrameImage, PlayerFrameTexture);
	SetTex(EndFlameImage,    EndFlameTexture);
	SetTex(LeafLeftImage,    LeafLeftTexture);
	SetTex(LeafRightImage,   LeafRightTexture);
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal rebuild
// ─────────────────────────────────────────────────────────────────────────────

void UPlayerHealthBarWidget::RebuildOrbContainer(int32 FilledOrbs, bool bIsLow)
{
	OrbContainer->ClearChildren();

	for (int32 i = 0; i < MaxOrbs; ++i)
	{
		// ── Connection strip before every orb except the first ────────────────
		if (i > 0 && ConnectionTexture)
		{
			UImage* Conn = NewObject<UImage>(this);
			Conn->SetBrushFromTexture(ConnectionTexture, false);
			Conn->SetBrushSize(ConnectionSize);

			UHorizontalBoxSlot* ConnSlot = OrbContainer->AddChildToHorizontalBox(Conn);
			ConnSlot->SetHorizontalAlignment(HAlign_Center);
			ConnSlot->SetVerticalAlignment(VAlign_Center);
		}

		// ── Orb ───────────────────────────────────────────────────────────────
		UImage* Orb = NewObject<UImage>(this);

		const bool bFilled = (i < FilledOrbs);
		UTexture2D* Tex = nullptr;

		if (bFilled)
		{
			// Use OrbLowTexture for the highest filled orb when HP is low
			const bool bIsLastFilled = (i == FilledOrbs - 1);
			Tex = (bIsLow && bIsLastFilled && OrbLowTexture) ? OrbLowTexture : OrbFullTexture;
		}
		else
		{
			Tex = OrbEmptyTexture;
		}

		if (Tex) Orb->SetBrushFromTexture(Tex, false);
		Orb->SetBrushSize(OrbSize);

		UHorizontalBoxSlot* OrbSlot = OrbContainer->AddChildToHorizontalBox(Orb);
		OrbSlot->SetHorizontalAlignment(HAlign_Center);
		OrbSlot->SetVerticalAlignment(VAlign_Center);
	}
}
