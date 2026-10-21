#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "S47Walker.generated.h"

class UCameraComponent;

// First person at SARO: WASD walks, the mouse looks, E or a click uses the thing in front of
// you. Input is polled, so no input mappings are needed. Things that can be used carry an actor
// tag "Use:<id>" (S47Saro).
UCLASS()
class SIGNAL47_API AS47Walker : public ACharacter
{
	GENERATED_BODY()

public:
	AS47Walker();
	virtual void Tick(float DeltaSeconds) override;

	void SetActive(bool bOn) { bActive = bOn; }
	FString FocusedId() const { return Focused; }
	TFunction<void(const FString&)> OnUse;
	void PlaceAt(const FTransform& T);
	FVector EyeLocation() const;
	FVector EyeDirection() const;

	float WalkSpeed = 190.f;
	float LookSpeed = 0.09f;
	float Reach = 230.f;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	bool bActive = false;
	FString Focused;
	float Settle = 0.f; // seconds left to ignore the mouse after a placement
};
