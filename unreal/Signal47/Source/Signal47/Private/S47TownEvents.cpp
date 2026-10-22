#include "S47TownEvents.h"

#include "S47Docs.h"
#include "S47Lines.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Town.h"
#include "S47UI.h"

namespace
{
	constexpr double GAP = 10 * 60, RING = 30, LINE = 47;
	FVector Phone() { const FVector2D P = S47Lines::PLACES()[TEXT("phone")]; return FVector(P.X, 1.4, P.Y); }
}

TArray<FS47TownMark> S47TownMarks(const FS47TownSeen& Seen)
{
	TArray<FS47TownMark> Out;
	for (const FString& K : Seen.Keys) if (K.StartsWith(TEXT("mark:")))
	{
		FString Kind, C; if (K.Mid(5).Split(TEXT("@"), &Kind, &C)) Out.Add({ Kind, FCString::Atod(*C) });
	}
	Out.Sort([](const FS47TownMark& A, const FS47TownMark& B) { return A.Clock < B.Clock; }); return Out;
}
void FS47TownEvents::Note(const FString& Text) { if (M && M->Story) M->Story->Note(Text); }
void FS47TownEvents::Toast(const FString& Text, double Secs) { if (M && M->UI) M->UI->Toast(Text, float(Secs)); }
void FS47TownEvents::Save() { if (M && M->Mode == US47Main::EMode::Play) M->RequestAutosave(); }
void FS47TownEvents::Bind(FS47Town* InTown)
{
	Town = InTown; if (!Town) return; Town->OnAnswer = [this] { Answer(); };
	Town->SetCutout(!M || !M->TownSeen.Contains(TEXT("ev:cutout")));
}
void FS47TownEvents::Reset()
{
	StopAll(); bWaiting = false; bCarSeen = M && M->TownSeen.Contains(TEXT("ev:car"));
	if (Town) Town->SetCarOut(false); bCutoutSeen = M && M->TownSeen.Contains(TEXT("ev:cutout"));
	if (Town) Town->SetCutout(!bCutoutSeen);
}
double FS47TownEvents::Last() const
{
	double V = -1e9; if (M) for (const FString& K : M->TownSeen.Keys) if (K.StartsWith(TEXT("mark:")))
	{
		int32 At; if (K.FindChar(TEXT('@'), At)) V = FMath::Max(V, FCString::Atod(*K.Mid(At + 1)));
	}
	return V;
}
bool FS47TownEvents::Free(double Clock) const { return Now == TEXT("none") && Clock - Last() >= GAP; }
void FS47TownEvents::Mark(const FString& Kind, double Clock) { if (M) M->TownSeen.Add(FString::Printf(TEXT("mark:%s@%.0f"), *Kind, FMath::FloorToDouble(Clock + 0.5))); }
FVector FS47TownEvents::Pos(const FVector& Local) const { return Town ? Town->Group.LocalToWorld(Local) : Local; }

