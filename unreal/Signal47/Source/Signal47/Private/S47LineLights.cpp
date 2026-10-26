// story/lineLights.ts
#include "S47LineLights.h"

#include "Components/MaterialBillboardComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "S47Docs.h"
#include "S47Lines.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	/** Along B towards STATION 01 (the transit), in SARO's terms (the same as the map's). */
	FVector TO_STATION() { const S47Lines::FStraight& B = S47Lines::LINES('B'); return FVector(-B.Dir.X, 0, -B.Dir.Y); }
	/** How far ahead along B they make for when they turn off (metres). */
	const double AHEAD = 250;
	/** PointsMaterial({ color: 0xfff4e0, size: 4, sizeAttenuation: false, ... }) */
	const uint32 COLOR = 0xfff4e0;
	const double SIZE_PX = 4;
}

FS47LineLights::FS47LineLights(AS47Web* InWeb, TFunction<void(const FString&)> InNote, TFunction<void(double)> InOnDone)
	: Note(MoveTemp(InNote)), OnDone(MoveTemp(InOnDone)), Web(InWeb), Off(TO_STATION())
{
	Base.Init(FVector::ZeroVector, 3); Shown = Base;
	UMaterialInterface* Glow = InWeb ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow")) : nullptr;
	if (!Glow) return;
	for (int32 I = 0; I < 3; ++I)
	{
		UMaterialBillboardComponent* B = NewObject<UMaterialBillboardComponent>(InWeb);
		B->SetupAttachment(InWeb->GetRootComponent());
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Glow, InWeb);
		D->SetVectorParameterValue(TEXT("Color"), FLinearColor::Black);
		B->AddElement(D, nullptr, false, 50, 50, nullptr);
		B->SetCastShadow(false);
		B->RegisterComponent();
		B->SetVisibility(false);
		Pts.Add(B);
	}
}

void FS47LineLights::SetVisible(bool bOn)
{
	bVisible = bOn;
	for (const TWeakObjectPtr<UMaterialBillboardComponent>& B : Pts) if (B.IsValid()) B->SetVisibility(bOn);
}

void FS47LineLights::Apply()
{
	// mat.opacity on an additive point: its colour times the opacity
	const FLinearColor C = S47Three::Hex(COLOR) * float(Op);
	for (int32 I = 0; I < Pts.Num(); ++I)
	{
		UMaterialBillboardComponent* B = Pts[I].Get();
		if (!B) continue;
		B->SetWorldLocation(S47Three::ToUE(Shown[I]));
		if (B->Elements.Num()) if (UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(B->Elements[0].Material)) D->SetVectorParameterValue(TEXT("Color"), FLinearColor(C.R, C.G, C.B, 1.f));
	}
}

void FS47LineLights::Draw(const FS47Cam& Cam, double ViewportH)
{
	if (!bVisible || ViewportH <= 0) return;
	// four pixels at the point's distance: 2 tan(fov / 2) * d / height (the element's size is half each way)
	for (int32 I = 0; I < Pts.Num(); ++I)
	{
		UMaterialBillboardComponent* B = Pts[I].Get();
		if (!B || !B->Elements.Num()) continue;
		const double Dist = FVector::Dist(Cam.Pos, Shown[I]);
		const float Half = float(SIZE_PX * 2 * FMath::Tan(FMath::DegreesToRadians(Cam.Fov) / 2) * Dist / ViewportH * 100 / 2);
		B->Elements[0].BaseSizeX = B->Elements[0].BaseSizeY = Half;
		B->MarkRenderStateDirty();
	}
}

