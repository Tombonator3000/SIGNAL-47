// core/glimpses.ts
#include "S47Glimpses.h"

#include "S47Three.h"
#include "S47Web.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"

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
	struct FPeek { const TCHAR* Id; double X,Z; FVector Cover; bool bWindow; };
	// The control windows start at Y=.94, with a real raised strip at Y=0
	// outside them. A 1.20m body can look over the sill without floating.
	const FPeek PEEKS[] = {
		{ TEXT("control-window-west"), -4.5,-5.25,FVector::ZeroVector,true },
		{ TEXT("control-window-east"), .9,-5.25,FVector::ZeroVector,true },
		{ TEXT("control-corner"), 6.95,-5.25,FVector(0,0,1.25),false },
		{ TEXT("wing-corner"), 19.75,-6.35,FVector(0,0,1.25),false },
	};
	double Ease(double X) { X=FMath::Clamp(X,0.,1.); return X*X*(3.-2.*X); }
}

FS47Glimpses::FS47Glimpses(AS47Web* InWeb) : Web(InWeb)
{
	// new Silhouette('glimpse', 0x14100c), hidden, under the scene (the export has it)
	Fig = S47SceneObject(InWeb, TEXT("glimpse"));
	if (InWeb && !FParse::Param(FCommandLine::Get(),TEXT("S47RoswellOff")))
	{
		auto* Root=NewObject<USceneComponent>(InWeb,TEXT("RoswellGlimpseRoot"));
		InWeb->AddInstanceComponent(Root); Root->SetMobility(EComponentMobility::Movable);
		Root->SetupAttachment(InWeb->GetRootComponent()); Root->RegisterComponent(); BodyRoot=Root;
		bRoswell=Body.Build(InWeb,Root,TEXT("RoswellGlimpseBody"),
			TEXT("/Game/Licensed/TripoNPC/Roswell/SK_Tripo_Roswell.SK_Tripo_Roswell"));
		if (bRoswell) Fig.Traverse([&](FS47Obj O)
		{
			if (const auto* N=InWeb->Node(O.I)) if (auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))
			{ C->SetRenderInMainPass(false); C->SetRenderInDepthPass(false); C->SetVisibleInRayTracing(false); C->SetCastShadow(false); }
		});
	}
	SetVisible(false);
	Wait = Rnd(120, 300);
}

TOptional<FVector> FS47Glimpses::Where() const
{
	if (Up.IsEmpty()) return {};
	return FVector(FigPos.X, FigPos.Y + (bRoswell?S47Roswell::HeadM-(bWindow?S47Roswell::DuckM*(1.-ExposureNow):0):.6), FigPos.Z);
}

FS47Glimpses::FSight FS47Glimpses::Sight(const FS47Cam& Cam, const FVector& At, double HeadY)
{
	// degrees from the middle of the view to the figure, and whether it is in the view at all
	const FVector Head(At.X, At.Y + HeadY, At.Z);
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
	if (bPreviewDriver) return; // Opt-in isolated preview owns this same controller's clock.
	if (bRoswell) { UpdateRoswell(Dt,S); return; }
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
	if (bRoswell && S.Scene==TEXT("saro") && !S.Truck)
		for (const FPeek& P:PEEKS) if ((Only.IsEmpty() || Only==P.Id) &&
			(!P.bWindow || !Only.IsEmpty() || (S.Cam.Pos.X>-6.3 && S.Cam.Pos.X<6.3 && S.Cam.Pos.Z>-4.5 && S.Cam.Pos.Z<4.5)))
			Out.Add({P.Id,FVector(P.X,Ground(P.X,P.Z),P.Z),0,P.Cover,P.bWindow,S.Scene});
	for (const FSpot& Sp : SPOTS) if (S.Scene == Sp.Scene && (Only.IsEmpty() || Only == Sp.Id)) Out.Add({ Sp.Id, FVector(Sp.X, Ground(Sp.X, Sp.Z), Sp.Z), Sp.Rim,FVector::ZeroVector,false,S.Scene });
	// on the highway, at the far end of the headlights on the shoulder
	if (S.Truck && S.Scene == TEXT("road") && (Only.IsEmpty() || Only == TEXT("shoulder")))
	{
		const FVector D = FVector(S.Truck->Dir.X, 0, S.Truck->Dir.Z).GetSafeNormal(), Side(-D.Z, 0, D.X);
		FVector P = S.Truck->Pos + D * (bRoswell?Rnd(22,36):Rnd(110,160)) + Side * ((FMath::FRand() < 0.5 ? -1 : 1) * 8.5);
		P.Y = Ground(P.X, P.Z);
		Out.Add({ TEXT("shoulder"), P, 0x1c1610,FVector::ZeroVector,false,S.Scene });
	}
	TArray<FAt> Ok;
	for (const FAt& O : Out)
	{
		const FSight Sg = Sight(S.Cam,O.P,bRoswell?S47Roswell::HeadM:.6);
		if (bRoswell)
		{
			// Close enough to identify a small body. Opaque world geometry still
			// blocks a candidate: this never reveals an alien through a building.
			if (Sg.Dist>2.5 && Sg.Dist<(S.Truck?45.:26.) && Sg.bInView &&
				(!Only.IsEmpty() || Sg.Off>8.) && ClearLine(S.Cam.Pos,O.P+FVector(0,S47Roswell::ProbeM,0))) Ok.Add(O);
		}
		else if (Sg.Dist > 40 && Sg.Dist < 300 && Sg.bInView && (!Only.IsEmpty() || Sg.Off > 30)) Ok.Add(O);
	}
	if (!Ok.Num()) return {};
	return Ok[FMath::Min(Ok.Num() - 1, int32(FMath::FRand() * Ok.Num()))];
}

