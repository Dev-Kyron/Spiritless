#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpiritDepositPoint.generated.h"

class USphereComponent;
class UPaperSpriteComponent;
class UWidgetComponent;
class USoundBase;
class UNiagaraSystem;
class APlayerCharacter;

UCLASS(Blueprintable, BlueprintType)
class PROJECTSPIRITLESS_API ASpiritDepositPoint : public AActor
{
	GENERATED_BODY()

public:
	ASpiritDepositPoint();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPaperSpriteComponent* SpriteComponent;

	// Floating "Press E" prompt — assign WBP_InteractPrompt in Blueprint Details
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* PromptWidget;

	// Distance at which the prompt appears and E press works
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deposit")
	float InteractRadius = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deposit")
	float BobHeight = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deposit")
	float BobSpeed = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DepositSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* DepositFX;

	// True when player is in range — readable by HUD
	UPROPERTY(BlueprintReadOnly, Category = "Deposit")
	bool bPlayerInRange = false;

	// Called by PlayerCharacter::Interact()
	UFUNCTION(BlueprintCallable, Category = "Deposit")
	void TryDeposit(APlayerCharacter* Player);

private:
	APlayerCharacter* CachedPlayer = nullptr;
	FVector           OriginLocation;
	float             BobAccumulator = 0.f;
};
