// story/Chapter6.ts: Roswell Road and the end of the night.
#include "S47Chapter6.h"
#include <limits>

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Misc/CommandLine.h"
#include "S47Away.h"
#include "S47CB.h"
#include "S47Chapter4.h"
#include "S47Chapter5.h"
#include "S47Flow.h"
#include "S47FinaleSky.h"
#include "S47HUD.h"
#include "S47Image.h"
#include "S47Main.h"
#include "S47TheThird.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47UI.h"
#include "S47Walkie.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47Ending.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Ch6, Log, All);

namespace
{
	constexpr double T = 5 * 3600 + 29 * 60, LEAVE = 5 * 3600 + 20 * 60;
	constexpr double PACE = 24.6, EVENT = 47, PULL = 34;
	constexpr double LAST_LOOK_AZ = 31, LAST_LOOK_EL = 48;
	const TArray<TPair<double, FString>> RADIO = {
		{ 0.15, TEXT("The radio, low: \"...cattle at the sale barn steady, a dollar higher on calves...\"") },
		{ 1.4, TEXT("The radio: \"...clear today, high near seventy-four in Roswell, light winds out of the southwest...\"") },
		{ 2.9, TEXT("The radio: \"...road crews on the highway north of town all week, so give yourself a few extra minutes...\"") },
		{ 4.3, TEXT("The radio: \"...and Halley's comet is still low in the south after midnight this week, if you can get away from the town lights.\"") },
		{ 5.5, TEXT("The radio: \"...if you're out on the county roads this early, watch the low spots, there's deer...\"") },
	};
	double Sm(double A, double B, double X) { const double V = FMath::Clamp((X - A) / (B - A), 0.0, 1.0); return V * V * (3 - 2 * V); }
	FVector Dir(double Deg) { const double R = FMath::DegreesToRadians(Deg); return FVector(FMath::Sin(R), 0, -FMath::Cos(R)); }
	const FVector S47_UP(0, 1, 0); // UP in Chapter6.ts; UP is an Unreal namespace.
	const FLinearColor HEADS[] = { { 0.12f, 0.25f, 1.0f }, { 1.0f, 0.55f, 0.12f }, { 0.2f, 1.0f, 0.35f }, { 1.0f, 0.08f, 0.05f } };
	struct FThirdSpec { double Until = 32, Ahead = 9, LetGo = 1.2; double Flares[4] = { 11.0, 12.2, 14.4, 16.6 }; double Brake = 3.2, React = 0.8, Near = 3, Far = 120; };
	const FThirdSpec THIRD;
	struct FLastMileSpec
	{
		double Lead = 47, Least = 40, Room = 200, FromMi = 7.0, Before = 6.95, Tau = 1.5, Front = 2.3;
		struct FKey { double From = 1.5, Until = 31.4, First = 0.15, Breath = 0.6, Again = 0.8; int32 Said = 4; double Lamp = 0.14; } Key;
		double Door = 0.6; int32 DoorSaid = 3;
		double Words[2] = { 23.6, 30.0 };
	};
	const FLastMileSpec LAST_MILE;
	struct FDash { double From = 4, Until = 10.8; };
	const FDash DASH;
	struct FCbSay { double At; const TCHAR* Text; double Secs; };
	const FCbSay CB_SAYS[] = {
		{ 0.3, TEXT("The CB opens by itself. Nobody calls."), 3.2 },
		{ 1.7, TEXT("On the channel, small and a long way off, an engine idling. It sounds like yours."), 5.6 },
		{ 8.0, TEXT("It coughs twice and dies. Yours keeps running."), 4.6 },
		{ 9.8, TEXT("The channel stays open on nothing at all. Then the squelch shuts."), 3.8 },
	};
	const FLinearColor HEAD_WHITE(1.0f, 0.9f, 0.72f), THIRD_AMB(0.035f, 0.038f, 0.048f);
	FS47Sound Snd(float Gain, float Rate = 1.f) { FS47Sound O; O.Gain = Gain; O.Rate = Rate; return O; }

	// three Matrix4.lookAt / Quaternion.setFromRotationMatrix, entirely in three space.
	FQuat LookAt(const FVector& Eye, const FVector& Target)
	{
		FVector Z = (Eye - Target).GetSafeNormal();
		if (Z.IsNearlyZero()) Z.Z = 1;
		FVector X = FVector::CrossProduct(S47_UP, Z).GetSafeNormal();
		if (X.IsNearlyZero()) { Z.Z += 0.0001; Z.Normalize(); X = FVector::CrossProduct(S47_UP, Z).GetSafeNormal(); }
		const FVector Y = FVector::CrossProduct(Z, X);
		double Mat[16] = { X.X, X.Y, X.Z, 0, Y.X, Y.Y, Y.Z, 0, Z.X, Z.Y, Z.Z, 0, 0, 0, 0, 1 };
		FVector P, S; FQuat Q; S47Three::Decompose(Mat, P, Q, S); return Q;
	}
	void ViewOf(const FVector& Eye, const FQuat& Q, FS47View& Out)
	{
		// FS47View carries Euler YXZ instead of a camera quaternion.
		double E[16]; S47Three::Compose(FVector::ZeroVector, Q, FVector::OneVector, E);
		Out.Eye = Eye;
		Out.Pitch = FMath::Asin(-FMath::Clamp(E[9], -1.0, 1.0));
		Out.Yaw = FMath::Abs(E[9]) < 0.9999999 ? FMath::Atan2(E[8], E[10]) : FMath::Atan2(-E[2], E[0]);
		Out.Roll = FMath::Abs(E[9]) < 0.9999999 ? FMath::Atan2(E[1], E[5]) : 0;
	}
	bool DashOn(double E)
	{
		const double V = E - DASH.From;
		return V < 2 ? FMath::Fmod(V, 0.5) < 0.25 : V >= 3.2 && V < 6.7 ? FMath::Fmod(V - 3.2, 0.5) < 0.25 : false;
	}
	bool RevisedDeparture(const S47J::FObj& Ch5)
	{
		const auto R = S47J::Obj(Ch5, TEXT("sc12"));
		return S47J::Int(R,TEXT("v")) == 1 && S47J::Bool(Ch5,TEXT("p13")) && S47J::Bool(R,TEXT("compared"))
			&& S47J::Int(R,TEXT("read")) == 7 && S47J::Int(R,TEXT("checks")) == 7 && S47J::Bool(R,TEXT("warningSent"))
			&& S47J::Bool(R,TEXT("patrolRead")) && S47J::Bool(R,TEXT("courier104")) && S47J::Bool(R,TEXT("routeReadback"));
	}
}

US47Chapter6* US47Chapter6::Register()
{
	if (!M || !Host) return this;
	TWeakObjectPtr<US47Chapter6> W = this;
	Host->Jumps.Add(TEXT("chapter6"), [W] { if (W.IsValid()) W->JumpChapter6(); });
	Host->Jumps.Add(TEXT("chapter6:lastmile"), [W] { if (W.IsValid()) W->JumpChapter6(TEXT("lastmile")); });
	Host->Jumps.Add(TEXT("chapter6:line"), [W] { if (W.IsValid()) W->JumpChapter6(TEXT("line")); });
	return this;
}

