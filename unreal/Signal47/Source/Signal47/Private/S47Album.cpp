// core/album.ts
#include "S47Album.h"

#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "S47Docs.h"
#include "S47FieldCamera.h"
#include "S47FilmFace.h"
#include "S47Image.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	/** Seconds of the night for an instant print to come up in the album. */
	const double DEVELOP = 90;
	const double MIN_SCORE = 0.05;
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
	double NowMs() { return FPlatformTime::Seconds() * 1000.0; }
	FVector DirToThree(const FVector& U) { return FVector(U.X, U.Z, U.Y); }
}

const TCHAR* FS47Album::NOBODY = TEXT("Nobody was there when I took it.");
const TCHAR* FS47Album::FACE_LINE = TEXT("Nobody was there when I took it.");

FString FS47Album::RollText(int32 InLeft) { return FString::Printf(TEXT("%d of %d free frames left"), InLeft, ROLL); }

FS47Album::FS47Album(FS47AlbumDeps InD) : D(MoveTemp(InD))
{
	// the same figure as the glimpses, with a little more of a rim (the film sees more than the eye),
	// and round a figure the faint pale smear the film keeps; all three under the scene, hidden
	Ghost = S47SceneObject(D.Web, TEXT("album:ghost"));
	if (D.Web)
	{
		for (int32 I = 0; I < D.Web->PartCount(); ++I)
		{
			const FS47WebNode* N = D.Web->Node(I);
			if (N && N->Parent < 0 && N->Name == TEXT("album:halo")) Halos.Add(FS47Obj(D.Web, I));
		}
	}
	ShowFilmOnly(false);
}

FS47Cam FS47Album::PhotoCam() const
{
	// fcam.photoCam: the player's eye, the photograph's lens (PHOTO_FOV, 16:10)
	FS47Cam C;
	C.Fov = US47FieldCamera::PHOTO_FOV;
	C.Aspect = double(US47FieldCamera::PHOTO_W) / US47FieldCamera::PHOTO_H;
	if (!D.Fcam) return C;
	const FRotationMatrix R(D.Fcam->PhotoRot);
	C.Pos = S47Three::ToThree(D.Fcam->PhotoLoc);
	C.Forward = DirToThree(R.GetUnitAxis(EAxis::X));
	C.Right = DirToThree(R.GetUnitAxis(EAxis::Y));
	C.Up = DirToThree(R.GetUnitAxis(EAxis::Z));
	return C;
}

void FS47Album::ShowFilmOnly(bool bOn)
{
	Ghost.SetVisible(bOn);
	for (FS47Obj& H : Halos) H.SetVisible(bOn);
}

void FS47Album::PlaceHalos(const TArray<FVector>& Figs)
{
	// the smears behind the figures in this frame (the glimpse, the one on the film only), and the
	// rest far under the ground: drawn in every free frame all the same
	const FS47Cam Cam = PhotoCam();
	for (int32 I = 0; I < Halos.Num(); ++I)
	{
		if (Figs.IsValidIndex(I))
		{
			const FVector& F = Figs[I];
			const FVector Away = FVector(F.X - Cam.Pos.X, 0, F.Z - Cam.Pos.Z).GetSafeNormal();
			Halos[I].SetPosition(FVector(F.X, F.Y + 0.62, F.Z) + Away * 0.35);
		}
		else Halos[I].SetPosition(FVector(Cam.Pos.X, -9000, Cam.Pos.Z));
		Halos[I].SetQuaternion(Cam.Quaternion());
	}
}

int32 FS47Album::Left() const { return FMath::Max(0, ROLL - Data.Shots.Num() - (D.Extra ? D.Extra() : 0)); }