void FS47LineLights::Update(double Dt, const FS47LineLightsIn& S)
{
	// Unlike the old web port, a pause is not an aborted observation and zero time cannot trigger it.
	if (S.bPaused || !FMath::IsFinite(Dt) || Dt <= 0) return;
	const TOptional<FVector>& Tr = S.Truck;
	if (State == TEXT("none"))
	{
		if (bDone || !S.bOk || !Tr) { if (Tr) LastZ = Tr->Z; return; }
		const double Dz = Tr->Z - LastZ; LastZ = Tr->Z;
		if ((Dz > 0.05 || (bAnyWay && Dz < -0.05)) && Tr->Z > 120 && Tr->Z < 240 && S.May != TOptional<bool>(false)) Begin(*Tr, S, Dz > 0 ? 1 : -1);
		return;
	}
	if (!S.bOk || !Tr) { Finish(false, S.Clock); return; }
	T += Dt;
	if (State == TEXT("pace"))
	{
		// abreast of the truck, 200 m off to the west, about twenty metres up
		const double Dz = Tr ? Tr->Z - LastZ : 0; if (Tr) LastZ = Tr->Z;
		for (FVector& B : Base) B.Z += Dz;
		Op = FMath::Min(0.95, T / 2.5);
		if (T > 16)
		{
			State = TEXT("off"); T = 0; TurnOff();
			ViewTime = 0;
		}
	}
	else
	{
		for (FVector& B : Base) B += Off * (38 * Dt);
		Op = FMath::Max(0.0, 0.95 - T / 7);
		if (T > 7.5) { Finish(true, S.Clock); return; }
	}
	for (int32 I = 0; I < Base.Num(); ++I) Shown[I] = FVector(Base[I].X, Base[I].Y + FMath::Sin(T * 0.7 + I) * 0.4, Base[I].Z);
	Apply();
	Observe(Dt, S);
}

bool FS47LineLights::Clear(const FVector& From, const FVector& To) const
{
	AS47Web* W = Web.Get();
	UWorld* World = W ? W->GetWorld() : nullptr;
	if (!World) return false;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(S47LineLightsSight), true);
	const FVector Dir = (To - From).GetSafeNormal();
	FVector A = S47Three::ToUE(From + Dir * .02);
	const FVector B = S47Three::ToUE(To - Dir * .1);
	for (int32 I = 0; I < 48; ++I)
	{
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, A, B, ECC_Visibility, Query)) return true;
		UPrimitiveComponent* C = Hit.GetComponent();
		bool bSolid = C && C->IsVisible();
		if (bSolid) if (const FS47WebNode* Node = W->Node(W->NodeOf(C)))
		{
			// Invisible interaction volumes are not opaque cover.
			if (Node->Name.StartsWith(TEXT("proxy"))) bSolid = false;
		}
		if (bSolid) if (const UMaterialInterface* Mat = C->GetMaterial(FMath::Max<int32>(0, Hit.ElementIndex)))
		{
			float Opacity = 1; Mat->GetScalarParameterValue(FName(TEXT("Opacity")), Opacity);
			if (Mat->GetBlendMode() == BLEND_Additive || (Mat->GetBlendMode() == BLEND_Translucent && Opacity < .5f)) bSolid = false;
		}
		if (bSolid) return false;
		if (!C) return false;
		Query.AddIgnoredComponent(C); A = Hit.ImpactPoint;
	}
	return false;
}

void FS47LineLights::Observe(double Dt, const FS47LineLightsIn& S)
{
	bool bDrawn = Web.IsValid() ? Pts.Num() == 3 : bool(S.Clear);
	if (Web.IsValid()) for (const auto& B : Pts) bDrawn &= B.IsValid() && B->IsVisible() && B->Elements.Num() && B->Elements[0].Material;
	bool bSeen = bDrawn && bVisible && Op > .12 && S.Camera.IsSet();
	if (bSeen) for (const FVector& P : Shown)
	{
		if (!S47Facing(*S.Camera, P, 900, 50) || !(S.Clear ? S.Clear(S.Camera->Pos, P) : Clear(S.Camera->Pos, P))) { bSeen = false; break; }
	}
	ViewTime = bSeen ? ViewTime + FMath::Min(Dt, .1) : 0;
	if (ViewTime < .3) return;
	if (State == TEXT("pace")) bPaceSeen = true;
	else { bDepartureSeen = true; if (T >= 4) bFadeSeen = true; }
	if (bObserved) return;
	bObserved = true; ObservedClock = S.Clock;
	if (OnObserved) OnObserved(S.Clock);
}