double US47Chapter6::Clock() const { return Host ? Host->Clock() : TestClock; }
void US47Chapter6::SetClock(double C) { if (Host) Host->SetClock(Host->bEveningTimeline ? FMath::Max(Host->Clock(), C) : C); else TestClock = C; }
FString US47Chapter6::Time() const { return S47Text::ClockText(Clock(), false); }
void US47Chapter6::Toast(const FString& Text, float Seconds) const { if (M && M->UI) M->UI->Toast(Text, Seconds); }
void US47Chapter6::Note(const FString& Text) const { if (Host) Host->Note(Text); }

// ---------- lifecycle ----------
void US47Chapter6::Begin(const S47J::FObj& Saved)
{
	Reset(); bActive = true; bStarted = true; Stage = TEXT("drive");
	const auto* C5 = Host ? Host->Chapter(TEXT("ch5")) : nullptr;
	bRevisedSC13 = Host && Host->bEveningTimeline && C5 && RevisedDeparture(C5->SaveState());
	// This is the existing accepted diner-truck choice, before the ordinary trip starts.
	// Selecting SARO 07 changes neither a vehicle pose nor Evening's parked personal car.
	if (bRevisedSC13 && (!M || !M->World || !M->World->SelectServiceForOldRoad()))
	{
		bActive = bStarted = false; Stage = TEXT("blocked-service");
		UE_LOG(LogS47Ch6, Error, TEXT("S47: SC13 departure rejected: SARO 07 must be selected at the diner before the old-road trip"));
		Toast(TEXT("Departure blocked: select SARO 07 at the diner. The safe departure checkpoint is unchanged."), 7);
		return;
	}
	if (bRevisedSC13) FinaleSky = MakeShared<FS47FinaleSky>(M.Get());
	// Art is local in Unreal. Decode the photograph now and draw FLASH before the moment needs it.
	S47Image::Art(TEXT("photo1947"));
	if (M && M->Audio) M->Audio->AmRadioThree({});
	TWeakObjectPtr<US47Chapter6> W = this;
	if (M && M->World) M->World->DriveOldRoad([W]
	{
		if (!W.IsValid() || !W->bActive) return;
		W->SetClock(W->Host && W->Host->bEveningTimeline ? FMath::Max(W->Clock(), LEAVE) : LEAVE);
		if (W->M->Audio) W->M->Audio->CabRadio(TEXT("talk"));
		W->Note(W->Time() + (W->bRevisedSC13
			? TEXT(". The warning is in Ward's log. The originals are going to the confirmed courier at 10.4. Across the highway, take the old road east-north-east. Pass C at 8.15 without stopping or taking a reading. The patrol was one drive at 04:15, not a promise about the road now.")
			: TEXT(". Across the highway from the diner the old road goes off east-north-east, into the grey coming up in the east. C crosses it just past the eight-mile post.")));
		if (W->FinaleSky) W->FinaleSky->Prepare();
		W->PlaceJump();
	});
	if (Host) Host->After(3, [W] { if (W.IsValid() && W->bActive && !W->Frames) W->DrawFrames(); });
}

void US47Chapter6::DrawFrames()
{
	Frames = S47FlashFrames::FlashFrames(Host && Host->bEveningTimeline); FrameTextures.Reset();
	for (const S47FlashFrames::FFrame& F : *Frames) FrameTextures.Add(S47Image::FromPixels(F.Canvas.Colors(), F.Canvas.W, F.Canvas.H));
}

void US47Chapter6::Reset()
{
	bActive = false; bStarted = false; Stage = TEXT("drive");
	FinaleSky.Reset(); bRevisedSC13 = false;
	Et = 0; Pt = 0; Done.Empty(); Idle = 0; BackT = 0; StopS.Reset();
	if (LastMile.Cb && !LastMile.ClosedAt && M && M->CB) M->CB->Stop();
	LastMile = FLastMile(); Tries = FTries(); Cues.Empty(); KeyLamps.Reset();
	SkyLift.Reset(); SunElev.Reset();
	if (Ending) Ending->Dispose(); Ending.Reset();
	if (M && M->Away) M->Away->HideFrame();
	if (M && M->World) M->World->OldControl.Reset();
	if (M && M->Web) { M->Web->SetChapter6Fog({}); M->Web->SetDawnLift({}); M->Web->SetSunElevation({}); }
	if (M && M->Audio) { M->Audio->CabRadio(TEXT("")); M->Audio->Silence(false); }
	Bed.Stop(); Bed = FS47Pressure(); DropThird(); bJumpPlaced = false; JumpAt.Reset();
}

void US47Chapter6::BeginDestroy()
{
	// Native resources may outlive Main's scene tables during editor shutdown.
	if (Ending) Ending->Dispose(); Ending.Reset(); Bed.Stop(); Bed = FS47Pressure(); Third.Reset(); FinaleSky.Reset();
	Super::BeginDestroy();
}

void US47Chapter6::DropThird()
{
	if (Third) Third->RemoveFromScene(); Third.Reset(); bThirdShown = false; ThirdGlints(nullptr);
}

void US47Chapter6::ThirdGlints(const TArray<FVector>* Pts, const FLinearColor& Col)
{
	// OldRoad.thirdGlints: its constructor appends these seven points last, before glow.build().
	if (!ThirdGlow.IsValid() && M && M->Web)
	{
		ThirdGlow = FS47GlowView(M->Web, TEXT("world.oldRoad.glow.points"));
		if (ThirdGlow.Num() >= 7) for (int32 I = ThirdGlow.Num() - 7; I < ThirdGlow.Num(); ++I) ThirdIds.Add(I);
	}
	for (int32 I = 0; I < ThirdIds.Num(); ++I)
	{
		const bool bOn = Pts && Pts->IsValidIndex(I);
		if (bOn) ThirdGlow.SetPos(ThirdIds[I], (*Pts)[I]);
		ThirdGlow.SetCol(ThirdIds[I], bOn ? Col : FLinearColor::Black);
	}
}

TOptional<FVector> US47Chapter6::ThirdAt() const { return Third && bThirdShown ? TOptional<FVector>(Third->Head()) : TOptional<FVector>(); }
FString US47Chapter6::Objective() const { return Stage == TEXT("drive") ? bRevisedSC13 ? TEXT("OLD ROSWELL ROAD // ORIGINALS TO THE COURIER AT 10.4. PASS C AT 8.15 WITHOUT TAKING A READING") : TEXT("THE OLD ROSWELL ROAD // C CROSSES JUST PAST MILE 8") : FString(); }
TArray<FS47Task> US47Chapter6::Tasks() const { return {{ bRevisedSC13 ? TEXT("Carry the originals to the courier at 10.4; pass C without a reading") : TEXT("Drive out to where C crosses the old road, just past mile 8"), !bRevisedSC13 && Stage != TEXT("drive") }}; }