FS47AlbumShot FS47Album::Expose()
{
	US47FieldCamera* F = D.Fcam;
	const double T0 = NowMs();
	if (F) F->Sync();
	const double Clock = D.Clock ? D.Clock() : 0;
	const FString Place = D.Place ? D.Place() : FString();
	const int32 N = Data.Shots.Num();
	// the face a foot from the lens, once (core/filmFace.ts); a frame that keeps it keeps nothing else
	const bool bFace = bForceFace || (!Data.Face && D.Face && D.Face());
	bForceFace = false;
	// someone at the edge of the frame, on the film only
	const double Day = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0), K = FMath::Clamp((Day - 2.5 * 3600) / (2.5 * 3600), 0.0, 1.0);
	TFunction<double(double, double)> Ground;
	if (!bFace && Data.Ghosts < 2 && (bForceGhost || (N >= 3 && FMath::FRand() < 0.05 + 0.12 * K)) && D.GhostGround) Ground = D.GhostGround();
	bForceGhost = false;
	const TOptional<FVector> GhostAt = Ground ? PlaceGhost(Ground) : TOptional<FVector>();
	TArray<FS47Subject> List = D.Subjects ? D.Subjects() : TArray<FS47Subject>();
	if (GhostAt)
	{
		FS47Subject S;
		S.Name = TEXT("someone on the film only"); S.At = { FVector(GhostAt->X, GhostAt->Y + 0.7, GhostAt->Z) }; S.R = 0.45; S.Max = 60; S.Odd = NOBODY; S.H = 1.25;
		List.Add(S);
	}
	const TArray<FSeen> Seen = InFrame(List);
	const double T1 = NowMs();
	const bool bSmall = D.Small && D.Small();
	const FS47Cam Cam = PhotoCam();
	// the figure and the smears are drawn in every free frame, far under the ground when they are not
	// in this one, so the frame that keeps one costs no more than any other (no stutter to give it away)
	if (!GhostAt) Ghost.SetPosition(FVector(Cam.Pos.X, -9000, Cam.Pos.Z));
	const bool bGlimpse = Seen.ContainsByPredicate([](const FSeen& X) { return X.S->Odd == NOBODY && X.S->Name == TEXT("someone"); });
	const FS47Obj G = bGlimpse ? S47SceneObject(D.Web, TEXT("glimpse")) : FS47Obj();
	TArray<FVector> Figs;
	if (GhostAt) Figs.Add(*GhostAt);
	if (G && G.Visible()) Figs.Add(G.Position());
	PlaceHalos(Figs);
	const double Seed = FMath::FloorToDouble(Clock * 1000) + N * 7919;
	const int32 Side = (US47FieldCamera::ToUint32(Seed) >> 4) & 1 ? 1 : -1;
	FS47SnapOptions O;
	O.Before = [this] { if (D.Before) D.Before(); ShowFilmOnly(true); };
	O.After = [this] { ShowFilmOnly(false); if (D.After) D.After(); };
	O.Seed = Seed;
	O.W = bSmall ? 320 : 640;
	if (bFace) O.Post = [Seed, Side](TArray<FColor>& Raw, int32 W, int32 H) { S47FilmFace::FilmFace(Raw, W, H, Seed, Side); };
	FS47Snap Snap;
	if (F) Snap = F->Snap(O);
	else { Snap.W = O.W; Snap.H = O.W * 400 / 640; Snap.Px.Init(FColor(0, 0, 0, 255), Snap.W * Snap.H); }
	// (view.draw: the screen is never touched by the photograph here)
	const FString Url = S47Image::ToDataUrl(Snap.Px, Snap.W, Snap.H, true, bSmall ? 62 : 74);
	TArray<FString> Odd, Names;
	for (const FSeen& X : Seen) if (!X.S->Odd.IsEmpty()) Odd.AddUnique(X.S->Odd);
	for (const FSeen& X : Seen) if (X.S->Odd.IsEmpty()) Names.Add(X.S->Name);
	if (GhostAt && Odd.Contains(NOBODY)) Data.Ghosts++;
	// "S-03 and B-12.", "Halley's comet, low in the south, and the truck."
	const FString Main = Names.Num() ? FString::Join(Names, Names[0].Contains(TEXT(",")) ? TEXT(", and ") : TEXT(" and ")) + TEXT(".") : Snap.Mean < 15 && !Odd.Num() ? FString(TEXT("Too dark. Nothing came out.")) : FString();
	// the print with the face is written about for that alone
	TArray<FString> Parts;
	if (!Main.IsEmpty()) Parts.Add(Main.Left(1).ToUpper() + Main.Mid(1));
	Parts.Append(Odd);
	FS47AlbumShot Shot;
	Shot.Id = FString::Printf(TEXT("free%d@%d"), N + 1, int32(FMath::FloorToDouble(Clock + 0.5)));
	Shot.Url = Url; Shot.Clock = Clock; Shot.Place = Place;
	Shot.Text = bFace ? FString(FACE_LINE) : FString::Join(Parts, TEXT(" "));
	if (Odd.Num() || bFace) Shot.bOdd = true;
	for (const FSeen& X : Seen)
	{
		if (X.S->Target.IsEmpty() || !F) continue;
		const FS47Projection Q = F->Project(X.P);
		Shot.Targets.RemoveAll([&X](const TPair<FString, FVector2D>& T) { return T.Key == X.S->Target; });
		Shot.Targets.Add(TPair<FString, FVector2D>(X.S->Target, FVector2D(Q.U, Q.V)));
	}
	if (D.Tag && Snap.bRendered) D.Tag(Shot);
	Data.Shots.Add(Shot);
	if (bFace) Data.Face = FS47FaceSave{ Shot.Id };
	FLast L;
	L.Names = Names; L.Odd = Odd; L.Mean = Snap.Mean; L.Ms = NowMs() - T0; L.LookMs = T1 - T0; L.bGhost = GhostAt.IsSet(); L.bFace = bFace;
	if (bFace) L.Eye = S47FilmFace::FilmFaceEye(Side);
	Last = L;
	if (D.Done) D.Done(Shot, N == 0, Left());
	return Shot;
}