void FS47LineLights::Finish(bool bNatural, double Clock)
{
	State = TEXT("none"); Op = 0; SetVisible(false); Apply();
	bCompleted = bNatural; ViewTime = 0;
	// An unseen or obstructed occurrence remains eligible on a later pass. A real partial sighting
	// is a legitimate once-per-night event, but must not claim the unseen ending.
	if (!bObserved || bReported) return;
	bDone = true; bReported = true;
	FString Text;
	if (bPaceSeen && bDepartureSeen && bFadeSeen && bNatural) Text = TEXT("Three lights, low, west of the highway, keeping pace with the truck. Then they turned off along B and went out.");
	else if (bPaceSeen && bDepartureSeen) Text = TEXT("Three lights, low, west of the highway, kept pace with the truck and then moved away towards B. I did not see the whole departure.");
	else if (bPaceSeen) Text = TEXT("Three lights, low, west of the highway, keeping pace with the truck. I lost sight of them before I could follow their departure.");
	else Text = TEXT("Three low lights west of the highway, moving away to the south-west. I did not see how they arrived.");
	if (Note) Note(S47Text::ClockText(ObservedClock, false) + TEXT(". ") + Text);
	if (OnDone) OnDone(ObservedClock);
}

void FS47LineLights::TurnOff()
{
	// which way they go when they leave the truck: onto B and along it towards STATION 01 (from the
	// middle one: a point of B AHEAD metres on from the nearest, in SARO's terms)
	const FVector M = Base[1];
	const TOptional<FVector2D> P = S47Lines::ToMap(TEXT("saro"), M.X, M.Z);
	if (!P) { Off = TO_STATION(); return; }
	// foot('B', p): pointAt('B', alongFrom('B', p)); then AHEAD metres back towards the transit
	const FVector2D Ft = S47Lines::PointAt('B', S47Lines::AlongFrom('B', P->X, P->Y));
	const FVector2D Q = S47Lines::PointAt('B', S47Lines::AlongFrom('B', Ft.X, Ft.Y) - AHEAD);
	const TOptional<FVector2D> L = S47Lines::FromMap(TEXT("saro"), Q.X, Q.Y);
	if (!L) { Off = TO_STATION(); return; }
	Off = FVector(L->X - M.X, 0, L->Y - M.Z).GetSafeNormal();
}

TOptional<FVector> FS47LineLights::At() const
{
	if (State == TEXT("none") || !Base.Num()) return {};
	return Base[1];
}

void FS47LineLights::Begin(const FVector& Tr, const FS47LineLightsIn& S, int32 Ahead)
{
	// `ahead`: +1 driving south, -1 north; the three stand a little ahead of abreast either way
	bStarted = true; bObserved = bCompleted = bPaceSeen = bDepartureSeen = bFadeSeen = bReported = false;
	State = TEXT("pace"); T = ViewTime = ObservedClock = 0; LastZ = Tr.Z;
	for (int32 I = 0; I < 3; ++I)
	{
		const double X = Tr.X - 210 - I * 6, Z = Tr.Z + Ahead * (30 + I * 16);
		Base[I] = FVector(X, (S.Ground ? S.Ground(X, Z) : 0) + 19 + I * 1.5, Z);
	}
	Shown = Base;
	SetVisible(true); Op = 0;
	Apply();
}

bool FS47LineLights::Now(const FS47LineLightsIn& S, int32 Ahead)
{
	if (State != TEXT("none") || bDone || !S.Truck || !S.bOk || S.bPaused) return false;
	Begin(*S.Truck, S, Ahead);
	return true;
}

void FS47LineLights::Reset(bool bInDone)
{
	bDone = bInDone; bLegacyConsumed = bInDone;
	bStarted = bObserved = bCompleted = bPaceSeen = bDepartureSeen = bFadeSeen = bReported = false;
	State = TEXT("none"); SetVisible(false); bAnyWay = false;
	T = LastZ = ViewTime = ObservedClock = Op = 0; Apply();
}

