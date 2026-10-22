#include "S47WorldLife.h"

#include "Components/LightComponent.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "S47Audio.h"
#include "S47Chapter1.h"
#include "S47Flora.h"
#include "S47Flow.h"
#include "S47Geo.h"
#include "S47GroundCover.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightLife.h"
#include "S47OldRoadLayout.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"
#include "S47Seen.h"
#include "S47Sky.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Ultra.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47WebAudio.h"
#include "S47Wind.h"
#include "S47WindowLight.h"
#include "S47World.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Life, Log, All);

namespace
{
	// World.ts: the areas' origins, the light in the air, the lit places on the sky, the shared lights
	const FVector ROAD_ORIGIN(8000, 0, 0), STATION_ORIGIN(0, 0, 8000);
	constexpr double AIR_CONES = 0.065, AIR_HAZE = 1;
	constexpr double HEAD_ON_MOTEL = 1.6, HEAD_ON_DINER = 2.5, NEON_REACH = 2, NEON_ON_ROAD = 0.3, HEAD_ON_TOWN = 1.8, TOWN_ON_ROAD = 0.8;
	// main.ts: how much of the sky's haze the far land takes at night and at dawn in the night areas
	constexpr double NIGHT_FOG = 0.36, DAWN_FOG = 0.42;
	// Crossing.ts WEST: where SARO's own ground ends and the road's land begins (site terms)
	constexpr double WEST_Z1 = 64;

	double Smoothstep(double X, double Min, double Max)
	{
		if (X <= Min) return 0;
		if (X >= Max) return 1;
		const double T = (X - Min) / (Max - Min);
		return T * T * (3 - 2 * T);
	}
	double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double D = 0) { double V = D; if (O) O->TryGetNumberField(K, V); return V; }
	const TArray<TSharedPtr<FJsonValue>>& Arr(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		static const TArray<TSharedPtr<FJsonValue>> Empty;
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O && O->TryGetArrayField(K, A) ? *A : Empty;
	}
	TSharedPtr<FJsonObject> Obj(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		const TSharedPtr<FJsonObject>* P = nullptr;
		return O && O->TryGetObjectField(K, P) ? *P : nullptr;
	}
	FVector V3(const TArray<TSharedPtr<FJsonValue>>& A) { return A.Num() >= 3 ? FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber()) : FVector::ZeroVector; }
	FS47FloraRect FR(const FS47Rect& R, double Dx = 0, double Dz = 0) { FS47FloraRect O; O.MinX = R.MinX - Dx; O.MaxX = R.MaxX - Dx; O.MinZ = R.MinZ - Dz; O.MaxZ = R.MaxZ - Dz; return O; }
	/** A leg's obstacles (export.js drive: circles [x, z, r], boxes [minX, maxX, minZ, maxZ]; world terms). */
	TArray<S47GroundCover::FObstacle> Obstacles(const AS47Web* W, const TCHAR* Leg)
	{
		TArray<S47GroundCover::FObstacle> Out;
		const TSharedPtr<FJsonObject> L = Obj(Obj(W->DriveData(), TEXT("legs")), Leg);
		for (const TSharedPtr<FJsonValue>& V : Arr(L, TEXT("obstacles")))
		{
			const TArray<TSharedPtr<FJsonValue>>& A = V->AsArray();
			S47GroundCover::FObstacle O;
			if (A.Num() == 3) { O.bCircle = true; O.X = A[0]->AsNumber(); O.Z = A[1]->AsNumber(); O.R = A[2]->AsNumber(); }
			else if (A.Num() == 4) { O.bCircle = false; O.MinX = A[0]->AsNumber(); O.MaxX = A[1]->AsNumber(); O.MinZ = A[2]->AsNumber(); O.MaxZ = A[3]->AsNumber(); }
			else continue;
			Out.Add(O);
		}
		return Out;
	}
	/** A leg's headlight pools (export.js drive: head {set, slots}). */
	bool Headlights(const AS47Web* W, const FString& Leg, int32& Set, TArray<int32>& Slots)
	{
		const TSharedPtr<FJsonObject> H = Obj(Obj(Obj(W->DriveData(), TEXT("legs")), *Leg), TEXT("head"));
		if (!H) return false;
		Set = int32(Num(H, TEXT("set"), -1));
		Slots.Reset();
		for (const TSharedPtr<FJsonValue>& V : Arr(H, TEXT("slots"))) Slots.Add(int32(V->AsNumber()));
		return Set >= 0;
	}
	TArray<int32> Ints(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		TArray<int32> Out;
		for (const TSharedPtr<FJsonValue>& V : Arr(O, K)) Out.Add(int32(V->AsNumber()));
		return Out;
	}
}

// ---------- boot ----------

