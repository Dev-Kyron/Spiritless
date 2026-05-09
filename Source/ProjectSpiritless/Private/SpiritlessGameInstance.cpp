#include "SpiritlessGameInstance.h"
#include "SpiritlessSaveGame.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/GameUserSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

void USpiritlessGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &USpiritlessGameInstance::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USpiritlessGameInstance::OnPostLoadMap);

	// Restore persisted difficulty from the save slot.
	LoadSettings();

	// Apply saved resolution + scalability so the window opens at the correct size.
	// Skip in PIE — the editor controls the viewport there and applying resolution
	// causes a resize event that breaks the settings widget panel visibility.
#if !WITH_EDITOR
	if (UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings())
	{
		GUS->ApplySettings(false);
	}
#endif
}

void USpiritlessGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	StopMenuMusic();
	Super::Shutdown();
}

// ── Loading Screen ────────────────────────────────────────────────────────────

void USpiritlessGameInstance::OnPreLoadMap(const FString& MapName)
{
	if (MapName.Contains(TEXT("PlatformLVL1")))
	{
		ShowLoadingScreen();
		StopMenuMusic();
	}
}

// ── Difficulty ────────────────────────────────────────────────────────────────
//               Easy    Medium   Hard
// Player dmg    0.6x    1.0x     1.5x
// Enemy HP      0.7x    1.0x     1.4x
// Enemy dmg     0.7x    1.0x     1.35x

float USpiritlessGameInstance::GetPlayerDamageTakenMultiplier() const
{
	switch (CurrentDifficulty)
	{
		case EDifficulty::Easy: return 0.6f;
		case EDifficulty::Hard: return 1.5f;
		default:                return 1.0f;
	}
}

float USpiritlessGameInstance::GetEnemyHealthMultiplier() const
{
	switch (CurrentDifficulty)
	{
		case EDifficulty::Easy: return 0.7f;
		case EDifficulty::Hard: return 1.4f;
		default:                return 1.0f;
	}
}

float USpiritlessGameInstance::GetEnemyDamageMultiplier() const
{
	switch (CurrentDifficulty)
	{
		case EDifficulty::Easy: return 0.7f;
		case EDifficulty::Hard: return 1.35f;
		default:                return 1.0f;
	}
}

void USpiritlessGameInstance::SetDifficulty(EDifficulty NewDifficulty)
{
	CurrentDifficulty = NewDifficulty;
	SaveSettings();
}

void USpiritlessGameInstance::SaveSettings()
{
	USpiritlessSaveGame* Save = Cast<USpiritlessSaveGame>(
		UGameplayStatics::CreateSaveGameObject(USpiritlessSaveGame::StaticClass()));
	if (!Save) return;

	Save->SavedDifficulty = CurrentDifficulty;
	UGameplayStatics::SaveGameToSlot(Save, TEXT("SpiritlessSettings"), 0);
}

void USpiritlessGameInstance::LoadSettings()
{
	if (!UGameplayStatics::DoesSaveGameExist(TEXT("SpiritlessSettings"), 0)) return;

	USpiritlessSaveGame* Save = Cast<USpiritlessSaveGame>(
		UGameplayStatics::LoadGameFromSlot(TEXT("SpiritlessSettings"), 0));
	if (!Save) return;

	CurrentDifficulty = Save->SavedDifficulty;
}

// ── Resolution ────────────────────────────────────────────────────────────────

void USpiritlessGameInstance::ApplyResolution(int32 Width, int32 Height, int32 WindowModeInt)
{
	UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings();
	if (!GUS) return;

	const EWindowMode::Type Mode = (WindowModeInt == 0) ? EWindowMode::Fullscreen
	                             : (WindowModeInt == 1) ? EWindowMode::WindowedFullscreen
	                                                    : EWindowMode::Windowed;

	GUS->SetScreenResolution(FIntPoint(Width, Height));
	GUS->SetFullscreenMode(Mode);
	GUS->ApplySettings(false);

	// Lock the OS window so the user can't manually drag it to a different size (packaged only)
#if !WITH_EDITOR
	if (Mode == EWindowMode::Windowed && FSlateApplication::IsInitialized())
	{
		TSharedPtr<SWindow> Window = FSlateApplication::Get().GetActiveTopLevelWindow();
		if (Window.IsValid())
			Window->SetSizingRule(ESizingRule::FixedSize);
	}
#endif
}

