#include "SpiritDepositPoint.h"
#include "PlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

ASpiritDepositPoint::ASpiritDepositPoint()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionRoot = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionRoot"));
	CollisionRoot->InitSphereRadius(50.f);
	CollisionRoot->SetCollisionProfileName(TEXT("NoCollision"));
	SetRootComponent(CollisionRoot);

	SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	SpriteComponent->SetupAttachment(RootComponent);
	SpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidget"));
	PromptWidget->SetupAttachment(RootComponent);
	PromptWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	PromptWidget->SetWidgetSpace(EWidgetSpace::Screen);
	PromptWidget->SetDrawSize(FVector2D(200.f, 60.f));
	PromptWidget->SetVisibility(false);
}

// ─────────────────────────────────────────────────────────────────────────────
// Begin Play
// ─────────────────────────────────────────────────────────────────────────────

void ASpiritDepositPoint::BeginPlay()
{
	Super::BeginPlay();
	OriginLocation = GetActorLocation();
	CachedPlayer   = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick — proximity check + prompt visibility + bob
// ─────────────────────────────────────────────────────────────────────────────

void ASpiritDepositPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Bob
	BobAccumulator += DeltaTime * BobSpeed;
	SetActorLocation(OriginLocation + FVector(0.f, 0.f, FMath::Sin(BobAccumulator) * BobHeight));

	// Proximity — show/hide prompt
	if (CachedPlayer && !CachedPlayer->bIsDead)
	{
		const float Dist = static_cast<float>(FVector::Dist(GetActorLocation(), CachedPlayer->GetActorLocation()));
		bPlayerInRange = Dist <= InteractRadius;
	}
	else
	{
		bPlayerInRange = false;
	}

	PromptWidget->SetVisibility(bPlayerInRange);
}

// ─────────────────────────────────────────────────────────────────────────────
// Deposit — called by PlayerCharacter when E is pressed
// ─────────────────────────────────────────────────────────────────────────────

void ASpiritDepositPoint::TryDeposit(APlayerCharacter* Player)
{
	if (!Player || Player->bIsDead || Player->SpiritCount <= 0) return;

	Player->DepositedSpiritCount += Player->SpiritCount;
	Player->SpiritCount           = 0;
	Player->OnSpiritDeposited();

	if (DepositSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DepositSound, GetActorLocation());

	if (DepositFX)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DepositFX,
			GetActorLocation(), FRotator::ZeroRotator, FVector(2.f));
}