void US47WorldLife::Init(US47Main* InM)
{
	M = InM;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	const TSharedPtr<FJsonObject>& L = W->LifeData();
	// the sky and the painted horizon (world/Sky.ts, world/backdrop.ts); its thunder after the delay
	// (main.ts: setTimeout, not while the morning holds its breath in the diner)
	Sky = MakeShared<FS47Sky>();
	Sky->Build(W);
	Backdrop = MakeShared<FS47Backdrop>();
	Backdrop->bHold = FParse::Param(FCommandLine::Get(), TEXT("S47NoBand"));   // (tests: the same picture without the painted horizon)
	TWeakObjectPtr<US47WorldLife> Self = this;
	Sky->OnThunder = [Self](double Delay, double Strength)
	{
		if (!Self.IsValid() || !Self->M || !Self->M->Audio || !Self->M->Audio->Ctx.IsValid()) return;
		Self->M->Audio->Ctx->SetTimeout(Delay, [Self, Strength]
		{
			if (!Self.IsValid() || !Self->M || Self->M->Mode != US47Main::EMode::Play || (Self->Hooks.MorningBusy && Self->Hooks.MorningBusy())) return;
			Self->M->Audio->Thunder(float(Strength));
		});
	};
	// the ground cover round the camera (world/flora.ts, groundCover.ts)
	BuildFlora(true);
	// the light in the air (world/airlight.ts): each area's cones, the haze, the lamps that tint it
	if (const TSharedPtr<FJsonObject> Air = Obj(L, TEXT("air")))
	{
		for (const TSharedPtr<FJsonValue>& V : Arr(Air, TEXT("cones")))
		{
			TSharedPtr<FS47LightCones> C = MakeShared<FS47LightCones>();
			if (C->Build(W, V->AsObject())) Cones.Add(C);
			else Cones.Add(nullptr);
		}
		TownCones = int32(Num(Air, TEXT("town"), -1));
		Haze = MakeShared<FS47GroundHaze>();
		if (!Haze->Build(W, int32(Num(Air, TEXT("haze"), -1)))) Haze.Reset();
		const TSharedPtr<FJsonObject> La = Obj(Air, TEXT("lamps"));
		for (const TPair<const TCHAR*, TArray<FS47HazeLamp>*>& P : { TPair<const TCHAR*, TArray<FS47HazeLamp>*>(TEXT("saro"), &HazeSaro), TPair<const TCHAR*, TArray<FS47HazeLamp>*>(TEXT("road"), &HazeRoad) })
		{
			for (const TSharedPtr<FJsonValue>& V : Arr(La, P.Key))
			{
				const TSharedPtr<FJsonObject> O = V->AsObject();
				FS47HazeLamp H;
				H.P = V3(Arr(O, TEXT("p"))); H.R = Num(O, TEXT("r"), 7); H.K = Num(O, TEXT("k"), 1);
				const FVector C = V3(Arr(O, TEXT("col")));
				H.Col = FLinearColor(C.X, C.Y, C.Z);
				P.Value->Add(H);
			}
		}
	}
	// the windows' light on the ground (world/windowLight.ts)
	for (const TSharedPtr<FJsonValue>& V : Arr(L, TEXT("windows")))
	{
		TSharedPtr<FS47WindowLights> Wl = MakeShared<FS47WindowLights>();
		if (Wl->Build(W, V->AsObject())) Windows.Add(Wl);
	}
	// the PC tier's lamps (core/ultra.ts): the desk lamp in the control room casts shadows too
	Ultra = MakeShared<FS47Ultra>(W);
	Ultra->AddShadowLight(W->Light(TEXT("room.lights.lamps[0]")).Component());
	// the life in the night (world/nightlife.ts)
	Life = MakeShared<FS47NightLife>();
	Life->Build(M);
	FParse::Value(FCommandLine::Get(), TEXT("S47Clock="), ClockTest);
	// -S47Meteor[=seconds]: a shooting star then (three seconds in); with -S47MeteorAim where the camera looks
	if (!FParse::Value(FCommandLine::Get(), TEXT("S47Meteor="), MeteorAt) && FParse::Param(FCommandLine::Get(), TEXT("S47Meteor"))) MeteorAt = 3;
	UE_LOG(LogS47Life, Display, TEXT("S47: the world's own life: live sky %s, %d flora patches, %d cones, haze %s, %d window lights, %d glow sets"),
		Sky->IsLive() ? TEXT("on") : TEXT("off"), Flora.Num(), Cones.Num(), Haze.IsValid() ? TEXT("yes") : TEXT("no"), Windows.Num(), int32(L.IsValid()));
}