// ---------- the drive ----------
void US47Chapter6::Drive(double Dt)
{
	US47World* W = M ? M->World.Get() : nullptr;
	FS47OldRoad* Road = W ? W->OldRoad() : nullptr; FS47DriveController* Drv = W ? W->OldDrive() : nullptr;
	if (!Road || !Drv || W->Area != TEXT("roswell")) return;
	const FS47OldRoad::FWhere At = Road->Where(Drv->Pos.X, Drv->Pos.Z);
	const double Pace = T - FMath::Max(0.0, -At.S) / PACE;
	if (!LastMileStep(Dt, At, Drv->Speed))
	{
		SetClock(Host && Host->bEveningTimeline ? S47Timeline::ApproachClock(Clock(), Pace, LastMile.bSkipped) : FMath::Clamp(Clock(), Pace - 60, Pace));
		const double Left = T - Clock();
		if (Left < 20 && !(Host && Host->bEveningTimeline)) SetClock(Clock() - Dt * (1 - FMath::Max(0.02, Left / 20)));
	}
	SetClock(FMath::Min(Clock(), T - 0.05));
	const double H = Drv->Heading, Fx = Drv->Pos.X - FMath::Sin(H) * 2.3, Fz = Drv->Pos.Z - FMath::Cos(H) * 2.3;
	if (Road->PastC(Fx, Fz) >= 0) { StartEvent(); return; }
	if (Road->bHitBack) { Road->bHitBack = false; if (BackT <= 0) { Toast(bRevisedSC13 ? TEXT("The handover is at 10.4, beyond C. The originals stay together.") : TEXT("Ward can wait. The bolt is ahead."), 3.4f); BackT = 6; } }
	BackT -= Dt;
	Idle = FMath::Abs(Drv->Speed) < 0.5 ? Idle + Dt : 0;
	for (const auto& R : RADIO) if (At.Mi >= R.Key && !Done.Contains(R.Value)) { Done.Add(R.Value); if (At.Mi < R.Key + 0.4) Toast(R.Value, 5); }
	if (Idle > 40 && At.Mi > 7.6 && !Done.Contains(TEXT("nudge"))) { Done.Add(TEXT("nudge")); Toast(bRevisedSC13 ? TEXT("C is ahead at 8.15. Keep going. The courier is beyond it at 10.4.") : TEXT("The line is just ahead, past the eight-mile post."), 4); }
	if (Idle > 40 && At.Mi < 0.2 && !Done.Contains(TEXT("across"))) { Done.Add(TEXT("across")); Toast(TEXT("The old road is across the highway, going off to the east."), 4); }
}

// ---------- the last mile ----------
bool US47Chapter6::LastMileStep(double Dt, const FS47OldRoad::FWhere& At, double Speed)
{
	FLastMile& L = LastMile; const FLastMileSpec& Spec = LAST_MILE;
	const double Rem = FMath::Max(0.0, -At.S - Spec.Front), V = FMath::Max(0.0, Speed);
	L.Vs = L.Vs < 0 ? V : L.Vs + (V - L.Vs) * (1 - FMath::Exp(-Dt / Spec.Tau));
	if (At.Mi < Spec.Before) L.bApproach = true;
	L.Eta = L.Vs > 0.5 ? Rem / L.Vs : std::numeric_limits<double>::infinity();
	if (!L.Cb && !L.bSkipped && L.bApproach && At.Mi >= Spec.FromMi && L.Vs > 0.5 && Rem / L.Vs <= Spec.Lead)
	{
		if (Rem / L.Vs < Spec.Least || Rem < Spec.Room) L.bSkipped = true;
		else OpenCB(Rem);
	}
	if (L.Cb && !L.ClosedAt)
	{
		const double Was = *L.Cb; *L.Cb += Dt;
		for (const FCbSay& C : CB_SAYS) if (Was < C.At && *L.Cb >= C.At) Toast(C.Text, float(C.Secs));
		if (*L.Cb >= S47CB::ENGINE().End) CloseCB();
	}
	if (!L.D0) return false;
	L.Clock = FMath::Max(L.Clock, T - Spec.Lead * FMath::Min(1.0, Rem / L.D0)); SetClock(L.Clock); return true;
}

void US47Chapter6::OpenCB(double Rem)
{
	FLastMile& L = LastMile;
	L.Cb = 0; L.D0 = Rem; L.Clock = T - LAST_MILE.Lead; L.OpenAt = L.Clock;
	if (M) UE_LOG(LogS47Ch6, Display, TEXT("S47: ch6 CB opens at 05:28:13, %.3f metres left"), Rem);
	if (M && M->Audio)
	{
		US47Audio* A = M->Audio;
		// engineSound.ts uses Audio's decoded idle recording in this port.
		if (A->Ctx && M->CB) M->CB->Engine(A->Buf.FindRef(TEXT("engine_idle")), A->SfxBus);
		A->CabLevel(0.4f);
	}
}

void US47Chapter6::CloseCB()
{
	FLastMile& L = LastMile; L.ClosedAt = FMath::FloorToDouble(Clock());
	if (M) UE_LOG(LogS47Ch6, Display, TEXT("S47: ch6 CB shuts at %s after %.3f seconds"), *S47Text::ClockText(Clock(), true), L.Cb.Get(0));
	if (M && M->CB) M->CB->Stop();
	if (M && M->Audio) M->Audio->CabLevel(1);
	if (L.Cb && *L.Cb >= S47CB::ENGINE().Dead) Note(Time() + TEXT(". Past the seven-mile post the CB opened by itself. Nobody on it, only an engine a long way off, like this one. It coughed twice and died. This one kept running."));
}

bool US47Chapter6::CbDue() const
{
	const FLastMile& L = LastMile;
	if (Stage != TEXT("drive") || L.bSkipped || L.ClosedAt) return false;
	return L.Cb.IsSet() || (L.bApproach && L.Eta < LAST_MILE.Lead + 8);
}

FString US47Chapter6::ClockShow() const
{
	if (!LastMile.D0 || !(Stage == TEXT("drive") || Stage == TEXT("event"))) return FString();
	const int32 C = FMath::FloorToInt(Clock() + 1e-6);
	return S47Text::ClockText(C, false) + FString::Printf(TEXT(":%02d"), ((C % 60) + 60) % 60);
}

// ---------- the key and the door, in THE EVENT ----------
bool US47Chapter6::WantsKey() const
{
	const auto& K = LAST_MILE.Key; const double E = Et;
	if (Tries.Key == 0 && E >= DASH.From && E < DASH.Until) return false;
	return Stage == TEXT("event") && E >= K.From && E < K.Until && E >= Tries.KeyFree;
}
bool US47Chapter6::DoorOffered() const { const auto& K = LAST_MILE.Key; return Stage == TEXT("event") && Et >= K.From && Et < K.Until; }

