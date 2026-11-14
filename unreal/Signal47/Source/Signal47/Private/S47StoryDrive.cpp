#include "S47StoryDrive.h"
#include "S47StoryDirector.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47World.h"
#include "S47Truck.h"
#include "S47Evening.h"
#include "S47EveningVisual.h"
#include "S47Audio.h"
#include "S47Media.h"
#include "S47Radio.h"
#include "S47Score.h"
#include "S47UI.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// STORY-DRIVE-OWN-CAR-20261113: see S47StoryDrive.h.
DEFINE_LOG_CATEGORY_STATIC(LogS47StoryDrive, Log, All);

namespace S47StoryDriveLocal
{
	const TCHAR* PhaseName(FS47StoryDrive::EPhase P) { return P == FS47StoryDrive::EPhase::Drive ? TEXT("drive") : P == FS47StoryDrive::EPhase::Parked ? TEXT("parked") : TEXT("none"); }
	bool LiveTest() { return FParse::Param(FCommandLine::Get(), TEXT("S47StoryDriveTest")); }
	FVector Vec(const S47J::FObj& O, const TCHAR* X, const TCHAR* Y, const TCHAR* Z) { return FVector(S47J::Num(O, X), S47J::Num(O, Y), S47J::Num(O, Z)); }
	bool Finite(const FVector& V, double H) { return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z) && FMath::IsFinite(H); }
}

FS47StoryDrive::FS47StoryDrive(US47Main* InMain) : M(InMain) {}

FS47StoryDrive::~FS47StoryDrive()
{
	// Destruction releases only the owned audio; the world may be gone already.
	if (Intro) { Intro->Stop(); Intro->Disconnect(); }
	if (Bed) { Bed->Stop(); Bed->Disconnect(); }
	if (IntroLevel) IntroLevel->Disconnect();
	if (BedLevel) BedLevel->Disconnect();
}

FS47StoryDrive* FS47StoryDrive::Of(const US47Main* Main)
{
	FS47StoryDirector* D = FS47StoryDirector::Of(Main);
	return D ? D->DriveHost() : nullptr;
}

FS47EveningVisual* FS47StoryDrive::Visual() const
{
	return M && M->Evening ? M->Evening->VisualHost() : nullptr;
}

FString FS47StoryDrive::IntroFile()
{
	// MUSIC-20261113 stages Tom's track with the score's media (Tools/music/build_music.py): the mono copy narrowed to AM is the
	// car radio's; the full recording if only that is there. (A media name, as LoopBed: FS47MediaSource::Open resolves it.)
	if (S47Score::MediaPresent(S47Score::IntroDriveRadioFile)) return S47Score::IntroDriveRadioFile;
	if (S47Score::MediaPresent(S47Score::IntroDriveFile)) return S47Score::IntroDriveFile;
	// As the menu's own track (lic:signal47_menu): the licensed folder's index names the file; a bare file of the id's name also does.
	const FString Dir = FPaths::ProjectContentDir() / TEXT("Licensed/Raw");
	FString Text;
	if (FFileHelper::LoadFileToString(Text, *(Dir / TEXT("index.json"))))
	{
		TSharedPtr<FJsonObject> J;
		const TSharedPtr<FJsonObject>* Sources = nullptr;
		FString File;
		if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) && J.IsValid() && J->TryGetObjectField(TEXT("sources"), Sources) && Sources && Sources->IsValid()
			&& (*Sources)->TryGetStringField(FString(TEXT("lic:")) + IntroBed, File) && !File.IsEmpty() && IFileManager::Get().FileExists(*(Dir / File)))
			return Dir / File;
	}
	for (const FString& Path : {Dir / FString(IntroBed) + TEXT(".wav"), FPaths::ProjectContentDir() / TEXT("Audio/Raw/media") / FString(IntroBed) + TEXT(".wav")})
		if (IFileManager::Get().FileExists(*Path)) return Path;
	return FString();
}

