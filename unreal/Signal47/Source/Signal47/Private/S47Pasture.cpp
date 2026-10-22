// story/pasture.ts
#include "S47Pasture.h"

#include "S47Docs.h"
#include "S47Herd.h"

#include <cmath>
#include <limits>

namespace
{
	constexpr double GAP = 10 * 60, NEAR = 150, FAR = 260;
}

double FS47Pasture::Last() const
{
	double T = -1e9;
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(TEXT("mark:")))
	{
		const int32 At = K.Find(TEXT("@"));
		T = FMath::Max(T, FCString::Atod(*K.Mid(At + 1)));
	}
	return T;
}

void FS47Pasture::Reset()
{
	const FS47TownSeen* S = D.Seen;
	Stage = S && S->Contains(TEXT("ev:cows")) ? TEXT("after") : S && S->Contains(TEXT("cows:short")) ? TEXT("short")
		: S && S->Contains(TEXT("cows:on")) ? TEXT("still") : TEXT("graze");
	Passes = S && S->Contains(TEXT("cows:by")) ? 1 : 0; bNear = false; bForce = false;
	Apply();
}

void FS47Pasture::Apply()
{
	const bool bGone = NinthGone();
	if (D.Herds) for (FS47Herd* H : D.Herds()) if (H) H->Set(Stage == TEXT("still") || Stage == TEXT("short"), bGone ? S47Herd::MISSING : -1, bGone);
}

void FS47Pasture::Update(double Dt, const FS47PastureIn& S)
{
	(void)Dt;
	Apply();
	FS47Herd* H = nullptr;
	if (!S.Area.IsEmpty() && D.Herds) for (FS47Herd* X : D.Herds()) if (X && X->Area == S.Area) { H = X; break; }
	const FVector Cam = S.Cam.Pos;
	const double Dist = H ? FVector::Dist(Cam, H->Middle()) : std::numeric_limits<double>::infinity();
	const bool bFar = Dist > FAR;
	bAway = bFar;
	const double Day = std::fmod(std::fmod(S.Clock, 86400.0) + 86400, 86400.0);
	if (!D.Seen) return;
	FS47TownSeen& Seen = *D.Seen;
	if (Dist < NEAR) bNear = true;
	else if (bFar && bNear) { bNear = false; if (Stage == TEXT("graze")) { Passes++; Seen.Add(TEXT("cows:by")); } }
	if (bFar)
	{
		if (Stage == TEXT("graze") && (bForce || (S.bOk && (Passes > 0 || Day >= 3 * 3600 + 45 * 60) && Day >= 2 * 3600 + 40 * 60 && Day < 5 * 3600
			&& S.Clock - Last() >= GAP)))
		{
			Stage = TEXT("still"); bForce = false; Seen.Add(TEXT("cows:on")); if (D.Save) D.Save();
		}
		else if (Stage == TEXT("still") && Seen.Contains(TEXT("ev:cows1")))
		{
			Stage = TEXT("short"); Seen.Add(TEXT("cows:short")); if (D.Save) D.Save();
		}
		else if (Stage == TEXT("short") && Seen.Contains(TEXT("ev:cows"))) Stage = TEXT("after");
		Apply();
	}
	if (!H || !S.bOk || !S.bDriving || Dist > 120 || (Stage != TEXT("still") && Stage != TEXT("short"))) return;
	const int32 Lit = LitCows(*H, S, Cam);
	const FString T = S47Text::ClockText(S.Clock, false);
	if (Stage == TEXT("still") && !Seen.Contains(TEXT("ev:cows1")) && Lit >= 3)
	{
		Seen.Add(TEXT("ev:cows1")); Seen.Add(FString::Printf(TEXT("mark:cows@%.0f"), std::floor(S.Clock + 0.5)));
		if (D.Note) D.Note(T + TEXT(". The pasture south of the motel: the whole herd standing facing the same way, north-north-east, on 026. Nine head. Not one of them moved, with the headlights right on them."));
		if (D.Save) D.Save();
	}
	else if (Stage == TEXT("short") && !Seen.Contains(TEXT("ev:cows")) && Lit >= 3)
	{
		Seen.Add(TEXT("ev:cows"));
		bool bMark = false;
		for (const FString& K : Seen.Keys) if (K.StartsWith(TEXT("mark:cows@"))) { bMark = true; break; }
		if (!bMark) Seen.Add(FString::Printf(TEXT("mark:cows@%.0f"), std::floor(S.Clock + 0.5)));
		const bool bFound = D.NinthFound && D.NinthFound();
		const FString Text = bFound ? TEXT("The herd in the pasture, all facing the same way. Eight of them. Where the ninth stood, the grass is flattened in a ring, white with frost. She is lying inside the fence at STATION 01.")
			: FString::Printf(TEXT("The herd in the pasture, %s. Eight of them. There were nine. Where the ninth stood, the grass is flattened in a ring, white with frost."), Seen.Contains(TEXT("ev:cows1")) ? TEXT("still facing the same way") : TEXT("all facing the same way"));
		if (D.Toast) D.Toast(Text, 6.5);
		if (D.Note) D.Note(T + TEXT(". Pasture by the highway: the herd facing 026. One missing. A ring of frost in the grass.") + (bFound ? TEXT(" The one missing is lying at STATION 01, on B.") : TEXT("")));
		if (D.Save) D.Save();
	}
}

