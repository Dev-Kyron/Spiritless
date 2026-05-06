#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHealthBarWidget.generated.h"

class UHorizontalBox;
class UImage;
class UTexture2D;

/**
 * Orb-based player health bar.
 *
 * Blueprint setup (WBP_PlayerHealthBar, parent = PlayerHealthBarWidget):
 *   - Root: HorizontalBox or Canvas — lay out PlayerFrameImage, OrbContainer,
 *     EndFlameImage, LeafLeftImage, LeafRightImage however you like.
 *   - OrbContainer (HorizontalBox) — REQUIRED BindWidget. C++ fills this with
 *     orb + connection images at runtime.
 *   - All Image widgets are optional BindWidgets; assign them in the BP if you
 *     want C++ to push their textures automatically.
 *
 * Assign textures in the BP Class Defaults:
 *   OrbFullTexture, OrbEmptyTexture, OrbLowTexture, ConnectionTexture,
 *   EndFlameTexture, PlayerFrameTexture, LeafLeftTexture, LeafRightTexture
 *
 * Call RefreshOrbs(CurrentHP, MaxHP) whenever health changes.
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTSPIRITLESS_API UPlayerHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ── Required ──────────────────────────────────────────────────────────────
	// C++ fills this with orb + connection images. Name it exactly "OrbContainer"
	// in your WBP hierarchy.
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UHorizontalBox* OrbContainer;

	// ── Optional frame / decoration (BindWidgetOptional) ─────────────────────
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* PlayerFrameImage;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* EndFlameImage;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* LeafLeftImage;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* LeafRightImage;

	// ── Textures — assign in BP Class Defaults ────────────────────────────────
	// Import your PNGs into Content/UI/HealthBar/ then assign here.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar|Textures")
	UTexture2D* OrbFullTexture;    // T_Orb_Full.png   — full blue orb

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar|Textures")
	UTexture2D* OrbLowTexture;     // T_Orb_Low.png    — last orb when HP <= LowThreshold

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar|Textures")
	UTexture2D* OrbEmptyTexture;   // T_Orb_Empty.png  — empty orb slot

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar|Textures")
	UTexture2D* ConnectionTexture; // T_Connection.png — wavy line between orbs

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar|Textures")
	UTexture2D* EndFlameTexture;   // T_EndFlame_Player.png

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar|Textures")
	UTexture2D* PlayerFrameTexture;// T_PlayerFrame.png

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar|Textures")
	UTexture2D* LeafLeftTexture;   // T_Leaf_Left.png

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar|Textures")
	UTexture2D* LeafRightTexture;  // T_Leaf_Right.png

	// ── Layout settings ───────────────────────────────────────────────────────
	// Number of orbs displayed (each orb = MaxHealth / MaxOrbs HP)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar", meta = (ClampMin = "1"))
	int32 MaxOrbs = 8;

	// Size of each orb image in the horizontal box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar")
	FVector2D OrbSize = FVector2D(64.f, 64.f);

	// Size of the connection strip between orbs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar")
	FVector2D ConnectionSize = FVector2D(32.f, 20.f);

	// HP fraction at which the last filled orb shows OrbLowTexture (e.g. 0.25 = bottom 25%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthBar", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthThreshold = 0.25f;

	// ── API ───────────────────────────────────────────────────────────────────
	// Called by PlayerCharacter whenever HP changes. Rebuilds the orb images.
	UFUNCTION(BlueprintCallable, Category = "HealthBar")
	void RefreshOrbs(float CurrentHP, float MaxHP);

	// Convenience: refreshes static frame/decoration images from the texture properties.
	// Called once in NativeConstruct; also callable manually after changing textures.
	UFUNCTION(BlueprintCallable, Category = "HealthBar")
	void RefreshDecorations();

protected:
	virtual void NativeConstruct() override;

private:
	void RebuildOrbContainer(int32 FilledOrbs, bool bIsLow);
};
