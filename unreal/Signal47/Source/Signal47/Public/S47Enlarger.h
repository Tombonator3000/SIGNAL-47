#pragma once
#include "CoreMinimal.h"
class US47Main;
struct FS47AlbumShot;

// ENLARGE-20261108 (Tom, 8 November: "bilder med detaljer som først synes ved forstørring i mørkerommet"). The photo lab's enlarger, by
// the wet bench. A developed print, a part of it (a third of the frame each way, nine parts), twice or four times as large. Freeform
// exposures keep their negative at twice the print's size (FS47Album, US47FieldCamera::Snap's Super), as it was rendered: what was
// out there and too small for the print is on the negative, a few grains of it, and shows here; nothing is added and nothing is
// named. At four times the grain shows too. A print of the enlargement goes in the album (paper, $1), with the first print's time,
// place and pose, so it goes up on the case board beside it. Freeform only; it keeps nothing of its own.
class FS47Enlarger
{
public:
	explicit FS47Enlarger(US47Main* InMain) : M(InMain), Life(MakeShared<uint8>(0)) {}
	// The panel, on a print (empty: the newest developed one).
	void Open(const FString& ShotId = FString());
	// F10: the enlargement of this print's part at this power, printed into the album; its id or empty.
	FString DevMake(const FString& ShotId, int32 Part, int32 Power);
	void DevSet(int32 Part, int32 Power) { Cell = FMath::Clamp(Part, 0, 8); Mag = Power >= 4 ? 4 : 2; }

	// ---------- the pure rules (tested) ----------
	// The middle of each of the nine parts, as a share of the frame (u right, v down), and its words: "the upper right".
	static FVector2D PartCentre(int32 Part);
	static FString PartWords(int32 Part);
	// A window Frac of Src's width and height, centred on U, V (kept inside the frame), resampled to OW by OH.
	static void Crop(const TArray<FColor>& Src, int32 SW, int32 SH, double U, double V, double Frac, int32 OW, int32 OH, TArray<FColor>& Out);
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TSharedPtr<uint8> Life;
	FString Shot;
	int32 Cell = 4, Mag = 2;
	TArray<const FS47AlbumShot*> Prints() const;
	// The enlargement: from the negative where it is kept, else from the print. Seed: its grain.
	bool Make(const FS47AlbumShot& S, int32 Part, int32 Power, TArray<FColor>& Out, int32& OutW, int32& OutH, bool& bFromNeg) const;
	FString Caption(const FS47AlbumShot& S, int32 Part, int32 Power) const;
	FString Print(const FS47AlbumShot& S, int32 Part, int32 Power);
	bool DarkroomReady(bool bSay) const;
};