bool US47Chapter6::TurnKey()
{
	if (Stage != TEXT("event")) return false;
	if (!WantsKey()) return true;
	const auto& K = LAST_MILE.Key; US47Audio* A = M ? M->Audio.Get() : nullptr;
	FS47EngineSound* Eng = M && M->World ? M->World->EngineSound() : nullptr;
	const int32 N = ++Tries.Key; const double E = Et;
	auto Say = [this, N](const FString& Text, float Secs) { if (!OnWords() && N <= LAST_MILE.Key.Said) Toast(Text, Secs); };
	if (A) A->Sfx(A->Or(TEXT("keys")), Snd(0.32f));
	if (N == 1)
	{
		const TArray<double> Clicks = S47Walkie::FourSeven(K.First); const double Four = Clicks[3], Seven = Clicks[10];
		if (Eng) Eng->Solenoid(Clicks);
		if (E < DASH.From) { KeyLamps = Clicks; for (double& C : *KeyLamps) C += E; }
		Tries.KeyFree = E + Seven + K.Breath + 0.4;
		Cue(E + Four + 0.05, [Say] { Say(TEXT("You turn the key. Under the hood the solenoid clicks, four times."), 3.4f); });
		Cue(E + Seven + 0.05, [Say] { Say(TEXT("A pause. Then seven. Nothing turns over."), 3.4f); });
		Cue(E + Seven + K.Breath, [this, Say]
		{
			US47Audio* Audio = M ? M->Audio.Get() : nullptr;
			if (Audio && Audio->Ctx && Audio->NoiseBuffer())
			{
				const auto Ctx = Audio->Ctx.ToSharedRef(); FS47AudioContext::FBatch B(*Ctx);
				const double At = Ctx->CurrentTime();
				S47Walkie::Breath(Ctx, Audio->SfxBus, Audio->NoiseBuffer(), At, 0.9);
				S47Walkie::Breath(Ctx, Audio->SfxBus, Audio->NoiseBuffer(), At + 1.4, 0.8);
			}
			Say(TEXT("Breathing, close, in the cab. Your own."), 3.4f);
		});
	}
	else if (N % 2 == 0)
	{
		if (Eng) Eng->Solenoid({ K.First }, 0.55);
		Tries.KeyFree = E + K.Again; Say(TEXT("One click. Nothing turns over."), 2.6f);
	}
	else { Tries.KeyFree = E + K.Again; Say(TEXT("Nothing. Not even the click."), 2.6f); }
	return true;
}

bool US47Chapter6::TryDoor()
{
	if (Stage != TEXT("event")) return false;
	if (!DoorOffered() || Et < Tries.DoorFree) return true;
	Tries.DoorFree = Et + LAST_MILE.Door; const int32 N = ++Tries.Door;
	if (M && M->Audio) M->Audio->Sfx(M->Audio->Or(TEXT("door_latch"), TEXT("switch")), Snd(0.5f, 0.8f));
	if (!OnWords() && N <= LAST_MILE.DoorSaid) Toast(N == 2 ? TEXT("The handle moves. The door does not. The lock button is up.") : TEXT("The handle moves. The door does not."), 3.2f);
	return true;
}
bool US47Chapter6::OnWords() const { return Et >= LAST_MILE.Words[0] && Et < LAST_MILE.Words[1]; }

// ---------- THE EVENT ----------
void US47Chapter6::StartEvent()
{
	US47World* W = M ? M->World.Get() : nullptr;
	Stage = TEXT("event"); Et = 0; SetClock(T);
	if (LastMile.Cb && !LastMile.ClosedAt) CloseCB();
	bCalm = M && M->Flow && M->Flow->bCalmFlash; bSteady = bCalm || (M && M->Flow && M->Flow->bStillShots);
	if (!Frames) DrawFrames();
	Ending = SNew(SS47Ending).BoughtSfx(M && M->Audio && M->Audio->BoughtSfx()).LargeText(M && M->UI && M->UI->bLargeText);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->AddViewportWidgetContent(Ending.ToSharedRef(), 90);
	FS47DriveController* Drv = W ? W->OldDrive() : nullptr; FS47OldRoad* Road = W ? W->OldRoad() : nullptr;
	if (Drv && Road)
	{
		const double V = FMath::Max(0.0, Drv->Speed);
		StopS = Road->Where(Drv->Pos.X, Drv->Pos.Z).S + FMath::Clamp(V * V / (2 * THIRD.Brake) + V * THIRD.React, THIRD.Near, THIRD.Far);
	}
	HeadNow = HEAD_WHITE;
	TWeakObjectPtr<US47Chapter6> Self = this;
	if (W) { W->OldControl = US47World::FOldControl(); W->OldControl->Input = [Self](double Dt) { return Self.IsValid() ? Self->Coast(Dt) : FS47DriveIn(); }; }
	if (M && M->UI) { M->UI->Objective(TEXT("")); M->UI->ClearToasts(); }
	UE_LOG(LogS47Ch6, Display, TEXT("S47: ch6 THE EVENT begins, 47 seconds"));
}

FS47DriveIn US47Chapter6::Coast(double Dt) const
{
	FS47DriveController* Drv = M && M->World ? M->World->OldDrive() : nullptr; FS47OldRoad* Road = M && M->World ? M->World->OldRoad() : nullptr;
	if (!Drv || !Road) return {};
	const double S = Road->Where(Drv->Pos.X, Drv->Pos.Z).S; const FVector Tg = Road->Lane(S + 14);
	double Err = FMath::Atan2(-(Tg.X - Drv->Pos.X), -(Tg.Z - Drv->Pos.Z)) - Drv->Heading; Err = FMath::Atan2(FMath::Sin(Err), FMath::Cos(Err));
	double Throttle = 0;
	if (Et > THIRD.React && Drv->Speed > 0.05)
	{
		const double Rem = StopS.Get(S + 40) - S, V = Drv->Speed;
		Throttle = Rem <= 0.3 ? -1 : -FMath::Clamp((V * V / (2 * Rem) - 0.2) / 7.8, 0.05, 1.0);
	}
	return { FMath::Clamp(-Err * 2.2, -1.0, 1.0), Throttle };
}

void US47Chapter6::Once(const FString& K, TFunctionRef<void()> Fn)
{
	if (!Done.Contains(K)) { Done.Add(K); UE_LOG(LogS47Ch6, Display, TEXT("S47: ch6 %s at event %.3f, pullout %.3f"), *K, Et, Pt); Fn(); }
}

