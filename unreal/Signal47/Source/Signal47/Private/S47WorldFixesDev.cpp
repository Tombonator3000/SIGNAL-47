#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47FarmWorld.h"
#include "S47Main.h"
#include "S47RouteNature.h"
#include "S47Sky.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47WorldLife.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47WorldFixes, Log, All);

// F10, "World fixes (T)" (Tom's test of d36, 9 November, with pictures): PT-MOON, the moon was a flat white disc and shone through the
// clouds; PT-ROCKS, the big rocks by the road were cut off on one side, parts missing; PT-RANGE-FAR, the far ranges showed their texture
// in stripes. Each line stands where it can be seen and says what it shows. In a sweep (-S47DevSweep=dir -S47DevSweepGroup="World fixes (T)")
// each line also writes its own pictures, named <line>-<what>.png: the moon wide and close, as now and as the painted disc before; the
// rocks as they stand now and as they stood (the authored turn), and with no ray tracing on them; the ranges wide and close. The range's
// before is the material as it was: run the group once before Tools/freeformterrain (S47_FF_MATERIAL_ONLY=1) and once after. Nothing
// here is saved; the moon, the clouds and the rocks are given back when another line starts.
namespace
{
	FS47Sky* SkyOf(US47Main* M) { return M && M->Life ? M->Life->Sky.Get() : nullptr; }
}