// townEvents.ts update: line A, cutout, car, payphone, lamps, in the original order.
void FS47TownEvents::Update(double Dt, const FS47TownEventsIn& S)
{
	T = S.T; if (!Town || !M) return;
	const double Day = FMath::Fmod(FMath::Fmod(S.Clock, 86400.0) + 86400, 86400.0);
	const FVector Cam = S.Cam.Pos, Local = Town->Group.WorldToLocal(Cam);
	FS47TownSeen& Seen = M->TownSeen; US47Audio* A = M->Audio;
	LineCheck += Dt;
	if (LineCheck > 1 && !Seen.Contains(TEXT("lineA")))
	{
		LineCheck = 0; int32 N = 0; for (const auto& Mark0 : S47TownMarks(Seen)) if (S47Lines::LineOfMark(Mark0.Kind) == TEXT("A")) ++N;
		if (N >= 3) { Seen.Add(TEXT("lineA")); Note(S47Text::ClockText(S.Clock, false) + TEXT(". Three on one line. Drawn in on the map: through Caliche, and on to the north-west.")); Save(); }
	}
	if (Town->bCutoutOut)
	{
		const FVector P = Town->CutoutAt; const double Dist = FMath::Sqrt(FMath::Square(Local.X - P.X) + FMath::Square(Local.Z - P.Z));
		if (!bCutoutSeen && S.bOk && Dist < 45)
		{
			const FVector To = (Pos(FVector(P.X, P.Y + 0.6, P.Z)) - Cam).GetSafeNormal();
			if (FVector::DotProduct(S.Cam.Forward, To) > 0.85)
			{
				bCutoutSeen = true; Seen.Add(TEXT("ev:cutout")); Mark(TEXT("cutout"), S.Clock);
				Note(S47Text::ClockText(S.Clock, false) + TEXT(". The highway north of Caliche: the cardboard alien from the souvenir shop, standing on the shoulder. Kids, probably.")); Save();
			}
		}
		if (bCutoutSeen && Dist > 150) Town->SetCutout(false);
	}
	{
		const FVector P = Town->CarShoulder; const double Dist = FMath::Sqrt(FMath::Square(Local.X - P.X) + FMath::Square(Local.Z - P.Z));
		if (!Town->bCarOut && !bCarSeen && !Town->bDeputyIn && Day >= 3 * 3600 + 12 * 60 && Day < 4 * 3600 + 30 * 60 && Dist > 200) Town->SetCarOut(true);
		if (Town->bCarOut && !bCarSeen && S.bOk && Dist < 25)
		{
			bCarSeen = true; Seen.Add(TEXT("ev:car")); Mark(TEXT("car"), S.Clock);
			Note(S47Text::ClockText(S.Clock, false) + TEXT(". The deputy's car on the shoulder of the highway north of Caliche: engine running, the door open, nobody in it.")); Save();
		}
		if (Town->bCarOut && bCarSeen && Dist > 150) Town->SetCarOut(false);
	}
	const FVector AtPhone = Pos(Phone()); const double DPhone = FVector::Distance(Cam, AtPhone);
	if (Now == TEXT("ringing"))
	{
		RingT += Dt; if (RingT > RING || !S.bInTown || !S.bOk) { StopRing(); NotBefore = S.Clock + 4 * 60; }
	}
	else if (Now == TEXT("line"))
	{
		if (DPhone > 6 || !S.bInTown || !S.bOk) { StopAll(); NotBefore = S.Clock + 4 * 60; }
		else { LineT += Dt; if (LineT >= LINE) HangUp(S.Clock); }
	}
	Near = S.bOk && S.bInTown && DPhone < 45 ? Near + Dt : 0;
	const int32 Call0 = !Seen.Contains(TEXT("ev:phone1")) ? 1 : S.bLater && !Seen.Contains(TEXT("ev:phone2")) ? 2 : 0;
	const double PhoneUntil = Call0 == 2 ? 5 * 3600 + 19 * 60 : 4 * 3600 + 45 * 60;
	const bool bCould = Call0 != 0 && Near > 0 && S.Clock >= NotBefore && Day >= 2 * 3600 + 40 * 60 && Day < PhoneUntil && Free(S.Clock);
	bWaiting = bCould && Call0 == 1;
	const double U = bWaiting ? FMath::Clamp(S.Unrest, 0.0, 1.0) : 0;
	if (U > 0.15) { TinkT -= Dt * (0.4 + U); if (TinkT <= 0) { TinkT = 4 + FMath::FRand() * 6; Tink(AtPhone, U); } }
	if (bCould && Near > 20 && (Call0 == 2 || S.bMayLong))
	{
		Call = Call0; Now = TEXT("ringing"); RingT = 0; Town->bRinging = true;
		if (A) { FS47Sound O; O.At = AtPhone; O.Gain = 0.8f; A->LoopSfx(TEXT("town-ring"), TEXT("phoneRing"), O); }
	}
	Street = S.bOk && S.bInTown ? Street + Dt : 0;
	if (Now == TEXT("lamps"))
	{
		const double W = T - WaveT; const TArray<FVector> Order = Town->WaveOrder();
		for (int32 K = 0; K < Order.Num(); ++K) if (!Dogs.Contains(K) && W >= K * FS47Town::WAVE_STEP)
		{
			Dogs.Add(K); if (A) { FS47Sound O; O.At = Order[K] + FVector(30 + 10 * (K % 3), 0, 0); O.Gain = 0.55f; O.Rate = float(0.92 + 0.05 * (K % 3)); A->Sfx(K % 2 ? TEXT("dog1") : TEXT("dog0"), O); }
		}
		if (W > FS47Town::WAVE_HOLD + Order.Num() * FS47Town::WAVE_STEP + 1)
		{
			Now = TEXT("none"); Seen.Add(TEXT("ev:lamps")); Mark(TEXT("lamps"), S.Clock);
			Note(S47Text::ClockText(S.Clock, false) + TEXT(". Main Street, Caliche: the street lights out one by one, south to north, and the dogs barking in the same order. Back on in the same order.")); Save();
		}
	}
	else if (!Seen.Contains(TEXT("ev:lamps")) && Street > 40 && Day >= 3 * 3600 + 5 * 60 && Day < 4 * 3600 + 55 * 60 && Free(S.Clock) && S.bMay)
	{
		Now = TEXT("lamps"); WaveT = T; Dogs.Reset(); Town->LampWave(T);
	}
}