TPair<TArray<FString>, TArray<FString>> FS47Album::Peek()
{
	if (D.Fcam) D.Fcam->Sync();
	const TArray<FS47Subject> List = D.Subjects ? D.Subjects() : TArray<FS47Subject>();
	const TArray<FSeen> Seen = InFrame(List);
	TPair<TArray<FString>, TArray<FString>> Out;
	for (const FSeen& X : Seen) (X.S->Odd.IsEmpty() ? Out.Key : Out.Value).Add(X.S->Odd.IsEmpty() ? X.S->Name : X.S->Odd);
	return Out;
}

void FS47Album::Restore(const S47J::FObj& V)
{
	Data = FS47AlbumSave();
	if (V.IsValid())
	{
		for (const TSharedPtr<FJsonValue>& Sv : S47J::Arr(V, TEXT("shots")))
		{
			const TSharedPtr<FJsonObject>* O = nullptr;
			if (!Sv.IsValid() || !Sv->TryGetObject(O) || !O) continue;
			FS47AlbumShot S;
			S.Id = S47J::Str(*O, TEXT("id")); S.Url = S47J::Str(*O, TEXT("url")); S.Clock = S47J::Num(*O, TEXT("clock"));
			S.Place = S47J::Str(*O, TEXT("place")); S.Text = S47J::Str(*O, TEXT("text")); S.bOdd = S47J::Bool(*O, TEXT("odd"));
			S.bUnavailable = S47J::Bool(*O, TEXT("unavailable"));
			if (const S47J::FObj T = S47J::Obj(*O, TEXT("targets")))
			{
				for (const auto& KV : T->Values)
				{
					const FString Key(*KV.Key);
					if (const TOptional<FVector2D> Uv = S47J::Uv(T, *Key)) S.Targets.Add(TPair<FString, FVector2D>(Key, *Uv));
				}
			}
			Data.Shots.Add(S);
		}
		Data.Ghosts = S47J::Int(V, TEXT("ghosts"), 0);
		if (const S47J::FObj Fc = S47J::Obj(V, TEXT("face"))) Data.Face = FS47FaceSave{ S47J::Str(Fc, TEXT("id")), S47J::Bool(Fc, TEXT("up")), S47J::Bool(Fc, TEXT("seen")) };
	}
	ShowFilmOnly(false);
}