bool FS47StoryDrive::BeginNew()
{
	if (!M || !IsValid(M->World) || M->World->bBusy || M->World->bDriving) return false;
	FS47EveningVisual* V = Visual();
	if (!V) return false;
	Stop();
	V->SetParked(false);
	// The old Story's drive start: the road leg at the south end of Caliche, heading north (FS47StoryDirector::DriveStart is the same).
	M->World->BeginCommute(nullptr);
	// (The trip itself starts behind the fade, 0.4 s on: busy now, driving then.)
	if (!(M->World->bBusy || M->World->bDriving) || !M->World->bCommuteVehicle)
	{
		UE_LOG(LogS47StoryDrive, Warning, TEXT("S47: story drive: the wagon's drive did not start"));
		M->World->EndPersonalVehicle();
		return false;
	}
	Phase = EPhase::Drive;
	bRadioOn = true; bIntroDone = false; bNoIntro = false; IntroAt = BedAt = 0;
	UE_LOG(LogS47StoryDrive, Display, TEXT("S47: story night starts in the player's own station wagon on the road north to SARO (radio %d, intro %s)"), Dial, IntroFile().IsEmpty() ? TEXT("absent: the old bed throughout") : TEXT("present"));
	return true;
}

void FS47StoryDrive::Load(const S47J::FObj& Saved)
{
	Stop();
	if (!Saved.IsValid() || S47J::Int(Saved, TEXT("v"), 0) != 1 || !M || !IsValid(M->World)) return;
	FS47EveningVisual* V = Visual();
	if (!V) return;
	const FString Ph = S47J::Str(Saved, TEXT("phase"));
	bRadioOn = S47J::Bool(Saved, TEXT("radioOn"), true);
	bIntroDone = S47J::Bool(Saved, TEXT("introDone"), false);
	IntroAt = FMath::Max(0., S47J::Num(Saved, TEXT("introAt")));
	BedAt = FMath::Max(0., S47J::Num(Saved, TEXT("bedAt")));
	if (!FMath::IsFinite(IntroAt)) IntroAt = 0;
	if (!FMath::IsFinite(BedAt)) BedAt = 0;
	if (Ph == TEXT("drive"))
	{
		const FVector P = S47StoryDriveLocal::Vec(Saved, TEXT("x"), TEXT("y"), TEXT("z"));
		const double H = S47J::Num(Saved, TEXT("h"));
		const FString At = S47J::Str(Saved, TEXT("at"), TEXT("diner"));
		V->SetParked(false);
		if (S47J::Bool(Saved, TEXT("driving")) || At.IsEmpty() || !S47StoryDriveLocal::Finite(P, H))
		{
			// (Freeform saves only on foot, so this is a save from outside the rules: the drive again from where it was, or its start.)
			const S47J::FObj Car = S47J::New();
			S47J::Set(Car, TEXT("leg"), S47J::Str(Saved, TEXT("leg"), TEXT("road")));
			S47J::Set(Car, TEXT("x"), P.X); S47J::Set(Car, TEXT("y"), P.Y); S47J::Set(Car, TEXT("z"), P.Z); S47J::Set(Car, TEXT("h"), H);
			M->World->BeginCommute(S47StoryDriveLocal::Finite(P, H) ? Car : S47J::FObj());
		}
		else
		{
			// Left on the way (the diner's lot, or the road): the wagon stands where it was, the truck rig hidden behind it.
			M->World->bCommuteVehicle = true;
			const S47J::FObj Pose = S47J::New();
			S47J::Set(Pose, TEXT("at"), At);
			S47J::Set(Pose, TEXT("x"), P.X); S47J::Set(Pose, TEXT("y"), P.Y); S47J::Set(Pose, TEXT("z"), P.Z); S47J::Set(Pose, TEXT("h"), H);
			M->World->PlaceTruck(At, Pose);
		}
		Phase = EPhase::Drive;
	}
	else if (Ph == TEXT("parked"))
	{
		const FVector P = S47StoryDriveLocal::Vec(Saved, TEXT("px"), TEXT("py"), TEXT("pz"));
		const double H = S47J::Num(Saved, TEXT("ph"));
		V->RestorePersonal(S47StoryDriveLocal::Finite(P, H) ? P : FS47EveningVisual::ArrivalPark(), S47StoryDriveLocal::Finite(P, H) ? H : 0., 2);
		V->SetParked(true);
		// The wagon's parked pose as the world's second vehicle (as FinishCommute leaves it), without moving SARO 07.
		M->World->RestoreTransport(nullptr);
		Phase = EPhase::Parked;
	}
	UE_LOG(LogS47StoryDrive, Display, TEXT("S47: story drive restored: %s"), S47StoryDriveLocal::PhaseName(Phase));
}