void FS47TownEvents::Tink(const FVector& At, double U)
{
	if (!M || !M->Audio) return; US47Audio* A = M->Audio;
	FS47Sound O; O.At = At; O.Gain = float(0.16 + 0.22 * U); O.From = 0.03f; O.Dur = 0.16f; O.Rate = float(0.97 + 0.06 * FMath::FRand());
	const FS47Play H = A->Sfx(TEXT("phoneRing"), O); if (!H || !A->Ctx) return;
	FS47AudioContext::FBatch Batch(*A->Ctx); const double Now0 = A->Ctx->CurrentTime();
	H.Gain->Gain.SetValueAtTime(H.Gain->Gain.Value(), Now0 + 0.035); H.Gain->Gain.SetTargetAtTime(0, Now0 + 0.04, 0.025); ++Tinks;
}
void FS47TownEvents::Answer()
{
	if (Now != TEXT("ringing") || !Town) return; StopRing(); Now = TEXT("line"); LineT = 0;
	const FVector At = Pos(Phone()); US47Audio* A = M ? M->Audio.Get() : nullptr;
	if (Call == 1)
	{
		if (A) { FS47Sound O; O.At = At; O.Gain = 0.35f; A->LoopSfx(TEXT("town-line"), TEXT("printer"), O); }
		Toast(TEXT("No dial tone. On the line, a printer: the control room printer, line by line, the way it printed at 02:13."), 6);
	}
	else
	{
		if (A)
		{
			FS47Sound O; O.At = At; O.Gain = 0.12f; A->LoopSfx(TEXT("town-line"), A->Or(TEXT("line_hush"), TEXT("gust")), O);
			const TSharedPtr<FS47AudioNode> Ear = A->Spatial(At, 1.2f);
			if (Ear)
			{
				FS47AudioContext::FBatch Batch(*A->Ctx); const FS47PhoneLine L = A->PhoneLine(); L.Gain->Disconnect(); L.Gain->Connect(Ear); L.Gain->Gain.SetValue(0.4);
				LineSpeaker = Ear; LineBand = L.Gain;
				FS47Sound R; R.DestNode = L.Input; R.When = 0.4f; R.From = 0.7f; R.Dur = 1.5f; Recordings.Add(A->Sfx(TEXT("engineStart"), R));
				R.When = 1.9f; R.From = 0.75f; R.Dur = 1.1f; R.Rate = 0.93f; Recordings.Add(A->Sfx(TEXT("engineStart"), R));
				R.When = 2.9f; R.From = 0; R.Dur.Reset(); R.Rate = 0.9f; Recordings.Add(A->Sfx(TEXT("engineOff"), R));
			}
			for (int32 I = 0; I < 11; ++I) { FS47Sound C; C.At = At; C.Gain = 0.5f; C.When = float(14 + I * 0.32 + (I >= 4 ? 0.9 : 0)); C.Lp = 2200; Recordings.Add(A->Sfx(TEXT("click"), C)); }
		}
		Toast(TEXT("Far off, an engine. It coughs, coughs again and stops. Then nothing at all."), 6);
	}
}
void FS47TownEvents::HangUp(double Clock)
{
	StopRecording(); if (M && M->Audio) M->Audio->Stop(TEXT("town-line"), 0.2f); Now = TEXT("none"); if (Town) Town->bRinging = false;
	const FString Clock0 = S47Text::ClockText(Clock, false);
	if (M && Call == 1)
	{
		M->TownSeen.Add(TEXT("ev:phone1")); M->TownSeen.Add(TEXT("nickel")); Mark(TEXT("phone"), Clock);
		Toast(TEXT("The line clicks and goes dead. Something drops into the coin return: a nickel, dated 1947, bright as new."), 5.5);
		Note(Clock0 + TEXT(". Payphone, Main Street, Caliche: 47 s of the control room printer on the line. A 1947 nickel in the coin return."));
	}
	else if (M)
	{
		M->TownSeen.Add(TEXT("ev:phone2")); Mark(TEXT("phone2"), Clock);
		Toast(TEXT("Clicks on the line: four, and seven. Then the dial tone, as if nothing had been there."), 5);
		Note(Clock0 + TEXT(". Payphone, Main Street, Caliche, again: an engine coughing and dying on the line, then nothing, then relay clicks, four and seven."));
	}
	Save();
}
void FS47TownEvents::StopRing()
{
	if (M && M->Audio) M->Audio->Stop(TEXT("town-ring"), 0.05f); if (Town) Town->bRinging = false;
	if (Now == TEXT("ringing")) Now = TEXT("none");
}
void FS47TownEvents::StopAll()
{
	StopRing(); if (M && M->Audio) M->Audio->Stop(TEXT("town-line"), 0.05f); StopRecording(); Now = TEXT("none"); Near = 0; Street = 0; NotBefore = 0;
}
void FS47TownEvents::StopRecording()
{
	for (const FS47Play& H : Recordings) if (H) { H.Src->Stop(); if (H.Gain) H.Gain->Disconnect(); } Recordings.Reset();
	if (LineBand) LineBand->Disconnect(); LineBand.Reset(); if (LineSpeaker) LineSpeaker->Disconnect(); LineSpeaker.Reset();
}

