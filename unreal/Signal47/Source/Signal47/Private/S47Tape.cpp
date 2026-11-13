#include "S47Tape.h"
#include "S47AlienKinds.h"
#include "S47Audio.h"
#include "S47Dossier.h"
#include "S47EventPlaces.h"
#include "S47FarmSite.h"
#include "S47Freeform.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Keys.h"
#include "S47Main.h"
#include "S47Omen.h"
#include "S47Props.h"
#include "S47Script.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47FieldMap.h"
#include "Components/StaticMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Tape, Log, All);

namespace S47TapeLocal
{
	const TCHAR* const DeskProp = TEXT("cassette");   // Config/Props/saro.json: the portable recorder by the array's PC
	const TCHAR* const DeskUse = TEXT("ff:tape:desk");
	const TCHAR* const HissKey = TEXT("tape-hiss"), * const BedKey = TEXT("tape-bed");
	const TCHAR* const SigKey = TEXT("tape"); // EVIDENCE-DOSSIER slice 2: a known one's signature at the desk (S47AlienKinds::Follow)
	// The odd-sound generator's kind for a take whose one is known, -1 for none.
	int32 AlienOf(const FS47Tape::FTake& Tk) { return Tk.Odd.IsEmpty() ? -1 : S47Dossier::AlienKindOf(S47Dossier::KindOfId(Tk.OddKind)); }
	// A cassette's sound: the top end gone, the level down a little.
	constexpr float TapeLp = 3800, TapeGain = .7f;
	// REVIEW (dossier slice 2): the counter and the clock as the player reads them never print 47 (S47Script's shared rule)
	FString Counter(double Seconds) { return S47Script::PlayerCounter(Seconds); }
	FString ClockText(double Clock) { return S47Script::PlayerClock(Clock); }
	// The record of a take whose odd sound was heard through at the desk (and the self-test's sweep of it).
	FString HeardText(int32 N, const FString& Where, int32 Day, double Clock, double OddAt, const FString& Words)
	{
		// (the cassette's number is counted past any that would read 47, so no two cassettes share a number)
		int32 Shown=0;
		for(int32 K=0;K<FMath::Clamp(N,0,5000);++K){++Shown;while(FString::FromInt(Shown).Contains(TEXT("47"))){++Shown;}}
		return FString::Printf(TEXT("Cassette %d, recorded %s, day %d, %s. Played back at SARO: at %s on the counter you can hear %s. I did not hear it out there. It is on the tape."),
			Shown, *Where, Day, *ClockText(Clock), *Counter(OddAt), *Words);
	}
}

// ---------- the pure rules ----------

float FS47Tape::GainAt(double Metres)
{
	if (!FMath::IsFinite(Metres) || Metres >= 70) return 0.f;
	return float(FMath::Pow(FMath::Clamp(4.0 / FMath::Max(4.0, Metres), 0.0, 1.0), 1.1));
}

bool FS47Tape::Unheard(double Sense, double Level)
{
	return Sense >= .12 && Level < .15;
}

FString FS47Tape::OddSound(const FString& SourceId)
{
	// The kind of thing it was, as a microphone takes it: a body is something like voices, a light a low hum, marks and rings a pulse.
	if (SourceId.StartsWith(TEXT("body")) || SourceId == TEXT("visit") || SourceId == TEXT("door") || SourceId == TEXT("peek")) return TEXT("voices");
	if (SourceId.StartsWith(TEXT("night:landing")) || SourceId == TEXT("highway") || SourceId == TEXT("taken")) return TEXT("drone_52");
	if (SourceId.StartsWith(TEXT("director"))) return TEXT("carrier_hum");
	return TEXT("signal_pulses");
}

FString FS47Tape::OddWords(const FString& Sound)
{
	if (Sound == TEXT("voices")) return TEXT("something like voices, too far off or too low to make out");
	if (Sound == TEXT("drone_52")) return TEXT("a low hum that comes up under the hiss and stops");
	if (Sound == TEXT("carrier_hum")) return TEXT("a steady tone, faint, that is not the wind");
	return TEXT("a slow pulse, regular, under everything else");
}

FString FS47Tape::OddWordsOf(const FString& Sound, const FString& KindId)
{
	// EVIDENCE-DOSSIER slice 2: what each one's signature is like on a cassette, as the table's TAPE column has it. Never what it was.
	switch (S47Dossier::KindOfId(KindId))
	{
	case S47Dossier::EKind::Small: return TEXT("short runs of notes, each climbing a step and ending like a question, and a pause after each");
	case S47Dossier::EKind::Low: return TEXT("a double beat like a heart, quick, and breath that rasps close to the microphone");
	case S47Dossier::EKind::Tall: return TEXT("slow deep pulses, two low notes grinding on each other, and something like metal creaking");
	default: return OddWords(Sound);
	}
}

