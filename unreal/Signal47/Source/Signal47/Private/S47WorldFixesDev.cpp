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
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
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

	// PT-MOON self-test on a sweep's own pictures: the luminance (Rec. 709, 0..255) of a PNG, top row first.
	bool LoadLuma(const FString& File, TArray<uint8>& Out, int32& W, int32& H)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *File, FILEREAD_Silent) || Bytes.Num() < 8) return false;
		IImageWrapperModule& IW = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		TSharedPtr<IImageWrapper> Wr = IW.CreateImageWrapper(EImageFormat::PNG);
		TArray<uint8> Raw;
		if (!Wr.IsValid() || !Wr->SetCompressed(Bytes.GetData(), Bytes.Num()) || !Wr->GetRaw(ERGBFormat::BGRA, 8, Raw)) return false;
		W = int32(Wr->GetWidth()); H = int32(Wr->GetHeight());
		if (W <= 0 || H <= 0 || Raw.Num() < W * H * 4) return false;
		Out.SetNumUninitialized(W * H);
		for (int32 K = 0; K < W * H; ++K)
		{
			const uint8* P = Raw.GetData() + K * 4; // B G R A
			Out[K] = uint8((P[2] * 54 + P[1] * 183 + P[0] * 19) >> 8);
		}
		return true;
	}

	// The disc in a close moon picture (Aim put it in the middle): the pixels of the middle window brighter than halfway from the
	// window's edge (the sky) to its brightest; how many, their spread (10th to 90th percentile), and the mask itself.
	struct FMoonDisc { int32 N = 0; int32 P10 = 0, P50 = 0, P90 = 0; TArray<bool> Mask; };
	FMoonDisc DiscOf(const TArray<uint8>& L, int32 W, int32 H)
	{
		FMoonDisc D;
		const int32 R = FMath::Min(W, H) * 45 / 100, X0 = FMath::Max(0, W / 2 - R), X1 = FMath::Min(W, W / 2 + R), Y0 = FMath::Max(0, H / 2 - R), Y1 = FMath::Min(H, H / 2 + R);
		TArray<int32> Edge, All;
		for (int32 Y = Y0; Y < Y1; ++Y) for (int32 X = X0; X < X1; ++X)
		{
			All.Add(L[Y * W + X]);
			if (Y == Y0 || Y == Y1 - 1 || X == X0 || X == X1 - 1) Edge.Add(L[Y * W + X]);
		}
		if (All.IsEmpty() || Edge.IsEmpty()) return D;
		Edge.Sort(); All.Sort();
		const int32 Sky = Edge[Edge.Num() / 2], Top = All[FMath::Min(All.Num() - 1, All.Num() * 995 / 1000)];
		if (Top - Sky < 24) return D; // nothing stands out of the sky
		const int32 Cut = (Sky + Top) / 2;
		D.Mask.SetNumZeroed(W * H);
		TArray<int32> In;
		for (int32 Y = Y0; Y < Y1; ++Y) for (int32 X = X0; X < X1; ++X)
			if (L[Y * W + X] >= Cut) { D.Mask[Y * W + X] = true; In.Add(L[Y * W + X]); }
		if (In.IsEmpty()) return D;
		In.Sort();
		D.N = In.Num(); D.P10 = In[In.Num() / 10]; D.P50 = In[In.Num() / 2]; D.P90 = In[In.Num() * 9 / 10];
		return D;
	}

	// "-now-close" against "-before-close": the textured moon and the painted disc must not be the same picture. False only when
	// both pictures are there and are the same inside the disc (the 9 November fault); Note says what was found either way.
	bool MoonPicturesDiffer(const FString& Dir, const FString& Name, FString& Note)
	{
		TArray<uint8> A, B; int32 Wa = 0, Ha = 0, Wb = 0, Hb = 0;
		if (!LoadLuma(Dir / (Name + TEXT("-now-close.png")), A, Wa, Ha) || !LoadLuma(Dir / (Name + TEXT("-before-close.png")), B, Wb, Hb) || Wa != Wb || Ha != Hb)
		{
			Note = TEXT("pictures not read (not written yet?)");
			return true;
		}
		const FMoonDisc Now = DiscOf(A, Wa, Ha), Before = DiscOf(B, Wb, Hb);
		if (Now.N == 0) { Note = TEXT("no disc found in the now-close picture"); return true; }
		// mean difference where both have the disc, 0..255 (the rim moves by a pixel between two pictures of the same moon: a thin
		// crescent of the same disc measured up to 2.7 so, the textured moon against the painted disc far more)
		double Sum = 0; int32 N = 0;
		if (Before.Mask.Num() == Now.Mask.Num())
			for (int32 K = 0; K < Wa * Ha; ++K)
				if (Now.Mask[K] && Before.Mask[K]) { Sum += FMath::Abs(int32(A[K]) - int32(B[K])); ++N; }
		if (N == 0) { Note = TEXT("no disc found in the before-close picture"); return true; }
		const double Diff = Sum / N;
		Note = FString::Printf(TEXT("now-close disc %d px, luminance p10/p50/p90 %d/%d/%d (spread %d); before-close disc %d px, %d/%d/%d (spread %d); now vs before %.1f per pixel"),
			Now.N, Now.P10, Now.P50, Now.P90, Now.P90 - Now.P10, Before.N, Before.P10, Before.P50, Before.P90, Before.P90 - Before.P10, Diff);
		return Diff >= 6.0;
	}
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
	const auto Shot = [this](const FString& Name)
	{
		if (SweepDir.IsEmpty()) return;
		FScreenshotRequest::RequestScreenshot(SweepDir / (Name + TEXT(".png")), false, false);
		// PT-MOON: what the moon's plane has on it as its picture is taken (the material and the values on it)
		if (Name.StartsWith(TEXT("moon-")))
			if (FS47Sky* S = SkyOf(M)) UE_LOG(LogS47WorldFixes, Display, TEXT("S47: PT-MOON shot %s: %s"), *Name, *S->MoonDrawn());
	};
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
			After(1.4, [this, Shot, List, Strong, I]
			{
				if (!List[I].Key.IsEmpty()) Shot(List[I].Key);
				// PT-MOON 9 November: the picture is taken when this frame is drawn, after the sky has drawn it; the next step must not
				// set up its state before then. It did (run in the same frame), and every "-now-close" moon showed the painted disc of the
				// "-before" step after it, every "-before-wide" the textured moon. A quarter second is at least one frame (S47DevFreeform::Step).
				After(.25, [Strong, I] { (*Strong)(I + 1); });
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
						// what the plane has on it now ("loaded" alone said textured while every close picture showed the painted disc)
						const bool bTexDrawn = S3 && S3->MoonDrawsTexture();
						const bool bWantTex = S3 && S3->MoonTextured();
						const FString Line = FString::Printf(TEXT("%s: %s, %s, az %.0f el %.0f lit %.2f%s"), *Name, bDrawn ? TEXT("drawn") : TEXT("not drawn"),
							bTexDrawn ? TEXT("textured (M_S47_Moon on the plane)") : bWantTex ? TEXT("M_S47_Moon loaded but NOT on the plane") : TEXT("painted disc (M_S47_Moon not imported)"),
							L.Az, L.El, L.Lit,
							L.Cover >= 0 ? (bClouds ? TEXT(", clouds on") : TEXT(", volumetric clouds off here (packaged builds, or -S47EditorClouds)")) : TEXT(""));
						if (S3) UE_LOG(LogS47WorldFixes, Display, TEXT("S47: PT-MOON drawn %s: %s"), *Name, *S3->MoonDrawn());
						// the pictures themselves (a sweep only): the close "now" must not be the painted "before"
						bool bPictures = true;
						if (!SweepDir.IsEmpty() && bWantTex)
						{
							FString Note;
							bPictures = MoonPicturesDiffer(SweepDir, Name, Note);
							UE_LOG(LogS47WorldFixes, Display, TEXT("S47: PT-MOON TEXTURE %s %s: %s"), bPictures ? TEXT("PASS") : TEXT("FAIL"), *Name,
								bPictures ? *Note : *(Note + TEXT(": now and before are the same picture, the textured moon is not what the close picture shows")));
						}
						const bool bPass = bDrawn && (!bWantTex || bTexDrawn) && bPictures;
						UE_LOG(LogS47WorldFixes, Display, TEXT("S47: PT-MOON %s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *Line);
						Tell(bPass, Line, 10);
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