bool FS47TownEvents::SelfTest(TArray<FString>& Lines)
{
	US47Main* Main = NewObject<US47Main>(); FS47Town Town0(nullptr); FS47TownEvents E(Main); E.Bind(&Town0);
	FS47TownEventsIn S; S.bOk = S.bInTown = true; S.Clock = 3 * 3600; S.Cam.Pos = Phone(); S.bMay = false; S.bMayLong = false;
	E.Update(21, S); bool Ok = E.bWaiting && E.Now == TEXT("none"); S.bMayLong = true; E.Update(0.1, S); Ok &= E.Now == TEXT("ringing");
	E.Answer(); E.Update(46.9, S); Ok &= !Main->TownSeen.Contains(TEXT("nickel")); E.Update(0.1, S); Ok &= Main->TownSeen.Contains(TEXT("nickel")) && E.Now == TEXT("none");
	S.bLater = true; S.Clock = 5 * 3600; E.Reset(); E.Update(21, S); E.Answer(); E.Update(47, S); Ok &= Main->TownSeen.Contains(TEXT("ev:phone2"));
	Main->TownSeen.Clear(); E.Reset(); S.Clock = 3 * 3600; E.Update(21, S); E.Answer(); S.Cam.Pos.X += 7; E.Update(1, S); Ok &= E.Now == TEXT("none") && !Main->TownSeen.Contains(TEXT("nickel"));
	Main->TownSeen.Add(TEXT("mark:lamps@3")); Main->TownSeen.Add(TEXT("mark:phone@1")); Ok &= S47TownMarks(Main->TownSeen)[0].Kind == TEXT("phone");
	Lines.Add(FString::Printf(TEXT("%s TownEvents: curve wait, ringing, full 47 s, return call, interrupted recording and sorted marks"), Ok ? TEXT("PASS") : TEXT("FAIL"))); return Ok;
}