void USpiritlessGameInstance::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld) return;

	// Re-lock the window to fixed size after every level transition (packaged only)
#if !WITH_EDITOR
	if (FSlateApplication::IsInitialized())
	{
		TSharedPtr<SWindow> Window = FSlateApplication::Get().GetActiveTopLevelWindow();
		if (Window.IsValid())
			Window->SetSizingRule(ESizingRule::FixedSize);
	}
#endif

	// Schedule menu music — store the world so the callback uses the right context
	if (!LoadedWorld->GetName().Contains(TEXT("PlatformLVL1")))
	{
		MusicPendingWorld = LoadedWorld;
		LoadedWorld->GetTimerManager().SetTimer(MusicStartHandle, this,
			&USpiritlessGameInstance::DelayedStartMenuMusic, 1.0f, false);
	}

	// Hold the loading screen after gameplay level is ready
	if (LoadingScreenWidget)
	{
		LoadedWorld->GetTimerManager().SetTimer(
			LoadingHoldHandle,
			this,
			&USpiritlessGameInstance::DoHideLoadingScreen,
			LoadingScreenHoldTime,
			false);
	}
}

void USpiritlessGameInstance::ShowLoadingScreen()
{
	if (!LoadingScreenClass || LoadingScreenWidget) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	LoadingScreenWidget = CreateWidget<UUserWidget>(PC, LoadingScreenClass);
	if (LoadingScreenWidget)
		LoadingScreenWidget->AddToViewport(100);
}

void USpiritlessGameInstance::HideLoadingScreen()
{
	DoHideLoadingScreen();
}

void USpiritlessGameInstance::DoHideLoadingScreen()
{
	if (LoadingScreenWidget)
	{
		LoadingScreenWidget->RemoveFromParent();
		LoadingScreenWidget = nullptr;
	}
}

// ── Menu Music ────────────────────────────────────────────────────────────────

void USpiritlessGameInstance::DelayedStartMenuMusic()
{
	if (MenuMusicComponent && MenuMusicComponent->IsPlaying()) return;
	if (!MenuMusicSound) return;

	// Prefer the stored world from OnPostLoadMap; fall back to GetWorld()
	UWorld* World = MusicPendingWorld.IsValid() ? MusicPendingWorld.Get() : GetWorld();
	if (!World) return;

	MenuMusicComponent = UGameplayStatics::SpawnSound2D(
		World, MenuMusicSound, MenuMusicVolume, 1.f, 0.f, nullptr, true, true);
	MusicPendingWorld.Reset();
}

void USpiritlessGameInstance::PlayMenuMusic()
{
	if (!MenuMusicSound) return;
	if (MenuMusicComponent && MenuMusicComponent->IsPlaying()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	MenuMusicComponent = UGameplayStatics::SpawnSound2D(
		World, MenuMusicSound, MenuMusicVolume, 1.f, 0.f, nullptr, true, true);
}

void USpiritlessGameInstance::StopMenuMusic()
{
	if (MenuMusicComponent)
	{
		MenuMusicComponent->FadeOut(1.5f, 0.f);
		MenuMusicComponent = nullptr;
	}
}

// ── UI SFX ───────────────────────────────────────────────────────────────────

void USpiritlessGameInstance::PlayButtonHover()
{
	if (ButtonHoverSound)
		UGameplayStatics::PlaySound2D(this, ButtonHoverSound, ButtonHoverVolume);
}

void USpiritlessGameInstance::PlayButtonClick()
{
	if (ButtonClickSound)
		UGameplayStatics::PlaySound2D(this, ButtonClickSound, ButtonClickVolume);
}

void USpiritlessGameInstance::PlayMenuOpen()
{
	if (MenuOpenSound)
		UGameplayStatics::PlaySound2D(this, MenuOpenSound, MenuOpenVolume);
}

void USpiritlessGameInstance::PlayMenuClose()
{
	if (MenuCloseSound)
		UGameplayStatics::PlaySound2D(this, MenuCloseSound, MenuCloseVolume);
}
