#include "S47Enlarger.h"
#include "S47Album.h"
#include "S47FieldCamera.h"
#include "S47Freeform.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47PinBoard.h"
#include "S47UI.h"
#include "S47Web.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Enlarger, Log, All);

namespace S47EnlargerLocal
{
	constexpr int32 PrintW = 640, PrintH = 400; // the enlargement, the size of a print
	constexpr int32 PaperCost = 1;
	const TCHAR* const PartNames[9] = { TEXT("the upper left"), TEXT("the top"), TEXT("the upper right"), TEXT("the left"), TEXT("the middle"), TEXT("the right"), TEXT("the lower left"), TEXT("the bottom"), TEXT("the lower right") };
}

// ---------- the pure rules ----------

FVector2D FS47Enlarger::PartCentre(int32 Part)
{
	const int32 Pt = FMath::Clamp(Part, 0, 8);
	return FVector2D((Pt % 3 + .5) / 3.0, (Pt / 3 + .5) / 3.0);
}

FString FS47Enlarger::PartWords(int32 Part)
{
	return S47EnlargerLocal::PartNames[FMath::Clamp(Part, 0, 8)];
}

void FS47Enlarger::Crop(const TArray<FColor>& Src, int32 SW, int32 SH, double U, double V, double Frac, int32 OW, int32 OH, TArray<FColor>& Out)
{
	Out.Reset();
	if (SW <= 0 || SH <= 0 || Src.Num() != SW * SH || OW <= 0 || OH <= 0) return;
	const double Fr = FMath::Clamp(Frac, .05, 1.0);
	const double Cu = FMath::Clamp(U, Fr * .5, 1 - Fr * .5), Cv = FMath::Clamp(V, Fr * .5, 1 - Fr * .5);
	const double Xs = (Cu - Fr * .5) * SW, Ys = (Cv - Fr * .5) * SH, Span = Fr * SW, SpanY = Fr * SH;
	Out.SetNumUninitialized(OW * OH);
	for (int32 Y = 0; Y < OH; ++Y) for (int32 X = 0; X < OW; ++X)
	{
		// The middle of each output pixel, in the source, read between its four nearest (bilinear).
		const double Sx = FMath::Clamp(Xs + (X + .5) * Span / OW - .5, 0.0, SW - 1.0), Sy = FMath::Clamp(Ys + (Y + .5) * SpanY / OH - .5, 0.0, SH - 1.0);
		const int32 Ix = FMath::Min(int32(Sx), SW - 2 < 0 ? 0 : SW - 2), Iy = FMath::Min(int32(Sy), SH - 2 < 0 ? 0 : SH - 2);
		const double Tx = FMath::Clamp(Sx - Ix, 0.0, 1.0), Ty = FMath::Clamp(Sy - Iy, 0.0, 1.0);
		const FColor& A = Src[Iy * SW + Ix];
		const FColor& B = Src[Iy * SW + FMath::Min(Ix + 1, SW - 1)];
		const FColor& Pc = Src[FMath::Min(Iy + 1, SH - 1) * SW + Ix];
		const FColor& D = Src[FMath::Min(Iy + 1, SH - 1) * SW + FMath::Min(Ix + 1, SW - 1)];
		const auto Mix = [Tx, Ty](uint8 Qa, uint8 Qb, uint8 Qc, uint8 Qd) { return uint8(FMath::Clamp(FMath::RoundToInt(FMath::Lerp(FMath::Lerp(double(Qa), double(Qb), Tx), FMath::Lerp(double(Qc), double(Qd), Tx), Ty)), 0, 255)); };
		Out[Y * OW + X] = FColor(Mix(A.R, B.R, Pc.R, D.R), Mix(A.G, B.G, Pc.G, D.G), Mix(A.B, B.B, Pc.B, D.B), 255);
	}
}

// ---------- the darkroom ----------

TArray<const FS47AlbumShot*> FS47Enlarger::Prints() const
{
	TArray<const FS47AlbumShot*> Out;
	if (!M || !M->Night || !M->Night->Album) return Out;
	for (const FS47AlbumShot& Sh : M->Night->Album->Shots())
		if (!Sh.bUnavailable && !Sh.Url.IsEmpty() && Sh.IsDeveloped()) Out.Add(&Sh);
	return Out;
}

