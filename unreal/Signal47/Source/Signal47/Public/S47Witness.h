#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
struct FS47AlbumShot;

// N1110 PHOTO-03: what townspeople and callers say they saw is kept in the notebook as a record (who, when, where), apart from
// the meeting itself. It can be wrong or out of date and it pays nothing alone. If the player photographs the same place, the same
// night or the next, the pair counts as corroborated and the station pays a small bonus once, as field allowance (EarnSide).
// Freeform only. Nothing here says what was seen: the words are the speaker's own.
class FS47Witness
{
public:
	struct FRec
	{
		FString Id, Who, PlaceId, Text;
		double Clock = 0;
		bool bPaid = false;
	};
	static constexpr int32 MaxKept = 24, MaxText = 220;
	/** A small bonus, inside the week's field allowance. */
	static constexpr int32 Bonus = 8; // N1110 (pace): was 3, too small to notice
	/** The photograph is of the place if it was taken within this many metres, facing it unless it was taken close up. */
	static constexpr double Reach = 160, CloseUp = 30, FacingDegrees = 60;
	/** Same night or the next: a photograph this many seconds after the statement, or less. */
	static constexpr double Window = 36 * 3600.0;

	/** False when it is empty, already kept (by id) or has no place on the map. The oldest paid record makes room when full. */
	bool Add(const FRec& In);
	const TArray<FRec>& Records() const { return Kept; }
	const FRec* Find(const FString& Id) const { return Kept.FindByPredicate([&Id](const FRec& R) { return R.Id == Id; }); }
	/** The first unpaid record this photograph (XZ of the eye on the road's plan, facing, clock) corroborates, or -1. */
	int32 Match(double EyeX, double EyeZ, double ForwardX, double ForwardZ, double ShotClock) const;
	/** Marks a record paid; false if there is none or it was paid before. */
	bool MarkPaid(int32 Index);
	/** Whether this photograph is of the record's place (the geometry only). */
	static bool IsOfPlace(const FRec& R, double EyeX, double EyeZ, double ForwardX, double ForwardZ, double ShotClock);

	S47J::FObj Save() const;
	void Load(const S47J::FObj& Saved);
	void Reset() { Kept.Reset(); }

	/** OPS pays for a registered photograph: finds the record it corroborates, pays Bonus once, files the receipt. What was paid; -1 if none. */
	static int32 PayPhoto(US47Main* Main, const FS47AlbumShot& Shot);
	/** The notebook's line: who said what. */
	static FString NoteText(const FRec& R);

	static bool SelfTest(TArray<FString>& Lines);

private:
	TArray<FRec> Kept;
};