void FS47DevFreeform::AddWorldFixes(FS47DevEvents& E)
{
	static const TCHAR* const Group = TEXT("World fixes (T)");
	WorldFixesClear = [this]
	{
		FS47Sky::DevMoon = FS47Sky::FDevMoon();
		if (FS47RouteNature* R = FS47RouteNature::Live()) { R->DevBefore(false); R->DevRayTracing(true); }
		if (M && M->Player && SavedFov > 0) { M->Player->Fov = SavedFov; SavedFov = -1; }
	};
	// A picture with this name, in a sweep only (its own directory).
	const auto Shot = [this](const FString& Name) { if (!SweepDir.IsEmpty()) FScreenshotRequest::RequestScreenshot(SweepDir / (Name + TEXT(".png")), false, false); };
	// A run of steps a second apart for the textures to come in: each sets something up and takes its picture.
	using FStep = TPair<FString, TFunction<void()>>;
	const auto Steps = [this, Shot](const TArray<FStep>& List, TFunction<void()> Done)
	{
		// The step holds itself only weakly (critic round 7: a strong self-capture kept every run alive for good); the timer that waits
		// holds it strongly, so the chain lives while a step is due and is freed after the last.
		TSharedRef<TFunction<void(int32)>> Next = MakeShared<TFunction<void(int32)>>();
		const TWeakPtr<TFunction<void(int32)>> WeakNext = Next;
		*Next = [this, Shot, List, Done, WeakNext](int32 I)
		{
			if (I >= List.Num()) { if (Done) Done(); return; }
			if (List[I].Value) List[I].Value();
			const TSharedPtr<TFunction<void(int32)>> Strong = WeakNext.Pin();
			if (!Strong) return;
			After(1.4, [Shot, List, Strong, I]
			{
				if (!List[I].Key.IsEmpty()) Shot(List[I].Key);
				// (the screenshot is written in the next frame: nothing changes until then)
				(*Strong)(I + 1);
			});
		};
		After(1.5, [Next] { (*Next)(0); });
	};
	const auto Zoom = [this](float VerticalDeg)
	{
		if (!M || !M->Player) return;
		if (SavedFov < 0) SavedFov = M->Player->Fov;
		M->Player->Fov = VerticalDeg > 0 ? VerticalDeg : SavedFov;
	};

	// ---- PT-MOON: the moon set where it shows, at its phases, by day and behind cloud ----
	struct FMoonLine { const TCHAR* Name; const TCHAR* Label; double Hour, Az, El, Lit, Cover; };
	static const FMoonLine MoonLines[] = {
		{ TEXT("moon-half"), TEXT("Moon: half, high in the south, 21:30"), 21.5, 180, 42, .5, -1 },
		{ TEXT("moon-crescent"), TEXT("Moon: thin crescent low in the west, 19:20 (earthshine)"), 19.33, 262, 11, .12, -1 },
		{ TEXT("moon-full"), TEXT("Moon: full, rising in the east, 20:30"), 20.5, 96, 7, 1., -1 },
		{ TEXT("moon-gibbous-day"), TEXT("Moon: gibbous by day, 15:00"), 15, 120, 35, .72, -1 },
		{ TEXT("moon-clouds"), TEXT("Moon: three-quarter behind broken cloud, 22:00"), 22, 160, 32, .8, .62 },
	};
	for (const FMoonLine& L : MoonLines)
	{
		E.AddFreeform(Group, L.Label, TEXT("SARO's yard, looking at the moon: as now, close, and as the painted disc of 8 November"), [this, L, Shot, Steps, Zoom]
		{
			FStage S; S.From = L.Hour; S.To = L.Hour + .5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(7, 19); };
			Go(S, [this, L, Steps, Zoom]
			{
				FS47Sky* Sky = SkyOf(M);
				if (!Sky || !M->Player) { Tell(false, TEXT("no sky here")); return; }
				FS47Sky::DevMoon.bOn = true; FS47Sky::DevMoon.Az = L.Az; FS47Sky::DevMoon.El = L.El; FS47Sky::DevMoon.Lit = L.Lit;
				FS47Sky::DevMoon.CloudCover = L.Cover; FS47Sky::DevMoon.bOld = false;
				const FString Name = L.Name;
				const auto Aim = [this]
				{
					FVector At;
					if (FS47Sky* S2 = SkyOf(M); S2 && S2->MoonWorld(At)) AimAt(S47Three::ToThree(At));
				};
				After(.3, [this, Name, Aim, Steps, Zoom, L]
				{
					Aim();
					Steps({
						{ Name + TEXT("-now-wide"), Aim },
						{ Name + TEXT("-now-close"), [Zoom, Aim] { Zoom(6.f); Aim(); } },
						{ Name + TEXT("-before-close"), [] { FS47Sky::DevMoon.bOld = true; } },
						{ Name + TEXT("-before-wide"), [Zoom] { Zoom(-1.f); } },
						{ FString(), [Zoom] { FS47Sky::DevMoon.bOld = false; } },
					}, [this, Name, L]
					{
						FS47Sky* S3 = SkyOf(M);
						FVector At;
						const bool bDrawn = S3 && S3->MoonWorld(At);
						const bool bClouds = M->Web && M->Web->CloudsActive();
						const FString Line = FString::Printf(TEXT("%s: %s, %s, az %.0f el %.0f lit %.2f%s"), *Name, bDrawn ? TEXT("drawn") : TEXT("not drawn"),
							S3 && S3->MoonTextured() ? TEXT("textured (M_S47_Moon)") : TEXT("painted disc (M_S47_Moon not imported)"), L.Az, L.El, L.Lit,
							L.Cover >= 0 ? (bClouds ? TEXT(", clouds on") : TEXT(", volumetric clouds off here (packaged builds, or -S47EditorClouds)")) : TEXT(""));
						UE_LOG(LogS47WorldFixes, Display, TEXT("S47: PT-MOON %s %s"), bDrawn ? TEXT("PASS") : TEXT("FAIL"), *Line);
						Tell(bDrawn, Line, 10);
					});
				});
			});
		});
	}
	E.AddFreeform(Group, TEXT("Moon: tonight's own, by the date"), TEXT("SARO's yard at 23:00, looking at it if it is up"), [this, Shot]
	{
		FStage S; S.From = 23; S.To = 23.5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(7, 19); };
		Go(S, [this, Shot]
		{
			After(.5, [this, Shot]
			{
				FVector At;
				FS47Sky* Sky = SkyOf(M);
				const bool bUp = Sky && Sky->MoonWorld(At);
				if (bUp) AimAt(S47Three::ToThree(At));
				After(1.5, [this, Shot, bUp]
				{
					Shot(TEXT("moon-tonight"));
					Tell(true, bUp ? TEXT("tonight's moon, as the date has it") : TEXT("the moon is not up at 23:00 tonight"), 8);
				});
			});
		});
	});

	// ---- PT-ROCKS: each composed group by the road, from the road and from behind ----
	E.AddFreeform(Group, TEXT("Rocks: what was measured"), TEXT("which families are turned or sunk (Tools/routenature/cut_faces.py), and where each big rock stands"), [this]
	{
		FS47RouteNature* R = FS47RouteNature::Live();
		if (!R) { Tell(false, TEXT("no route nature (the private kit is not in this build)")); return; }
		for (const FS47RouteNature::FRock& K : R->Rocks())
			UE_LOG(LogS47WorldFixes, Display, TEXT("S47: PT-ROCKS %s group %d side %+.0f at %s"), *K.Family, K.Cluster, K.Side, *K.UE.ToCompactString());
		Tell(true, FString::Printf(TEXT("rocks: %s (the list in LogS47WorldFixes)"), *R->CutNote()), 10);
	});
	for (int32 G = 0; G < 7; ++G)
	{
		E.AddFreeform(Group, FString::Printf(TEXT("Rocks: group %d from the road and from behind"), G + 1),
			TEXT("16:30, on the road's edge 30 m short of the group, then out behind it: now, as before, and without ray tracing"), [this, G, Steps]
		{
			FS47RouteNature* R = FS47RouteNature::Live();
			if (!R) { Tell(false, TEXT("no route nature (the private kit is not in this build)")); return; }
			// The group's cliff (or, if the clearances took it, any of its rocks), in the road's own metres.
			FVector RoadAt = FVector::ZeroVector; double Side = 0; FString Family;
			for (const FS47RouteNature::FRock& K : R->Rocks()) if (K.Cluster == G && (Family.IsEmpty() || K.Family == TEXT("Cliff_A")))
			{
				RoadAt = K.Road; Side = K.Side; Family = K.Family;
			}
			if (Family.IsEmpty()) { Tell(false, TEXT("no rock in this group (the clearances took it)")); return; }
			const FString Area = RoadAt.Z < 100 ? FString(TEXT("saro")) : FString(TEXT("diner"));
			// on the road's edge on the rock's side, 30 m short of it; and out behind it, 22 m further from the road, a little past it
			const FVector Road = S47FarmWorld::RoadToArea(Area, FVector(Side * 3.5, 0, RoadAt.Z - 30));
			const FVector Behind = S47FarmWorld::RoadToArea(Area, FVector(RoadAt.X + Side * 22, 0, RoadAt.Z + 9));
			const FVector Rock = S47FarmWorld::RoadToArea(Area, FVector(RoadAt.X, 0, RoadAt.Z));
			FStage S; S.From = 16.5; S.To = 17; S.Area = Area; S.Where = [Road] { return FVector2D(Road.X, Road.Z); };
			Go(S, [this, G, Road, Behind, Rock, Steps, Family]
			{
				const auto LookAt = [this, Rock] { if (M->Player) AimAt(FVector(Rock.X, M->Player->FloorY + M->Player->Eye + .5, Rock.Z)); };
				const auto Stand = [this, LookAt](const FVector& At) { if (M->Player) { M->Player->Place(At.X, At.Z, 0); LookAt(); } };
				const FString N = FString::Printf(TEXT("rocks-%d"), G + 1);
				LookAt();
				Steps({
					{ N + TEXT("-road-now"), LookAt },
					{ N + TEXT("-road-before"), [] { if (auto* L = FS47RouteNature::Live()) L->DevBefore(true); } },
					{ N + TEXT("-road-nort"), [] { if (auto* L = FS47RouteNature::Live()) { L->DevBefore(false); L->DevRayTracing(false); } } },
					{ N + TEXT("-behind-nort"), [Stand, Behind] { Stand(Behind); } },
					{ N + TEXT("-behind-now"), [] { if (auto* L = FS47RouteNature::Live()) L->DevRayTracing(true); } },
					{ N + TEXT("-behind-before"), [] { if (auto* L = FS47RouteNature::Live()) L->DevBefore(true); } },
					{ FString(), [Stand, Road] { if (auto* L = FS47RouteNature::Live()) L->DevBefore(false); Stand(Road); } },
				}, [this, G, Family]
				{
					FS47RouteNature* R = FS47RouteNature::Live();
					if (!R) { Tell(false, TEXT("the route nature went away")); return; }
					const FString Line = FString::Printf(TEXT("rock group %d (%s): pictures from the road and behind, now, before and without ray tracing; %s"), G + 1, *Family, *R->CutNote());
					UE_LOG(LogS47WorldFixes, Display, TEXT("S47: PT-ROCKS %s"), *Line);
					Tell(true, Line, 10);
				});
			});
		});
	}

	// ---- PT-RANGE-FAR: the far ranges from the highway and from SARO, wide and close ----
	struct FRangeLine { const TCHAR* Name; const TCHAR* Label; const TCHAR* Area; double Hour, X, Z, LookX, LookZ, Lift; };
	static const FRangeLine RangeLines[] = {
		{ TEXT("range-west-11"), TEXT("Far range: west from the highway, 11:00"), TEXT("diner"), 11, 3, 900, -6000, 900, 500 },
		{ TEXT("range-east-11"), TEXT("Far range: east from the highway, 11:00"), TEXT("diner"), 11, -3, 900, 6000, 900, 500 },
		{ TEXT("range-west-1630"), TEXT("Far range: west from the SARO approach, 16:30"), TEXT("saro"), 16.5, 4, -120, -6000, -400, 450 },
		{ TEXT("range-east-1630"), TEXT("Far range: east from the SARO approach, 16:30"), TEXT("saro"), 16.5, -4, -120, 6000, 200, 450 },
		{ TEXT("range-south-1800"), TEXT("Far range: down the highway to the south, 18:00"), TEXT("diner"), 18, 0, 1500, 300, 9000, 300 },
	};
	for (const FRangeLine& L : RangeLines)
	{
		E.AddFreeform(Group, L.Label, TEXT("on the road, looking at the range: wide, then close (a fifth of the view)"), [this, L, Steps, Zoom]
		{
			const FString Area = L.Area;
			const FVector At = S47FarmWorld::RoadToArea(Area, FVector(L.X, 0, L.Z));
			const FVector Look = S47FarmWorld::RoadToArea(Area, FVector(L.LookX, 0, L.LookZ));
			FStage S; S.From = L.Hour; S.To = L.Hour + .5; S.Area = Area; S.Where = [At] { return FVector2D(At.X, At.Z); };
			Go(S, [this, L, Look, Steps, Zoom]
			{
				const auto Aim = [this, Look, L] { if (M->Player) AimAt(FVector(Look.X, M->Player->FloorY + M->Player->Eye + L.Lift, Look.Z)); };
				const FString N = L.Name;
				Aim();
				Steps({
					{ N + TEXT("-wide"), Aim },
					{ N + TEXT("-close"), [Zoom] { Zoom(14.f); } },
					{ FString(), [Zoom] { Zoom(-1.f); } },
				}, [this, N]
				{
					UE_LOG(LogS47WorldFixes, Display, TEXT("S47: PT-RANGE-FAR %s: pictures wide and close"), *N);
					Tell(true, FString::Printf(TEXT("%s: the range wide and close (before: run before the range material is rebuilt)"), *N), 8);
				});
			});
		});
	}
}