int32 FS47Tape::OddHeardCount() const
{
	int32 Count = 0;
	for (const FTake& Tk : Kept) Count += Tk.bOddHeard ? 1 : 0;
	return Count;
}

// ---------- a game's life ----------

void FS47Tape::Begin(const S47J::FObj& Saved)
{
	Stop();
	Life = MakeShared<uint8>(0);
	Kept.Reset(); Next = 1; bCarried = false;
	if (Saved.IsValid())
	{
		bCarried = S47J::Bool(Saved, TEXT("carried"));
		Next = FMath::Clamp(S47J::Int(Saved, TEXT("next"), 1), 1, 100000);
		for (const auto& V : S47J::Arr(Saved, TEXT("takes")))
		{
			const S47J::FObj* O = nullptr;
			if (!V.IsValid() || !V->TryGetObject(O) || !O) continue;
			FTake Tk;
			Tk.N = S47J::Int(*O, TEXT("n")); Tk.Day = S47J::Int(*O, TEXT("day")); Tk.Clock = S47J::Num(*O, TEXT("clock"));
			Tk.Where = S47J::Str(*O, TEXT("where")).Left(120); Tk.bOutdoors = S47J::Bool(*O, TEXT("out"), true);
			Tk.RoadX = S47J::Num(*O, TEXT("x"), 0); Tk.RoadZ = S47J::Num(*O, TEXT("z"), 0);
			Tk.bPlaced = (*O)->HasField(TEXT("x")) && FMath::IsFinite(Tk.RoadX) && FMath::IsFinite(Tk.RoadZ) && FMath::Abs(Tk.RoadX) < 1e6 && FMath::Abs(Tk.RoadZ) < 1e6;
			Tk.Seconds = FMath::Clamp(float(S47J::Num(*O, TEXT("seconds"))), 0.f, SideSeconds);
			Tk.Odd = S47J::Str(*O, TEXT("odd")).Left(40); Tk.OddAt = float(S47J::Num(*O, TEXT("oddAt"), -1)); Tk.OddGain = FMath::Clamp(float(S47J::Num(*O, TEXT("oddGain"))), 0.f, 1.f);
			{ const FString Kind = S47J::Str(*O, TEXT("oddKind")); if (!Tk.Odd.IsEmpty() && S47Dossier::KindOfId(Kind) != S47Dossier::EKind::None) Tk.OddKind = Kind; } // slice 2
			Tk.bPlayed = S47J::Bool(*O, TEXT("played")); Tk.bOddHeard = S47J::Bool(*O, TEXT("oddHeard"));
			for (const auto& Sv : S47J::Arr(*O, TEXT("sounds")))
			{
				const S47J::FObj* So = nullptr;
				if (!Sv.IsValid() || !Sv->TryGetObject(So) || !So || Tk.Sounds.Num() >= MostSounds) continue;
				FSound Clip; Clip.At = float(S47J::Num(*So, TEXT("t"))); Clip.Name = S47J::Str(*So, TEXT("s")).Left(80);
				Clip.Gain = FMath::Clamp(float(S47J::Num(*So, TEXT("g"), 1)), 0.f, 2.f); Clip.Rate = FMath::Clamp(float(S47J::Num(*So, TEXT("r"), 1)), .25f, 4.f); Clip.Lp = float(S47J::Num(*So, TEXT("lp")));
				if (!Clip.Name.IsEmpty() && Clip.At >= 0 && Clip.At <= SideSeconds) Tk.Sounds.Add(Clip);
			}
			if (Tk.N > 0 && Kept.Num() < MostTakes) Kept.Add(Tk);
		}
	}
	// The key, once: through the session, so that a new one finds it too (as the counter's).
	if (M && !M->Keys.ContainsByPredicate([](const US47Main::FKeyHandler& K) { return K.Action == FName(TEXT("tape")); }))
	{
		US47Main* Owner = M;
		M->OnAction(TEXT("tape"), [Owner] { return IsValid(Owner) && Owner->IsFreeform() && Owner->Freeform && Owner->Freeform->Tape && Owner->Freeform->Tape->ToggleRecord(); });
	}
	bHooked = false;
}

void FS47Tape::Stop()
{
	if (bRec) Finish();
	StopPlay();
	if (M && IsValid(M->Audio)) M->Audio->Tap = nullptr;
	Life.Reset();
}