S47J::FObj FS47StoryDrive::Save() const
{
	if (Phase == EPhase::None || !M || !IsValid(M->World)) return nullptr;
	const S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("v"), 1);
	S47J::Set(O, TEXT("phase"), S47StoryDriveLocal::PhaseName(Phase));
	S47J::Set(O, TEXT("radioOn"), bRadioOn);
	S47J::Set(O, TEXT("introDone"), bIntroDone);
	S47J::Set(O, TEXT("introAt"), Intro ? Intro->CurrentTime() : IntroAt);
	S47J::Set(O, TEXT("bedAt"), Bed ? Bed->CurrentTime() : BedAt);
	if (Phase == EPhase::Drive)
	{
		const FS47Truck* T = M->World->CabTruck();
		if (!T) T = M->World->TruckHere();
		S47J::Set(O, TEXT("driving"), M->World->bDriving);
		S47J::Set(O, TEXT("leg"), M->World->bDriving ? M->World->Leg : M->World->Area);
		S47J::Set(O, TEXT("at"), M->World->TruckAt);
		S47J::Set(O, TEXT("area"), M->World->Area);
		if (T) { S47J::Set(O, TEXT("x"), T->Position.X); S47J::Set(O, TEXT("y"), T->Position.Y); S47J::Set(O, TEXT("z"), T->Position.Z); S47J::Set(O, TEXT("h"), T->Rotation.Y); }
	}
	else if (const FS47EveningVisual* V = Visual())
	{
		const FVector P = V->ParkedPosition();
		S47J::Set(O, TEXT("px"), P.X); S47J::Set(O, TEXT("py"), P.Y); S47J::Set(O, TEXT("pz"), P.Z); S47J::Set(O, TEXT("ph"), V->ParkedHeading());
	}
	return O;
}

void FS47StoryDrive::Stop()
{
	StopBeds();
	if (Phase != EPhase::None && M && IsValid(M->World)) M->World->EndPersonalVehicle();
	if (Phase != EPhase::None)
		if (FS47EveningVisual* V = Visual()) { V->SetParked(false); V->Hide(); }
	Phase = EPhase::None;
	bRadioOn = true; bIntroDone = false; bNoIntro = false; IntroAt = BedAt = 0;
}

void FS47StoryDrive::Gate(const TSharedPtr<FS47MediaSource>& Src, const TSharedPtr<FS47GainNode>& Level, bool bOn, double Want)
{
	if (!Src || !Level || !M || !M->Audio || !M->Audio->Ctx) return;
	FS47AudioParam& P = Level->Gain;
	const double Now = M->Audio->Ctx->CurrentTime(), V = P.ValueAt(Now);
	if (bOn)
	{
		if (Src->Paused()) { P.CancelScheduledValues(Now); P.SetValueAtTime(0., Now); Src->Pause(false); }
		else { P.CancelScheduledValues(Now); P.SetValueAtTime(V, Now); }
		P.SetTargetAtTime(Want, Now, .015);
		return;
	}
	if (Src->Paused()) return;
	if (V < 1e-3) { Src->Pause(true); return; }
	P.CancelScheduledValues(Now); P.SetValueAtTime(V, Now); P.SetTargetAtTime(0., Now, .012);
}