S47J::FObj FS47Album::SaveData() const
{
	const S47J::FObj O = S47J::New();
	TArray<TSharedPtr<FJsonValue>> Shots;
	for (const FS47AlbumShot& S : Data.Shots)
	{
		const S47J::FObj J = S47J::New();
		S47J::Set(J, TEXT("id"), S.Id); S47J::Set(J, TEXT("url"), S.Url); S47J::Set(J, TEXT("clock"), S.Clock);
		S47J::Set(J, TEXT("place"), S.Place); S47J::Set(J, TEXT("text"), S.Text);
		if (S.bOdd) S47J::Set(J, TEXT("odd"), true);
		if (S.bUnavailable) S47J::Set(J, TEXT("unavailable"), true);
		if (S.Targets.Num())
		{
			const S47J::FObj T = S47J::New();
			for (const TPair<FString, FVector2D>& K : S.Targets) S47J::Set(T, *K.Key, TArray<double>{ K.Value.X, K.Value.Y });
			S47J::Set(J, TEXT("targets"), T);
		}
		Shots.Add(MakeShared<FJsonValueObject>(J));
	}
	O->SetArrayField(TEXT("shots"), Shots);
	S47J::Set(O, TEXT("ghosts"), Data.Ghosts);
	if (Data.Face)
	{
		const S47J::FObj Fc = S47J::New();
		S47J::Set(Fc, TEXT("id"), Data.Face->Id);
		if (Data.Face->bUp) S47J::Set(Fc, TEXT("up"), true);
		if (Data.Face->bSeen) S47J::Set(Fc, TEXT("seen"), true);
		S47J::Set(O, TEXT("face"), Fc);
	}
	return O;
}

double FS47Album::Dev(const FS47AlbumShot& S, double Clock) const { return FMath::Clamp((Clock - S.Clock) / DEVELOP, 0.0, 1.0); }

const FS47AlbumShot* FS47Album::FaceShot() const
{
	if (!Data.Face || Data.Face->Id.IsEmpty()) return nullptr;
	return Data.Shots.FindByPredicate([this](const FS47AlbumShot& S) { return S.Id == Data.Face->Id; });
}

const FS47AlbumShot* FS47Album::FaceUp(double Clock)
{
	const FS47AlbumShot* S = FaceShot();
	if (!Data.Face || Data.Face->bUp || !S || Dev(*S, Clock) < 1) return nullptr;
	Data.Face->bUp = true;
	return S;
}

void FS47Album::Looked(const FString& Id)
{
	const FS47AlbumShot* S = FaceShot();
	if (!Data.Face || Data.Face->bSeen || !S || S->Id != Id || Dev(*S, D.Clock ? D.Clock() : 0) < 1) return;
	Data.Face->bSeen = Data.Face->bUp = true;
	if (D.FaceSeen) D.FaceSeen(*S);
}

FS47AlbumView FS47Album::View(double Clock)
{
	// for the notebook: each print, how far it has come up by now; the caption on the print with the
	// face is written once it has come up
	FS47AlbumView V;
	const FString Face = Data.Face ? Data.Face->Id : FString();
	V.Roll = ROLL; V.Left = Left();
	for (const FS47AlbumShot& S : Data.Shots)
	{
		const double Dv = Dev(S, Clock);
		V.Shots.Add({ S.Id, S.Url, Dv < 1 && (S.Id == Face || S.Targets.ContainsByPredicate([](const auto& T){return T.Key==TEXT("anomaly:n02trace")||T.Key==TEXT("anomaly:printshadow");})) ? FString() : S.Text, FString::Printf(TEXT("%s, %s"), *S.Place, *S47Text::ClockText(S.Clock, false)), Dv, S.bOdd });
	}
	V.Look = [this](const FString& Id) { Looked(Id); };
	return V;
}