S47J::FObj FS47Tape::Save() const
{
	auto O = S47J::New();
	if (!bCarried && Kept.IsEmpty()) return O;
	S47J::Set(O, TEXT("carried"), bCarried);
	S47J::Set(O, TEXT("next"), Next);
	TArray<TSharedPtr<FJsonValue>> List;
	for (const FTake& Tk : Kept)
	{
		auto E = S47J::New();
		S47J::Set(E, TEXT("n"), Tk.N); S47J::Set(E, TEXT("day"), Tk.Day); S47J::Set(E, TEXT("clock"), Tk.Clock); S47J::Set(E, TEXT("where"), Tk.Where);
		S47J::Set(E, TEXT("out"), Tk.bOutdoors); S47J::Set(E, TEXT("seconds"), double(Tk.Seconds));
		if (Tk.bPlaced) { S47J::Set(E, TEXT("x"), FMath::RoundToDouble(Tk.RoadX * 10) / 10); S47J::Set(E, TEXT("z"), FMath::RoundToDouble(Tk.RoadZ * 10) / 10); }
		if (!Tk.Odd.IsEmpty()) { S47J::Set(E, TEXT("odd"), Tk.Odd); S47J::Set(E, TEXT("oddAt"), double(Tk.OddAt)); S47J::Set(E, TEXT("oddGain"), double(Tk.OddGain)); if (!Tk.OddKind.IsEmpty()) S47J::Set(E, TEXT("oddKind"), Tk.OddKind); }
		S47J::Set(E, TEXT("played"), Tk.bPlayed); S47J::Set(E, TEXT("oddHeard"), Tk.bOddHeard);
		TArray<TSharedPtr<FJsonValue>> Snds;
		for (const FSound& Clip : Tk.Sounds)
		{
			auto So = S47J::New();
			S47J::Set(So, TEXT("t"), FMath::RoundToDouble(Clip.At * 100) / 100); S47J::Set(So, TEXT("s"), Clip.Name); S47J::Set(So, TEXT("g"), FMath::RoundToDouble(Clip.Gain * 1000) / 1000);
			if (Clip.Rate != 1.f) S47J::Set(So, TEXT("r"), double(Clip.Rate));
			if (Clip.Lp > 0) S47J::Set(So, TEXT("lp"), double(Clip.Lp));
			Snds.Add(MakeShared<FJsonValueObject>(So));
		}
		E->SetArrayField(TEXT("sounds"), Snds);
		List.Add(MakeShared<FJsonValueObject>(E));
	}
	O->SetArrayField(TEXT("takes"), List);
	return O;
}

// ---------- the desk ----------

bool FS47Tape::Hook()
{
	if (bHooked) return true;
	if (!M || !M->Web || !M->Inter) return false;
	const int32 Node = AS47Props::PlacedNode(S47TapeLocal::DeskProp);
	if (Node < 0) return false;
	// FIX-20261110: on the hall dresser; without the dresser's model the recorder goes back where it was, on the desk.
	bHall = AS47Props::PlacedMesh(TEXT("hallDresser")) != nullptr;
	if (!bHall) if (UStaticMeshComponent* Mesh = AS47Props::PlacedMesh(S47TapeLocal::DeskProp)) { Mesh->SetWorldLocation(S47Three::ToUE(FVector(-3.12, .78, -3.82))); UE_LOG(LogS47Tape, Display, TEXT("S47: tape: no hall dresser in this build, the recorder is on the control room desk")); }
	FS47Interactable I;
	I.Id = S47TapeLocal::DeskUse;
	I.Object = M->Web->ObjAt(Node);
	I.Range = 2.6;
	const TWeakPtr<uint8> Guard = Life;
	I.Label = [this, Guard]() -> FString
	{
		if (!Guard.IsValid() || !M || !M->IsFreeform()) return FString();
		if (bCarried) return FString::Printf(TEXT("Put the field recorder back on the %s"), Top());
		if (Playing()) return TEXT("Stop the tape");
		return Kept.IsEmpty() ? FString(TEXT("Field recorder / take it with you")) : FString(TEXT("Field recorder / play a cassette, or take it with you"));
	};
	I.Use = [this, Guard] { if (Guard.IsValid()) UseDesk(); };
	I.Examine = [this, Guard]() -> FString
	{
		if (!Guard.IsValid()) return FString();
		return bCarried ? FString::Printf(TEXT("The recorder's place on the %s, a clean square in the dust."), Top()) : FString(TEXT("A portable cassette recorder, the kind radio people carry: a meter, a microphone socket, a C-60 in it."));
	};
	M->Inter->Add(I);
	M->Web->MarkUsable(Node, I.Id);
	bHooked = true;
	Show();
	return true;
}

void FS47Tape::Show()
{
	if (UStaticMeshComponent* Mesh = AS47Props::PlacedMesh(S47TapeLocal::DeskProp)) Mesh->SetVisibility(!bCarried, true);
}

