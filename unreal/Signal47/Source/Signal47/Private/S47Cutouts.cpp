// core/cutouts.ts: Cutouts. The card and stick come from the exported web scene, not new geometry.
#include "S47Cutouts.h"
#include "S47Image.h"
#include "S47Main.h"
#include "S47Web.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include <limits>

namespace { double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); } }
FS47Cutouts::FS47Cutouts(US47Main* InM, TFunction<void(bool)> InNote, TFunction<void(bool)> InPostNote)
	: M(InM), Note(MoveTemp(InNote)), PostNote(MoveTemp(InPostNote)), Wait(Rnd(40, 120))
{
	Fig = S47SceneObject(M->Web, TEXT("cutout:wander"));
	const auto C = Fig.Children(); if (!C.IsEmpty()) Card = C[0];
	Fig.SetVisible(false);
}
FS47Cutouts::FView FS47Cutouts::View(const FS47Cam& Cam, const FVector& At)
{
	const FVector Mid = At + FVector(0, 0.8, 0), Ndc = Cam.Project(Mid);
	return { FVector::Distance(Mid, Cam.Pos), Ndc.Z < 1 && FMath::Abs(Ndc.X) < 1.05 && FMath::Abs(Ndc.Y) < 1.05, FMath::Abs(Ndc.X) };
}
void FS47Cutouts::Update(double Dt, const FS47CutoutsIn& S)
{
	// litMat: same exported card, lit by the shown area's fake lamps (roadProps uses 0.012 too).
	const int32 Want = (Up == TEXT("shoulder") || Up == TEXT("post")) && S.Flood >= 0 ? S.Flood : -1;
	if (Want != Lit) { Lit = Want; if (M->Web) M->Web->UseFlood(Card, Lit, 0.012); }
	if (Up == TEXT("post")) { PostStep(Dt, S); return; }
	if (!Up.IsEmpty())
	{
		Age += Dt; const FView V = View(S.Cam, FigPos);
		const bool bLost = !S.bOk || (Up == TEXT("foot") ? !S.Foot || !S.Foot->Walkable(FigPos.X, FigPos.Z) : !S.Drive);
		if (V.bInView && V.Dist < (Up == TEXT("shoulder") ? 90 : 32))
		{
			Looked += Dt; Away = 0;
			if (Looked > 0.6 && !bWasSeen) { bWasSeen = true; ++Seen; if (Note) Note(Seen == 1); }
		}
		else Away += Dt;
		const bool bDone = (bWasSeen && Away > 2.5) || (Up == TEXT("shoulder") && V.Dist > 260 && Age > 8) || Age > 300;
		const bool bWithdrawn = !bWasSeen && S.Keep.IsSet() && !*S.Keep;
		if (bLost || ((bDone || bWithdrawn) && !V.bInView)) Hide(bWithdrawn ? Rnd(20, 60) : Rnd(70, 200));
		return;
	}
	if (!S.bOk) return;
	if (!bAsked) { bAsked = true; Paint(); } // Native art is local, so there is no asynchronous fetch.
	if (S.May.IsSet() && !*S.May) return;
	Wait -= Dt * S.Pace; if (Wait > 0) return;
	if (!Place(S)) Wait = Rnd(4, 10);
}
bool FS47Cutouts::Place(const FS47CutoutsIn& S)
{
	TOptional<FVector> At; FString Kind = TEXT("foot");
	if (S.Drive)
	{
		FVector D = S.Drive->Dir; D.Y = 0; D.Normalize(); const FVector Side(-D.Z, 0, D.X);
		FVector P = S.Drive->Pos + D * Rnd(140, 220) + Side * (FMath::FRand() < 0.5 ? -1 : 1) * Rnd(6.5, 8);
		P.Y = S.Drive->Ground(P.X, P.Z); At = P; Kind = TEXT("shoulder");
	}
	else if (S.Foot)
	{
		const auto& F = *S.Foot;
		for (int32 I = 0; I < 60 && !At; ++I)
		{
			const double A = FMath::FRand() * PI * 2, R = Rnd(6, 40), X = F.Pos.X + FMath::Sin(A) * R, Z = F.Pos.Z + FMath::Cos(A) * R;
			if (!F.Walkable(X, Z) || F.Blocked(X, Z)) continue;
			const FVector P(X, F.Floor(X, Z), Z); if (View(S.Cam, P).bInView) continue; At = P;
		}
	}
	if (!At) return false;
	FigPos = *At; Fig.SetPosition(FigPos); Fig.SetRotY(FMath::Atan2(S.Cam.Pos.X - At->X, S.Cam.Pos.Z - At->Z) + Rnd(-0.35, 0.35));
	Fig.SetVisible(true); Up = Kind; ++Placed; Looked = Away = Age = 0; bWasSeen = false; return true;
}
bool FS47Cutouts::PlacePost(const FVector& InPost, const FVector& From)
{
	if (!Up.IsEmpty()) return false;
	FVector To(From.X - InPost.X, 0, From.Z - InPost.Z); if (To.SizeSquared() < 1) return false; To.Normalize();
	FigPos = InPost + To * 0.16; Fig.SetPosition(FigPos); Fig.SetRotY(FMath::Atan2(To.X, To.Z));
	Fig.SetVisible(true); Up = TEXT("post"); Post.Reset(); Reveal = -1; Looked = Away = Age = 0; bWasSeen = false; return true;
}
void FS47Cutouts::PostStep(double Dt, const FS47CutoutsIn& S)
{
	Age += Dt; const FView V = View(S.Cam, FigPos);
	const TOptional<FVector> Src = S.Drive ? TOptional<FVector>(S.Drive->Pos) : S.Foot ? TOptional<FVector>(S.Foot->Pos) : TOptional<FVector>();
	const double D = Src ? FVector2D(FigPos.X - Src->X, FigPos.Z - Src->Z).Size() : std::numeric_limits<double>::infinity();
	if (V.bInView && V.Dist < 70) Looked += Dt;
	if (Reveal >= 0)
	{
		if ((Reveal += Dt) >= 0.09) { const bool bLooked = Looked > 0.6; Post = FPost{D, bLooked}; Hide(Rnd(70, 200)); if (bLooked && PostNote) PostNote(bLooked); }
		return;
	}
	const bool bNear = D < (S.Drive ? 20 : 12) || (S.Drive && D < 45 && V.bInView && V.Side > 0.42);
	if (S.bOk && bNear) { Reveal = 0; if (S.Blink) S.Blink(); return; }
	if (!V.bInView && (!S.bOk || D > 280 || Age > 90)) Hide(Rnd(30, 90));
}
void FS47Cutouts::Paint()
{
	// cutoutTexture's fallback is already exported on the card. Keep it unless round 14's art exists.
	if (UTexture2D* T = S47Image::Art(TEXT("cutout")))
	{
		Card.SetMap(T);
		if (M->Web) for (auto* Mid : M->Web->NodeMids(Card.I))
		{
			Mid->SetTextureParameterValue(TEXT("EmissiveMap"), T); Mid->SetScalarParameterValue(TEXT("UseEmissiveMap"), 1);
			Mid->SetVectorParameterValue(TEXT("EmU"), FLinearColor(1, 0, 0, 0)); Mid->SetVectorParameterValue(TEXT("EmV"), FLinearColor(0, -1, 1, 0));
		}
	}
}
void FS47Cutouts::Hide(double InWait) { Fig.SetVisible(false); Up.Empty(); Wait = InWait; Reveal = -1; }
void FS47Cutouts::Reset() { Hide(Rnd(40, 120)); Seen = Placed = 0; Post.Reset(); Reveal = -1; }
bool FS47Cutouts::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true; auto Check = [&](bool B, const TCHAR* S) { bOk &= B; Lines.Add(FString(B ? TEXT("ok cutouts: ") : TEXT("FAIL cutouts: ")) + S); };
	US47Main* M = NewObject<US47Main>(); int32 Noted = 0;
	FS47Cutouts C(M, [&](bool) { ++Noted; }, [&](bool) { ++Noted; });
	FS47CutoutsIn S; S.bOk = true; S.Cam.Pos = FVector(0, 1.62, 0); S.Foot.Emplace(); S.Foot->Pos = S.Cam.Pos; S.Foot->Walkable = [](double, double) { return true; }; S.Foot->Floor = [](double, double) { return 0.; }; S.Foot->Blocked = [](double, double) { return false; };
	Check(C.Place(S) && C.Up == TEXT("foot") && C.Placed == 1 && !View(S.Cam, C.Position()).bInView, TEXT("a reachable card starts out of view"));
	S.Cam.Pos = C.Position() + FVector(0, 1.62, 10);
	const FVector To = (C.Position() + FVector(0, .8, 0) - S.Cam.Pos).GetSafeNormal(); S.Cam.Forward = To; S.Cam.Right = FVector::CrossProduct(To, FVector(0, 1, 0)).GetSafeNormal(); S.Cam.Up = FVector::CrossProduct(S.Cam.Right, To);
	C.Update(.61, S); Check(C.Seen == 1 && Noted == 1, TEXT("a look longer than 0.6 seconds counts once"));
	S.Cam.Forward = -To; C.Update(2.6, S); Check(C.Up.IsEmpty(), TEXT("a seen card goes only after looking away"));
	C.Reset(); Check(C.PlacePost(FVector(0, 0, -10), FVector::ZeroVector) && C.Placed == 0, TEXT("the post is separate from wandering placements"));
	S.Cam = FS47Cam(); S.Cam.Pos.Y = 1.62; S.Foot->Pos = FVector::ZeroVector; C.Update(.61, S); C.Update(.1, S);
	Check(C.Up.IsEmpty() && C.Post && C.Post->bLooked && Noted == 2, TEXT("the light blinks for 0.09 seconds, revealing the post"));
	C.Reset(); Check(C.Seen == 0 && C.Placed == 0 && !C.Post, TEXT("a reset clears the night's sightings")); return bOk;
}
