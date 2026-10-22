#pragma once

#include "CoreMinimal.h"
#include "S47Radio.h"

// story/tripRadio.ts TripNow: a trip as the programme sees it (World.tripNow).
struct FS47TripNow
{
	int32 N = 0;
	FString From, To, Bound;
	double Metres = 0;
};
struct FS47TripRadioIn
{
	TOptional<FS47TripNow> Trip;
	FString Phase;
	double Clock = 0;
	FString Tuned; // empty: off, not driving, or a cinematic
};
struct FS47TripRadioLast
{
	FString Id;
	double Metres = 0, Clock = 0;
	FString Trip;
};
struct FS47TripRadioPlan
{
	FString Trip;
	double At = 0, Reach = 450;
	TArray<FString> Ch;
	FString On;
	TArray<FString> Ids, First;
	TArray<TOptional<FString>> By;
};

// story/tripRadio.ts: each way's radio lines at marks in metres driven, once per night.
class SIGNAL47_API FS47TripRadio
{
public:
	FS47TripRadio(FS47Radio& InRadio, struct FS47TownSeen& InSeen);
	TOptional<FS47TripRadioLast> Last;
	TArray<FS47TripRadioPlan> Plan;
	int32 Lines = 0;
	void Update(const FS47TripRadioIn& S);
	static int32 ProgrammeSize();
	static bool SelfTest(TArray<FString>& Out);

private:
	FS47Radio& Radio;
	struct FS47TownSeen& Seen; // main.ts townSeen (its add logs to the seen log)
	struct FTaken
	{
		TSharedPtr<FS47Station> St;
		TSharedPtr<FS47RadioSeg> X;
		FS47TripRadioLast On;
	};
	TOptional<FTaken> Taken;
	// A Mark's identity in the static PROGRAMME is its array index, as Set<Mark> in TypeScript.
	struct FTrip { int32 N; TSet<int32> Done; };
	TOptional<FTrip> Trip;
};