FVector FS47Tape::Desk() const
{
	if (const UStaticMeshComponent* Mesh = AS47Props::PlacedMesh(S47TapeLocal::DeskProp)) return S47Three::ToThree(Mesh->GetComponentLocation());
	return FVector(-3.12, .9, -3.82);
}

FVector FS47Tape::Ear() const
{
	if (!M || !M->Player) return FVector::ZeroVector;
	return FVector(M->Player->Pos.X, M->Player->FloorY + M->Player->Eye, M->Player->Pos.Z);
}

FString FS47Tape::PlaceTitle() const
{
	if (M && M->Freeform && M->Freeform->EventPlaces)
		if (const FS47EventPlace* Pl = M->Freeform->EventPlaces->Nearest(600)) return Pl->Title;
	return TEXT("out on the land");
}

void FS47Tape::UseDesk()
{
	if (!M || !M->Freeform) return;
	if (bCarried)
	{
		if (bRec) Finish();
		bCarried = false; Show();
		if (M->Audio) M->Audio->Play(TEXT("click"), .5f, Desk());
		if (M->UI) M->UI->Toast(Kept.IsEmpty() ? FString::Printf(TEXT("Recorder back on the %s."), Top()) : FString::Printf(TEXT("Recorder back on the %s. Use it to play a cassette."), Top()), 4);
		M->RequestAutosave();
		return;
	}
	if (Playing()) { StopPlay(); return; }
	OpenDesk();
}

