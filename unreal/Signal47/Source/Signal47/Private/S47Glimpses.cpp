// core/glimpses.ts
#include "S47Glimpses.h"

#include "S47Three.h"
#include "S47Web.h"

namespace
{
	struct FSpot { const TCHAR* Id; const TCHAR* Scene; double X, Z; uint32 Rim; };
	const double ROAD_X = 8000; // World.ROAD_ORIGIN
	const FSpot SPOTS[] = {
		// SARO: behind antennas in the array's far rows
		{ TEXT("S-11"), TEXT("saro"), -12, -208, 0x14100c },
		{ TEXT("S-14"), TEXT("saro"), 43, -238, 0x14100c },
		{ TEXT("S-19"), TEXT("saro"), -42, -308, 0x14100c },
		{ TEXT("S-22"), TEXT("saro"), 8, -368, 0x14100c },
		// Caliche and the diner, in the road's terms (Town.ts), out past the last of the light
		{ TEXT("town-south"), TEXT("road"), ROAD_X + 3, 2296, 0x2a1a08 },
		{ TEXT("town-mast"), TEXT("road"), ROAD_X + 47, 2052, 0x2a0806 },
		{ TEXT("town-west"), TEXT("road"), ROAD_X - 62, 2086, 0x1a1208 },
		{ TEXT("diner-west"), TEXT("road"), ROAD_X - 56, 1992, 0x101a1a },
		{ TEXT("diner-north"), TEXT("road"), ROAD_X - 34, 1952, 0x101a1a },
	};
	const int32 MAX = 8;
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
}

FS47Glimpses::FS47Glimpses(AS47Web* InWeb) : Web(InWeb)
{
	// new Silhouette('glimpse', 0x14100c), hidden, under the scene (the export has it)
	Fig = S47SceneObject(InWeb, TEXT("glimpse"));
	SetVisible(false);
	Wait = Rnd(120, 300);
}

TOptional<FVector> FS47Glimpses::Where() const
{
	if (Up.IsEmpty()) return {};
	return FVector(FigPos.X, FigPos.Y + 0.6, FigPos.Z);
}

FS47Glimpses::FSight FS47Glimpses::Sight(const FS47Cam& Cam, const FVector& At)
{
	// degrees from the middle of the view to the figure, and whether it is in the view at all
	const FVector Head(At.X, At.Y + 0.6, At.Z);
	const FVector To = Head - Cam.Pos;
	FSight S;
	S.Dist = To.Size();
	S.Off = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Cam.Forward, To.GetSafeNormal()), -1.0, 1.0)));
	const FVector N = Cam.Project(Head);
	S.bInView = N.Z < 1 && FMath::Abs(N.X) < 0.96 && FMath::Abs(N.Y) < 0.96;
	return S;
}

void FS47Glimpses::Update(double Dt, const FS47GlimpsesIn& S)
{
	T += Dt;
	if (!Up.IsEmpty())
	{
		const FSight Sg = Sight(S.Cam, FigPos);
		// (the peak over, and it stood there unseen: it goes as soon as it is out of the view)
		if (T > 40 || !S.bOk || Sg.Off < 25 || Sg.Dist < 40 || S.Zoom > 0.25 || (S.bFlashlight && Sg.Off < 40 && Sg.Dist < 70) || (S.May == TOptional<bool>(false) && !Sg.bInView)) Hide();
		return;
	}
	if (!S.bOk || S.Scene.IsEmpty() || Count >= MAX || S.May == TOptional<bool>(false)) return;
	// (sil.wake(): the painted outlines are asked for; not in the export)
	// rarely at first, more often towards morning: about every four minutes at three, two at five
	// (the night's curve lets them only at its peak, and sooner there)
	const double Day = FMath::Fmod(FMath::Fmod(S.Clock, 86400.0) + 86400.0, 86400.0), K = FMath::Clamp((Day - 2.5 * 3600) / (2.5 * 3600), 0.0, 1.0);
	Wait -= Dt * (0.6 + 1.4 * K) * S.Pace.Get(1);
	if (Wait > 0) return;
	Wait = Rnd(20, 40);   // nowhere to stand just now: try again soon
	const TOptional<FAt> At = Pick(S);
	if (!At) return;
	Place(*At, S.Cam.Pos);
	Wait = Rnd(150, 330);
}

