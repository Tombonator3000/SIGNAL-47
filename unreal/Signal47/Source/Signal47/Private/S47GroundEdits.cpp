#include "S47GroundEdits.h"
#include "S47Relief.h"
#include "Dom/JsonObject.h"
#include "HAL/CriticalSection.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeRWLock.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <atomic>

DEFINE_LOG_CATEGORY_STATIC(LogS47GroundEdits, Log, All);

namespace S47GroundEdits
{
namespace
{
	struct FSiteEdits { FString Id; TArray<FPad> Pads; TArray<FCut> Cuts; FBox2D Box = FBox2D(ForceInit); };
	FRWLock Lock;
	TArray<FSiteEdits> Sites;
	std::atomic<int32> Count{ 0 };
	TArray<TPair<FString, FRect>> BareRects;      // SetBare
	std::atomic<int32> BareCount{ 0 };
	bool bBakedRead = false;
	TArray<FSiteEdits> BakedSites;

	void Local(const FRect& R, double X, double Z, double& U, double& V)
	{
		const double A = FMath::DegreesToRadians(R.Yaw), C = FMath::Cos(A), S = FMath::Sin(A), Dx = X - R.X, Dz = Z - R.Z;
		U = Dx * C + Dz * S; V = -Dx * S + Dz * C;
	}
	FBox2D BoxOf(const FRect& R, double Grow)
	{
		const double A = FMath::DegreesToRadians(R.Yaw), C = FMath::Abs(FMath::Cos(A)), S = FMath::Abs(FMath::Sin(A));
		const double Ex = (R.HalfX + Grow) * C + (R.HalfZ + Grow) * S, Ez = (R.HalfX + Grow) * S + (R.HalfZ + Grow) * C;
		return FBox2D(FVector2D(R.X - Ex, R.Z - Ez), FVector2D(R.X + Ex, R.Z + Ez));
	}
	FString RectJson(const FRect& R) { return FString::Printf(TEXT("\"x\":%.4f,\"z\":%.4f,\"hx\":%.4f,\"hz\":%.4f,\"yaw\":%.4f"), R.X, R.Z, R.HalfX, R.HalfZ, R.Yaw); }
	bool Near(const FRect& A, const FRect& B) { return FMath::Abs(A.X - B.X) < .02 && FMath::Abs(A.Z - B.Z) < .02 && FMath::Abs(A.HalfX - B.HalfX) < .02 && FMath::Abs(A.HalfZ - B.HalfZ) < .02 && FMath::Abs(A.Yaw - B.Yaw) < .01; }
	FRect ReadRect(const FJsonObject& O)
	{
		FRect R; R.X = O.GetNumberField(TEXT("x")); R.Z = O.GetNumberField(TEXT("z")); R.HalfX = O.GetNumberField(TEXT("hx")); R.HalfZ = O.GetNumberField(TEXT("hz")); R.Yaw = O.GetNumberField(TEXT("yaw"));
		return R;
	}
	void ReadBaked()
	{
		if (bBakedRead) return;
		bBakedRead = true;
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("FreeformTerrain/site_ground.json")))) return;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()) return;
		const TSharedPtr<FJsonObject>* All = nullptr;
		if (!Root->TryGetObjectField(TEXT("sites"), All)) return;
		for (const auto& Pair : (*All)->Values)
		{
			const TSharedPtr<FJsonObject>* O = nullptr;
			if (!Pair.Value->TryGetObject(O)) continue;
			FSiteEdits E; E.Id = Pair.Key;
			const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
			if ((*O)->TryGetArrayField(TEXT("pads"), List)) for (const auto& V : *List) if (const TSharedPtr<FJsonObject> P = V->AsObject()) { FPad Pad; Pad.R = ReadRect(*P); Pad.Y = P->GetNumberField(TEXT("y")); Pad.Bank = P->GetNumberField(TEXT("bank")); E.Pads.Add(Pad); }
			if ((*O)->TryGetArrayField(TEXT("cuts"), List)) for (const auto& V : *List) if (const TSharedPtr<FJsonObject> P = V->AsObject()) { FCut Cut; Cut.R = ReadRect(*P); Cut.Y = P->GetNumberField(TEXT("y")); E.Cuts.Add(Cut); }
			BakedSites.Add(MoveTemp(E));
		}
	}
	double Smooth01(double T) { T = FMath::Clamp(T, 0.0, 1.0); return T * T * (3 - 2 * T); }
}

