#include "S47Walker.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

AS47Walker::AS47Walker()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(30.f, 90.f);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Eyes"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0, 0, 72.f)); // eye height about 1.62 m
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(80.f);
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = 1200.f;
	GetCharacterMovement()->MaxAcceleration = 900.f;
	GetMesh()->SetVisibility(false);
}

void AS47Walker::PlaceAt(const FTransform& T)
{
	SetActorLocation(T.GetLocation());
	if (AController* C = GetController()) C->SetControlRotation(FRotator(T.Rotator().Pitch, T.Rotator().Yaw, 0));
	// The first mouse delta after the viewport takes the cursor can be large; let it pass.
	Settle = 0.5f;
}

FVector AS47Walker::EyeLocation() const { return Camera->GetComponentLocation(); }
FVector AS47Walker::EyeDirection() const { return Camera->GetForwardVector(); }

void AS47Walker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APlayerController* PC = Cast<APlayerController>(GetController());
	Focused.Reset();
	if (!PC || !bActive) return;

	float DX = 0, DY = 0;
	PC->GetInputMouseDelta(DX, DY);
	if (Settle > 0.f) { Settle -= DeltaSeconds; DX = DY = 0.f; }
	AddControllerYawInput(FMath::Clamp(DX, -150.f, 150.f) * LookSpeed);
	AddControllerPitchInput(-FMath::Clamp(DY, -150.f, 150.f) * LookSpeed);

	const FRotator Yaw(0, GetControlRotation().Yaw, 0);
	const float F = (PC->IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (PC->IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
	const float R = (PC->IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (PC->IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
	if (F != 0.f) AddMovementInput(Yaw.Vector(), F);
	if (R != 0.f) AddMovementInput(FRotationMatrix(Yaw).GetScaledAxis(EAxis::Y), R);

	// What the player looks at, if it can be used.
	FHitResult Hit;
	FCollisionQueryParams P(SCENE_QUERY_STAT(S47Use), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation(), EyeLocation() + EyeDirection() * Reach, ECC_Visibility, P) && Hit.GetActor())
	{
		for (const FName& Tag : Hit.GetActor()->Tags)
		{
			const FString S = Tag.ToString();
			if (S.StartsWith(TEXT("Use:"))) Focused = S.Mid(4);
		}
	}
	if (!Focused.IsEmpty() && OnUse && (PC->WasInputKeyJustPressed(EKeys::E) || PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)))
	{
		OnUse(Focused);
	}
}