void US47WorldLife::BuildFlora(bool bBind)
{
	AS47Web* W = M->Web.Get();
	// arrayLayout.ts FIELD_ZONES: SARO's walk zones of the antenna field
	TArray<FS47FloraRect> Field, SaroZones, SaroColliders;
	for (const FS47Rect& Z : W->ZonesMutable(TEXT("saro")))
	{
		if (Z.Id.StartsWith(TEXT("fSpine")) || Z.Id.StartsWith(TEXT("fRow")) || Z.Id.StartsWith(TEXT("fApron:")) || Z.Id.StartsWith(TEXT("fSpur:"))) Field.Add(FR(Z));
		if (Z.Id != TEXT("fHighway")) SaroZones.Add(FR(Z));
	}
	for (const FS47Rect& C : W->CollidersMutable(TEXT("saro"))) SaroColliders.Add(FR(C));
	S47GroundCover::SetField(Field);
	// World.ts coverOf: the diner's and the town's walls and walks (the road area's walks without its
	// own stops on the road), moved into a patch's terms
	auto CoverOf = [W](const FVector& O, TArray<FS47FloraRect>& Blocks, TArray<FS47FloraRect>& Walks)
	{
		for (const FS47Rect& C : W->CollidersMutable(TEXT("road"))) Blocks.Add(FR(C, O.X, O.Z));
		for (const FS47Rect& Z : W->ZonesMutable(TEXT("road"))) if (!Z.Id.StartsWith(TEXT("rs"))) Walks.Add(FR(Z, O.X, O.Z));
	};
	const TPair<const TCHAR*, int32> Groups[] = { { TEXT("world.flora.patches[0].group"), 0 }, { TEXT("world.flora.patches[1].group"), 1 }, { TEXT("world.flora.patches[2].group"), 2 }, { TEXT("world.oldRoad.flora.group"), 3 } };
	for (const auto& G : Groups)
	{
		const int32 Node = W->Handle(G.Key);
		const FS47WebNode* N = W->Node(Node);
		if (!N) continue;
		const FString Name = N->Name.StartsWith(TEXT("flora:")) ? N->Name.Mid(6) : FString();
		TSharedPtr<FS47Flora> F;
		if (Name == TEXT("saro"))
		{
			F = MakeShared<FS47Flora>(S47GroundCover::SaroFlora(SaroColliders, SaroZones));
			// World.initSaro: the posts and poles of SARO's stretch of the highway, as the road's patch has them
			F->Add(S47GroundCover::PostsOf(Obstacles(W, TEXT("saro")), -S47Geo::SARO_IN_ROAD.X, -S47Geo::SARO_IN_ROAD.Z), {});
		}
		else if (Name == TEXT("road"))
		{
			F = MakeShared<FS47Flora>(S47GroundCover::RoadFlora(S47GroundCover::PostsOf(Obstacles(W, TEXT("road")), ROAD_ORIGIN.X, ROAD_ORIGIN.Z)));
			TArray<FS47FloraRect> B, Wk;
			CoverOf(ROAD_ORIGIN, B, Wk);
			F->Add(B, Wk);
		}
		else if (Name == TEXT("station01"))
		{
			TArray<FS47FloraRect> Cs, Zs, Posts;
			for (const FS47Rect& C : W->CollidersMutable(TEXT("station01"))) Cs.Add(FR(C));
			for (const FS47Rect& Z : W->ZonesMutable(TEXT("station01"))) Zs.Add(FR(Z));
			// the last of the track's posts, in the road's terms (the station's turned back)
			for (const S47GroundCover::FObstacle& O : Obstacles(W, TEXT("station01")))
			{
				if (!O.bCircle) continue;
				const S47Geo::FXZ Rr = S47Geo::StationToRoad(O.X - STATION_ORIGIN.X, O.Z - STATION_ORIGIN.Z);
				FS47FloraRect P; P.MinX = Rr.X - O.R; P.MaxX = Rr.X + O.R; P.MinZ = Rr.Z - O.R; P.MaxZ = Rr.Z + O.R;
				Posts.Add(P);
			}
			F = MakeShared<FS47Flora>(S47GroundCover::StationFlora(Cs, Zs, STATION_ORIGIN, Posts));
		}
		else if (Name == TEXT("oldroad"))
		{
			// what stands (the posts, the bolts' witness posts, Kessler's gate and things), not long walls, local
			const FVector O = N->Parent >= 0 ? FS47Obj(W, N->Parent).WorldPosition() : FVector::ZeroVector;
			TArray<FS47FloraRect> Posts;
			for (const S47GroundCover::FObstacle& Ob : Obstacles(W, TEXT("roswell")))
			{
				FS47FloraRect P;
				if (Ob.bCircle) { P.MinX = Ob.X - Ob.R - O.X; P.MaxX = Ob.X + Ob.R - O.X; P.MinZ = Ob.Z - Ob.R - O.Z; P.MaxZ = Ob.Z + Ob.R - O.Z; }
				else if (FMath::Max(Ob.MaxX - Ob.MinX, Ob.MaxZ - Ob.MinZ) < 30) { P.MinX = Ob.MinX - O.X; P.MaxX = Ob.MaxX - O.X; P.MinZ = Ob.MinZ - O.Z; P.MaxZ = Ob.MaxZ - O.Z; }
				else continue;
				Posts.Add(P);
			}
			F = MakeShared<FS47Flora>(S47GroundCover::OldRoadFlora(Posts));
			TArray<FS47FloraRect> B, Wk;
			CoverOf(O, B, Wk);
			F->Add(B, Wk);
		}
		if (F && (!bBind || F->Bind(W, Node))) Flora.Add(F);
	}
}

// ---------- every frame ----------

FVector US47WorldLife::CamPos() const { return FS47Cam::Of(M->Player).Pos; }
FVector US47WorldLife::CamDir() const { return FS47Cam::Of(M->Player).Forward; }

double US47WorldLife::Dawn() const { return Sky.IsValid() ? Sky->Dawn : 0; }