bool FS47Enlarger::Make(const FS47AlbumShot& Sh, int32 Part, int32 Power, TArray<FColor>& Out, int32& OutW, int32& OutH, bool& bFromNeg) const
{
	using namespace S47EnlargerLocal;
	TArray<FColor> Src;
	int32 SW = 0, SH = 0;
	bFromNeg = !Sh.Neg.IsEmpty() && S47Image::DecodePixels(Sh.Neg, Src, SW, SH) && SW > 0 && SH > 0;
	if (!bFromNeg && !S47Image::DecodePixels(Sh.Url, Src, SW, SH)) return false;
	const FVector2D At = PartCentre(Part);
	OutW = OutW <= 0 ? PrintW : OutW; OutH = OutH <= 0 ? PrintH : OutH;
	Crop(Src, SW, SH, At.X, At.Y, 1.0 / FMath::Max(1, Power), OutW, OutH, Out);
	if (Out.IsEmpty()) return false;
	const double Seed = FMath::FloorToDouble(Sh.Clock * 1000) + Part * 131 + Power * 7;
	if (bFromNeg) US47FieldCamera::FilmLook(Out, OutW, OutH, Seed); // the paper's own grain, as any print
	else
	{
		// From the print: its grain is already in it, larger now; a little more from the second paper.
		uint32 Rng = US47FieldCamera::ToUint32(Seed);
		for (FColor& Px : Out)
		{
			Rng = Rng * 1664525u + 1013904223u;
			const int32 Nz = int32((Rng >> 24) % 15) - 7;
			Px.R = uint8(FMath::Clamp(Px.R + Nz, 0, 255)); Px.G = uint8(FMath::Clamp(Px.G + Nz, 0, 255)); Px.B = uint8(FMath::Clamp(Px.B + Nz, 0, 255));
		}
	}
	return true;
}

FString FS47Enlarger::Caption(const FS47AlbumShot& Sh, int32 Part, int32 Power) const
{
	const FString Where = Sh.Place.IsEmpty() ? FString() : FString::Printf(TEXT(", %s"), *Sh.Place);
	return FString::Printf(TEXT("Enlarged %dx, %s of the frame. %s%s."), Power, *PartWords(Part), *FS47PinBoard::StampOf(Sh.Clock), *Where);
}

bool FS47Enlarger::DarkroomReady(bool bSay) const
{
	if (!M || !M->Freeform) return false;
	const TCHAR* Why = !M->Freeform->HasPowerForReactiveLight() ? TEXT("PHOTO LAB: No power for the enlarger's lamp.")
		: M->Web && M->Web->IsDoorOpen(TEXT("labDoor")) ? TEXT("PHOTO LAB: Shut the darkroom door before you print.") : nullptr;
	if (Why && bSay && M->UI) M->UI->Toast(Why, 3.5f);
	return !Why;
}

FString FS47Enlarger::Print(const FS47AlbumShot& Sh, int32 Part, int32 Power)
{
	if (!M || !M->Night || !M->Night->Album || !M->Freeform || !DarkroomReady(true)) return FString();
	TArray<FColor> Px; int32 Wd = 0, Hgt = 0; bool bNeg = false;
	if (!Make(Sh, Part, Power, Px, Wd, Hgt, bNeg)) return FString();
	if (!M->Freeform->Spend(S47EnlargerLocal::PaperCost)) { if (M->UI) M->UI->Toast(TEXT("PHOTO LAB: No money left for paper."), 3.5f); return FString(); }
	const FS47AlbumShot From = Sh; // the album grows: keep a copy
	const FString Id = M->Night->Album->AddEnlargement(From, Px, Wd, Hgt, Caption(From, Part, Power));
	UE_LOG(LogS47Enlarger, Display, TEXT("S47: enlarger: %s, part %d at %dx from the %s, printed as %s"), *From.Id, Part, Power, bNeg ? TEXT("negative") : TEXT("print"), *Id);
	if (!Id.IsEmpty())
	{
		if (M->UI) M->UI->Toast(TEXT("PHOTO LAB: The enlargement is in your album. It goes up on the PINBOARD with the first print."), 4.5f);
		M->RequestAutosave();
	}
	return Id;
}