S47J::FObj FS47LineLights::Save() const
{
	auto O = S47J::New();
	S47J::Set(O, TEXT("version"), 1); S47J::Set(O, TEXT("state"), State); S47J::Set(O, TEXT("t"), T);
	S47J::Set(O, TEXT("lastZ"), LastZ); S47J::Set(O, TEXT("anyWay"), bAnyWay);
	S47J::Set(O, TEXT("started"), bStarted); S47J::Set(O, TEXT("observed"), bObserved);
	S47J::Set(O, TEXT("completed"), bCompleted); S47J::Set(O, TEXT("done"), bDone);
	S47J::Set(O, TEXT("legacyConsumed"), bLegacyConsumed); S47J::Set(O, TEXT("reported"), bReported);
	S47J::Set(O, TEXT("paceSeen"), bPaceSeen); S47J::Set(O, TEXT("departureSeen"), bDepartureSeen); S47J::Set(O, TEXT("fadeSeen"), bFadeSeen);
	S47J::Set(O, TEXT("observedClock"), ObservedClock);
	S47J::Set(O, TEXT("off"), TArray<double>{Off.X, Off.Y, Off.Z});
	TArray<double> Points; for (const FVector& P : Base) { Points.Add(P.X); Points.Add(P.Y); Points.Add(P.Z); }
	S47J::Set(O, TEXT("base"), Points);
	return O;
}

void FS47LineLights::Restore(const S47J::FObj& Saved, bool bLegacyDone)
{
	Reset(bLegacyDone);
	// No migration callback: old ev:lights is preserved without manufacturing new observed credit.
	if (!Saved.IsValid() || !Saved->HasTypedField<EJson::Number>(TEXT("version")) || S47J::Num(Saved, TEXT("version")) != 1) return;
	const FString Phase = S47J::Str(Saved, TEXT("state"));
	const double Time = S47J::Num(Saved, TEXT("t"), -1), Z = S47J::Num(Saved, TEXT("lastZ"), 1e100);
	const auto Points = S47J::Arr(Saved, TEXT("base")), Direction = S47J::Arr(Saved, TEXT("off"));
	if ((Phase != TEXT("none") && Phase != TEXT("pace") && Phase != TEXT("off")) || !FMath::IsFinite(Time) || Time < 0 || Time > 16.5 || !FMath::IsFinite(Z) || FMath::Abs(Z) > 1e6 || Points.Num() != 9 || Direction.Num() != 3) return;
	TArray<double> Values;
	for (const auto& V : Points) { double N = 0; if (!V.IsValid() || V->Type != EJson::Number || !V->TryGetNumber(N) || !FMath::IsFinite(N) || FMath::Abs(N) > 1e6) return; Values.Add(N); }
	FVector Dir;
	for (int32 I = 0; I < 3; ++I) { double N = 0; if (!Direction[I].IsValid() || Direction[I]->Type != EJson::Number || !Direction[I]->TryGetNumber(N) || !FMath::IsFinite(N) || FMath::Abs(N) > 1) return; Dir[I] = N; }
	if (Dir.IsNearlyZero()) return;
	bLegacyConsumed = bLegacyDone || S47J::Bool(Saved, TEXT("legacyConsumed"));
	bStarted = S47J::Bool(Saved, TEXT("started"));
	bPaceSeen = S47J::Bool(Saved, TEXT("paceSeen")); bDepartureSeen = S47J::Bool(Saved, TEXT("departureSeen")); bFadeSeen = S47J::Bool(Saved, TEXT("fadeSeen"));
	ObservedClock = S47J::Num(Saved, TEXT("observedClock"), -1);
	bObserved = bStarted && S47J::Bool(Saved, TEXT("observed")) && (bPaceSeen || bDepartureSeen) && FMath::IsFinite(ObservedClock) && ObservedClock >= 0;
	bCompleted = bStarted && Phase == TEXT("none") && S47J::Bool(Saved, TEXT("completed"));
	bDone = bLegacyConsumed || (Phase == TEXT("none") && bObserved && S47J::Bool(Saved, TEXT("done")));
	bReported = bDone && S47J::Bool(Saved, TEXT("reported"));
	bAnyWay = S47J::Bool(Saved, TEXT("anyWay"));
	LastZ = Z;
	if (bDone || Phase == TEXT("none") || !bStarted) return;
	State = Phase; T = Time; LastZ = Z; Off = Dir.GetSafeNormal();
	for (int32 I = 0; I < 3; ++I) Base[I] = FVector(Values[I * 3], Values[I * 3 + 1], Values[I * 3 + 2]);
	Shown = Base; ViewTime = 0;
	Op = State == TEXT("pace") ? FMath::Min(.95, T / 2.5) : FMath::Max(0.0, .95 - T / 7);
	SetVisible(true); Apply();
}