void FS47Tape::OpenDesk()
{
	if (!M || !M->UI) return;
	using namespace S47TapeLocal;
	TArray<S47Kit::FW> Offer;
	const TWeakPtr<uint8> Guard = Life;
	Offer.Add(S47Kit::Paper(TEXT("A portable cassette recorder. The station's field recorder, a C-60 in it, a stick microphone on a cord. Press RECORD out there, play it back here.")));
	for (int32 I = Kept.Num() - 1, Shown = 0; I >= 0 && Shown < 6; --I, ++Shown)
	{
		const FTake& Tk = Kept[I];
		const FString Label = FString::Printf(TEXT("PLAY / Tape %d, day %d, %s, %s / %s"), Tk.N, Tk.Day, *ClockText(Tk.Clock), *Tk.Where, *Counter(Tk.Seconds));
		Offer.Add(S47Kit::Button(Label, [this, Guard, I] { if (Guard.IsValid()) { if (M && M->UI) M->UI->Close(); PlayTake(I); } }, I == Kept.Num() - 1));
	}
	Offer.Add(S47Kit::Button(TEXT("Take the recorder with you"), [this, Guard]
	{
		if (!Guard.IsValid() || !M) return;
		bCarried = true; Show();
		if (M->UI) { M->UI->Close(); M->UI->Toast(FString::Printf(TEXT("Field recorder. %s starts and stops it; a side runs about half an hour."), *S47Keys::Said(TEXT("tape"))), 5); }
		if (M->Audio) M->Audio->Play(TEXT("click"), .5f, Desk());
		UE_LOG(LogS47Tape, Display, TEXT("S47: tape: recorder taken from the desk"));
		M->RequestAutosave();
	}, Kept.IsEmpty()));
	Offer.Add(S47Kit::Button(TEXT("Leave it"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("FIELD RECORDER"), S47Kit::Col(Offer), 860));
}

// ---------- recording ----------

bool FS47Tape::ToggleRecord()
{
	if (!M || !M->Freeform || !M->Freeform->Active()) return false;
	if (bRec) { Finish(); return true; }
	if (!bCarried)
	{
		if (M->UI) M->UI->Toast(FString::Printf(TEXT("The field recorder is %s."), Where()), 3.5f);
		return true;
	}
	if (!IsValid(M->Audio)) return false;
	bRec = true; RecT = 0; SenseTry = 0; BestSense = 0; BestSource.Empty(); BestKind.Empty();
	Rec = FTake();
	Rec.N = Next; Rec.Day = M->Freeform->Day(); Rec.Clock = M->Freeform->Clock(); Rec.Where = PlaceTitle();
	Rec.bOutdoors = !(M->World && M->World->Indoors(Ear()));
	{
		double Facing = 0; FString OnLeg; bool bInCab = false;
		Rec.bPlaced = S47FieldMap::Me(M, Rec.RoadX, Rec.RoadZ, Facing, OnLeg, bInCab);
	}
	const TWeakPtr<uint8> Guard = Life;
	M->Audio->Tap = [this, Guard](const FString& Name, const FS47Sound& O) { if (Guard.IsValid()) Heard(Name, O); };
	M->Audio->Play(TEXT("click"), .45f);
	if (M->UI) M->UI->Toast(FString::Printf(TEXT("Recording. Tape %d. %s stops it."), Rec.N, *S47Keys::Said(TEXT("tape"))), 3.5f);
	UE_LOG(LogS47Tape, Display, TEXT("S47: tape %d recording at %s, %s"), Rec.N, *Rec.Where, Rec.bOutdoors ? TEXT("outdoors") : TEXT("indoors"));
	return true;
}

void FS47Tape::Heard(const FString& Name, const FS47Sound& O)
{
	if (!bRec || Rec.Sounds.Num() >= MostSounds) return;
	// The score is not in the room; the radio and the walkie are.
	if (O.Dest == TEXT("music")) return;
	float G = O.Gain;
	// M7 (Tom, 9 November): the better field recorder's gun microphone (FS47Openings "field_recorder") hears twice as far
	const bool bGun = M && M->Freeform && M->Freeform->HasKit(TEXT("field_recorder"));
	if (O.At.IsSet()) G *= GainAt(FVector::Distance(*O.At, Ear()) * (bGun ? 0.5 : 1.0));
	else G *= O.Dest == TEXT("radio") || O.Dest == TEXT("radioBus") ? .6f : .5f;
	if (G < .01f) return;
	FSound Clip; Clip.At = float(RecT + O.When); Clip.Name = Name; Clip.Gain = G; Clip.Rate = O.Rate; Clip.Lp = O.Lp;
	if (Clip.At <= SideSeconds) Rec.Sounds.Add(Clip);
}

void FS47Tape::Finish()
{
	if (!bRec) return;
	bRec = false;
	if (M && IsValid(M->Audio)) { M->Audio->Tap = nullptr; M->Audio->Play(TEXT("click"), .45f); }
	Rec.Seconds = float(FMath::Min(RecT, double(SideSeconds)));
	// What the microphone had that the player did not: the nearest it came, if it came at all.
	if (BestSense > 0) { Rec.Odd = OddSound(BestSource); Rec.OddGain = float(FMath::Clamp(BestSense, .15, .6)); Rec.OddKind = BestKind; }
	else { Rec.Odd.Empty(); Rec.OddAt = -1; Rec.OddKind.Empty(); }
	if (Rec.Seconds >= 2)
	{
		Kept.Add(Rec); ++Next;
		while (Kept.Num() > MostTakes) Kept.RemoveAt(0); // taped over
		if (M && M->UI) M->UI->Toast(FString::Printf(TEXT("Tape %d stopped at %s. Play it back on the %s at SARO."), Rec.N, *S47TapeLocal::Counter(Rec.Seconds), Top()), 4);
		UE_LOG(LogS47Tape, Display, TEXT("S47: tape %d stopped: %.0f s, %d sounds, %s"), Rec.N, Rec.Seconds, Rec.Sounds.Num(), Rec.Odd.IsEmpty() ? TEXT("nothing else") : *FString::Printf(TEXT("%s at %.1f s (sense %.2f, %s)"), *Rec.Odd, Rec.OddAt, BestSense, *BestSource));
		if (M) M->RequestAutosave();
	}
}

// ---------- playing back ----------

bool FS47Tape::PlayTake(int32 Index)
{
	if (!Kept.IsValidIndex(Index) || bCarried || !M || !IsValid(M->Audio)) return false;
	StopPlay();
	PlayIndex = Index; PlayNext = 0; PlayT = -.6; bOddPlayed = false;
	const FVector At = Desk();
	M->Audio->Play(TEXT("click"), .5f, At);
	M->Audio->Loop(S47TapeLocal::HissKey, TEXT("static_noise"), .035f, At);
	if (Kept[Index].bOutdoors) M->Audio->Loop(S47TapeLocal::BedKey, TEXT("wind"), .07f, At);
	UE_LOG(LogS47Tape, Display, TEXT("S47: tape %d playing on the desk"), Kept[Index].N);
	return true;
}

void FS47Tape::StopPlay()
{
	if (PlayIndex == INDEX_NONE) return;
	if (Kept.IsValidIndex(PlayIndex)) Kept[PlayIndex].bPlayed = true;
	PlayIndex = INDEX_NONE;
	S47AlienKinds::Hush(S47TapeLocal::SigKey); // EVIDENCE-DOSSIER slice 2
	if (M && IsValid(M->Audio) && !IsEngineExitRequested()) { M->Audio->Stop(S47TapeLocal::HissKey, .1f); M->Audio->Stop(S47TapeLocal::BedKey, .1f); M->Audio->Play(TEXT("click"), .5f, Desk()); }
}

void FS47Tape::Step(double Dt)
{
	if (!M || !M->Freeform || !M->Freeform->Active()) return;
	if (!bHooked) Hook();
	const bool bHeld = M->Freeform->Held();
	if (bRec)
	{
		if (!bCarried) { Finish(); return; }
		if (!bHeld) RecT += Dt;
		// Four times a second: what is out there within the microphone's reach, against what the player can feel of it.
		SenseTry -= Dt;
		if (SenseTry <= 0 && M->Omen)
		{
			SenseTry = .25;
			FVector From = FVector::ZeroVector; bool bComing = false;
			const double Sense = M->Omen->SenseAt(Ear(), M->Freeform && M->Freeform->HasKit(TEXT("field_recorder")) ? 3.6 : 2.4, &From, &bComing);
			if (Unheard(Sense, M->Omen->Level()) && Sense > BestSense)
			{
				BestSense = Sense; Rec.OddAt = float(RecT);
				double Nearest = 1e18;
				for (const FS47OmenSource& Src : M->Omen->Sources())
				{
					const double D = FVector::DistSquared(Src.At, From);
					if (D < Nearest) { Nearest = D; BestSource = Src.Id; }
				}
				// EVIDENCE-DOSSIER slice 2: which one, where its source knows, at this moment (the visit's kind is the visit's now)
				const S47Dossier::EKind Was = S47Dossier::KindOfOmen(BestSource, M->Freeform && M->Freeform->FarmSite ? M->Freeform->FarmSite->VisitKind() : 0);
				BestKind = Was == S47Dossier::EKind::None ? FString() : FString(S47Dossier::KindId(Was));
			}
		}
		if (RecT >= SideSeconds) Finish();
	}
	if (PlayIndex != INDEX_NONE && Kept.IsValidIndex(PlayIndex))
	{
		FTake& Tk = Kept[PlayIndex];
		PlayT += Dt;
		const FVector At = Desk();
		while (PlayNext < Tk.Sounds.Num() && Tk.Sounds[PlayNext].At <= PlayT)
		{
			const FSound& Clip = Tk.Sounds[PlayNext++];
			FS47Sound O; O.Gain = Clip.Gain * S47TapeLocal::TapeGain; O.Rate = Clip.Rate; O.At = At; O.Lp = Clip.Lp > 0 ? FMath::Min(Clip.Lp, S47TapeLocal::TapeLp) : S47TapeLocal::TapeLp;
			M->Audio->Sfx(Clip.Name, O);
		}
		const int32 Alien = S47TapeLocal::AlienOf(Tk);
		if (!Tk.Odd.IsEmpty() && !bOddPlayed && Tk.OddAt >= 0 && PlayT >= Tk.OddAt)
		{
			bOddPlayed = true;
			// EVIDENCE-DOSSIER slice 2: a known one's own signature plays in its place (below), at the level of the hour it was taped
			if (Alien < 0) { FS47Sound O; O.Gain = Tk.OddGain * .55f; O.Rate = .85f; O.At = At; O.Lp = 900; O.Dur = 7.f; M->Audio->Sfx(Tk.Odd, O); }
			UE_LOG(LogS47Tape, Display, TEXT("S47: tape %d at %s: %s%s"), Tk.N, *S47TapeLocal::Counter(Tk.OddAt), *Tk.Odd, Alien >= 0 ? *FString::Printf(TEXT(", the signature of %s"), *Tk.OddKind) : TEXT(""));
		}
		if (Alien >= 0 && Tk.OddAt >= 0)
		{
			const float Level = S47AlienKinds::TierLevel(S47NightTiers::AtClock(Tk.Clock, FS47FreeformSession::NightOf(Tk.Clock))) * FMath::Clamp(Tk.OddGain / .45f, .35f, 1.f);
			S47AlienKinds::Follow(M->Audio, S47TapeLocal::SigKey, true, PlayT - Tk.OddAt + S47AlienKinds::SignalFrom, ES47AlienKind(Alien), S47AlienKinds::TakeSeed(M->Freeform->Seed(), FString::Printf(TEXT("tape%d"), Tk.N)), Level);
		}
		// Heard through to a little past it: it is in the record now, as the player has it.
		if (bOddPlayed && !Tk.bOddHeard && HeardThrough(PlayT, Tk.OddAt))
		{
			Tk.bOddHeard = true;
			const FString Key = FString::Printf(TEXT("tape_%d"), Tk.N);
			M->Freeform->RecordEvidence(Key, S47TapeLocal::HeardText(Tk.N, Tk.Where, Tk.Day, Tk.Clock, Tk.OddAt, OddWordsOf(Tk.Odd, Tk.OddKind)));
			// EVIDENCE-DOSSIER slice 2: heard, a known one's tape goes in the files, with the hour and the place it was taped
			if (Alien >= 0)
			{
				const FVector2D Taped(Tk.RoadX, Tk.RoadZ);
				const int32 Hour = FMath::Clamp(int32(FMath::Fmod(FMath::Max(0., Tk.Clock), 86400.) / 3600.), 0, 23);
				M->Freeform->TagEvidenceAt(Key, S47Dossier::KindOfId(Tk.OddKind), S47Dossier::EType::Tape, Tk.Day, Hour, Tk.bPlaced ? &Taped : nullptr);
			}
			M->RequestAutosave();
		}
		if (PlayT >= Tk.Seconds + .5) StopPlay();
	}
}

void FS47Tape::DevGive()
{
	bCarried = true; Show();
}

// ---------- the self-test ----------

bool FS47Tape::SelfTest(TArray<FString>& Lines)
{
	bool bAll = true;
	const auto Check = [&](bool bOk, const FString& What) { bAll &= bOk; Lines.Add(FString(bOk ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("tape: ") + What); };
	Check(GainAt(1) == 1.f && GainAt(4) == 1.f && GainAt(20) < GainAt(10) && GainAt(10) > 0 && GainAt(70) == 0 && GainAt(500) == 0, TEXT("close is as loud, farther fainter, past 70 m nothing"));
	Check(Unheard(.3, 0) && !Unheard(.3, .4) && !Unheard(.05, 0), TEXT("the tape has what was near enough for a microphone and not for the player"));
	bool bWords = true;
	for (const TCHAR* Src : { TEXT("body:3"), TEXT("night:landing"), TEXT("director:service_presence"), TEXT("visit"), TEXT("highway"), TEXT("something") })
	{
		const FString Clip = OddSound(Src), Said = OddWords(Clip);
		bWords &= !Clip.IsEmpty() && Said.Len() > 10;
		for (const TCHAR* Word : { TEXT("alien"), TEXT("UFO"), TEXT("visitor"), TEXT("Roswell") }) bWords &= !Said.Contains(Word);
	}
	Check(bWords, TEXT("what is on the tape is a sound and plain words for it, never what it was"));
	{
		// EVIDENCE-DOSSIER slice 2: each known one has its own words, in Freeform's limits; an unknown kind keeps the plain ones
		const FString S = OddWordsOf(TEXT("voices"), TEXT("roswell")), P = OddWordsOf(TEXT("voices"), TEXT("pale")), T = OddWordsOf(TEXT("voices"), TEXT("tall"));
		Check(S != P && P != T && S != T && OddWordsOf(TEXT("voices"), FString()) == OddWords(TEXT("voices")) && OddWordsOf(TEXT("voices"), TEXT("nordic")) == OddWords(TEXT("voices"))
			&& S47Dossier::LintWords({ S, P, T }).IsEmpty() && !S.Contains(TEXT("as if")) && S47TapeLocal::HeardText(1, TEXT("x"), 2, 0, 0, T).Contains(TEXT("you can hear slow deep pulses")) && HeardThrough(35.5, 31.5f) && !HeardThrough(33, 31.5f) && !HeardThrough(99, -1),
			TEXT("EVIDENCE-DOSSIER slice 2: the small ones, the low one and the tall one each sound their own on the tape, in plain words, heard through only past it"));
	}
	{
		// REVIEW (dossier slice 2, 14 Nov): every minute of the day and every counter from 0:00 to 20:00, through the heard line with
		// each kind's words: never digits that read 47, never 08:15 or 8:15; a :47 shows a minute (or a second) on, and only that
		bool bClean = true, bNear = true;
		// (18:15 is a quarter past six in the evening, or eighteen minutes on the counter: not the hour the rule keeps out)
		const auto Clean = [](const FString& Line) { return !Line.Contains(TEXT("47")) && !Line.Replace(TEXT("18:15"), TEXT("")).Contains(TEXT("8:15")) && !Line.Contains(TEXT("8.15")); };
		const FString Place = TEXT("the windmill on the Baca lease");
		for (const TCHAR* Kind : { TEXT("roswell"), TEXT("pale"), TEXT("tall"), TEXT("") })
		{
			const FString Words = OddWordsOf(TEXT("voices"), Kind);
			for (int32 Min = 0; Min < 24 * 60; ++Min)
			{
				const FString Shown = S47TapeLocal::ClockText(Min * 60. + 30);
				bClean &= Clean(S47TapeLocal::HeardText(2, Place, 4, Min * 60. + 30, 31.5, Words)) && Clean(Shown.Replace(TEXT(":"), TEXT(""))) && Shown.Len() == 5;
				bNear &= (FCString::Atoi(*Shown.Left(2)) * 60 + FCString::Atoi(*Shown.Right(2)) - Min + 1440) % 1440 <= 1;
			}
			for (int32 Sec = 0; Sec <= 1200; ++Sec)
			{
				const FString Shown = S47TapeLocal::Counter(Sec);
				bClean &= Clean(S47TapeLocal::HeardText(2, Place, 4, 23 * 3600. + 46 * 60, Sec, Words)) && Clean(Shown.Replace(TEXT(":"), TEXT("")));
				int32 Colon = INDEX_NONE;
				const bool bColon = Shown.FindChar(TEXT(':'), Colon) && Colon > 0;
				const int32 Back = bColon ? FCString::Atoi(*Shown.Left(Colon)) * 60 + FCString::Atoi(*Shown.Mid(Colon + 1)) - Sec : -1;
				bNear &= Back >= 0 && Back <= 1;
			}
		}
		Check(bClean && bNear && S47Script::PlayerClock(3 * 3600. + 47 * 60) == TEXT("03:48") && S47Script::PlayerClock(23 * 3600. + 47 * 60 + 59) == TEXT("23:48") && S47Script::PlayerClock(8 * 3600. + 15 * 60) == TEXT("08:16")
			&& S47Script::PlayerClock(20 * 3600. + 15 * 60) == TEXT("20:16") && S47Script::PlayerClock(-60) == TEXT("23:59") && S47Script::PlayerClock(2 * 3600. + 14 * 60) == TEXT("02:14")
			&& S47Script::PlayerCounter(47) == TEXT("0:48") && S47Script::PlayerCounter(107) == TEXT("1:48") && S47Script::PlayerCounter(495) == TEXT("8:16") && S47Script::PlayerCounter(31.5) == TEXT("0:32") && S47Script::PlayerCounter(-3) == TEXT("0:00"),
			TEXT("REVIEW: a clock or a counter in the tape's words never reads 47, 08:15 or 8:15: a minute or a second on, and only that"));
	}
	{
		FS47Tape A(nullptr);
		A.bCarried = true; A.Next = 3;
		FTake Tk; Tk.N = 2; Tk.Day = 4; Tk.Clock = 23 * 3600.; Tk.Where = TEXT("the windmill on the Baca lease"); Tk.Seconds = 64; Tk.Odd = TEXT("drone_52"); Tk.OddAt = 31.5f; Tk.OddGain = .4f; Tk.bOddHeard = true; Tk.OddKind = TEXT("tall");
		FSound Clip; Clip.At = 12.25f; Clip.Name = TEXT("dog0"); Clip.Gain = .3f; Tk.Sounds.Add(Clip);
		A.Kept.Add(Tk);
		FS47Tape B(nullptr);
		B.Begin(S47J::Read(S47J::Write(A.Save())));
		Check(B.bCarried && B.Next == 3 && B.Kept.Num() == 1 && B.Kept[0].Odd == TEXT("drone_52") && FMath::IsNearlyEqual(B.Kept[0].OddAt, 31.5f) && B.Kept[0].Sounds.Num() == 1 && B.Kept[0].Sounds[0].Name == TEXT("dog0") && B.OddHeardCount() == 1
			&& B.Kept[0].OddKind == TEXT("tall") && S47TapeLocal::AlienOf(B.Kept[0]) == int32(ES47AlienKind::Tall), TEXT("the recorder, its tapes and what was heard on them survive a Continue (EVIDENCE-DOSSIER slice 2: and which one it was)"));
		// EVIDENCE-DOSSIER slice 2: an older tape has no kind and plays the plain sound; a kind with nothing odd on the tape is not kept
		{
			const S47J::FObj Was = S47J::Read(S47J::Write(A.Save()));
			const TArray<TSharedPtr<FJsonValue>> Takes = S47J::Arr(Was, TEXT("takes"));
			const S47J::FObj* First = nullptr;
			if (Takes.Num() == 1 && Takes[0]->TryGetObject(First) && First) { (*First)->RemoveField(TEXT("oddKind")); }
			FS47Tape Old(nullptr); Old.Begin(Was);
			FTake Plain; Plain.N = 5; Plain.Day = 2; Plain.Seconds = 30; Plain.OddKind = TEXT("pale");
			FS47Tape Odd(nullptr); Odd.Kept.Add(Plain); Odd.bCarried = true;
			FS47Tape Back(nullptr); Back.Begin(S47J::Read(S47J::Write(Odd.Save())));
			Check(Old.Kept.Num() == 1 && Old.Kept[0].OddKind.IsEmpty() && S47TapeLocal::AlienOf(Old.Kept[0]) == -1 && Back.Kept.Num() == 1 && Back.Kept[0].OddKind.IsEmpty(),
				TEXT("EVIDENCE-DOSSIER slice 2: a tape from before has no kind and plays as it did; no kind is kept without something odd on it"));
		}
		FS47Tape Cn(nullptr);
		Cn.Begin(nullptr);
		Check(Cn.Save()->Values.Num() == 0 && !Cn.Carried(), TEXT("an older save has the recorder on the desk and writes nothing"));
	}
	return bAll;
}