void US47WorldLife::Step(double Dt)
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W || !W->IsLoaded() || !M->Player) return;
	const double T = M->T;
	const FString Area = M->World ? M->World->Area : FString(TEXT("saro"));
	const bool bTitle = M->Mode == US47Main::EMode::Title;
	const double Clock = M->Story ? M->Story->Clock() : 0;
	const FS47Cam Cam = FS47Cam::Of(M->Player, Area == TEXT("roswell") ? 12000 : 5000);
	TestFlags(Dt);
	// World.update's tail: the light in the air, and the life in it
	AirStep(Dt, T);
	// main.ts step(), at its end: sky.group.rotation.y = world.skyYaw; sky.update(dt, t, camera.position)
	const double Yaw = M->World ? M->World->SkyYaw() : 0;
	Sky->Update(Dt, T);
	// in the truck the player stays where they got in: the lamps that become real lights are the ones round the truck
	const FString Quality = M->Flow ? M->Flow->Quality : FString(TEXT("high"));
	Ultra->Set(Quality == TEXT("ultra"));
	const FVector LampsAt = M->World && M->World->bDriving ? Cam.Pos : M->Player->Pos;
	bool bRoomLamps = false;
	const TArray<FString> Keys = UltraLamps(LampsAt, bRoomLamps);
	Ultra->Update(Dt, LampsAt, TOptional<TArray<FString>>(Keys), bRoomLamps);
	ShareLights();
	Sky->SetCometClock(Clock);
	// the dawn follows the clock; the night areas are lit for the night, so they show less of it
	const TOptional<double> Lift = Hooks.SkyLift ? Hooks.SkyLift() : TOptional<double>();
	Sky->SetDawnLift(Lift.IsSet() ? *Lift : Area == TEXT("roswell") ? 1 : 0.7);
	// no stars where the dishes point from the moment they have come round to 026 / 32 (world/Sky.ts)
	if (M->Story && M->Story->At(ES47Phase::Turning))
	{
		double Ev = 0;
		if (M->Story->At(ES47Phase::Ch1)) if (const US47Chapter1* C1 = Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")))) Ev = C1->S.EventClock;
		Sky->HoleFrom = (Ev ? Ev : M->Story->EventClockSec()) + STAR_HOLE.After;
	}
	else Sky->HoleFrom = TNumericLimits<double>::Max(); // (Infinity: not tonight yet)
	const TOptional<double> SunElev = !bTitle && Hooks.SunElev ? Hooks.SunElev() : TOptional<double>();
	Sky->SetClock(bTitle ? 0 : Clock, SunElev);
	// out in the night areas the haze over the far land stays darker than the dawn sky behind the mesas
	const double DawnK = Smoothstep(Sky->Dawn, 0, 0.35);
	const double FogK = Area != TEXT("roswell") ? NIGHT_FOG + (DAWN_FOG - NIGHT_FOG) * DawnK : NIGHT_FOG + (1 - NIGHT_FOG) * DawnK;
	const FLinearColor FogC = Sky->FogColor * float(FogK);
	W->SetFogColor(FogC);
	// World.update: the dawn's haze on the near mesas (world/horizon.ts), as on the land in front of
	// them, none at night; its colour and density are the fog's (M_WebTone)
	if (Area != TEXT("roswell")) W->SetEnvScalar(TEXT("HorizonHazeK"), float(FMath::Min(0.3, Sky->Dawn)));
	// the painted horizon: the area's air, the land where it ends, the lit places' glow where they are from here
	Backdrop->Update(*Sky, FogC, W->FogDensity, Area == TEXT("roswell"), Cam.Pos.Y - GroundUnder(Cam.Pos), SkyPlaces(), Cam.Pos, Yaw);
	Sky->Draw(Cam.Pos, Yaw);
	// ext.update, yard.update, annex.update ...: the glows' blink (glow.update(t)), on main.ts's t
	W->UpdateGlows(T);
	// room.updateOutside (the control room's windows on the ground, as bright as its ceiling), and the
	// motel's and the town's window lights with their own areas' updates (World.update: motel, town)
	for (const TSharedPtr<FS47WindowLights>& Wl : Windows)
	{
		if (Wl->Name == TEXT("saro"))
		{
			const double C0 = W->Light(TEXT("room.lights.ceiling[0]")).Intensity(), C1 = W->Light(TEXT("room.lights.ceiling[1]")).Intensity();
			Wl->SetLevel(TEXT("room"), (C0 + C1) / 16);
			Wl->Update();
		}
		else if (Wl->Name == TEXT("motel")) { if (Area == TEXT("saro")) Wl->Update(); }
		else if (Area == TEXT("diner") || Area == TEXT("road") || Area == TEXT("roswell")) Wl->Update();
	}
	// room.lights.hemi.intensity = hemiBase * saroDark.adapt + sky.uniforms.uFlash.value * 2.4
	if (Area == TEXT("saro")) W->SetHemiIntensity(float(0.9 * (Hooks.SaroDarkAdapt ? Hooks.SaroDarkAdapt() : 1.0) + Sky->Flash * 2.4));
	// flora.ts followFlora: before each picture the wind moves on, and every patch shown fills round the camera
	static const double Start = FPlatformTime::Seconds();
	S47Wind::WindAt(FPlatformTime::Seconds() - Start);
	const S47Wind::FWind& Wd = S47Wind::Wind();
	W->SetEnvScalar(TEXT("WindTime"), float(Wd.Time));
	W->SetEnvScalar(TEXT("WindAmp"), float(Wd.Strength));
	for (const TSharedPtr<FS47Flora>& F : Flora)
	{
		F->SyncVisibility();
		F->SetWind(Wd.Dir);
		F->Follow(Cam.Pos, Cam.Forward, Cam.Fov, Cam.Aspect, Quality == TEXT("low"));
	}
}