double Outside(const FRect& R, double X, double Z)
{
	double U, V;
	Local(R, X, Z, U, V);
	const double Du = FMath::Max(0.0, FMath::Abs(U) - R.HalfX), Dv = FMath::Max(0.0, FMath::Abs(V) - R.HalfZ);
	return FMath::Sqrt(Du * Du + Dv * Dv);
}

void Set(const FString& Id, const TArray<FPad>& Pads, const TArray<FCut>& Cuts)
{
	{
		FWriteScopeLock W(Lock);
		Sites.RemoveAll([&Id](const FSiteEdits& S) { return S.Id == Id; });
		if (Pads.Num() || Cuts.Num())
		{
			FSiteEdits E; E.Id = Id; E.Pads = Pads; E.Cuts = Cuts;
			for (const FPad& P : Pads) E.Box += BoxOf(P.R, P.Bank);
			for (const FCut& C : Cuts) E.Box += BoxOf(C.R, 0);
			Sites.Add(MoveTemp(E));
		}
		Count = Sites.Num();
	}
	UE_LOG(LogS47GroundEdits, Display, TEXT("S47: ground edits: %s gives %d pads and %d cuts (relief %s, baked %s)"), *Id, Pads.Num(), Cuts.Num(), S47Relief::Active() ? TEXT("on") : TEXT("off"), Baked(Id) ? TEXT("yes") : TEXT("no"));
	static const bool bDump = FParse::Param(FCommandLine::Get(), TEXT("S47GroundEditsDump"));
	if (bDump && S47Relief::Active())
	{
		const FString Path = FPaths::ProjectSavedDir() / TEXT("S47GroundEdits/site_ground.json");
		if (FFileHelper::SaveStringToFile(Json(), *Path)) UE_LOG(LogS47GroundEdits, Display, TEXT("S47: ground edits: written to %s"), *Path);
	}
}

void SetBare(const FString& Id, const TArray<FRect>& Bare)
{
	FWriteScopeLock W(Lock);
	BareRects.RemoveAll([&Id](const TPair<FString, FRect>& B) { return B.Key == Id; });
	for (const FRect& R : Bare) BareRects.Add({ Id, R });
	BareCount = BareRects.Num();
}

double Apply(double X, double Z, double Y)
{
	if (Count.load(std::memory_order_relaxed) == 0 || !S47Relief::Active()) return Y;
	FReadScopeLock R(Lock);
	for (const FSiteEdits& S : Sites)
	{
		if (X < S.Box.Min.X || X > S.Box.Max.X || Z < S.Box.Min.Y || Z > S.Box.Max.Y) continue;
		for (const FPad& P : S.Pads)
		{
			const double D = Outside(P.R, X, Z);
			if (D >= P.Bank) continue;
			const double W = P.Bank > 0 ? 1 - Smooth01(D / P.Bank) : 1;
			Y += (P.Y - Y) * W;
		}
		for (const FCut& C : S.Cuts) if (Outside(C.R, X, Z) <= 0) Y = C.Y;
	}
	return Y;
}

bool Clears(double X, double Z, double Margin)
{
	if ((Count.load(std::memory_order_relaxed) == 0 && BareCount.load(std::memory_order_relaxed) == 0) || !S47Relief::Active()) return false;
	FReadScopeLock R(Lock);
	for (const TPair<FString, FRect>& B : BareRects) if (Outside(B.Value, X, Z) <= Margin) return true;
	for (const FSiteEdits& S : Sites)
	{
		if (X < S.Box.Min.X - Margin || X > S.Box.Max.X + Margin || Z < S.Box.Min.Y - Margin || Z > S.Box.Max.Y + Margin) continue;
		for (const FPad& P : S.Pads) if (Outside(P.R, X, Z) < P.Bank + Margin) return true;
		for (const FCut& C : S.Cuts) if (Outside(C.R, X, Z) <= Margin) return true;
	}
	return false;
}

TArray<FBox2D> ClearBoxes()
{
	TArray<FBox2D> Out;
	FReadScopeLock R(Lock);
	for (const FSiteEdits& S : Sites)
	{
		for (const FPad& P : S.Pads) Out.Add(BoxOf(P.R, P.Bank));
		for (const FCut& C : S.Cuts) Out.Add(BoxOf(C.R, 0));
	}
	return Out;
}

