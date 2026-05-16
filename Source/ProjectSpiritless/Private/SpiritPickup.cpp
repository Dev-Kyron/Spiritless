#include "SpiritPickup.h"
#include "PlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

ASpiritPickup::ASpiritPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(40.f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(CollisionSphere);

	SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	SpriteComponent->SetupAttachment(RootComponent);
	SpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASpiritPickup::OnOverlapBegin);
}

// ─────────────────────────────────────────────────────────────────────────────
// Begin Play
// ─────────────────────────────────────────────────────────────────────────────

void ASpiritPickup::BeginPlay()
{
	Super::BeginPlay();
	OriginLocation = GetActorLocation();

	// Stencil value 2 — the desaturation post-process material reads this and skips
	// desaturating these pixels so spirit orbs always appear in full colour.
	SpriteComponent->SetRenderCustomDepth(true);
	SpriteComponent->CustomDepthStencilValue = 2;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick — bob
// ─────────────────────────────────────────────────────────────────────────────

void ASpiritPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bCollected) return;

	// Magnet — pull toward player when nearby, updating origin so bob follows
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		if (!Player->bIsDead)
		{
			const float Dist = static_cast<float>(FVector::Dist(OriginLocation, Player->GetActorLocation()));
			if (Dist <= MagnetRadius)
			{
				const FVector Dir = (Player->GetActorLocation() - OriginLocation).GetSafeNormal2D();
				OriginLocation += Dir * MagnetSpeed * DeltaTime;
			}
		}
	}

	BobAccumulator += DeltaTime * BobSpeed;
	SetActorLocation(OriginLocation + FVector(0.f, 0.f, FMath::Sin(BobAccumulator) * BobHeight));
}

// ─────────────────────────────────────────────────────────────────────────────
// Collection
// ─────────────────────────────────────────────────────────────────────────────

void ASpiritPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bCollected) return;

	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (!Player || Player->bIsDead) return;

	bCollected = true;
	Player->SpiritCount += SpiritValue;

	if (PickupSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PickupSound, GetActorLocation(), PickupVolume);

	if (Player->SpiritPickupSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Player->SpiritPickupSound,
			Player->GetActorLocation(), Player->SpiritPickupVolume);

	Destroy();
}