bool FS47LineLights::SelfTest(TArray<FString>& Lines)
{
 bool bOk = true;
 auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s line lights: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
 TArray<FString> Notes; int32 Done = 0, Observations = 0;
 auto Note = [&](const FString& Text) { Notes.Add(Text); };
 auto Complete = [&](double) { ++Done; };
 auto Watched = [&](double) { ++Observations; };
 FS47LineLights L(nullptr, Note, Complete); L.OnObserved = Watched;
 FS47LineLightsIn S; S.bOk = true; S.Clock = 3 * 3600; S.Ground = [](double, double) { return 0.0; };
 bool bBlocked = false;
 S.Clear = [&](const FVector&, const FVector&) { return !bBlocked; };
 auto Aim = [&](const FS47LineLights& Lights, bool bLook = true)
 {
  FS47Cam C; C.Pos = S.Truck.Get(FVector(-24,0,130)) + FVector(0,1.5,0);
  C.Forward = (Lights.At().Get(FVector(-240,20.5,176)) - C.Pos).GetSafeNormal() * (bLook ? 1 : -1);
  C.Right = FVector::CrossProduct(C.Forward,FVector(0,1,0)).GetSafeNormal();
  C.Up = FVector::CrossProduct(C.Right,C.Forward).GetSafeNormal(); S.Camera = C;
 };
 auto Run = [&](FS47LineLights& Lights, int32 Frames, bool bLook = true)
 {
  for (int32 I=0;I<Frames;++I) { if(S.Truck) S.Truck->Z += 1; Aim(Lights,bLook); Lights.Update(.1,S); }
 };
 S.Truck = FVector(-24,0,100); L.Update(.1,S); S.Truck->Z = 130; L.Update(.1,S);
 Check(L.bStarted && !L.bDone && !L.bObserved && !L.bCompleted && L.State==TEXT("pace"),TEXT("starting a candidate does not consume it or fabricate an observation"));
 Check(L.At() && FMath::IsNearlyEqual(L.At()->X,-240.0) && FMath::IsNearlyEqual(L.At()->Y,20.5),TEXT("original three-point placement west of SARO is preserved"));
 const FString BeforePause=S47J::Write(L.Save()); S.bPaused=true;S.bOk=false;L.Update(20,S);
 Check(S47J::Write(L.Save())==BeforePause && Done==0 && Observations==0,TEXT("a modal freezes position, time and unobserved opportunity"));
 S.bPaused=false;S.bOk=true;Run(L,245,false);
 Check(L.bCompleted && !L.bObserved && !L.bDone && Notes.IsEmpty() && Done==0 && Observations==0,TEXT("an entirely offscreen completed occurrence has no note, mark or consumed flag"));
 Check(L.Now(S),TEXT("the missed occurrence remains eligible on a later legitimate pass"));
 bBlocked=true;Run(L,12);
 Check(!L.bObserved && Observations==0,TEXT("looking towards three occluded points produces no observed event"));
 const auto Unseen=S47J::Read(S47J::Write(L.Save()));
 FS47LineLights UnseenRestore(nullptr,Note,Complete);UnseenRestore.OnObserved=Watched;UnseenRestore.Restore(Unseen);
 Check(UnseenRestore.bStarted && !UnseenRestore.bObserved && !UnseenRestore.bDone && UnseenRestore.State==TEXT("pace") && FVector::Dist(UnseenRestore.At().Get(FVector::ZeroVector),L.At().Get(FVector::ZeroVector))<.001,TEXT("Continue restores an unseen active occurrence and its actual world positions"));
 S.Truck->Z=150;L.Update(.1,S);S.Truck.Reset();L.Update(.1,S);
 Check(L.State==TEXT("none") && !L.bCompleted && !L.bDone && Notes.IsEmpty() && Done==0,TEXT("losing the truck before any observation quietly releases the opportunity"));
 FS47LineLights IdleRestore(nullptr,Note,Complete);IdleRestore.Restore(S47J::Read(S47J::Write(L.Save())));
 S.Truck=FVector(-24,0,150);IdleRestore.Update(.1,S);
 Check(IdleRestore.State==TEXT("none") && !IdleRestore.bDone,TEXT("Continue after an unseen abort requires new driving rather than replaying beside a stationary truck"));
 S.Truck=FVector(-24,0,150);bBlocked=false;Check(L.Now(S),TEXT("an aborted unseen encounter can start again"));
 Run(L,12);
 Check(L.bObserved && !L.bDone && !L.bCompleted && Observations==1 && Notes.IsEmpty(),TEXT("sustained unobstructed camera sight records the real encounter without inventing its ending"));
 const auto Seen=S47J::Read(S47J::Write(L.Save()));
 FS47LineLights Continued(nullptr,Note,Complete);Continued.OnObserved=Watched;Continued.Restore(Seen);
 Check(Continued.bObserved && Continued.State==TEXT("pace") && Observations==1 && Done==0,TEXT("Continue keeps a real mid-sequence sighting without repeating observation callbacks"));
 const FString SeenPaused=S47J::Write(Continued.Save()); S.bPaused=true;S.bOk=false;S.Truck.Reset();Continued.Update(20,S);
 Check(S47J::Write(Continued.Save())==SeenPaused && Done==0 && Notes.IsEmpty(),TEXT("a panel after sighting does not finish the sequence or discard the truck"));
 S.bPaused=false;S.bOk=true;Continued.Update(.1,S);
 Check(Continued.bDone && !Continued.bCompleted && Notes.Num()==1 && Done==1 && !Notes.Last().Contains(TEXT("Then they turned off")) && Notes.Last().Contains(TEXT("lost sight")),TEXT("a real partial sighting survives loss of truck as an honest once-only partial note"));
 S.Truck=FVector(-24,0,150);Check(!Continued.Now(S),TEXT("a consumed legitimate sighting is not repeated"));
 FS47LineLights FinishedRestore(nullptr,Note,Complete);FinishedRestore.OnObserved=Watched;FinishedRestore.Restore(S47J::Read(S47J::Write(Continued.Save())));
 Check(FinishedRestore.bDone && FinishedRestore.bObserved && !FinishedRestore.Now(S) && Notes.Num()==1 && Done==1,TEXT("Continue after a partial conclusion neither duplicates the note nor restarts it"));
 FS47LineLights Legacy(nullptr,Note,Complete);Legacy.OnObserved=Watched;Legacy.Restore(nullptr,true);
 Check(Legacy.bDone && !Legacy.bObserved && !Legacy.Now(S) && Observations==1,TEXT("legacy consumed flags remain consumed without manufacturing new observation evidence"));
 const auto Bad=S47J::Clone(Unseen);S47J::Set(Bad,TEXT("version"),true);Legacy.Restore(Bad,false);
 Check(!Legacy.bDone && !Legacy.bStarted && Legacy.State==TEXT("none"),TEXT("a boolean masquerading as the save version is rejected"));
 S47J::Set(Bad,TEXT("version"),1);S47J::Set(Bad,TEXT("base"),TArray<double>{1,2,3});Legacy.Restore(Bad,false);
 Check(!Legacy.bDone && !Legacy.bStarted,TEXT("malformed point geometry cannot restore an encounter or give evidence"));
 Notes.Reset();Done=Observations=0;L.Reset(false);L.Now(S);Run(L,12);
 const auto Mid=S47J::Read(S47J::Write(L.Save()));
 FS47LineLights Whole(nullptr,Note,Complete);Whole.OnObserved=Watched;Whole.Restore(Mid);
 Run(Whole,150);Check(Whole.State==TEXT("off") && Notes.IsEmpty() && Done==0,TEXT("reaching departure does not claim that lights have already gone out"));
 const FVector A=Whole.At().Get(FVector::ZeroVector);Run(Whole,10);const FVector B=Whole.At().Get(FVector::ZeroVector);
 Check(FVector::DotProduct((B-A).GetSafeNormal(),TO_STATION())>.5 && FMath::IsNearlyEqual((B-A).Size(),38.0,.01),TEXT("the original departure follows B towards STATION 01 at 38 metres per second"));
 Run(Whole,80);
 Check(Whole.State==TEXT("none") && Whole.bDone && Whole.bCompleted && Whole.bObserved && Notes.Num()==1 && Done==1 && Observations==1 && Notes.Last().Contains(TEXT("Then they turned off along B and went out.")),TEXT("only an observed pace, departure and fading conclusion writes the full original sequence"));
 Whole.Update(1,S);Check(Notes.Num()==1 && Done==1,TEXT("idle updates cannot duplicate a completed observation"));
 return bOk;
}