void US47WorldLife::AirStep(double Dt, double T)
{
	// High and Ultra only, at night, and the haze only out in the open of SARO, the road, the diner's
	// lot and the town, and STATION 01
	const FString Quality = M->Flow ? M->Flow->Quality : FString(TEXT("high"));
	const bool bOn = !bAirOff && Quality != TEXT("low");
	const double Night = 1 - Smoothstep(Dawn(), 0.12, 0.5);
	for (const TSharedPtr<FS47LightCones>& C : Cones) if (C) C->Update(bOn, AIR_CONES * Night);
	const FS47Cam Cam = FS47Cam::Of(M->Player);
	const FVector Cp = Cam.Pos;
	const FString Area = M->World ? M->World->Area : FString(TEXT("saro"));
	// World.airOut (main.ts): the title's view, the cab, outdoors
	const bool bOut = M->Mode != US47Main::EMode::Play || (M->World && M->World->bDriving) || M->Space == TEXT("yard");
	TOptional<double> Ground;
	const TArray<FS47HazeLamp>* Lamps = nullptr;
	TArray<FS47HazeLamp> Road;
	if (bOn && Night > 0 && bOut)
	{
		// SARO's own ground is flat; south of it the land is the road's, and the slabs and floors at 0 stand above the haze
		if (Area == TEXT("saro")) { Ground = Cp.Z > WEST_Z1 ? US47Night::SaroFloorAt(M->Web.Get(), Cp.X, Cp.Z) : -0.62; Lamps = &HazeSaro; }
		else if (Area == TEXT("road") || Area == TEXT("diner"))
		{
			Ground = S47RoadTerrain::SurfaceY(Cp.X - ROAD_ORIGIN.X, Cp.Z - ROAD_ORIGIN.Z) + ROAD_ORIGIN.Y;
			// the town's street lamps tint it as long as their cones burn
			Road = HazeRoad;
			if (Cones.IsValidIndex(TownCones) && Cones[TownCones]) for (int32 i = 0; i < Cones[TownCones]->Lamps.Num() && i < Road.Num(); ++i) Road[i].K = Cones[TownCones]->LampK(i);
			Lamps = &Road;
		}
		else if (Area == TEXT("station01")) Ground = S47Legs::StationGroundAt(Cp.X, Cp.Z);
	}
	static const TArray<FS47HazeLamp> None;
	if (Haze) Haze->Update(Dt, T, Cp, Ground, AIR_HAZE * Night, M->Web->FogColor, Lamps ? *Lamps : None);
	// and the life in it (World.lifeStep)
	if (Life) Life->Update(Dt, T, Cam, Dawn(), Hooks.LifeQuiet ? Hooks.LifeQuiet() : M->Mode != US47Main::EMode::Play || (M->Story && M->Story->IsCinematic()));
}

TArray<FS47SkyPlace> US47WorldLife::SkyPlaces() const
{
	// SARO's lamps and dishes round its site's (0, -80), Caliche's main street with the diner, the motel
	TArray<FS47SkyPlace> Out;
	const FString A = M->World ? M->World->Area : FString(TEXT("saro"));
	if (A == TEXT("roswell") || A == TEXT("room6")) return Out;
	const S47Geo::FXZ& S = S47Geo::SARO_IN_ROAD;
	const double PLACES[3][4] = {
		{ S.X, S.Z - 80, 380, 1 }, { 0, (S47RoadShape::DINER().Z + S47RoadShape::TOWN().Z1) / 2, 200, 0.7 }, { S.X + S47Geo::MOTEL_AT().X - 38, S.Z + S47Geo::MOTEL_AT().Z + 40, 50, 0.18 },
	};
	for (const auto& P : PLACES)
	{
		FS47SkyPlace Q;
		if (A == TEXT("saro")) { Q.X = P[0] - S.X; Q.Z = P[1] - S.Z; }
		else if (A == TEXT("station01")) { const S47Geo::FXZ St = S47Geo::RoadToStation(P[0], P[1]); Q.X = STATION_ORIGIN.X + St.X; Q.Z = STATION_ORIGIN.Z + St.Z; }
		else { Q.X = ROAD_ORIGIN.X + P[0]; Q.Z = ROAD_ORIGIN.Z + P[1]; }
		Q.R = P[2]; Q.K = P[3];
		Out.Add(Q);
	}
	return Out;
}

double US47WorldLife::GroundUnder(const FVector& P) const
{
	const FString A = M->World ? M->World->Area : FString(TEXT("saro"));
	if (A == TEXT("saro")) return P.Z > WEST_Z1 ? US47Night::SaroFloorAt(M->Web.Get(), P.X, P.Z) : -0.62;
	if (A == TEXT("road") || A == TEXT("diner")) return S47RoadTerrain::SurfaceY(P.X - ROAD_ORIGIN.X, P.Z - ROAD_ORIGIN.Z) + ROAD_ORIGIN.Y;
	if (A == TEXT("station01")) return S47Legs::StationGroundAt(P.X, P.Z);
	if (A == TEXT("roswell"))
	{
		// OldRoad.height: its origin puts mile 0 on the road area's highway across from the diner
		const S47OldRoadLayout::FXZ& J = S47OldRoadLayout::J();
		const FVector O = ROAD_ORIGIN + FVector(-J.X, 0, S47RoadShape::DINER().Z - J.Z);
		return O.Y + S47OldRoadLayout::HeightAt(P.X - O.X, P.Z - O.Z);
	}
	return P.Y - 1.6;
}

TArray<FString> US47WorldLife::UltraLamps(const FVector& P, bool& bRoomLamps) const
{
	bRoomLamps = false;
	const FString A = M->World ? M->World->Area : FString(TEXT("saro"));
	AS47Web* W = M->Web.Get();
	if (A == TEXT("saro"))
	{
		if (W->Rect(TEXT("room.bounds")).Contains(P)) { bRoomLamps = true; return {}; }
		for (const TCHAR* Z : { TEXT("annex.zone.corridor"), TEXT("annex.zone.records"), TEXT("annex.zone.recordsDoor"), TEXT("annex.zone.southDoor") }) if (W->Rect(Z).Contains(P)) return { TEXT("annex") };
		if (W->Rect(TEXT("yard.zone.lab")).Contains(P)) return { TEXT("lab") };
		if (W->Rect(TEXT("world.motel.zone.office")).Contains(P)) return { TEXT("court") };
		return { TEXT("site"), TEXT("court") };
	}
	if (A == TEXT("station01")) return { M->World && M->World->Indoors(P) ? TEXT("hut") : TEXT("field") };
	if (A == TEXT("room6")) return { TEXT("motel") };
	if (A == TEXT("diner")) return { TEXT("diner") };
	if (A == TEXT("roswell")) return { TEXT("oldroad") };
	return { TEXT("road") };
}