void FS47Enlarger::Open(const FString& ShotId)
{
	using namespace S47EnlargerLocal;
	if (!M || !M->UI) return;
	const TArray<const FS47AlbumShot*> All = Prints();
	if (!ShotId.IsEmpty()) Shot = ShotId;
	const FS47AlbumShot* const* Found = All.FindByPredicate([this](const FS47AlbumShot* Sh) { return Sh->Id == Shot; });
	const FS47AlbumShot* On = Found ? *Found : All.Num() ? All.Last() : nullptr;
	TArray<S47Kit::FW> Panel;
	Panel.Add(S47Kit::Section(TEXT("SARO / PHOTO LAB"), TEXT("THE ENLARGER")));
	if (M->Night && M->Night->Album && !M->Night->Album->PendingLabPhoto()) Panel.Add(S47Kit::Small(TEXT("No exposed film is waiting at the wet bench.")));
	if (!On)
	{
		Panel.Add(S47Kit::Text(TEXT("No developed prints yet. Develop a roll at the wet bench first; the enlarger takes a print and its negative.")));
		M->UI->Open(S47Kit::Shell(M->UI, TEXT("ENLARGER"), S47Kit::Col(Panel), 900));
		return;
	}
	Shot = On->Id;
	const TWeakPtr<uint8> Guard = Life;
	// The prints: the newest six.
	{
		TArray<S47Kit::FW> Choice;
		for (int32 I = FMath::Max(0, All.Num() - 6); I < All.Num(); ++I)
		{
			const FS47AlbumShot& Sh = *All[I];
			const FString Label = FString::Printf(TEXT("%s%s"), *FS47PinBoard::StampOf(Sh.Clock), Sh.Enlarged.IsEmpty() ? TEXT("") : TEXT(" (enl.)"));
			Choice.Add(S47Kit::Button(Label, [this, Guard, Id = Sh.Id] { if (Guard.IsValid()) Open(Id); }, Sh.Id == Shot));
		}
		Panel.Add(S47Kit::Row(Choice));
	}
	TArray<FColor> Px; int32 Wd = 0, Hgt = 0; bool bNeg = false;
	const bool bMade = Make(*On, Cell, Mag, Px, Wd, Hgt, bNeg);
	// LeftCol: the print and its nine parts; right: what the enlarger throws on the easel.
	TArray<S47Kit::FW> LeftCol;
	LeftCol.Add(SNew(SBox).HAlign(HAlign_Center)[S47Kit::Image(S47Image::FromDataUrl(On->Url), 330.f)]);
	for (int32 Row = 0; Row < 3; ++Row)
	{
		TArray<S47Kit::FW> PartRow;
		for (int32 Col = 0; Col < 3; ++Col)
		{
			const int32 Part = Row * 3 + Col;
			static const TCHAR* const Short[9] = { TEXT("UPPER LEFT"), TEXT("TOP"), TEXT("UPPER RIGHT"), TEXT("LEFT"), TEXT("MIDDLE"), TEXT("RIGHT"), TEXT("LOWER LEFT"), TEXT("BOTTOM"), TEXT("LOWER RIGHT") };
			PartRow.Add(S47Kit::Button(Short[Part], [this, Guard, Part] { if (Guard.IsValid()) { Cell = Part; Open(); } }, Part == Cell));
		}
		LeftCol.Add(S47Kit::Row(PartRow));
	}
	LeftCol.Add(S47Kit::Row({
		S47Kit::Button(TEXT("2x"), [this, Guard] { if (Guard.IsValid()) { Mag = 2; Open(); } }, Mag == 2),
		S47Kit::Button(TEXT("4x"), [this, Guard] { if (Guard.IsValid()) { Mag = 4; Open(); } }, Mag == 4) }));
	TArray<S47Kit::FW> RightCol;
	if (bMade) RightCol.Add(SNew(SBox).HAlign(HAlign_Center)[S47Kit::Image(S47Image::FromPixels(Px, Wd, Hgt), 520.f)]);
	RightCol.Add(S47Kit::Text(Caption(*On, Cell, Mag)));
	if (!bNeg) RightCol.Add(S47Kit::Small(TEXT("Its negative was not kept: this is the print, larger.")));
	RightCol.Add(S47Kit::Action(FString::Printf(TEXT("MAKE A PRINT OF THIS (PAPER, $%d)"), PaperCost), [this, Guard]
	{
		if (!Guard.IsValid() || !M || !M->Night || !M->Night->Album) return;
		const FS47AlbumShot* Sh = M->Night->Album->Shots().FindByPredicate([this](const FS47AlbumShot& X) { return X.Id == Shot; });
		if (Sh && !Print(*Sh, Cell, Mag).IsEmpty()) Open();
	}));
	Panel.Add(S47Kit::Two(S47Kit::Col(LeftCol), S47Kit::Col(RightCol), 1.f, 1.5f));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("ENLARGER"), S47Kit::Col(Panel), 1180));
}