// ---------- what is in the frame ----------
// Every point of every subject in the photograph and near enough is scored by how big it looks and
// how near the middle it is; then, most prominent first, a line of sight is looked for to at most
// three of its points. Two things are named at most; the odd ones are always looked for.
TArray<FS47Album::FSeen> FS47Album::InFrame(const TArray<FS47Subject>& List) const
{
	const US47FieldCamera* F = D.Fcam;
	const FVector C = PhotoCam().Pos;
	TArray<TArray<FSeen>> By;
	for (const FS47Subject& S : List)
	{
		TArray<FSeen> Pts;
		for (const FVector& P : S.At)
		{
			const double Dist = FVector::Dist(P, C);
			if (Dist > S.Max.Get(120) || Dist < 0.25) continue;
			if (S.H && *S.H / Dist * 400 / (2 * FMath::Tan(US47FieldCamera::PHOTO_FOV * UE_DOUBLE_PI / 360)) < 4) continue;
			if (!F) continue;
			const FS47Projection Q = F->Project(P);
			if (!Q.bFront || Q.U < 0.01 || Q.U > 0.99 || Q.V < 0.01 || Q.V > 0.99) continue;
			const double Ang = S.Ang ? *S.Ang : FMath::RadiansToDegrees(FMath::Atan(S.R.Get(1) / Dist));
			const double Off = FMath::Min(1.0, FMath::Sqrt(FMath::Square((Q.U - 0.5) * US47FieldCamera::PHOTO_W / US47FieldCamera::PHOTO_H) + FMath::Square(Q.V - 0.5)) / 0.9);
			Pts.Add({ &S, P, S.W.Get(1) * FMath::Min(1.0, Ang / 8) * (1 - 0.5 * Off) });
		}
		if (Pts.Num()) { Pts.StableSort([](const FSeen& A, const FSeen& B) { return A.Score > B.Score; }); By.Add(Pts); }
	}
	By.StableSort([](const TArray<FSeen>& A, const TArray<FSeen>& B) { return A[0].Score > B[0].Score; });
	TArray<FSeen> Out;
	TSet<FString> Groups, Names;
	const FString Place = (D.Place ? D.Place() : FString()).ToLower();
	int32 Rays = 0;
	for (const TArray<FSeen>& Pts : By)
	{
		const FS47Subject& S = *Pts[0].S;
		if (S.Odd.IsEmpty())
		{
			int32 Named = 0;
			for (const FSeen& X : Out) if (X.S->Odd.IsEmpty()) Named++;
			if (Named >= 2 || Rays >= 12 || Pts[0].Score < MIN_SCORE) continue;
			if ((!S.Group.IsEmpty() && Groups.Contains(S.Group)) || Names.Contains(S.Name) || Place.Contains(S.Name.ToLower())) continue;
		}
		for (int32 I = 0; I < FMath::Min(3, Pts.Num()); ++I)
		{
			Rays++;
			if (Blocked(C, Pts[I].P, S.R.Get(1), S.bSky ? 1500 : 1e30)) continue;
			Out.Add(Pts[I]);
			if (!S.Group.IsEmpty()) Groups.Add(S.Group);
			Names.Add(S.Name);
			break;
		}
	}
	return Out;
}