void US47Chapter6::Event(double Dt)
{
	US47Audio* Audio = M ? M->Audio.Get() : nullptr; US47World* W = M ? M->World.Get() : nullptr;
	FS47Truck* Truck = W ? W->OldTruck() : nullptr; FS47DriveController* Drv = W ? W->OldDrive() : nullptr;
	if (!Truck || !Drv || !Audio) return;
	Et += Dt; const double E = Et; SetClock(T + E);
	if (Cues.Num())
	{
		TArray<FCue> Due;
		for (const FCue& C : Cues) if (C.At <= E) Due.Add(C);
		Cues.RemoveAll([E](const FCue& C) { return C.At <= E; });
		for (const FCue& C : Due) C.Fn();
	}
	Once(TEXT("cut"), [&]
	{
		Audio->CabRadio(TEXT("carrier")); if (W->EngineSound()) W->EngineSound()->Stall(); Drv->bEngine = false;
		Truck->SetDashLevel(0); Truck->SetLightLevel(0.16);
		if (Audio->Amb) Audio->Amb->Gain.SetTargetAtTime(0.25, Audio->Ctx ? Audio->Ctx->CurrentTime() : 0, 0.8);
		Note(TEXT("05:29. On the line."));
	});
	if (KeyLamps)
	{
		const auto& K = *KeyLamps; const double Last = K.Last();
		if (E < Last + 0.4) Truck->SetDashLevel(K.ContainsByPredicate([E](double C) { return E >= C && E < C + LAST_MILE.Key.Lamp; }) ? 1 : 0);
		else Once(TEXT("dashOff"), [&] { Truck->SetDashLevel(0); });
	}
	else if (E >= DASH.From && E < DASH.Until) Truck->SetDashLevel(DashOn(E) ? 1 : 0);
	else if (E >= DASH.Until) Once(TEXT("dashOff"), [&] { Truck->SetDashLevel(0); });
	if (E >= 2) Once(TEXT("pressure"), [&] { Bed = Audio->Pressure(); });
	if (Bed && E < 34) Bed.Gain->Gain.SetValue(0.063 * Sm(2, 6, E) + 0.187 * Sm(6, 32, E));
	if (E >= 10 && E < 34)
	{
		const double V = (E - 10) / 2.2; const int32 I = FMath::Min(3, FMath::FloorToInt(V)); const double K = Sm(0.55, 1, V - I);
		const FLinearColor C = FMath::Lerp(HEADS[I], HEADS[FMath::Min(3, I + 1)], float(I < 3 ? K : 0)) * 1.6f;
		Truck->SetHeadColor(C); HeadNow = C; Truck->SetLightLevel(0.16 + 0.84 * Sm(10, 11.2, E));
	}
	ThirdStep(E);
	if (E >= 18) Once(TEXT("radio"), [&] { Audio->CabRadio(TEXT("hiss")); });
	auto Piece = [&](double At, const TCHAR* K, TFunctionRef<void()> Fn) { if (E >= At) Once(K, Fn); };
	Piece(18.4, TEXT("printer"), [&] { FS47Sound O = Snd(1.4f); O.DestNode = Audio->RadioIn(); const auto H = Audio->Sfx(TEXT("printer"), O); if (H) H.Src->Stop((Audio->Ctx ? Audio->Ctx->CurrentTime() : 0) + 2.4); });
	Piece(21.0, TEXT("phone"), [&] { FS47Sound O = Snd(1.2f); O.DestNode = Audio->RadioIn(); const auto H = Audio->Sfx(TEXT("phoneRing"), O); if (H) H.Src->Stop((Audio->Ctx ? Audio->Ctx->CurrentTime() : 0) + 2.6); });
	Piece(23.8, TEXT("tomas"), [&] { Audio->CabVoice(2.4f, 520); Toast(TEXT("On the radio, a man's voice, far away: \"Reference west. No. East.\""), 3.2f); });
	Piece(26.8, TEXT("nora"), [&] { Audio->CabVoice(2.0f, 820); Toast(TEXT("A woman's voice: \"Do not stop on the line.\""), 3.2f); });
	Piece(29.6, TEXT("cup"), [&] { Audio->CupDown(Audio->RadioIn()); });
	Piece(31.4, TEXT("carrier2"), [&] { Audio->CabRadio(TEXT("carrier")); });
	if (E >= 32) Flash(E - 32);
	Piece(34, TEXT("silence"), [&]
	{
		Audio->CabRadio(TEXT("")); if (W->EngineSound()) W->EngineSound()->Stop(); Audio->Silence(true);
		Bed.Stop(0.04f); Bed = FS47Pressure(); Truck->SetHeadColor({});
	});
	if (E >= 34) Truck->SetLightLevel(0.2 + 0.12 * (0.5 + 0.5 * FMath::Sin((E - 34) * 2.1)));
	if (E >= EVENT) StartPullout();
}

void US47Chapter6::ThirdStep(double E)
{
	US47World* W = M ? M->World.Get() : nullptr; FS47Truck* Truck = W ? W->OldTruck() : nullptr;
	FS47DriveController* Drv = W ? W->OldDrive() : nullptr; FS47OldRoad* Road = W ? W->OldRoad() : nullptr;
	if (!Truck || !Drv || !Road || !StopS || !M->Web) return;
	const double LetGo = bSteady ? THIRD.LetGo : 0;
	if (E >= THIRD.Until + LetGo) { if (Third && bThirdShown) { Third->SetVisible(false); bThirdShown = false; ThirdGlints(nullptr); } return; }
	if (!Third)
	{
		Third = MakeShared<FS47ThirdFigure>(M->Web, TEXT("third:road"));
		const FVector A = Road->Lane(*StopS), B = Road->Lane(*StopS + 4); const double H = FMath::Atan2(-(B.X - A.X), -(B.Z - A.Z));
		FVector At = A + FVector(-FMath::Sin(H), 0, -FMath::Cos(H)) * THIRD.Ahead; At.Y = Road->Height(At.X, At.Z);
		double D = -UE_DOUBLE_HALF_PI - H; D = FMath::Atan2(FMath::Sin(D), FMath::Cos(D));
		Third->Place(At, H + FMath::Clamp(D, -0.7, 0.7)); Third->AddToScene();
	}
	Third->SetVisible(true); bThirdShown = true; Third->SetFade(bSteady ? 1 - Sm(THIRD.Until, THIRD.Until + LetGo, E) : 1);
	const FVector Eye = Truck->LocalToWorld(Truck->DriverEye);
	// ThirdFigure keeps its world feet; its head is 1.6 m above them in the same orientation.
	const double Dist = FVector::Distance(Third->Head() - S47_UP * 1.6, Eye);
	const double Level = Truck->Beam(), Near = 1 / (1 + FMath::Square(Dist / 12)), Tape = 1 / (1 + FMath::Square(Dist / 45));
	double Flare = 0;
	if (!bSteady) for (const double Tc : THIRD.Flares) Flare = FMath::Max(Flare, FMath::Exp(-FMath::Square((E - Tc) / 0.2)));
	Third->Light(HeadNow * float(Level * Near * 1.8), THIRD_AMB, HeadNow * float(Level * Tape * (0.8 + 1.3 * Flare)));
	BandPts = Third->BandPoints(Eye); ThirdGlints(&BandPts, HeadNow * float(Level * Tape * (0.24 + 0.6 * Flare) * Third->Fade()));
}

void US47Chapter6::Flash(double V)
{
	if (!Frames || !Ending) return;
	auto Show = [this](int32 I, double Op = 1)
	{
		UTexture2D* Tx = FrameTextures.IsValidIndex(I) ? FrameTextures[I].Get() : nullptr;
		// Reuse the time-away overlay's frames; Ending's own layer is the fallback without Away.
		if (M && M->Away) { if (Tx) M->Away->ShowFrame(Tx, Op, true); else M->Away->HideFrame(); }
		else Ending->Flash(Tx, Op);
	};
	if (bCalm)
	{
		const double Per = 0.9; const int32 I = FMath::FloorToInt(V / Per); const double K = V - I * Per;
		if (I >= Frames->Num()) { Show(-1); return; }
		Show(I, 0.35 * FMath::Min3(1.0, K / 0.35, (Per - K) / 0.35)); return;
	}
	double At = 0;
	for (int32 I = 0; I < Frames->Num(); ++I)
	{
		const auto& F = (*Frames)[I];
		if (V < At + F.Ms / 1000) { Show(I); return; }
		At += F.Ms / 1000;
		if (V < At + 0.04) { Show(-1); return; }
		At += 0.04;
	}
	Show(-1);
}