FString FS47Enlarger::DevMake(const FString& ShotId, int32 Part, int32 Power)
{
	if (!M || !M->Night || !M->Night->Album) return FString();
	const FS47AlbumShot* Sh = M->Night->Album->Shots().FindByPredicate([&ShotId](const FS47AlbumShot& X) { return X.Id == ShotId; });
	return Sh ? Print(*Sh, Part, Power) : FString();
}

// ---------- tests ----------

bool FS47Enlarger::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	const auto Check = [&Lines, &bOk](bool bPass, const TCHAR* What)
	{
		Lines.Add(FString::Printf(TEXT("S47: %s enlarger: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), What));
		bOk &= bPass;
	};
	Check(PartCentre(0).Equals(FVector2D(1 / 6.0, 1 / 6.0), 1e-9) && PartCentre(4).Equals(FVector2D(.5, .5), 1e-9) && PartCentre(8).Equals(FVector2D(5 / 6.0, 5 / 6.0), 1e-9)
		&& PartWords(2) == TEXT("the upper right"), TEXT("nine parts of the frame, by their middles and their words"));
	// A dark 64 by 40 negative with one bright speck two pixels across, in the upper right: the print (its average, 32 by 20) keeps
	// a quarter of it; the enlargement of the upper right at 2x from the negative shows it as bright as it was, eight pixels across.
	const int32 NW = 64, NH = 40;
	TArray<FColor> Neg; Neg.Init(FColor(10, 10, 12, 255), NW * NH);
	for (int32 Y = 6; Y < 8; ++Y) for (int32 X = 53; X < 55; ++X) Neg[Y * NW + X] = FColor(240, 240, 240, 255);
	TArray<FColor> Print; Print.SetNumUninitialized(32 * 20);
	for (int32 Y = 0; Y < 20; ++Y) for (int32 X = 0; X < 32; ++X)
	{
		int32 Sum = 0;
		for (int32 Dy = 0; Dy < 2; ++Dy) for (int32 Dx = 0; Dx < 2; ++Dx) Sum += Neg[(Y * 2 + Dy) * NW + X * 2 + Dx].R;
		Print[Y * 32 + X] = FColor(uint8(Sum / 4), uint8(Sum / 4), uint8(Sum / 4), 255);
	}
	uint8 PrintPeak = 0; for (const FColor& Px : Print) PrintPeak = FMath::Max(PrintPeak, Px.R);
	TArray<FColor> Big;
	const FVector2D At = PartCentre(2);
	Crop(Neg, NW, NH, At.X, At.Y, .5, 128, 80, Big);
	uint8 BigPeak = 0; int32 Lit = 0;
	for (const FColor& Px : Big) { BigPeak = FMath::Max(BigPeak, Px.R); Lit += Px.R > 120 ? 1 : 0; }
	Check(PrintPeak < 140 && BigPeak > 200 && Lit >= 12, TEXT("what the print is too small to show, the negative shows enlarged"));
	TArray<FColor> Rim;
	Crop(Neg, NW, NH, 0, 0, .25, 16, 10, Rim);
	Check(Rim.Num() == 160, TEXT("a part at the frame's edge stays inside the frame"));
	return bOk;
}