int32 FS47Pasture::LitCows(const FS47Herd& H, const FS47PastureIn& S, const FVector& Cam) const
{
	int32 N = 0;
	for (int32 I = 0; I < H.Count(); ++I)
	{
		FVector P;
		if (!H.CowAt(I, P)) continue;
		const double Dist = FVector::Dist(P, Cam), Toward = FVector::DotProduct((P - Cam).GetSafeNormal(), S.Cam.Forward);
		if (Dist < 85 && Toward > 0.8 && H.LitAt(I) > 0.2) N++;
	}
	return N;
}

bool FS47Pasture::TakeNinth()
{
	if (NinthGone()) return true;
	if (!bAway) return false;
	if (D.Seen) { D.Seen->Add(TEXT("cows:on")); D.Seen->Add(TEXT("cows:short")); }
	Stage = TEXT("short"); bForce = false;
	Apply(); if (D.Save) D.Save();
	return true;
}

bool FS47Pasture::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool b, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s pasture: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= b; };
	FS47TownSeen Seen;
	FS47Herd H(nullptr, FString(), TEXT("road"));
	int32 Saved = 0;
	FS47PastureDeps D; D.Seen = &Seen; D.Herds = [&] { return TArray<FS47Herd*>{ &H }; }; D.Save = [&] { Saved++; };
	FS47Pasture P(MoveTemp(D));
	FS47PastureIn S; S.bOk = true; S.Area = TEXT("road"); S.Clock = 3 * 3600;
	S.Cam.Pos = H.Middle(); P.Update(1, S);
	Check(!P.TakeNinth() && P.Stage == TEXT("graze"), TEXT("nothing vanishes while near"));
	S.Cam.Pos += FVector(300, 0, 0); P.Update(1, S);
	Check(P.Passes == 1 && P.Stage == TEXT("still") && Seen.Contains(TEXT("cows:by")), TEXT("first pass turns the nine out of sight"));
	Seen.Add(TEXT("ev:cows1")); P.Update(1, S);
	Check(P.Stage == TEXT("short") && H.Missing == S47Herd::MISSING, TEXT("nine seen, then eight only out of sight"));
	Seen.Add(TEXT("ev:cows")); P.Update(1, S);
	Check(P.Stage == TEXT("after") && !H.bStill && P.NinthGone(), TEXT("after the eight are seen they graze, still missing one"));
	P.Reset(); Check(P.Stage == TEXT("after") && P.Passes == 1 && Saved == 2, TEXT("restore preserves the completed sequence without saving again"));
	Seen.Clear(); Seen.Add(TEXT("mark:door@13499")); P.Reset(); S.Clock = 13500; S.Area.Empty(); P.Update(1, S);
	Check(P.Stage == TEXT("graze"), TEXT("ten-minute gap after the latest map mark"));
	S.Clock = 14099; P.Update(1, S); Check(P.Stage == TEXT("still"), TEXT("turns at the exact ten-minute gap"));
	Seen.Clear(); P.Reset(); S.Clock = 0; S.bOk = false; P.Now(); P.Update(0, S);
	Check(P.Stage == TEXT("still"), TEXT("developer force waits only for out of sight"));
	Seen.Clear(); P.Reset(); Check(P.TakeNinth() && P.Stage == TEXT("short") && Seen.Contains(TEXT("cows:short")), TEXT("station takes the ninth before the nine were seen"));
	return bOk;
}