// ---------- the pull-out ----------
void US47Chapter6::StartPullout()
{
	US47World* W = M ? M->World.Get() : nullptr; FS47DriveController* Drv = W ? W->OldDrive() : nullptr;
	if (!W || !Drv || !W->OldTruck()) return;
	DropThird(); Stage = TEXT("pullout"); Pt = 0;
	if (Ending) Ending->Flash(nullptr); if (M->Away) M->Away->HideFrame();
	bStill = M->Flow && M->Flow->bStillShots;
	// The walker's real camera (also on turns in the cab), not the player's stationary on-foot pose.
	From.Pos = M->Player ? S47Three::ToThree(M->Player->EyeUE()) : Drv->CamPos;
	From.Q = M->Player ? FS47Cam::Of(M->Player).Quaternion() : Drv->CamQuat;
	TruckAt.Pos = Drv->Pos; TruckAt.Pos.Y = W->OldTruck()->Position.Y; TruckAt.Heading = Drv->Heading;
	TWeakObjectPtr<US47Chapter6> Self = this;
	W->OldControl = US47World::FOldControl();
	W->OldControl->Camera = [Self](double Dt, FS47View& Out) { if (Self.IsValid()) Self->Shoot(Out); };
	Note(TEXT("05:29:47."));
	UE_LOG(LogS47Ch6, Display, TEXT("S47: ch6 pullout begins at 05:29:47"));
}

US47Chapter6::FKeyframes US47Chapter6::Keyframes() const
{
	const FVector& P = TruckAt.Pos; const double H = TruckAt.Heading; const FVector F(-FMath::Sin(H), 0, -FMath::Cos(H));
	FKeyframes K;
	K.Front = P + F * 9 + S47_UP * 1.9; K.AtTruck = P + S47_UP * 1.2;
	K.High = P + Dir(104) * 170 + S47_UP * 140; K.Westward = P + Dir(284) * 160;
	K.SunSide = P + Dir(259) * 120 + S47_UP * 150; K.Sunward = P + Dir(79) * 3000 + S47_UP * 60;
	K.Top = P + Dir(259) * 220 + S47_UP * 650;
	const double Az = FMath::DegreesToRadians(LAST_LOOK_AZ), El = FMath::DegreesToRadians(LAST_LOOK_EL);
	K.Stars = K.Top + FVector(FMath::Sin(Az) * FMath::Cos(El), FMath::Sin(El), -FMath::Cos(Az) * FMath::Cos(El)) * 4000;
	return K;
}

void US47Chapter6::Shoot(FS47View& Out)
{
	const FKeyframes K = Keyframes(); const double V = Pt; FVector Pos; FQuat Q;
	if (bStill)
	{
		const TPair<FVector, FVector> Shots[] = { { K.Front, K.AtTruck }, { K.High, K.Westward }, { K.SunSide, K.Sunward }, { K.Top, K.Stars } };
		const int32 I = FMath::Min(3, FMath::FloorToInt(V / 8.5)); Pos = Shots[I].Key; Q = LookAt(Pos, Shots[I].Value);
		const double Into = V - I * 8.5; if (Ending) Ending->Black(Into < 0.5 || (Into > 8.0 && I < 3), 350);
	}
	else if (V < 6.5)
	{
		const double E = Sm(0, 6.5, V); Pos = FMath::Lerp(From.Pos, K.Front, E);
		Q = FQuat::Slerp(From.Q, LookAt(Pos, K.AtTruck), Sm(0.5, 5.5, V));
	}
	else if (V < 13) { const double E = Sm(6.5, 13, V); Pos = FMath::Lerp(K.Front, K.High, E); Q = LookAt(Pos, FMath::Lerp(K.AtTruck, K.Westward, E)); }
	else if (V < 21) { const double E = Sm(13, 21, V); Pos = FMath::Lerp(K.High, K.SunSide, E); Q = LookAt(Pos, FMath::Lerp(K.Westward, K.Sunward, E)); }
	else { const double E = Sm(21, PULL, V); Pos = FMath::Lerp(K.SunSide, K.Top, E); Q = LookAt(Pos, FMath::Lerp(K.Sunward, K.Stars, Sm(22, PULL, V))); }
	ViewOf(Pos, Q, Out);
}

void US47Chapter6::Pullout(double Dt)
{
	US47World* W = M ? M->World.Get() : nullptr; FS47Truck* Truck = W ? W->OldTruck() : nullptr; FS47OldRoad* Road = W ? W->OldRoad() : nullptr;
	Pt += Dt; const double V = Pt; SetClock(T + EVENT + V);
	if (!Truck || !Road) return;
	Truck->SetLightLevel(0.2 + 0.12 * (0.5 + 0.5 * FMath::Sin((EVENT - 34 + V) * 2.1)));
	const FVector Cam = M->Player ? S47Three::ToThree(M->Player->EyeUE()) : From.Pos;
	const double Hgt = Cam.Y - Road->Height(Cam.X, Cam.Z);
	if (M->Web) M->Web->SetChapter6Fog(0.00055 * (1 - Sm(10, 420, Hgt)) + 0.00006);
	SkyLift = 1 - 0.85 * Sm(180, 640, Hgt);
	// backdrop.ts SUN_MESA.top / horizonDip (no hand-made horizon geometry).
	const double MesaTop = 0.6 - FMath::RadiansToDegrees(FMath::Atan(FMath::Max(0.0, Hgt) / 20000));
	SunElev = MesaTop - 0.27 + (Clock() - (5 * 3600 + 30 * 60)) * 0.05;
	if (V >= PULL) Once(TEXT("black"), [this]
	{
		if (Ending) Ending->Black(true, 1400);
		TWeakObjectPtr<US47Chapter6> Wk = this;
		if (Host) Host->After(1.6, [Wk] { if (Wk.IsValid() && Wk->bActive) { Wk->Stage = TEXT("end"); if (Wk->M && Wk->M->Flow) Wk->M->Flow->Finale(); } });
	});
}

UTexture2D* US47Chapter6::Photo() const
{
	// Chapter6.photo: the whole image drawn on the 1200 x 800 canvas.
	FS47Pix Im;
	if (!S47Raster::Art(TEXT("photo1947"), Im)) return nullptr;
	FS47Pix C(1200, 800); C.DrawImage(Im, 0, 0, 1200, 800);
	return S47Image::FromPixels(C.Colors(), C.W, C.H);
}