bool FS47Album::Blocked(const FVector& From, const FVector& To, double R, double Cap) const
{
	// Is anything solid and shown between From and To? Stops short of the thing itself. (three's ray
	// over the shown meshes; here the world's traces, a hidden or see-through thing passed through:
	// a hidden mesh, an additive one, a transparent one under half opaque)
	UWorld* World = D.Web ? D.Web->GetWorld() : nullptr;
	if (!World) return false;
	const FVector Dir = (To - From).GetSafeNormal();
	const double Len = FMath::Min(FVector::Dist(From, To), Cap);
	const double Near = 0.2, Far = FMath::Max(0.25, Len - FMath::Max(0.5, R * 1.1));
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47AlbumSight), true);
	const FVector B = S47Three::ToUE(From + Dir * Far);
	FVector A = S47Three::ToUE(From + Dir * Near);
	for (int32 Tries = 0; Tries < 24; ++Tries)
	{
		FHitResult H;
		if (!World->LineTraceSingleByChannel(H, A, B, ECC_Visibility, Q)) return false;
		UPrimitiveComponent* C = H.GetComponent();
		bool bSolid = C && C->IsVisible();
		if (bSolid)
		{
			if (const UMaterialInterface* M = C->GetMaterial(FMath::Max<int32>(0, H.ElementIndex)))
			{
				const EBlendMode Bm = M->GetBlendMode();
				float Opacity = 1.f;
				M->GetScalarParameterValue(FName(TEXT("Opacity")), Opacity);
				if (Bm == BLEND_Additive || (Bm == BLEND_Translucent && Opacity < 0.5f)) bSolid = false;
			}
		}
		if (bSolid) return true;
		if (C) Q.AddIgnoredComponent(C);
		A = H.ImpactPoint;
	}
	return false;
}

// ---------- on the film only ----------
TOptional<FVector> FS47Album::PlaceGhost(const TFunction<double(double, double)>& Ground)
{
	// a place 14 to 34 m off near the left or right edge of the frame, on the ground, in plain sight of the camera
	const US47FieldCamera* F = D.Fcam;
	if (!F) return {};
	const FS47Cam Cam = PhotoCam();
	const FVector C = Cam.Pos;
	const double TanV = FMath::Tan(FMath::DegreesToRadians(Cam.Fov) / 2);
	for (int32 I = 0; I < 30; ++I)
	{
		const double Side = FMath::FRand() < 0.5 ? -1 : 1;
		// new Vector3(side * rnd(0.6, 0.86), 0, 0.5).unproject(cam).sub(c).setY(0).normalize()
		const FVector Ray = Cam.Forward + Cam.Right * (Side * Rnd(0.6, 0.86) * TanV * Cam.Aspect);
		const FVector Dir = FVector(Ray.X, 0, Ray.Z).GetSafeNormal();
		FVector P = C + Dir * Rnd(14, 34);
		P.Y = Ground(P.X, P.Z);
		if (!FMath::IsFinite(P.Y) || FMath::Abs(P.Y - (C.Y - 1.6)) > 5) continue;
		const FS47Projection Feet = F->Project(P), Head = F->Project(FVector(P.X, P.Y + 1.25, P.Z));
		if (!Feet.bFront || Feet.V > 0.97 || Head.V < 0.03 || Feet.U < 0.03 || Feet.U > 0.97) continue;
		if (Blocked(C, FVector(P.X, P.Y + 0.7, P.Z), 0.4)) continue;
		Ghost.SetPosition(P);
		Ghost.SetRotY(FMath::Atan2(C.X - P.X, C.Z - P.Z));   // facing the camera
		return P;
	}
	return {};
}

