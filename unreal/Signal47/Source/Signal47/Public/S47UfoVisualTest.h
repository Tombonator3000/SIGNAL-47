#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"

class US47Main;

// Opt-in native presentation/photo smoke test. Explicit fixture, never a player case.
class FS47UfoVisualTest
{
public:
	explicit FS47UfoVisualTest(US47Main* Main) : M(Main) {}
	void Step(double Dt);

private:
	US47Main* M = nullptr;
	bool bInit = false, bDone = false;
	int32 Stage = 0, Checks = 0, BudgetBefore = 0;
	double WallStart = 0, StageAt = 0, PausedAge = 0, PausedClock = 0;
	FVector FixedAnchor = FVector::ZeroVector;
	FString Root;
	TArray<TSharedPtr<FJsonValue>> Evidence;
	bool Check(bool Good, const FString& Text);
	void Go(int32 Next);
	void Capture(const FString& Name);
	void Finish(bool Good, const FString& Why);
	int32 VisibleMeshes() const;
	int32 ActiveLights() const;
};