void US47Chapter6::Update(double Dt, double InT)
{
	if (!bActive) return;
	if (Stage == TEXT("drive")) Drive(Dt);
	else if (Stage == TEXT("event")) Event(Dt);
	else if (Stage == TEXT("pullout")) Pullout(Dt);
	if (FinaleSky)
	{
		const auto* D = M && M->World ? M->World->OldDrive() : nullptr;
		const auto* R = M && M->World ? M->World->OldRoad() : nullptr;
		FinaleSky->Step(D && R ? R->Where(D->Pos.X,D->Pos.Z).Mi : 0,Stage,Et,Pt);
	}
	if (M && M->UI) M->UI->Objective(Objective());
	// main.ts sky.setDawnLift / setClock: the world-life renderer consumes AS47Web's overrides.
	if (M && M->Web) { M->Web->SetDawnLift(SkyLift.Get(M->World && M->World->Area == TEXT("roswell") ? 1.0 : 0.7)); M->Web->SetSunElevation(SunElev); }
}

S47J::FObj US47Chapter6::FinaleReadback() const
{
	auto O = S47J::New(); S47J::Set(O,TEXT("revised"),bRevisedSC13);S47J::Set(O,TEXT("active"),bActive);S47J::Set(O,TEXT("stage"),Stage);
	S47J::Set(O,TEXT("clock"),Clock());S47J::Set(O,TEXT("eventSeconds"),Et);S47J::Set(O,TEXT("pullSeconds"),Pt);S47J::Set(O,TEXT("keyTries"),Tries.Key);S47J::Set(O,TEXT("doorTries"),Tries.Door);
	S47J::Set(O,TEXT("finaleSky"),FinaleSky?FinaleSky->Readback():S47J::FObj());
	if(M&&M->Flow){S47J::Set(O,TEXT("calmFlash"),M->Flow->bCalmFlash);S47J::Set(O,TEXT("stillShots"),M->Flow->bStillShots);}
	if(M&&M->Web)S47J::Set(O,TEXT("graphics"),M->Web->GetGraphicsSettings().ToJson());
	if(M&&M->World)
	{
		S47J::Set(O,TEXT("area"),M->World->Area);S47J::Set(O,TEXT("driving"),M->World->bDriving);
		const auto* Cab=M->World->CabTruck();
		S47J::Set(O,TEXT("commuteVehicle"),M->World->bCommuteVehicle);S47J::Set(O,TEXT("cabPresent"),Cab!=nullptr);S47J::Set(O,TEXT("personalCab"),Cab&&Cab->IsPersonalVehicle());
		const auto* D=M->World->OldDrive();const auto* R=M->World->OldRoad();
		if(D){S47J::Set(O,TEXT("carThree"),TArray<double>{D->Pos.X,D->Pos.Y,D->Pos.Z});S47J::Set(O,TEXT("heading"),D->Heading);S47J::Set(O,TEXT("speed"),D->Speed);S47J::Set(O,TEXT("engine"),D->bEngine);if(R)S47J::Set(O,TEXT("mile"),R->Where(D->Pos.X,D->Pos.Z).Mi);}
	}
	if(M&&M->Player){const auto C=FS47Cam::Of(M->Player);S47J::Set(O,TEXT("cameraThree"),TArray<double>{C.Pos.X,C.Pos.Y,C.Pos.Z});S47J::Set(O,TEXT("fov"),C.Fov);}
	return O;
}

// ---------- the host (Prologue.ts) and the developer menu (main.ts) ----------
void US47Chapter6::BeginChapter6(const S47J::FObj& SavedCase)
{
	if (!Host) return;
	if (SavedCase) BeganCase = SavedCase;
	// (chapter five's OnEnd calls this: main.ts ch5.onEnd = beginChapter6)
	if (US47Chapter* C5 = Host->Chapter(TEXT("ch5"))) C5->bActive = false;
	if (M && M->Audio) M->Audio->AmRadioThree({});
	Host->SetCinematic(false);
	Host->BeginChapter(TEXT("ch6")); // no checkpoint or save: the diner remains Continue
}

void US47Chapter6::JumpChapter6(const FString& At)
{
	if (!Host) return;
	// Prologue.jumpChapter6: chapter five done in the diner (P13 found), straight out onto the old road.
	// jumpChapter5 first (chapter four's done state on jumpChapter4's, then beginChapter5 from the save
	// or fresh), then this.ch5.s = { ...this.ch5.s, stage: 'road', arrived: true, p13: true, confirmed: true }
	if (US47Chapter5* C5 = Cast<US47Chapter5>(Host->Chapter(TEXT("ch5"))))
	{
		C5->JumpChapter5();
		C5->S.Stage = TEXT("road"); C5->S.bArrived = true; C5->S.bP13 = true; C5->S.bConfirmed = true;
	}
	BeganCase = M && M->Flow ? M->Flow->ComposeCase() : S47J::New();
	BeginChapter6(BeganCase);
	JumpAt = At;
}

void US47Chapter6::PlaceJump()
{
	// main.ts developer menu: "6 The last mile" and "6 At the line". No driving is invented: the
	// truck stands at those exact miles; -S47Auto can drive it from there for the screenshot runs.
	if (bJumpPlaced || JumpAt.IsEmpty() || !M || !M->World) return;
	FS47OldRoad* R = M->World->OldRoad(); FS47DriveController* D = M->World->OldDrive();
	if (!R || !D) return;
	const bool bLast = JumpAt == TEXT("lastmile");
	const FS47OldRoad::FPose P = R->PoseAt(bLast ? 6.8 : 8.06); D->Place(P.Pos, P.Heading);
	SetClock(bLast ? 5 * 3600 + 27 * 60 : 5 * 3600 + 28 * 60 + 30); bJumpPlaced = true;
	float AutoSpeed = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47Auto="), AutoSpeed) && AutoSpeed > 0) M->World->Autopilot = AutoSpeed;
	UE_LOG(LogS47Ch6, Display, TEXT("S47: ch6 %s, mile %.2f"), *JumpAt, bLast ? 6.8 : 8.06);
}