bool FS47Album::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s album: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	double Clock = 3 * 3600;
	FS47AlbumDeps Deps;
	Deps.Clock = [&Clock] { return Clock; };
	Deps.Place = [] { return FString(TEXT("Service yard")); };
	FS47Album A(Deps);
	Check(A.Left() == 36 && RollText(36) == TEXT("36 of 36 free frames left"), TEXT("36 free frames"));
	// a save with two prints and the face, and back
	const S47J::FObj V = S47J::Read(TEXT("{\"shots\":[{\"id\":\"free1@10800\",\"url\":\"data:image/jpeg;base64,AA==\",\"clock\":10800,\"place\":\"Service yard\",\"text\":\"The antennas.\"},")
		TEXT("{\"id\":\"free2@10850\",\"url\":\"u\",\"clock\":10850,\"place\":\"Highway south\",\"text\":\"Nobody was there when I took it.\",\"odd\":true,\"targets\":{\"starhole\":[0.5,0.25]}}],\"ghosts\":1,\"face\":{\"id\":\"free2@10850\"}}"));
	A.Restore(V);
	Check(A.Shots().Num() == 2 && A.Data.Ghosts == 1 && A.Left() == 34 && A.Shots()[1].bOdd && A.Shots()[1].Targets.Num() == 1 && A.FaceShot() && A.FaceShot()->Id == TEXT("free2@10850"), TEXT("a save put back"));
	Check(S47J::Write(A.SaveData()) == S47J::Write(V), TEXT("and written as it came"));
	// the print with the face: no caption until it has come up, said once, looked at once
	Clock = 10850 + 45;
	FS47AlbumView View = A.View(Clock);
	Check(View.Shots[1].Text.IsEmpty() && FMath::IsNearlyEqual(View.Shots[1].Dev, 0.5) && View.Shots[0].Dev == 1 && View.Shots[0].Meta == TEXT("Service yard, 03:00") && !A.FaceUp(Clock), TEXT("the face's print coming up: no caption yet"));
	int32 Seen = 0;
	A.D.FaceSeen = [&Seen](const FS47AlbumShot&) { Seen++; };
	A.Looked(TEXT("free2@10850"));
	Clock = 10850 + 90;
	Check(Seen == 0 && A.FaceUp(Clock) && !A.FaceUp(Clock) && A.View(Clock).Shots[1].Text == FACE_LINE, TEXT("come up: said once, then the caption"));
	A.Looked(TEXT("free1@10800")); A.Looked(TEXT("free2@10850")); A.Looked(TEXT("free2@10850"));
	Check(Seen == 1 && A.Data.Face->bSeen && A.SaveData()->GetObjectField(TEXT("face"))->HasField(TEXT("seen")), TEXT("looked at closely once"));
	// En erstatning beholder den faktiske bildeteksten/klokka, men får aldri bli
	// originalfilm igjen bare fordi saken passerer en kapittel-/minnerundtur.
	auto Lost = S47J::Clone(V);
	const auto LostShot = S47J::Arr(Lost,TEXT("shots"))[0]->AsObject();
	S47J::Set(LostShot,TEXT("unavailable"),true);
	FS47Album Missing(Deps); Missing.Restore(Lost);
	Check(Missing.Shots()[0].bUnavailable && !Missing.Shots()[1].bUnavailable, TEXT("missing marker restored independently of picture bytes; legacy defaults available"));
	FS47Album Reloaded(Deps); Reloaded.Restore(S47J::Read(S47J::Write(Missing.SaveData())));
	Check(Reloaded.Shots()[0].bUnavailable && Reloaded.Shots()[0].Id==A.Shots()[0].Id && Reloaded.Shots()[0].Url==A.Shots()[0].Url && Reloaded.Shots()[0].Clock==A.Shots()[0].Clock && Reloaded.Shots()[1].Targets.Num()==1,
		TEXT("missing marker and original content metadata survive album JSON roundtrip"));
	// FS47SaveStore fjerner markøren bare når den opprinnelige filen kan leses.
	auto Recovered=Reloaded.SaveData(); S47J::Arr(Recovered,TEXT("shots"))[0]->AsObject()->RemoveField(TEXT("unavailable"));
	Reloaded.Restore(Recovered);
	Check(!Reloaded.Shots()[0].bUnavailable, TEXT("successfully recovered original clears missing marker on restore"));
	return bOk;
}