void US47WorldLife::ShareLights()
{
	AS47Web* W = M->Web.Get();
	const TSharedPtr<FJsonObject> Share = Obj(W->LifeData(), TEXT("share"));
	if (!Share || !M->World) return;
	const FString Area = M->World->Area, Leg = M->World->Leg;
	// a set's head slots taking another set's lamps, their colour k times (MotelFront/Diner/Town.shine)
	auto Shine = [W](const TSharedPtr<FJsonObject>& Own, int32 Src, const TArray<int32>& Slots, double K)
	{
		if (!Own) return;
		const int32 Set = int32(Num(Own, TEXT("set"), -1));
		const TArray<int32> Head = Ints(Own, TEXT("head"));
		for (int32 j = 0; j < Head.Num(); ++j)
		{
			const int32 H = Head[j], I = Slots.IsValidIndex(j) ? Slots[j] : -1;
			if (Src < 0 || I < 0) { W->SetFlood(Set, H, 0); continue; }
			const FLinearColor P = W->FloodPos(Src, I);
			W->SetFloodPool(Set, H, FVector(P.R, P.G, P.B), P.A, W->FloodCol(Src, I) * float(K), false);
		}
		W->FlushFlood(Set);
	};
	// the truck's headlights on the motel when it is driven at SARO (its lamps are its own set)
	int32 Set = -1;
	TArray<int32> Slots;
	if (Area == TEXT("saro") && Leg == TEXT("saro") && Headlights(W, TEXT("saro"), Set, Slots)) Shine(Obj(Share, TEXT("motel")), Set, Slots, HEAD_ON_MOTEL);
	else Shine(Obj(Share, TEXT("motel")), -1, {}, HEAD_ON_MOTEL);
	const TSharedPtr<FJsonObject> Diner = Obj(Share, TEXT("diner")), Town = Obj(Share, TEXT("town"));
	if (!Diner) return;
	Set = -1; Slots.Reset();
	const bool bHl = Area == TEXT("roswell") ? Headlights(W, TEXT("roswell"), Set, Slots) : (Area == TEXT("road") || Area == TEXT("diner")) ? Headlights(W, TEXT("road"), Set, Slots) : false;
	Shine(Diner, bHl ? Set : -1, Slots, HEAD_ON_DINER);
	Shine(Town, bHl ? Set : -1, Slots, HEAD_ON_TOWN);
	// Town.lampsTo: the street lamps and the canopy nearest the camera, as lamps in the road's set
	int32 RoadSet = -1, OldSet = -1;
	TArray<int32> RoadHead, OldHead;
	Headlights(W, TEXT("road"), RoadSet, RoadHead);
	Headlights(W, TEXT("roswell"), OldSet, OldHead);
	if (Town && RoadSet >= 0)
	{
		const int32 TSet = int32(Num(Town, TEXT("set"), -1));
		TArray<int32> Cand = Ints(Town, TEXT("lamps"));
		Cand.Add(int32(Num(Town, TEXT("canopy"), -1))); Cand.Add(int32(Num(Town, TEXT("blink"), -1)));
		const FVector At = CamPos();
		auto D = [W, TSet, &At](int32 I) { const FLinearColor P = W->FloodPos(TSet, I); return FMath::Square(P.R - At.X) + FMath::Square(P.B - At.Z); };
		TArray<int32> Near;
		for (int32 I : Cand) if (I >= 0 && W->FloodPos(TSet, I).A > 0) Near.Add(I);
		Near.StableSort([&D](int32 A, int32 B) { return D(A) < D(B); });
		const TArray<int32> Out = Ints(Share, TEXT("townSlots"));
		for (int32 j = 0; j < Out.Num(); ++j)
		{
			const int32 O = Out[j];
			if (O < 0) continue;
			if (!Near.IsValidIndex(j)) { W->SetFlood(RoadSet, O, 0); continue; }
			const FLinearColor P = W->FloodPos(TSet, Near[j]);
			W->SetFloodPool(RoadSet, O, FVector(P.R, P.G, P.B), P.A, W->FloodCol(TSet, Near[j]) * float(TOWN_ON_ROAD), false);
		}
		W->FlushFlood(RoadSet);
	}
	// Diner.neon: the sign's red and cyan as two lamps in the road's set and the old road's
	const TSharedPtr<FJsonObject> Neon = Obj(Share, TEXT("neon"));
	const int32 DSet = int32(Num(Diner, TEXT("set"), -1));
	const int32 Signs[2] = { int32(Num(Diner, TEXT("red"), -1)), int32(Num(Diner, TEXT("cyan"), -1)) };
	for (const TPair<const TCHAR*, int32>& Dst : { TPair<const TCHAR*, int32>(TEXT("road"), RoadSet), TPair<const TCHAR*, int32>(TEXT("old"), OldSet) })
	{
		if (Dst.Value < 0) continue;
		const TArray<int32> Out = Ints(Neon, Dst.Key);
		for (int32 j = 0; j < 2 && j < Out.Num(); ++j)
		{
			const int32 O = Out[j];
			if (O < 0 || Signs[j] < 0) continue;
			const FLinearColor P = W->FloodPos(DSet, Signs[j]);
			W->SetFloodPool(Dst.Value, O, FVector(P.R, P.G, P.B), P.A * float(NEON_REACH), W->FloodCol(DSet, Signs[j]) * float(NEON_ON_ROAD), false);
		}
		W->FlushFlood(Dst.Value);
	}
}

