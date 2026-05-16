#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpiritPickup.generated.h"

class USphereComponent;
class UPaperSpriteComponent;
class USoundBase;

UCLASS(Blueprintable, BlueprintType)
class PROJECTSPIRITLESS_API ASpiritPickup : public AActor
{
	GENERATED_BODY()

public:
	ASpiritPickup();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPaperSpriteComponent* SpriteComponent;

	// How many spirits this pickup grants the player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spirit")
	int32 SpiritValue = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spirit")
	float BobHeight = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spirit")
	float BobSpeed = 2.f;

	// Distance at which the orb starts pulling toward the player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spirit")
	float MagnetRadius = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spirit")
	float MagnetSpeed = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* PickupSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float PickupVolume = 1.0f;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	FVector OriginLocation;
	float   BobAccumulator = 0.f;
	bool    bCollected     = false;
};