TOptional<FS47Glimpses::FAt> FS47Glimpses::Pick(const FS47GlimpsesIn& S, const FString& Only) const
{
	// a place 40 to 300 m off, in the view but well off its middle (the tests can name one)
	TArray<FAt> Out;
	auto Ground = [&S](double X, double Z) { return S.Ground ? S.Ground(X, Z) : 0.0; };
	for (const FSpot& Sp : SPOTS) if (S.Scene == Sp.Scene && (Only.IsEmpty() || Only == Sp.Id)) Out.Add({ Sp.Id, FVector(Sp.X, Ground(Sp.X, Sp.Z), Sp.Z), Sp.Rim });
	// on the highway, at the far end of the headlights on the shoulder
	if (S.Truck && S.Scene == TEXT("road") && (Only.IsEmpty() || Only == TEXT("shoulder")))
	{
		const FVector D = FVector(S.Truck->Dir.X, 0, S.Truck->Dir.Z).GetSafeNormal(), Side(-D.Z, 0, D.X);
		FVector P = S.Truck->Pos + D * Rnd(110, 160) + Side * ((FMath::FRand() < 0.5 ? -1 : 1) * 8.5);
		P.Y = Ground(P.X, P.Z);
		Out.Add({ TEXT("shoulder"), P, 0x1c1610 });
	}
	TArray<FAt> Ok;
	for (const FAt& O : Out)
	{
		const FSight Sg = Sight(S.Cam, O.P);
		if (Sg.Dist > 40 && Sg.Dist < 300 && Sg.bInView && (!Only.IsEmpty() || Sg.Off > 30)) Ok.Add(O);
	}
	if (!Ok.Num()) return {};
	return Ok[FMath::Min(Ok.Num() - 1, int32(FMath::FRand() * Ok.Num()))];
}

void FS47Glimpses::Place(const FAt& At, const FVector& Cam)
{
	FigPos = At.P;
	Fig.SetPosition(At.P);
	Fig.SetRotY(FMath::Atan2(Cam.X - At.P.X, Cam.Z - At.P.Z));   // facing the player
	Stand(At, Cam);
	SetVisible(true); Up = At.Id; T = 0; Count++;
}

bool FS47Glimpses::Now(const FS47GlimpsesIn& S, const FString& Id)
{
	const TOptional<FAt> At = Pick(S, Id);
	if (!At) return false;
	Place(*At, S.Cam.Pos);
	return true;
}

void FS47Glimpses::Stand(const FAt& At, const FVector& Cam)
{
	// how it stands there: far off soft at the edges, nearer upright or turned half aside (the
	// painted outlines' poses: the figure in code has one), and the edge of light is the light of
	// the place behind it (sil.rim: the shell's colour)
	const TArray<FS47Obj> Parts = Fig.Children();
	if (Parts.Num() > 1) Parts[1].SetColor(S47Three::Hex(At.Rim));
}

void FS47Glimpses::SetVisible(bool bOn)
{
	bVisible = bOn;
	Fig.SetVisible(bOn);
}

void FS47Glimpses::Hide() { SetVisible(false); Up.Reset(); }

void FS47Glimpses::Reset() { Hide(); Count = 0; Wait = Rnd(120, 300); }

bool FS47Glimpses::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s glimpses: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47Glimpses G(nullptr);
	FS47GlimpsesIn S;
	S.bOk = true; S.Scene = TEXT("saro"); S.Clock = 3 * 3600; S.Ground = [](double, double) { return -0.6; };
	// in the yard, the eyes on S-11's head (three's right is forward x up)
	auto Aim = [&S](const FVector& Dir)
	{
		S.Cam.Forward = Dir.GetSafeNormal();
		S.Cam.Right = FVector::CrossProduct(S.Cam.Forward, FVector(0, 1, 0)).GetSafeNormal();
		S.Cam.Up = FVector::CrossProduct(S.Cam.Right, S.Cam.Forward).GetSafeNormal();
	};
	S.Cam.Pos = FVector(10, 1.02, -12);
	Aim(FVector(-12, 0, -208) - S.Cam.Pos);
	Check(FVector::CrossProduct(FVector(0, 0, -1), FVector(0, 1, 0)).Equals(FVector(1, 0, 0)), TEXT("three's right is forward x up"));
	Check(G.Now(S, TEXT("S-11")) && G.Up == TEXT("S-11") && G.Count == 1 && FMath::IsNearlyEqual(G.Where()->Y, 0.0), TEXT("a named spot in the view, 40 to 300 m off: it stands there"));
	G.Update(0.1, S);
	Check(G.Up.IsEmpty(), TEXT("looked at (within 25 degrees), it is not there"));
	// on its own, never in the middle of the view (the far rows are all within 30 degrees of S-11 from here)
	G.Reset();
	Check(!G.Pick(S).IsSet(), TEXT("on its own, never where the player looks"));
	// looking 60 degrees to the right of it: S-11 at the left edge, and it may stand there
	Aim(FVector(-12, 0, -208) - S.Cam.Pos);
	Aim(S.Cam.Forward.RotateAngleAxis(-35, FVector(0, 1, 0)));
	const TOptional<FAt> Edge = G.Pick(S);
	Check(Edge.IsSet() && Sight(S.Cam, Edge->P).Off > 30 && Sight(S.Cam, Edge->P).bInView, TEXT("at the edge of the view, more than 30 degrees off"));
	// at most eight in a night
	G.Count = 8;
	G.Update(1000, S);
	Check(G.Up.IsEmpty(), TEXT("at most eight in a night"));
	return bOk;
}