// ---------- the test flags ----------

void US47WorldLife::TestFlags(double Dt)
{
	// -S47Clock=05:20: the night's clock set there once the story runs (the dawn's runs)
	if (!ClockTest.IsEmpty() && !bClockSet && M->Story && M->Mode == US47Main::EMode::Play && M->T > 2)
	{
		TArray<FString> P;
		ClockTest.ParseIntoArray(P, TEXT(":"));
		if (P.Num() >= 2)
		{
			M->Story->SetClock(FCString::Atod(*P[0]) * 3600 + FCString::Atod(*P[1]) * 60);
			UE_LOG(LogS47Life, Display, TEXT("S47: clock set to %s"), *ClockTest);
		}
		bClockSet = true;
	}
	// -S47Meteor: the developer menu's shooting star, three seconds in
	if (MeteorAt > 0 && M->T > MeteorAt)
	{
		MeteorAt = -1;
		Sky->MeteorNow();
		if (FParse::Param(FCommandLine::Get(), TEXT("S47MeteorAim")))
		{
			// the camera's way in the sky's own terms (the sky turned back by the area's yaw), a little up
			const FVector F = CamDir();
			const double Yw = M->World ? -M->World->SkyYaw() : 0;
			Sky->MeteorFrom(FVector(F.X * FMath::Cos(Yw) + F.Z * FMath::Sin(Yw), F.Y + 0.1, -F.X * FMath::Sin(Yw) + F.Z * FMath::Cos(Yw)));
		}
		UE_LOG(LogS47Life, Display, TEXT("S47: shooting star now (%d)"), Sky->Meteors);
	}
	// -S47LifeLog: what the world's life is doing, every two seconds
	if (FParse::Param(FCommandLine::Get(), TEXT("S47LifeLog")) && (FloraLogT -= Dt) <= 0)
	{
		FloraLogT = 2;
		AS47Web* W = M->Web.Get();
		FString Fl;
		for (const TSharedPtr<FS47Flora>& F : Flora)
		{
			int32 N = 0;
			for (const auto& KV : F->Drawn()) N += KV.Value;
			Fl += FString::Printf(TEXT(" %s%s:%d"), *F->Spec.Name, F->Shown() ? TEXT("") : TEXT("(hidden)"), N);
		}
		const FS47GlowPoints* B = W ? W->Glow(TEXT("ext.dishArray.beacons.points")) : nullptr;
		const double Bk = B && B->Num() ? FS47GlowPoints::Factor(B->Time, B->Blink[0], B->Hold, B->Level) : -1;
		int32 ConesOn = 0;
		for (const TSharedPtr<FS47LightCones>& C : Cones) ConesOn += C && C->Visible() ? 1 : 0;
		UE_LOG(LogS47Life, Display, TEXT("S47: life clock %s dawn %.3f sun %.3f hole %.3f flash %.2f meteor %s | beacon %.3f | flora%s | cones %d haze %s | ultra %d"),
			M->Story ? *M->Story->ClockText(false) : TEXT("-"), Sky->Dawn, Sky->Sun, Sky->HoleK, Sky->Flash, Sky->MeteorVisible() ? TEXT("on") : TEXT("off"), Bk, *Fl,
			ConesOn, Haze.IsValid() && Haze->Visible() ? TEXT("on") : TEXT("off"), Ultra->Held().Num());
	}
}

// ---------- the self test ----------