void FS47StoryDrive::StopBeds(bool bFade)
{
	if (bFade && M && M->Audio && M->Audio->Ctx && (Intro || Bed))
	{
		// REVIEW-20261113: the arrival stopped the bed mid-waveform: down over 40 ms, then stopped and let go
		const TSharedRef<FS47AudioContext> Ctx = M->Audio->Ctx.ToSharedRef();
		const double Now = Ctx->CurrentTime();
		for (const TSharedPtr<FS47GainNode>& G : { IntroLevel, BedLevel })
			if (G) { G->Gain.CancelScheduledValues(Now); G->Gain.SetValueAtTime(G->Gain.ValueAt(Now), Now); G->Gain.LinearRampToValueAtTime(0., Now + .04); }
		Ctx->SetTimeout(.06, [I = Intro, IL = IntroLevel, B = Bed, BL = BedLevel]
		{
			if (I) { I->Stop(); I->Disconnect(); }
			if (IL) IL->Disconnect();
			if (B) { B->Stop(); B->Disconnect(); }
			if (BL) BL->Disconnect();
		});
		Intro.Reset(); IntroLevel.Reset(); Bed.Reset(); BedLevel.Reset();
		return;
	}
	if (Intro) { Intro->Stop(); Intro->Disconnect(); Intro.Reset(); }
	if (IntroLevel) { IntroLevel->Disconnect(); IntroLevel.Reset(); }
	if (Bed) { Bed->Stop(); Bed->Disconnect(); Bed.Reset(); }
	if (BedLevel) { BedLevel->Disconnect(); BedLevel.Reset(); }
}

void FS47StoryDrive::Arrive()
{
	US47World* W = M->World;
	FS47EveningVisual* V = Visual();
	// The old arrival (FS47Evening::Parked): the wagon kept where it stopped (moved to the marked bay only if it stands on the building or
	// the service pad), SARO 07 on its pad, the cab handed back to the truck.
	if (V) { if (const FS47Truck* T = W->TruckHere()) V->ParkPersonal(T->Position, T->Rotation.Y); }
	W->FinishCommute();
	if (V) V->SetParked(true);
	Phase = EPhase::Parked;
	StopBeds(true);
	UE_LOG(LogS47StoryDrive, Display, TEXT("S47: story drive: the wagon is parked at SARO (%.1f %.1f); SARO 07 on its pad"), V ? V->ParkedPosition().X : 0., V ? V->ParkedPosition().Z : 0.);
	M->RequestAutosave(false);
}

void FS47StoryDrive::Tick(double Dt, bool bHostOn)
{
	(void)Dt;
	if (Phase == EPhase::None || !M || !IsValid(M->World)) return;
	US47World* W = M->World;
	if (Phase == EPhase::Drive && !W->bCommuteVehicle)
	{
		// Something else took the cab back (a developer jump, a tow): the drive is over where it stands.
		UE_LOG(LogS47StoryDrive, Display, TEXT("S47: story drive: the cab is the truck's again; the wagon waits at SARO"));
		if (FS47EveningVisual* V = Visual()) V->SetParked(true);
		W->RestoreTransport(nullptr);
		Phase = EPhase::Parked;
		StopBeds();
	}
	// Out of the wagon anywhere in SARO's area: arrived (the visitor lot as the old drive asked, or wherever it was left there).
	if (Phase == EPhase::Drive && !W->bDriving && !W->bBusy && W->Area == TEXT("saro") && M->Mode == US47Main::EMode::Play) Arrive();
	FS47Truck* Carrier = W->CabTruck();
	if (!Carrier) Carrier = W->TruckHere();
	if (FS47EveningVisual* V = Visual())
		V->Tick(Phase == EPhase::Drive, W->bDriving, Carrier, Phase == EPhase::Parked, false, false, false, false, false);
	StepRadio(bHostOn);
	if (S47StoryDriveLocal::LiveTest())
	{
		const FS47EveningVisual* V = Visual();
		if (Phase == EPhase::Drive && W->bDriving)
		{
			Tell(W->bCommuteVehicle && V && V->PersonalVisible() && OwnsRadio(), TEXT("on the drive the player drives his own station wagon (the truck rig hidden), and its radio owns the dial"));
			Tell(V && V->ServiceWaitingVisible() == (W->Area == TEXT("saro")), TEXT("SARO 07 waits on its pad while the wagon is driven"));
			if (Audible()) Tell(Bed.IsValid() || Intro.IsValid(), FString::Printf(TEXT("the drive's radio has its music bed (%s)"), Intro ? TEXT("Intro Drive") : TEXT("the old bed")));
		}
		if (Phase == EPhase::Parked && W->Area == TEXT("saro") && !W->bDriving && !W->bBusy)
			Tell(!W->bCommuteVehicle && V && V->PersonalVisible() && W->TruckAt == TEXT("saro") && !OwnsRadio(),
				TEXT("on arrival the wagon stays parked at SARO, the truck is on its pad and the car radio is the truck's again"));
	}
}