bool US47Chapter6::SelfTest(TArray<FString>& Lines)
{
	bool bOk = SS47Ending::SelfTest(Lines);
	bOk &= FS47EngineSound::SelfTest(Lines);
	auto Check = [&](bool Pass, const TCHAR* What) { Lines.Add(FString(Pass ? TEXT("PASS ch6: ") : TEXT("FAIL ch6: ")) + What); bOk &= Pass; };
	Check(!RevisedDeparture(S47J::Read(TEXT("{\"v\":1,\"p13\":true,\"stage\":\"road\",\"ward\":true}"))),TEXT("legacy P13/Ward departure never acquires revised finale or new source requirements"));
	auto Revised=S47J::Read(TEXT("{\"p13\":true,\"sc12\":{\"v\":1,\"compared\":true,\"read\":7,\"checks\":7,\"warningSent\":true,\"patrolRead\":true,\"courier104\":true,\"routeReadback\":true}}"));
	Check(RevisedDeparture(Revised),TEXT("only complete saved SC12 source/warning/patrol/map/courier facts select revised finale"));
	S47J::Set(S47J::Obj(Revised,TEXT("sc12")),TEXT("warningSent"),false);
	Check(!RevisedDeparture(Revised),TEXT("a map and unacknowledged warning cannot select revised courier ending"));
	US47Chapter6* C = NewObject<US47Chapter6>();
	auto At = [](double Rem, double Mi) { return FS47OldRoad::FWhere{ -Rem - LAST_MILE.Front, 1.6, 0, Mi }; };
	Check(C->SaveBlock() == TEXT("Not on the road.") && !C->SaveState(), TEXT("road and ending are never saved"));
	Check(!C->LastMileStep(0.1, At(1200, 7.1), 24.6) && !C->LastMile.Cb, TEXT("a truck put beyond mile seven cannot open the CB"));
	C->Reset(); C->SetClock(T - 80);
	C->LastMileStep(0.1, At(2000, 6.8), 24.6);
	Check(C->LastMile.bApproach && !C->CbDue(), TEXT("approach must come from before mile 6.95"));
	C->LastMileStep(0.1, At(24.6 * 48, 7.1), 24.6);
	Check(!C->LastMile.Cb && C->CbDue(), TEXT("CB due eight seconds before the lead, still shut at 48 seconds"));
	Check(C->LastMileStep(0.1, At(24.6 * 47, 7.1), 24.6) && C->LastMile.Cb && C->LastMile.OpenAt == T - 47 && C->ClockShow() == TEXT("05:28:13"), TEXT("CB opens at 47 seconds, at 05:28:13"));
	const double D0 = C->LastMile.D0;
	C->LastMileStep(1, At(D0 / 2, 7.8), 24.6); const double Half = C->Clock();
	C->LastMileStep(8, At(D0 / 2, 7.8), 0);
	Check(FMath::IsNearlyEqual(Half, T - 23.5) && C->Clock() == Half, TEXT("clock follows remaining metres and waits when stopped"));
	C->LastMileStep(1, At(D0 * 0.75, 7.6), -5);
	Check(C->Clock() == Half, TEXT("backing up never turns the count backwards"));
	C->LastMileStep(4, At(0, 8.15), 0);
	Check(C->Clock() == T && C->LastMile.ClosedAt && !C->CbDue(), TEXT("CB closes at 12.8 seconds and the count reaches 05:29 on C"));
	C->Stage = TEXT("event"); C->SetClock(T + 47);
	Check(C->ClockShow() == TEXT("05:29:47"), TEXT("seconds continue through THE EVENT"));
	C->Stage = TEXT("pullout"); Check(C->ClockShow().IsEmpty(), TEXT("seconds go when the camera leaves"));
	C->Reset(); C->LastMileStep(0.1, At(2000, 6.8), 24.6); C->LastMileStep(0.1, At(24.6 * 39, 7.6), 24.6);
	Check(C->LastMile.bSkipped && !C->LastMile.Cb && !C->CbDue(), TEXT("too close in time skips the CB"));
	C->Reset(); C->LastMileStep(0.1, At(500, 6.8), 4); C->LastMileStep(0.1, At(180, 8.0), 4);
	Check(C->LastMile.bSkipped, TEXT("under 200 metres skips the CB even at 45 seconds"));
	C->Reset(); C->Stage = TEXT("event"); C->Et = 1.49;
	Check(C->TurnKey() && C->TryDoor() && C->Tries.Key == 0 && C->Tries.Door == 0, TEXT("EVENT consumes E and Q before either can answer"));
	C->Et = 1.5; Check(C->WantsKey() && C->DoorOffered(), TEXT("key and door offered at +1.5 seconds"));
	C->TurnKey();
	Check(C->Tries.Key == 1 && C->KeyLamps && C->KeyLamps->Num() == 11 && C->Cues.Num() == 3, TEXT("first key turn schedules four, seven, breath and eleven lamp flashes"));
	const double Free = C->Tries.KeyFree;
	C->TurnKey(); Check(C->Tries.Key == 1, TEXT("first turn cannot overlap its four and seven"));
	C->Et = Free; C->TurnKey(); Check(C->Tries.Key == 2 && FMath::IsNearlyEqual(C->Tries.KeyFree, Free + 0.8), TEXT("second turn is one click and waits 0.8 seconds"));
	C->Et = Free + 0.8; C->TurnKey(); Check(C->Tries.Key == 3, TEXT("third turn is silence"));
	C->TryDoor(); C->TryDoor(); Check(C->Tries.Door == 1, TEXT("door handle rate is 0.6 seconds"));
	C->Et += 0.6; C->TryDoor(); Check(C->Tries.Door == 2, TEXT("door can be tried again but never opens"));
	C->Reset(); C->Stage = TEXT("event"); C->Et = 4;
	Check(!C->WantsKey() && C->DoorOffered(), TEXT("unturned key waits for the dash count, Q remains offered"));
	C->Et = 10.8; Check(C->WantsKey(), TEXT("unturned key returns at +10.8"));
	C->Et = 23.6; Check(C->OnWords(), TEXT("key and door captions avoid the radio voices")); C->Et = 30; Check(!C->OnWords(), TEXT("radio-caption hold ends at +30"));
	C->Et = 31.4; Check(!C->WantsKey() && !C->DoorOffered() && C->TurnKey() && C->TryDoor(), TEXT("+31.4 takes the prompts but still consumes E/Q"));
	int32 Blinks = 0; bool Was = false;
	for (int32 I = 0; I < 680; ++I) { const bool On = DashOn(4 + I * 0.01); if (On && !Was) ++Blinks; Was = On; }
	Check(Blinks == 11 && !DashOn(6.5) && DashOn(7.21), TEXT("automatic dash count has four then seven with the long gap"));
	C->TruckAt.Pos = FVector(120, 5, -90); C->TruckAt.Heading = -0.8;
	const FKeyframes K = C->Keyframes();
	Check(FMath::IsNearlyEqual(FVector(K.Front.X - 120, 0, K.Front.Z + 90).Size(), 9.0, 1e-8) && FMath::IsNearlyEqual(K.Front.Y, 6.9), TEXT("front keyframe is nine metres forward and 1.9 metres up"));
	Check(FMath::IsNearlyEqual(K.High.Y, 145.0) && FMath::IsNearlyEqual(K.Top.Y, 655.0), TEXT("high and top keep 140 and 650 metre heights"));
	C->bStill = true; C->Pt = 2; FS47View A, B, D; C->Shoot(A); C->Pt = 6; C->Shoot(B); C->Pt = 9; C->Shoot(D);
	Check(A.Eye.Equals(B.Eye, 1e-9) && FVector::Distance(B.Eye, D.Eye) > 50, TEXT("still shots hold, then cut after 8.5 seconds"));
	C->bStill = false; C->Pt = PULL; C->Shoot(A);
	const FQuat Q = S47Three::Euler(A.Pitch, A.Yaw, A.Roll, TEXT("YXZ")); const FVector Look = Q.RotateVector(FVector(0, 0, -1));
	Check(A.Eye.Equals(K.Top, 1e-8) && FMath::IsNearlyEqual(FMath::RadiansToDegrees(FMath::Atan2(Look.X, -Look.Z)), LAST_LOOK_AZ, 1e-7) && FMath::IsNearlyEqual(FMath::RadiansToDegrees(FMath::Asin(Look.Y)), LAST_LOOK_EL, 1e-7), TEXT("final camera looks at azimuth 31, elevation 48 in three space"));
	return bOk;
}