void FS47Glimpses::Place(const FAt& At, const FVector& Cam)
{
	FigPos = At.P;
	BasePos=At.P; Cover=At.Cover; bWindow=At.bWindow; ActiveScene=At.Scene;
	Facing=FMath::Atan2(Cam.X-At.P.X,Cam.Z-At.P.Z); Noticed=0; Withdraw=-1;
	Fig.SetPosition(At.P);
	Fig.SetRotY(FMath::Atan2(Cam.X - At.P.X, Cam.Z - At.P.Z));   // facing the player
	Stand(At, Cam);
	SetVisible(true); Up = At.Id; T = 0; Count++;
	if (bRoswell) DrawRoswell(0,0);
}

bool FS47Glimpses::Now(const FS47GlimpsesIn& S, const FString& Id)
{
	if (S.Scene.IsEmpty()) return false;
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

bool FS47Glimpses::ClearLine(const FVector& Eye, const FVector& Head, FString* BlockedBy) const
{
	if (BlockedBy) BlockedBy->Reset();
	AS47Web* W=Web.Get();
	if (!W || !W->GetWorld()) return true; // Pure logic fixtures have no renderer.
	// Same opaque rule as the threat: interaction shims and clear window glass
	// do not block a view, but the actual walls and window sill do.
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47RoswellSight),true);
	// The old silhouette remains a logical/photo handle after its draw is
	// masked. It must not occlude its replacement when that handle is visible.
	Fig.Traverse([&](FS47Obj O)
	{
		if (const auto* N=W->Node(O.I)) if (const auto* C=Cast<UPrimitiveComponent>(N->Comp.Get())) Q.AddIgnoredComponent(C);
	});
	if (const auto* Root=BodyRoot.Get())
	{
		TArray<USceneComponent*> Parts; Root->GetChildrenComponents(true,Parts);
		for (const auto* Part:Parts) if (const auto* C=Cast<UPrimitiveComponent>(Part)) Q.AddIgnoredComponent(C);
	}
	for (int32 I=0; I<24; ++I)
	{
		FHitResult H;
		if (!W->GetWorld()->LineTraceSingleByChannel(H,S47Three::ToUE(Eye),S47Three::ToUE(Head),ECC_Visibility,Q)) return true;
		const auto* C=H.GetComponent(); if (!C) return false;
		bool Solid=C->IsVisible() && !C->bHiddenInGame;
		if (const auto* N=W->Node(W->NodeOf(C))) if (N->Name.StartsWith(TEXT("proxy"))) Solid=false;
		if (const auto* Mat=C->GetMaterial(FMath::Max<int32>(0,H.ElementIndex)))
		{
			float Opacity=1; Mat->GetScalarParameterValue(TEXT("Opacity"),Opacity);
			if (Mat->GetBlendMode()==BLEND_Additive || (Mat->GetBlendMode()==BLEND_Translucent && Opacity<.5f)) Solid=false;
		}
		if (Solid) { if (BlockedBy) *BlockedBy=C->GetName(); return false; }
		Q.AddIgnoredComponent(C);
	}
	return false;
}

void FS47Glimpses::DrawRoswell(double Exposure, double Clock)
{
	ExposureNow=Exposure;
	FigPos=BasePos+Cover*(1.-Exposure);
	Fig.SetPosition(FigPos);
	if (auto* Root=BodyRoot.Get())
	{
		Root->SetWorldLocation(S47Three::ToUE(FigPos));
		Root->SetWorldRotation(FRotationMatrix::MakeFromX(
			S47Three::DirToUE(FVector(FMath::Sin(Facing),0,FMath::Cos(Facing)))).Rotator());
	}
	const double HeadTurn=.20*FMath::Sin(Clock*.85);
	if (bWindow || Cover.IsNearlyZero() || Exposure>.995 || Exposure<.005) Body.Peek(bVisible,bWindow?Exposure:1.,HeadTurn,Clock);
	else Body.Apply(bVisible,HeadTurn,.65,Clock*5.,Clock,false,true);
}