bool Baked(const FString& Id)
{
	if (!S47Relief::Active()) return false;
	FReadScopeLock R(Lock);
	ReadBaked();
	const FSiteEdits* Mine = Sites.FindByPredicate([&Id](const FSiteEdits& S) { return S.Id == Id; });
	const FSiteEdits* Theirs = BakedSites.FindByPredicate([&Id](const FSiteEdits& S) { return S.Id == Id; });
	if (!Mine || !Theirs || Mine->Pads.Num() != Theirs->Pads.Num() || Mine->Cuts.Num() != Theirs->Cuts.Num()) return false;
	for (int32 I = 0; I < Mine->Pads.Num(); ++I) if (!Near(Mine->Pads[I].R, Theirs->Pads[I].R) || FMath::Abs(Mine->Pads[I].Y - Theirs->Pads[I].Y) > .02 || FMath::Abs(Mine->Pads[I].Bank - Theirs->Pads[I].Bank) > .02) return false;
	for (int32 I = 0; I < Mine->Cuts.Num(); ++I) if (!Near(Mine->Cuts[I].R, Theirs->Cuts[I].R)) return false;
	return true;
}

FString Json()
{
	FReadScopeLock R(Lock);
	FString Out = TEXT("{\n \"_doc\": \"CALICHE-GROUND: pads and cuts the sites registered (S47GroundEdits), road-local metres; written by the game with -S47GroundEditsDump, cut into the ground mesh by Tools/freeformterrain/site_ground.py\",\n \"sites\": {");
	for (int32 I = 0; I < Sites.Num(); ++I)
	{
		const FSiteEdits& S = Sites[I];
		Out += FString::Printf(TEXT("%s\n  \"%s\": {\n   \"pads\": ["), I ? TEXT(",") : TEXT(""), *S.Id);
		for (int32 K = 0; K < S.Pads.Num(); ++K) Out += FString::Printf(TEXT("%s\n    {%s,\"y\":%.4f,\"bank\":%.3f}"), K ? TEXT(",") : TEXT(""), *RectJson(S.Pads[K].R), S.Pads[K].Y, S.Pads[K].Bank);
		Out += TEXT("\n   ],\n   \"cuts\": [");
		for (int32 K = 0; K < S.Cuts.Num(); ++K) Out += FString::Printf(TEXT("%s\n    {%s,\"y\":%.4f}"), K ? TEXT(",") : TEXT(""), *RectJson(S.Cuts[K].R), S.Cuts[K].Y);
		Out += TEXT("\n   ]\n  }");
	}
	Out += TEXT("\n }\n}\n");
	return Out;
}

bool SelfTest(TArray<FString>& Lines)
{
	bool bAll = true;
	const auto Check = [&](bool bOk, const FString& What) { Lines.Add(FString::Printf(TEXT("%s: ground edits: %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *What)); bAll &= bOk; };
	FRect R; R.X = 10; R.Z = 20; R.HalfX = 4; R.HalfZ = 2;
	Check(Outside(R, 10, 20) == 0 && Outside(R, 14, 22) == 0 && FMath::IsNearlyEqual(Outside(R, 17, 20), 3.0) && FMath::IsNearlyEqual(Outside(R, 17, 26), 5.0), TEXT("a point's distance to a rectangle is 0 inside, straight out from a side, round at a corner"));
	FRect T = R; T.Yaw = 90;
	Check(Outside(T, 12, 24) == 0 && Outside(T, 14, 20) > 1.9, TEXT("a rectangle turned a quarter has its long side the other way"));
	{
		FRect B; B.X = -900; B.Z = -900; B.HalfX = 3; B.HalfZ = 2;
		SetBare(TEXT("selftest"), { B });
		const bool bOn = S47Relief::Active();
		const bool bIn = Clears(-900, -900), bOut = Clears(-890, -900), bLand = Apply(-900, -900, 7.5) == 7.5;
		SetBare(TEXT("selftest"), {});
		Check((bIn == bOn) && !bOut && bLand && !Clears(-900, -900), TEXT("bare ground keeps plants off its rectangle (with the relief on), leaves the land's height alone and goes when taken away"));
	}
	// (the list as it stands is whatever the sites gave: measured, not changed)
	const bool bWas = S47Relief::Active();
	int32 N = 0; double Worst = 0;
	{
		FReadScopeLock L(Lock);
		N = Sites.Num();
	}
	if (bWas)
		for (const FBox2D& B : ClearBoxes()) { const FVector2D C = B.GetCenter(); if (!Clears(C.X, C.Y)) Worst = 1; }
	Check(Worst == 0, FString::Printf(TEXT("%d sites have edits on the land; the middle of each pad and cut is kept clear of plants (relief %s)"), N, bWas ? TEXT("on") : TEXT("off")));
	return bAll;
}
}
