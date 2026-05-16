#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SpiritlessGameInstance.generated.h"

UENUM(BlueprintType)
enum class EDifficulty : uint8
{
	Easy   UMETA(DisplayName = "Easy"),
	Medium UMETA(DisplayName = "Medium"),
	Hard   UMETA(DisplayName = "Hard")
};

// Snapshot of every graphics quality knob — used to init the settings UI and detect unapplied changes.
// Individual levels: 0=Low  1=Medium  2=High  3=Epic.  OverallLevel=-1 when settings are mixed.
USTRUCT(BlueprintType)
struct FGraphicsQualitySettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) int32 OverallLevel    = 1;
	UPROPERTY(BlueprintReadWrite) int32 AntiAliasing    = 1;
	UPROPERTY(BlueprintReadWrite) int32 Shadow          = 1;
	UPROPERTY(BlueprintReadWrite) int32 Texture         = 1;
	UPROPERTY(BlueprintReadWrite) int32 Foliage         = 1;
	UPROPERTY(BlueprintReadWrite) int32 PostProcess     = 1;
	UPROPERTY(BlueprintReadWrite) int32 ViewDistance    = 1;
	UPROPERTY(BlueprintReadWrite) bool  bVSync          = false;
};

class UAudioComponent;
class USoundBase;
class UUserWidget;

UCLASS()
class PROJECTSPIRITLESS_API USpiritlessGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// ── Difficulty ────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	EDifficulty CurrentDifficulty = EDifficulty::Medium;

	// Called from the settings screen when the player picks Easy / Medium / Hard
	UFUNCTION(BlueprintCallable, Category = "Game")
	void SetDifficulty(EDifficulty NewDifficulty);

	// Persist difficulty to disk (called automatically by SetDifficulty)
	UFUNCTION(BlueprintCallable, Category = "Game")
	void SaveSettings();

	// Load persisted difficulty from disk (called at Init)
	UFUNCTION(BlueprintCallable, Category = "Game")
	void LoadSettings();

	// Damage multiplier applied to every hit the PLAYER receives
	UFUNCTION(BlueprintPure, Category = "Game")
	float GetPlayerDamageTakenMultiplier() const;

	// Scale applied to every enemy's MaxHealth at BeginPlay
	UFUNCTION(BlueprintPure, Category = "Game")
	float GetEnemyHealthMultiplier() const;

	// Scale applied to every enemy's AttackDamage at BeginPlay
	UFUNCTION(BlueprintPure, Category = "Game")
	float GetEnemyDamageMultiplier() const;

	// ── Game Flow ─────────────────────────────────────────────────────────────
	// Set to true by the New Game button Blueprint before opening PlatformLVL1.
	// PlayerCharacter reads and clears it in BeginPlay to show WDG_Tips_04 once.
	UPROPERTY(BlueprintReadWrite, Category = "Game")
	bool bIsNewGame = false;

	// Assign PlatformLVL1 (or whichever map the New Game / Continue buttons open) here
	// in the GameInstance Blueprint Details panel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	TSoftObjectPtr<UWorld> GameLevelToPreload;

	// Call from WDG_game_04 Event Construct so the engine starts streaming the game
	// level package into memory while the player reads the menu. OpenLevel will then
	// find the assets already cached and load near-instantly.
	// Safe to call multiple times — only the first call does any work per menu visit.
	UFUNCTION(BlueprintCallable, Category = "Game")
	void PreloadGameLevel();

	// ── Resolution ───────────────────────────────────────────────────────────
	// Call from your settings widget to apply a fixed resolution.
	// WindowMode: 0=Fullscreen  1=Borderless  2=Windowed (fixed size)
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyResolution(int32 Width, int32 Height, int32 WindowMode = 2);

	// ── Graphics Quality ──────────────────────────────────────────────────────
	// Read all current quality values — call this to initialise the settings UI on open.
	UFUNCTION(BlueprintPure, Category = "Settings")
	FGraphicsQualitySettings GetCurrentGraphicsSettings() const;

	// True if any value in Pending differs from what is currently applied.
	UFUNCTION(BlueprintPure, Category = "Settings")
	bool HasUnappliedGraphicsChanges(const FGraphicsQualitySettings& Pending) const;

	// Apply and save all quality settings at once.
	// Pass OverallLevel >= 0 to set everything to one preset; set it to -1 to use the individual values.
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyGraphicsQualitySettings(const FGraphicsQualitySettings& Settings);

	// ── Loading Screen ────────────────────────────────────────────────────────
	// Assign WDG_Loading_04 here — shown automatically on any level load
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> LoadingScreenClass;

	// How long (seconds) the loading screen stays visible after the level is ready
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (ClampMin = "0.0"))
	float LoadingScreenHoldTime = 2.5f;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowLoadingScreen();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideLoadingScreen();

	// ── Menu Music ────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Music")
	USoundBase* MenuMusicSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Music", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MenuMusicVolume = 0.4f;

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void PlayMenuMusic();

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void StopMenuMusic();

	// ── UI Sound FX ───────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX")
	USoundBase* ButtonHoverSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ButtonHoverVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX")
	USoundBase* ButtonClickSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ButtonClickVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX")
	USoundBase* MenuOpenSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MenuOpenVolume = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX")
	USoundBase* MenuCloseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|SFX", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MenuCloseVolume = 0.9f;

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayButtonHover();

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayButtonClick();

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayMenuOpen();

	UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
	void PlayMenuClose();

private:
	void OnPreLoadMap(const FString& MapName);
	void OnPostLoadMap(UWorld* LoadedWorld);
	void DoHideLoadingScreen();
	void DelayedStartMenuMusic();

	UPROPERTY()
	UAudioComponent* MenuMusicComponent = nullptr;

	UPROPERTY()
	UUserWidget* LoadingScreenWidget = nullptr;

	FTimerHandle LoadingHoldHandle;
	FTimerHandle MusicStartHandle;
	bool bLevelPreloadStarted = false;

	// Weak ref to the world that requested music — used inside the delayed callback
	TWeakObjectPtr<UWorld> MusicPendingWorld;
};
