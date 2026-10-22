#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47Seen.h"

class US47Main;
struct FS47CutoutsIn
{
	bool bOk = false;
	FS47Cam Cam;
	double Clock = 0;
	struct FFoot { FVector Pos = FVector::ZeroVector; TFunction<bool(double, double)> Walkable; TFunction<double(double, double)> Floor; TFunction<bool(double, double)> Blocked; };
	TOptional<FFoot> Foot;
	struct FDrive { FVector Pos = FVector::ZeroVector, Dir = FVector::ZeroVector; TFunction<double(double, double)> Ground; };
	TOptional<FDrive> Drive;
	TOptional<bool> May, Keep;
	double Pace = 1;
	TFunction<void()> Blink;
	int32 Flood = -1;
};

// core/cutouts.ts: the souvenir shop's cardboard alien, anywhere, and the shape over a post.
class SIGNAL47_API FS47Cutouts
{
public:
	FS47Cutouts(US47Main* InM, TFunction<void(bool)> InNote, TFunction<void(bool)> InPostNote = {});
	FString Up; // empty is TypeScript's null
	int32 Seen = 0, Placed = 0;
	struct FPost { double Dist = 0; bool bLooked = false; };
	TOptional<FPost> Post;
	void Update(double Dt, const FS47CutoutsIn& S);
	bool Place(const FS47CutoutsIn& S);
	bool PlacePost(const FVector& InPost, const FVector& From);
	FVector Position() const { return FigPos; }
	TOptional<FVector> Where() const { return Up.IsEmpty() ? TOptional<FVector>() : FigPos + FVector(0, 0.8, 0); }
	void Reset();
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	TFunction<void(bool)> Note, PostNote;
	double Reveal = -1;
	FS47Obj Fig, Card;
	FVector FigPos = FVector::ZeroVector;
	double Wait = 0, Looked = 0;
	bool bWasSeen = false;
	double Away = 0, Age = 0;
	bool bAsked = false;
	int32 Lit = -2;
	struct FView { double Dist = 0; bool bInView = false; double Side = 0; };
	static FView View(const FS47Cam& Cam, const FVector& At);
	void PostStep(double Dt, const FS47CutoutsIn& S);
	void Paint();
	void Hide(double InWait);
};