void FS47Glimpses::UpdateRoswell(double Dt, const FS47GlimpsesIn& S)
{
	if (!Up.IsEmpty())
	{
		if (S.Scene!=ActiveScene) { Hide(); return; }
		// Menus/other guarded scenes temporarily hide the body without spending
		// its readable interval or moving it while the player cannot respond.
		if (!S.bOk) { Body.Peek(false,1,0,T); return; }
		T+=Dt;
		const FSight Seen=Sight(S.Cam,BasePos,S47Roswell::HeadM);
		const bool bLooked=Seen.bInView && Seen.Off<18. && ClearLine(S.Cam.Pos,BasePos+FVector(0,S47Roswell::ProbeM,0));
		if (T>1.25 && bLooked) Noticed+=Dt;
		else Noticed=FMath::Max(0.,Noticed-Dt*.4);
		// Readable recognition first; no vanish on the same frame as a look,
		// flashlight or zoom. It then physically withdraws along its cover path.
		if (Withdraw<0 && (Noticed>1.15 || Seen.Dist<2.5 || T>12. ||
			(S.bFlashlight && bLooked && T>2.5) || (S.Zoom>.25 && bLooked && T>2.5) ||
			(S.May==TOptional<bool>(false) && !Seen.bInView))) Withdraw=T;
		const double Exposure=Withdraw>=0?1.-Ease((T-Withdraw)/1.5):Ease(T/1.25);
		DrawRoswell(Exposure,T);
		// A retreat must really reach cover. An open-ground sighting can remain
		// watching until the player turns away; it never pops out in clear sight.
		if (Withdraw>=0 && T-Withdraw>=1.5 &&
			(!Seen.bInView || !ClearLine(S.Cam.Pos,FigPos+FVector(0,bWindow?S47Roswell::HeightM-S47Roswell::DuckM:S47Roswell::HeadM,0)))) Hide();
		return;
	}
	if (!S.bOk || S.Scene.IsEmpty() || Count>=MAX || S.May==TOptional<bool>(false)) return;
	const double Day=FMath::Fmod(FMath::Fmod(S.Clock,86400.)+86400.,86400.);
	const double K=FMath::Clamp((Day-2.5*3600.)/(2.5*3600.),0.,1.);
	Wait-=Dt*(.6+1.4*K)*S.Pace.Get(1);
	if (Wait>0) return;
	Wait=Rnd(20,40);
	const auto At=Pick(S); if (!At) return;
	Place(*At,S.Cam.Pos); Wait=Rnd(150,330);
}

void FS47Glimpses::SetVisible(bool bOn)
{
	bVisible = bOn;
	Fig.SetVisible(bOn);
	if (!bOn && bRoswell) Body.Peek(false,1,0,0);
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
	// Native Roswell behaviour is independent of asset loading in this pure
	// fixture. Visual skin, cover and scale still need the ordinary GPU check.
	G.bRoswell=true; G.Reset();
	S.Ground=[](double,double) { return 0.; };
	S.Cam.Pos=FVector(-4.5,1.62,0);
	Aim(FVector(-4.5,S47Roswell::HeadM,-5.25)-S.Cam.Pos);
	Check(G.Now(S,TEXT("control-window-west")) && G.Count==1 && G.HasNativeBody(),
		TEXT("Roswell has a separate nearby window anchor, not the distant shadow"));
	for (int32 I=0; I<4; ++I) G.Update(.5,S);
	Check(!G.Up.IsEmpty() && G.Withdraw<0 && G.ExposureNow==1.,
		TEXT("looking at the small figure leaves a readable recognition interval"));
	const double HeldAt=G.T; S.bOk=false; G.Update(20,S); S.bOk=true;
	Check(G.T==HeldAt && !G.Up.IsEmpty(),TEXT("guarded/menu time does not consume the Roswell interval"));
	G.Update(.3,S); G.Update(.7,S);
	Check(!G.Up.IsEmpty() && G.Withdraw>=0 && G.ExposureNow>0 && G.ExposureNow<1.,
		TEXT("noticed Roswell withdraws over time instead of vanishing in one frame"));
	Aim(FVector(0,0,1)); G.Update(1,S);
	Check(G.Up.IsEmpty(),TEXT("retreat completes after actual cover or a turn away"));
	G.Count=8; G.Wait=0; G.Update(1000,S);
	Check(G.Up.IsEmpty(),TEXT("native Roswell preserves the eight-appearance night limit"));
	return bOk;
}
