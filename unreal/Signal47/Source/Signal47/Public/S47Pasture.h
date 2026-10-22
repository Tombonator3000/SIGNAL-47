#pragma once

#include "CoreMinimal.h"
#include "S47Seen.h"

class FS47Herd;

// story/pasture.ts: nine facing 026, then eight and frost. State lives in main.ts's townSeen.
struct FS47PastureIn
{
	bool bOk = false;
	FString Area; // 'saro', 'road', or empty
	FS47Cam Cam;
	double Clock = 0;
	bool bDriving = false;
};

struct FS47PastureDeps
{
	TFunction<TArray<FS47Herd*>()> Herds;
	FS47TownSeen* Seen = nullptr;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, double)> Toast;
	TFunction<void()> Save;
	TFunction<bool()> NinthFound;
};

class SIGNAL47_API FS47Pasture
{
public:
	explicit FS47Pasture(FS47PastureDeps InD) : D(MoveTemp(InD)) {}
	FString Stage = TEXT("graze");
	int32 Passes = 0;
	bool bAway = true;
	static constexpr double PASTURE_M = 856;
	void Reset();
	void Update(double Dt, const FS47PastureIn& S);
	void Now() { if (Stage == TEXT("graze")) bForce = true; }
	bool TakeNinth();
	bool NinthGone() const { return Stage == TEXT("short") || Stage == TEXT("after"); }
	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47PastureDeps D;
	bool bNear = false, bForce = false;
	double Last() const;
	void Apply();
	int32 LitCows(const FS47Herd& H, const FS47PastureIn& S, const FVector& Cam) const;
};