bool US47WorldLife::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What)
	{
		Lines.Add((bCond ? TEXT("ok   worldlife: ") : TEXT("FAIL worldlife: ")) + What); bOk &= bCond;
		UE_LOG(LogS47Life, Display, TEXT("S47: %s"), *Lines.Last());
	};
	// world/glow.ts: the blink (0.55 + 0.45 max(0, sin(t rate + phase))), held, steady
	Check(FMath::IsNearlyEqual(FS47GlowPoints::Factor(1.0, FVector2D(1.6, 0.3), 0, 1), 0.55 + 0.45 * FMath::Max(0.0, FMath::Sin(1.9)), 1e-6), TEXT("glow: a beacon's blink at t 1"));
	Check(FMath::IsNearlyEqual(FS47GlowPoints::Factor(2.5, FVector2D(1.6, 0.3), 0, 1), 0.55f, 1e-6f), TEXT("glow: at the bottom of its blink, 0.55"));
	Check(FMath::IsNearlyEqual(FS47GlowPoints::Factor(2.5, FVector2D(1.6, 0.3), 1, 0.2f), 0.2f, 1e-6f) && FS47GlowPoints::Factor(7, FVector2D::ZeroVector, 0, 1) == 1.f, TEXT("glow: held at a level; a steady one at 1"));
	// world/wind.ts
	S47Wind::WindAt(100);
	Check(FMath::IsNearlyEqual(S47Wind::Wind().Strength, 0.07 * (0.75 + 0.2 * FMath::Sin(11.0) + 0.15 * FMath::Sin(37.0 + 1.3)), 1e-12) && FMath::IsNearlyEqual(S47Wind::Wind().Dir.X, FMath::Cos(0.64 + 0.18 * FMath::Sin(2.3)), 1e-12), TEXT("wind at t 100"));
	// world/backdrop.ts horizonDip; Sky.ts starHoleAt
	Check(FMath::IsNearlyEqual(S47Backdrop::HorizonDip(100), FMath::Atan(100.0 / 20000) * 180 / UE_DOUBLE_PI, 1e-12) && S47Backdrop::HorizonDip(-5) == 0, TEXT("backdrop: the horizon's dip"));
	Check(S47StarHoleAt(2 * 3600, 8122) == 0 && S47StarHoleAt(8122 + 100, 8122) == 1 && FMath::IsNearlyEqual(S47StarHoleAt(8122 + 50, 8122), 0.5, 1e-12) && S47StarHoleAt(23 * 3600, 8122) == 0, TEXT("sky: the star hole comes in over 100 s, none in the evening"));
	// the web version's own numbers (export.js lifeOf: tests), when the world is there
	AS47Web* W = nullptr;
	for (TObjectIterator<AS47Web> It; It; ++It) if (It->IsLoaded()) { W = *It; break; }
	const TSharedPtr<FJsonObject> Tests = W ? Obj(W->LifeData(), TEXT("tests")) : nullptr;
	if (!Tests) { Lines.Add(TEXT("ok   worldlife: (no world or no exported tests: the web version's numbers not compared)")); return bOk; }
	// Sky.setClock at six clocks
	if (const TSharedPtr<FJsonObject> Sk = Obj(Tests, TEXT("sky")))
	{
		FS47Sky S;
		S.SetDawnLift(Num(Sk, TEXT("lift"), 0.7));
		S.HoleFrom = Num(Sk, TEXT("holeFrom"), 0);
		int32 Good = 0, All = 0;
		for (const TSharedPtr<FJsonValue>& V : Arr(Sk, TEXT("at")))
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			S.SetClock(Num(O, TEXT("clock")));
			const FVector Sd = V3(Arr(O, TEXT("sunDir"))), Fg = V3(Arr(O, TEXT("fog")));
			const bool bSame = FMath::IsNearlyEqual(S.Dawn, Num(O, TEXT("dawn")), 1e-9) && FMath::IsNearlyEqual(S.Sun, Num(O, TEXT("sun")), 1e-9) && S.SunDir.Equals(Sd, 1e-9)
				&& FMath::IsNearlyEqual(S.HoleK, Num(O, TEXT("hole")), 1e-9) && FVector(S.FogColor.R, S.FogColor.G, S.FogColor.B).Equals(Fg, 1e-6);
			Good += bSame ? 1 : 0; ++All;
			if (!bSame) Lines.Add(FString::Printf(TEXT("     sky at %.0f: dawn %.6f/%.6f sun %.6f/%.6f hole %.6f/%.6f"), Num(O, TEXT("clock")), S.Dawn, Num(O, TEXT("dawn")), S.Sun, Num(O, TEXT("sun")), S.HoleK, Num(O, TEXT("hole"))));
		}
		Check(All > 0 && Good == All, FString::Printf(TEXT("sky: setClock as the web version at %d of %d clocks (the dawn, the sun, the hole, the haze)"), Good, All));
	}
	// Flora.make: cells of each patch, drawn from the same seeds and cover rules (the specs built as Init builds them)
	US47Main* Mn = NewObject<US47Main>();
	Mn->Web = W;
	US47WorldLife* L = NewObject<US47WorldLife>();
	L->M = Mn;
	L->BuildFlora(false);
	for (const TSharedPtr<FJsonValue>& V : Arr(Tests, TEXT("flora")))
	{
		const TSharedPtr<FJsonObject> P = V->AsObject();
		FString Name;
		P->TryGetStringField(TEXT("name"), Name);
		const TSharedPtr<FS47Flora>* F = L->Flora.FindByPredicate([&Name](const TSharedPtr<FS47Flora>& X) { return X->Spec.Name == Name; });
		if (!F) { Check(false, FString::Printf(TEXT("flora: no patch %s"), *Name)); continue; }
		int32 Good = 0, All = 0;
		for (const TSharedPtr<FJsonValue>& Cv : Arr(P, TEXT("cells")))
		{
			const TSharedPtr<FJsonObject> C = Cv->AsObject();
			const int32 I = int32(Num(C, TEXT("i"))), J = int32(Num(C, TEXT("j"))), Ki = int32(Num(C, TEXT("ki"))), N = int32(Num(C, TEXT("n")));
			const FS47Flora::FKindCell& Mine = (*F)->CellOf(I, J, Ki);
			bool bSame = Mine.N == N;
			for (const TPair<const TCHAR*, int32>& Which : { TPair<const TCHAR*, int32>(TEXT("first"), 0), TPair<const TCHAR*, int32>(TEXT("last"), N - 1) })
			{
				const TArray<TSharedPtr<FJsonValue>>& Mx = Arr(C, Which.Key);
				if (!bSame || Mx.Num() != 16) continue;
				for (int32 k = 0; k < 16; ++k) bSame &= FMath::Abs(Mine.M[Which.Value * 16 + k] - Mx[k]->AsNumber()) < 1e-4;
			}
			const TArray<TSharedPtr<FJsonValue>>& Cl = Arr(C, TEXT("c"));
			if (bSame && Cl.Num() == 3) for (int32 k = 0; k < 3; ++k) bSame &= FMath::Abs(Mine.C[k] - Cl[k]->AsNumber()) < 1e-5;
			Good += bSame ? 1 : 0; ++All;
			if (!bSame) Lines.Add(FString::Printf(TEXT("     flora %s cell %d,%d kind %d: %d copies here, %d in the web version"), *Name, I, J, Ki, Mine.N, N));
		}
		Check(All > 0 && Good == All, FString::Printf(TEXT("flora %s: %d of %d cells as the web version draws them"), *Name, Good, All));
	}
	return bOk;
}