void FS47StoryDrive::Tell(bool bPass, const FString& What)
{
	if (Told.Contains(What)) return;
	Told.Add(What);
	UE_LOG(LogS47StoryDrive, Display, TEXT("S47: story drive test %s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *What);
}

// ---------- the wagon's radio ----------

bool FS47StoryDrive::OwnsRadio() const
{
	return Phase == EPhase::Drive && M && IsValid(M->World) && M->World->bCommuteVehicle;
}

bool FS47StoryDrive::Audible() const
{
	return OwnsRadio() && M->World->bDriving && bRadioOn && !M->ModalOpen();
}

bool FS47StoryDrive::HostMayTalk() const
{
	if (!OwnsRadio()) return true;
	// (Before the first tick opens it, the intro is assumed: the host waits a frame rather than talk over its first bar.)
	const bool bIntro = Intro.IsValid() || (!bNoIntro && !bIntroDone);
	const double Len = Intro ? Intro->Duration() : 1e9;
	return !IntroHolds(bIntro, bIntroDone, Intro ? Intro->CurrentTime() : IntroAt, Len);
}

void FS47StoryDrive::ToggleRadio()
{
	if (!OwnsRadio()) return;
	bRadioOn = !bRadioOn;
	if (M->Audio) { FS47Sound O; O.Gain = 0.5; O.Rate = 0.8; M->Audio->Sfx(TEXT("click"), O); }
	if (M->UI) M->UI->Toast(bRadioOn ? TEXT("Local evening radio on") : TEXT("Radio off"), 2);
	UE_LOG(LogS47StoryDrive, Display, TEXT("S47: story drive: radio %s"), bRadioOn ? TEXT("on") : TEXT("off"));
}

void FS47StoryDrive::StepRadio(bool bHostOn)
{
	if (!OwnsRadio()) { StopBeds(); return; }
	if (FS47Truck* T = M->World->CabTruck()) T->SetRadio(S47Radio::DialX(Dial), bRadioOn);
	if (!M->Audio || !M->Audio->Ctx) return;
	const bool bAudible = Audible();
	// Tom's Intro Drive once in full from the start; then the old bed, looped.
	if (!bIntroDone && !bNoIntro && !Intro)
	{
		const FString File = IntroFile();
		Intro = File.IsEmpty() ? TSharedPtr<FS47MediaSource>() : FS47MediaSource::Open(M->Audio->Ctx.ToSharedRef(), File);
		if (Intro)
		{
			IntroLevel = M->Audio->Ctx->CreateGain(); IntroLevel->Gain.SetValue(IntroGain);
			Intro->Connect(IntroLevel); IntroLevel->Connect(M->Audio->RadioBus);
			Intro->Seek(IntroAt); Intro->Play();
			UE_LOG(LogS47StoryDrive, Display, TEXT("S47: story drive: Intro Drive on the radio (%.0f s, from %.0f s)"), Intro->Duration(), IntroAt);
		}
		else bNoIntro = true; // not in this build (or not PCM WAVE): the old bed throughout
	}
	if (Intro)
	{
		// REVIEW-20261113: in and out, and the duck under the host, glide (no hard pause, no stepped gain)
		Gate(Intro, IntroLevel, bAudible, bHostOn ? IntroGain * IntroDuck : IntroGain);
		IntroAt = Intro->CurrentTime();
		if (Intro->Duration() > 0 && IntroAt >= Intro->Duration() - 0.2)
		{
			Intro->Stop(); Intro->Disconnect(); Intro.Reset(); IntroLevel->Disconnect(); IntroLevel.Reset();
			bIntroDone = true; IntroAt = 0;
		}
		return;
	}
	if (!Bed)
	{
		Bed = FS47MediaSource::Open(M->Audio->Ctx.ToSharedRef(), LoopBed);
		if (!Bed) return;
		BedLevel = M->Audio->Ctx->CreateGain(); BedLevel->Gain.SetValue(LoopGain);
		Bed->Connect(BedLevel); BedLevel->Connect(M->Audio->RadioBus);
		Bed->Seek(BedAt); Bed->Play();
	}
	// As the old drive: the bed waits while the host speaks, and goes round again at its end.
	const bool bQuiet = !bAudible || bHostOn;
	Gate(Bed, BedLevel, !bQuiet, LoopGain); // REVIEW-20261113: out over ~50 ms before it waits, back in over ~45 ms
	if (!bQuiet && Bed->CurrentTime() >= Bed->Duration() - 0.2) Bed->Seek(0);
	BedAt = Bed->CurrentTime();
}

bool FS47StoryDrive::Choice(US47Main* Main, FString& OutLabel, TFunction<void()>& OutUse)
{
	FS47StoryDrive* D = Of(Main);
	if (!D || !D->OwnsRadio() || !IsValid(Main->World) || Main->World->bDriving || Main->World->bBusy) return false;
	OutLabel = TEXT("Drive your car to SARO");
	OutUse = [Main]
	{
		if (!IsValid(Main->World) || Main->World->bDriving || Main->World->bBusy || !Main->World->bCommuteVehicle) return;
		FS47TripOpts O;
		O.Hint = TEXT("SARO: stop anywhere in the visitor parking area left of the sign.");
		Main->World->StartTrip(Main->World->TruckAt.IsEmpty() ? FString(TEXT("road")) : Main->World->TruckAt, TEXT("saro"), O);
	};
	return true;
}

bool FS47StoryDrive::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	const auto Check = [&](bool bPass, const FString& What) { Lines.Add(FString::Printf(TEXT("%s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bPass; };
	Check(Of(nullptr) == nullptr, TEXT("STORY-DRIVE-OWN-CAR: without a Story night's director there is no wagon and no evening host (Freeform keeps its truck and its radio)"));
	// The host never talks over the Intro Drive's opening; over its last seconds, or after it, or without it, he may.
	Check(IntroHolds(true, false, 10, 186) && !IntroHolds(true, false, 30, 186) && IntroHolds(true, false, 2, 8) && !IntroHolds(true, false, 5, 8) && !IntroHolds(true, true, 0, 186) && !IntroHolds(false, false, 0, 186),
		TEXT("STORY-DRIVE-OWN-CAR: the host waits for the Intro Drive and comes in over its tail; without it (or after it) he speaks at once"));
	Check(FString(LoopBed) == TEXT("radio/bama_country.mp3") && FString(IntroBed) == TEXT("signal47_intro_drive") && LoopGain == 0.23,
		TEXT("STORY-DRIVE-OWN-CAR: the drive's beds are named constants: Intro Drive first, the old bed at 0.23 after it"));
	{
		const FString Found = IntroFile();
		if (Found.IsEmpty()) Lines.Add(TEXT("NOTE story drive: no Intro Drive in Content/Licensed/Raw in this build; the old bed plays throughout"));
		else Lines.Add(FString::Printf(TEXT("NOTE story drive: Intro Drive found at %s"), *Found));
	}
	// The save: nothing while there is no drive (an older save has no car); a car save never acts without a game.
	{
		FS47StoryDrive D(nullptr);
		Check(!D.Save().IsValid() && D.GetPhase() == EPhase::None, TEXT("STORY-DRIVE-OWN-CAR: no drive, no \"car\" in the save (older saves load as before)"));
		D.Load(S47J::Read(TEXT("{\"v\":1,\"phase\":\"drive\"}")));
		Check(D.GetPhase() == EPhase::None, TEXT("STORY-DRIVE-OWN-CAR: a car save without a game is left alone"));
	}
	return bOk;
}
