// SIDEBAND-20261113: the self-test. Pure checks (drivers, caps, glides, FLARES' rules, the tables) and graph checks rendered
// offline through the real nodes: the bus is the score to the sample at zero, each band moves a known signal by what the offline
// prototype says it should (Tools/sideband/s47dsp.py on the same signal; the numbers are in the comments), the room's impulse
// dies away as designed, the hit and the duck are where they should be on the clock, missing files are silence and no error.
#include "S47Sideband.h"

#include "S47ScoreStems.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace S47SbTest
{
	constexpr int32 Rate = 48000;
	// The test's own signal: seven partials from 110 Hz to 7 kHz with a slow swell, both channels alike, at a deck's level.
	TSharedRef<FS47AudioBuffer> SbSignal(double Secs)
	{
		const int32 N = int32(Secs * Rate);
		auto B = MakeShared<FS47AudioBuffer>(2, N, float(Rate));
		const double Parts[7][3] = { { 110, .10, 0 }, { 220, .08, .3 }, { 440, .06, 1. }, { 880, .05, 2. }, { 1760, .03, .5 }, { 3520, .02, 1.5 }, { 7040, .01, 0 } };
		for (int32 I = 0; I < N; ++I)
		{
			const double T = double(I) / Rate;
			double X = 0;
			for (const double* P : Parts) X += P[1] * FMath::Sin(2. * UE_DOUBLE_PI * P[0] * T + P[2]);
			X *= (1. + .5 * FMath::Sin(2. * UE_DOUBLE_PI * .7 * T)) * FS47SidebandBus::Level;
			B->GetChannelData(0)[I] = float(X); B->GetChannelData(1)[I] = float(X);
		}
		return B;
	}
	struct FRig
	{
		TSharedRef<FS47AudioContext> C;
		TSharedPtr<FS47SidebandBus> Bus;
		TSharedPtr<FS47BufferSourceNode> Src;
		TArray<float> Pcm;
		explicit FRig(const TSharedPtr<FS47AudioBuffer>& In, bool bWithBus = true, bool bLoop = false) : C(FS47AudioContext::Make(float(Rate)))
		{
			Src = C->CreateBufferSource(); Src->Buffer = In; Src->bLoop = bLoop;
			if (bWithBus) { Bus = MakeShared<FS47SidebandBus>(C); Src->Connect(Bus->Head); Bus->Out->Connect(C->Destination); }
			else Src->Connect(C->Destination);
			if (In.IsValid()) Src->Start(0);
		}
		~FRig() { Bus.Reset(); C->Close(); }
		// Renders on, the bus stepped as the game steps it; the whole of it is kept in Pcm (stereo, interleaved).
		void Go(double Secs)
		{
			const int32 Chunk = 1024, Want = int32(Secs * Rate);
			for (int32 Done = 0; Done < Want; Done += Chunk)
			{
				if (Bus) Bus->Step(double(Chunk) / Rate);
				const int32 At = Pcm.Num(); Pcm.AddZeroed(Chunk * 2);
				C->Render(Pcm.GetData() + At, Chunk, 2);
				C->Tick(double(Chunk) / Rate);
			}
		}
		double Rms(double From, double To) const
		{
			double S = 0; int64 N = 0;
			for (int32 I = int32(From * Rate) * 2; I < FMath::Min(Pcm.Num(), int32(To * Rate) * 2); ++I) { S += double(Pcm[I]) * Pcm[I]; ++N; }
			return FMath::Sqrt(S / FMath::Max<int64>(1, N));
		}
		double Likeness(double From, double To) const
		{
			double LR = 0, LL = 0, RR = 0;
			for (int32 I = int32(From * Rate); I < FMath::Min(Pcm.Num() / 2, int32(To * Rate)); ++I) { const double L = Pcm[I * 2], R = Pcm[I * 2 + 1]; LR += L * R; LL += L * L; RR += R * R; }
			return LL > 0 && RR > 0 ? LR / FMath::Sqrt(LL * RR) : 1.;
		}
		bool Sound() const
		{
			for (float V : Pcm) if (!FMath::IsFinite(V) || FMath::Abs(V) >= 1.f) return false;
			return true;
		}
	};
	double SbDbOf(double Ratio) { return 20. * FMath::LogX(10., FMath::Max(Ratio, 1e-12)); }
}

bool FS47Sideband::SelfTest(TArray<FString>& Lines)
{
	using namespace S47Sideband;
	using namespace S47SbTest;
	bool bOk = S47Stems::SelfTest(Lines);
	const auto Check = [&Lines, &bOk](bool bPass, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s sideband: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= bPass; };
	const auto Near = [](double A, double B, double Tol = 1e-6) { return FMath::Abs(A - B) <= Tol; };

	// ---------- names and tables ----------
	Check(ModeFrom(TEXT("soft")) == EMode::Soft && ModeFrom(TEXT("OFF")) == EMode::Off && ModeFrom(TEXT("on")) == EMode::On && ModeFrom(TEXT("full")) == EMode::On && ModeFrom(TEXT("")) == EMode::On
		&& FString(ReactsId(EMode::On)) == TEXT("on") && FString(StingersId(EMode::On)) == TEXT("full") && FString(StingersId(EMode::Soft)) == TEXT("soft") && FString(ReactsId(EMode::Off)) == TEXT("off"),
		TEXT("the two settings read on/full, soft and off, and anything else as on"));
	{
		int32 PerTier[4] = { 0, 0, 0, 0 }; bool bNames = true; TSet<FString> Seen;
		for (const FStinger& S : S47Sideband::Stingers()) { ++PerTier[int32(S.Tier)]; bNames &= !Seen.Contains(S.Name) && FString(S.Name).StartsWith(TierName(S.Tier)); Seen.Add(S.Name); }
		Check(S47Sideband::Stingers().Num() == 9 && PerTier[int32(ETier::Full)] == 4 && PerTier[int32(ETier::Glimpse)] == 3 && PerTier[int32(ETier::Hint)] == 2 && bNames && Beds().Num() == 3,
			TEXT("the palette: four full, three glimpse and two hint stingers, each named for its tier, and three beds"));
	}
	{
		const FTierMix Full = TierMix(ETier::Full, false, 0), Soft = TierMix(ETier::Full, true, 0), Gl = TierMix(ETier::Glimpse, false, 4.), Hint = TierMix(ETier::Hint, false, 3.);
		Check(Near(Full.LevelDb, 9.5) && Near(Full.Lowpass, 7500) && Near(Full.Direct, 1) && Near(Full.Send, .22) && Near(Gl.LevelDb, 2.) && Near(Gl.Lowpass, 2600) && Near(Gl.Direct, .12) && Near(Gl.Send, .9)
			&& Near(Hint.LevelDb, -4.) && Near(Hint.Lowpass, 240) && Near(Hint.Send, 0), TEXT("each tier's chain: full +9.5 dB and bright with a little room, glimpse nearly all room, hint low and under 240 Hz"));
		Check(Near(Soft.LevelDb, 0) && Near(Soft.Lowpass, 3000) && TierMix(ETier::Hint, true, 0).LevelDb == -7., TEXT("a soft stinger is never over the score's own level, and darker"));
		double L0 = 0, C0 = 0, L1 = 0, C1 = 0; BedFor(0, L0, C0); BedFor(1, L1, C1);
		Check(Near(L0, -10) && Near(C0, 200, 1e-3) && Near(L1, 4) && Near(C1, 1000, 1e-3), TEXT("the bed opens from -10 dB under 200 Hz to +4 dB under 1 kHz with nearness"));
		Check(Near(FarTrimDb(TEXT("s47_cold_dish.mp3")), -10) && Near(FarTrimDb(TEXT("look_back_steelguitar.mp3")), -6.5) && Near(FarTrimDb(TEXT("no_such_piece.mp3")), -6.5),
			TEXT("FAR's trim is the piece's own, and -6.5 dB for one that was not measured"));
	}

	// ---------- the drivers ----------
	{
		FDrive D;
		D.Fatigue = 1; D.Presence = 1; D.bHunt = true; D.AfterEncounter = 1; D.bOutdoors = true; D.Open = 1; D.Wind = 1; D.Tier = ETier::Full; D.NightHour = 27.5;
		FWant W = Drive(D);
		Check(!W.Bands.Any() && W.Tension == 0, TEXT("outside Freeform nothing is asked for"));
		D.bFreeform = true; D.Reacts = EMode::Off; W = Drive(D);
		Check(!W.Bands.Any() && W.Tension == 0, TEXT("with Music reacts off nothing is asked for"));
		D.Reacts = EMode::On; W = Drive(D);
		Check(Near(W.Bands.Worn, WornCap) && Near(W.Bands.Broken, BrokenFullCap) && Near(W.Bands.Wide, WideCap) && Near(W.Bands.Far, 1) && Near(W.Tension, 1) && Near(W.BedCap, 1),
			TEXT("a hunt after two in a gale: every band at its cap (worn 0.4, broken 0.68, wide 0.8, far 1) and the tension full"));
		D.Reacts = EMode::Soft; W = Drive(D);
		Check(Near(W.Bands.Worn, SoftWorn) && Near(W.Bands.Broken, SoftBroken) && Near(W.Bands.Wide, SoftWide) && Near(W.Bands.Far, SoftFar) && W.Bands.Broken < .5,
			TEXT("soft caps every band (0.2, 0.3, 0.4, 0.6): BROKEN stays under where its dips begin"));
		D.Reacts = EMode::On;
		// the time tiers: BROKEN never above a small cap before midnight, the bed never in a hint
		D.Tier = ETier::Hint; D.NightHour = 22; W = Drive(D);
		Check(Near(W.Bands.Broken, BrokenHintCap) && W.BedCap == 0 && W.Tension <= .3 + 1e-9, TEXT("before midnight BROKEN is capped at 0.12 whatever is near, there is no bed, and the tension stays low"));
		D.Tier = ETier::Glimpse; D.NightHour = 25; W = Drive(D);
		Check(Near(W.Bands.Broken, BrokenGlimpseCap) && Near(W.BedCap, GlimpseBedCap) && W.Tension <= .6 + 1e-9, TEXT("from midnight to two BROKEN is capped at 0.35 and the bed at 0.35"));
	}
	{
		FDrive D; D.bFreeform = true; D.Tier = ETier::Full; D.NightHour = 22;
		// WORN: tired, late, uneasy
		FWant W = Drive(D);
		Check(!W.Bands.Any() && !W.bRoutine, TEXT("a rested operator outdoors by the building on a still evening: dry"));
		D.Fatigue = .5; W = Drive(D);
		Check(Near(W.Bands.Worn, .2) && W.Bands.Broken == 0 && W.Bands.Far == 0, TEXT("WORN follows fatigue (half tired: 0.2)"));
		D.Fatigue = 0; D.NightHour = 26; const double At2 = Drive(D).Bands.Worn; D.NightHour = 27.5; const double At330 = Drive(D).Bands.Worn; D.NightHour = 30.5; const double At630 = Drive(D).Bands.Worn;
		Check(At2 == 0 && Near(At330, .25) && At630 == 0, TEXT("WORN comes with the hour: nothing at 02:00, 0.25 by 03:30, gone again by the morning"));
		D.NightHour = 22; D.Omen = 1; const double Uneasy = Drive(D).Bands.Worn; D.Omen = 0; D.HintNudge = 1; const double Nudged = Drive(D).Bands.Worn; D.HintNudge = 0;
		Check(Near(Uneasy, .2) && Near(Nudged, .15), TEXT("WORN follows unease (0.2), and a deniable hint nudges it (0.15)"));
		// BROKEN: nearness, the gear, the power
		D.Presence = .5; const double Half = Drive(D).Bands.Broken; D.Presence = 1; const double Close = Drive(D).Bands.Broken; D.Presence = 0;
		Check(Half > .25 && Half < .35 && Near(Close, .6), TEXT("BROKEN follows nearness (0.3 half way in, 0.6 close)"));
		D.Gear = 1; const double Gear = Drive(D).Bands.Broken; D.Gear = 0; D.bPowerOut = true; const double Dark = Drive(D).Bands.Broken; D.bPowerOut = false;
		Check(Near(Gear, .45) && Near(Dark, .25), TEXT("BROKEN follows the gear failing (0.45) and SARO without power (0.25)"));
		// the omen silence: an unseen thing under a hush is held back; seen, it is not
		D.Presence = 1; D.Hush = 1; W = Drive(D);
		const bool bHeld = Near(W.Bands.Broken, HushBrokenCap) && Near(W.BedCap, HushBedCap);
		D.bSeen = true; W = Drive(D);
		Check(bHeld && Near(W.Bands.Broken, .6) && Near(W.BedCap, 1), TEXT("under the omen silence with nothing seen BROKEN and the bed are held down (0.25, 0.12); once it is seen they are free"));
		D.Presence = 0; D.Hush = 0; D.bSeen = false;
		// WIDE: outdoors on open land, more in wind
		D.bOutdoors = true; D.Open = 1; const double Calm = Drive(D).Bands.Wide; D.Wind = 1; const double Gale = Drive(D).Bands.Wide; D.Open = 0; const double ByWall = Drive(D).Bands.Wide;
		D.Open = 1; D.bOutdoors = false; const double Inside = Drive(D).Bands.Wide; D.Wind = 0;
		Check(Near(Calm, .35) && Near(Gale, .8) && ByWall == 0 && Inside == 0, TEXT("WIDE is open land on foot (0.35 calm, 0.8 in a gale), nothing by a building or indoors"));
		// FAR: after a meeting, hiding, after lost time
		D.AfterEncounter = .5; const double After = Drive(D).Bands.Far; D.AfterEncounter = 0; D.bHiding = true; const double Hid = Drive(D).Bands.Far; D.bHiding = false; D.AfterTaken = 1; const double Back = Drive(D).Bands.Far; D.AfterTaken = 0;
		Check(Near(After, .5) && Near(Hid, .55) && Near(Back, .8), TEXT("FAR follows the minute after a meeting, hiding (0.55) and coming to after lost time (0.8)"));
		// indoors at SARO with nothing about: dry, however late and tired
		D.bSaroIndoors = true; D.Fatigue = 1; D.NightHour = 27.5; W = Drive(D);
		const bool bDry = W.bRoutine && !W.Bands.Any();
		D.Approach = .3; W = Drive(D);
		Check(bDry && !W.bRoutine && W.Bands.Worn > 0 && W.Bands.Broken > 0, TEXT("the routine indoors at SARO is dry however late; it ends as soon as something is heard coming"));
		// tension
		D = FDrive(); D.bFreeform = true; D.Tier = ETier::Full;
		D.Approach = 1; const double Heard = Drive(D).Tension; D.Presence = .5; const double NearBy = Drive(D).Tension; D.bSeen = true; const double Seen = Drive(D).Tension; D.bHunt = true; const double Hunt = Drive(D).Tension;
		Check(Near(Heard, .35) && Near(NearBy, .55) && Near(Seen, .7) && Near(Hunt, 1), TEXT("the tension: 0.35 heard coming, 0.55 near, 0.7 seen, 1 hunted"));
	}
	{
		// the glides: the whole range in Up seconds, down in Down, never past the target
		FGlide G; G.Up = 2.5; G.Down = 5;
		bool bMono = true; double Last = 0;
		for (int32 I = 0; I < 20; ++I) { const double V = G.Step(.05, 1); bMono &= V >= Last && V <= 1; Last = V; }
		const double At1 = G.Now;
		for (int32 I = 0; I < 32; ++I) G.Step(.05, 1);
		const double At26 = G.Now;
		for (int32 I = 0; I < 50; ++I) G.Step(.05, 0);
		const double Down25 = G.Now;
		for (int32 I = 0; I < 52; ++I) G.Step(.05, 0);
		Check(bMono && Near(At1, .4, .01) && Near(At26, 1) && Near(Down25, .5, .01) && G.Now == 0, TEXT("a glide crosses the whole range in its own time (2.5 s up, 5 s down here) and stops on the target"));
		G.Step(.05, NAN); const double NotANumber = G.Now; G.Step(5., 1.);
		Check(NotANumber == 0 && G.Now <= .11, TEXT("a target that is no number counts as zero, and a long frame moves a band no more than a quarter second would"));
		const TSharedRef<FS47AudioContext> C = FS47AudioContext::Make();
		{
			FS47Sideband S(nullptr, C);
			bool bTimes = true;
			for (const FGlide& Each : S.Glide) bTimes &= Each.Up >= 2 && Each.Up <= 6 && Each.Down >= 2 && Each.Down <= 6;
			Check(bTimes, TEXT("the four bands glide in two to six seconds, up and down"));
		}
		C->Close();
	}

	// ---------- FLARES' rules ----------
	{
		FRules R(47); bool bSoft = false; FString Why;
		const int32 HintBed = R.Begin(0, ETier::Hint);
		const int32 First = R.Moment(10, false, bSoft, Why), Again = R.Moment(12, false, bSoft, Why);
		Check(HintBed == -1 && First >= 0 && S47Sideband::Stingers()[First].Tier == ETier::Hint && Again == -1 && R.MomentSpent(), TEXT("one stinger per event at most, and no bed in a hint"));
		R.End(); R.Begin(60, ETier::Hint); const int32 TooSoon = R.Moment(70, false, bSoft, Why);
		R.End(); R.Begin(100, ETier::Hint); const int32 Later = R.Moment(101, false, bSoft, Why);
		Check(TooSoon == -1 && Later >= 0 && Later != First, TEXT("after a hint stinger none for 90 s; then another, and not the same one"));
		R.End(); const int32 GlimpseBed = R.Begin(300, ETier::Glimpse); const int32 Gl = R.Moment(301, false, bSoft, Why);
		R.End(); R.Begin(400, ETier::Full); const int32 SoonFull = R.Moment(401, false, bSoft, Why);
		Check(GlimpseBed >= 0 && Gl >= 0 && S47Sideband::Stingers()[Gl].Tier == ETier::Glimpse && SoonFull == -1, TEXT("a glimpse has a bed and a stinger, and after it none of any tier for 120 s"));
	}
	{
		// of any three full events in a row at most two get a stinger; never the same twice running; 300 s between two
		FRules R(7); bool bSoft = false; FString Why;
		TArray<int32> Got; bool bNoRepeat = true, bBedsDiffer = true; int32 LastPick = -1, LastBed = -1;
		for (int32 I = 0; I < 30; ++I)
		{
			const int32 Bed = R.Begin(I * 400., ETier::Full); bBedsDiffer &= Bed >= 0 && Bed != LastBed; LastBed = Bed;
			const int32 Pick = R.Moment(I * 400. + 20, false, bSoft, Why); R.End();
			Got.Add(Pick);
			if (Pick >= 0) { bNoRepeat &= Pick != LastPick && S47Sideband::Stingers()[Pick].Tier == ETier::Full; LastPick = Pick; }
		}
		bool bTwoOfThree = Got[0] >= 0 && Got[1] >= 0 && Got[2] == -1;
		for (int32 I = 0; I + 2 < Got.Num(); ++I) bTwoOfThree &= !(Got[I] >= 0 && Got[I + 1] >= 0 && Got[I + 2] >= 0);
		Check(bTwoOfThree, TEXT("of any three full events in a row at most two get a stinger: the third is the duck alone"));
		Check(bNoRepeat && bBedsDiffer, TEXT("never the same stinger twice running, and never the last event's bed"));
		FRules Gap(3);
		Gap.Begin(0, ETier::Full); const int32 A = Gap.Moment(1, false, bSoft, Why); Gap.End();
		Gap.Begin(200, ETier::Full); const int32 B = Gap.Moment(201, false, bSoft, Why); Gap.End();
		Gap.Begin(310, ETier::Full); const int32 C3 = Gap.Moment(311, false, bSoft, Why); Gap.End();
		Check(A >= 0 && B == -1 && C3 >= 0, TEXT("two full stingers are at least 300 s apart"));
	}
	{
		// a voice or a signal goes first; the setting; missing files
		FRules R(11); bool bSoft = false; FString Why;
		R.Begin(0, ETier::Glimpse); const int32 UnderSpeech = R.Moment(1, true, bSoft, Why); R.End();
		R.Begin(10, ETier::Full); const int32 FullSpeech = R.Moment(11, true, bSoft, Why); const bool bWasSoft = bSoft; R.End();
		Check(UnderSpeech == -1 && FullSpeech >= 0 && bWasSoft, TEXT("under a voice or a signal a hint or glimpse stinger is dropped (not queued) and a full one plays soft"));
		FRules Off(11); Off.Setting = EMode::Off; Off.Begin(0, ETier::Full); const int32 None = Off.Moment(1, false, bSoft, Why);
		FRules SoftR(11); SoftR.Setting = EMode::Soft; SoftR.Begin(0, ETier::Full); const int32 Quiet = SoftR.Moment(1, false, bSoft, Why);
		Check(None == -1 && Quiet >= 0 && bSoft, TEXT("stingers off: none; soft: every one soft"));
		FRules Bare(11); Bare.Begin(0, ETier::Full); const int32 NoFile = Bare.Moment(1, false, bSoft, Why, [](int32) { return false; });
		Check(NoFile == -1 && Bare.MomentSpent(), TEXT("with no stinger's file there, the moment passes in silence"));
		FRules Cap(11); Cap.Begin(0, ETier::Full); Cap.Cap(ETier::Glimpse); const int32 Far = Cap.Moment(1, false, bSoft, Why);
		Check(Cap.EventTier() == ETier::Glimpse && Far >= 0 && S47Sideband::Stingers()[Far].Tier == ETier::Glimpse, TEXT("a light far off in the full hours is still a glimpse's moment"));
		const bool bWasIn = Cap.InEvent(); Cap.End();
		Check(bWasIn && !Cap.InEvent() && Cap.EventBed() == -1 && Cap.EventTier() == ETier::None, TEXT("an event that has ended has no bed and no tier"));
	}
	Check(TierAt(21 * 3600., 1) == ETier::Hint && TierAt(24.5 * 3600., 1) == ETier::Glimpse && TierAt(26.5 * 3600., 1) == ETier::Full && TierAt(25.2 * 3600., 22) == ETier::Full && TierAt(38 * 3600., 2) == ETier::Hint,
		TEXT("the tier is the game's own: hints to midnight, glimpses to two, full after; an hour earlier in the fourth week; hints again from noon"));
	Check(MomentTier(3, ETier::Full) == ETier::Glimpse && MomentTier(0, ETier::Full) == ETier::Full && MomentTier(1, ETier::Glimpse) == ETier::Glimpse && MomentTier(2, ETier::Hint) == ETier::Hint,
		TEXT("a moment is what the clock allows, and a light far off never more than a glimpse"));
	Check(OpenFor(TEXT("saro"), 30) == 0 && Near(OpenFor(TEXT("saro"), 140), .5) && OpenFor(TEXT("saro"), 400) == 1 && OpenFor(TEXT("road"), 0) == 1 && OpenFor(TEXT("diner"), 500) < .5,
		TEXT("open land: nothing by the station, all of it from 220 m and on the road, little in town"));
	Check(!SpeechNow(nullptr), TEXT("no game, no voice"));

	// ---------- the graph ----------
	const TSharedRef<FS47AudioBuffer> Music = SbSignal(8.);
	{
		// at zero the bus is the score to the sample, and nothing of the chain exists
		// (the signal goes round, so that there is music under every part of this)
		FRig Dry(Music, false, true), Rig(Music, true, true);
		Dry.Go(1.); Rig.Go(1.);
		bool bSame = Dry.Pcm.Num() == Rig.Pcm.Num(); double Energy = 0;
		for (int32 I = 0; bSame && I < Dry.Pcm.Num(); ++I) { bSame &= Dry.Pcm[I] == Rig.Pcm[I]; Energy += double(Dry.Pcm[I]) * Dry.Pcm[I]; }
		Check(bSame && Energy > 1 && !Rig.Bus->Engaged() && Rig.Bus->Nodes() == 0, TEXT("at zero the bus passes the score unchanged to the sample, with the chain not built: the idle cost is nil"));
		// a band comes in: the chain, and a parameter on its way in a tenth of a second
		const double T0 = Rig.C->CurrentTime();
		Rig.Bus->Set({ 1, 0, 0, 0 }); Rig.Go(.05); Dry.Go(.05);
		const bool bIn = Rig.Bus->Engaged() && Rig.Bus->Nodes() > 100 && !Rig.Bus->RoomEngaged();
		FS47AudioParam* Depth = Rig.Bus->WornAims.Num() ? Rig.Bus->WornAims[0].P : nullptr;
		Check(bIn && Depth && Depth->ValueAt(T0 + .08) > .5 && Depth->ValueAt(T0 + .08) < .75 && Depth->ValueAt(T0 + 1.) > .999, TEXT("a band above zero brings the chain in, and the graph follows it in about a tenth of a second"));
		Lines.Add(FString::Printf(TEXT("INFO sideband: the chain is %d nodes"), Rig.Bus->Nodes()));
		// back to zero: the chain is let go after its tail, and the score is again untouched
		Rig.Bus->Set({}); Rig.Go(FS47SidebandBus::Tail + 1.); Dry.Go(FS47SidebandBus::Tail + 1.);
		const bool bOut = !Rig.Bus->Engaged();
		Rig.Go(.5); Dry.Go(.5);
		bool bAgain = Dry.Pcm.Num() == Rig.Pcm.Num(); double Later = 0;
		for (int32 I = Rig.Pcm.Num() - 40000; bAgain && I < Rig.Pcm.Num(); ++I) { bAgain &= Dry.Pcm[I] == Rig.Pcm[I]; Later += FMath::Abs(double(Rig.Pcm[I])); }
		Check(bOut && bAgain && Later > 100 && Rig.Sound(), TEXT("back at zero the chain is let go after its tail and the score is again unchanged to the sample"));
	}
	{
		// each band at full against the dry signal, 3 to 7 s in. What the offline prototype gives on this signal: WORN 0.0 dB,
		// BROKEN -3.5 dB, WIDE -0.5 dB (left/right likeness 0.78), FAR -11.9 dB (likeness 0.31).
		FRig Dry(Music, false); Dry.Go(7.);
		const double Ref = Dry.Rms(3, 7);
		double Got[4] = { 0, 0, 0, 0 }, Like[4] = { 1, 1, 1, 1 }; bool bSound = true, bRoom = true;
		for (int32 Band = 0; Band < 4; ++Band)
		{
			FRig R(Music);
			FS47SidebandBands B; (Band == 0 ? B.Worn : Band == 1 ? B.Broken : Band == 2 ? B.Wide : B.Far) = 1;
			R.Bus->Set(B); R.Go(7.);
			Got[Band] = SbDbOf(R.Rms(3, 7) / Ref); Like[Band] = R.Likeness(3, 7); bSound &= R.Sound();
			bRoom &= R.Bus->RoomEngaged() == (Band == 3);
		}
		Lines.Add(FString::Printf(TEXT("INFO sideband: each band at full against dry: worn %+.1f dB, broken %+.1f dB, wide %+.1f dB (likeness %.2f), far %+.1f dB (likeness %.2f); the prototype: 0.0, -3.5, -0.5 (0.78), -11.9 (0.31)"),
			Got[0], Got[1], Got[2], Like[2], Got[3], Like[3]));
		Check(bSound && Near(Dry.Likeness(3, 7), 1, 1e-6), TEXT("every band at full renders finite and below clipping"));
		Check(Got[0] > -2.5 && Got[0] < 2.5 && Like[0] > .98, TEXT("WORN at full keeps the level (within 2.5 dB of dry) and the middle"));
		Check(Got[1] > -6.5 && Got[1] < -.5, TEXT("BROKEN at full is the prototype's (3.5 dB under dry on this signal, within 3 dB)"));
		Check(Got[2] > -3. && Got[2] < 2. && Like[2] < .93 && Like[2] > .5, TEXT("WIDE at full keeps the level and opens the sides (likeness 0.78 in the prototype)"));
		Check(Got[3] > -15.5 && Got[3] < -8.5 && Like[3] < .7, TEXT("FAR at full is far lower by design (11.9 dB under dry here, within 3.5 dB) and wide"));
		Check(bRoom, TEXT("the room is connected only while FAR is up"));
	}
	{
		// the room's impulse: an 80 ms burst through FAR at full with no trim. The prototype: one second from 4 s is 16.5 dB under
		// the first second, from 8 s 36 dB under. (A loop in this engine is one quantum longer than its delayTime: taken off.)
		const TSharedRef<FS47AudioBuffer> Burst = SbSignal(.08);
		FRig R(nullptr);
		R.Bus->FarTrim(0.); R.Bus->Set({ 0, 0, 0, 1 }); R.Go(.5);
		// (the band is in: now the burst, from a second source)
		auto Again = R.C->CreateBufferSource(); Again->Buffer = Burst; Again->Connect(R.Bus->Head);
		const double T0 = R.C->CurrentTime(); Again->Start(T0);
		R.Go(11.5);
		const double Off = T0;
		const double First = R.Rms(Off + .2, Off + 1.2), At4 = R.Rms(Off + 4, Off + 5), At8 = R.Rms(Off + 8, Off + 9), At10 = R.Rms(Off + 10, Off + 11);
		Lines.Add(FString::Printf(TEXT("INFO sideband: the room after an 80 ms burst: %.1f dB at 4 s, %.1f dB at 8 s, %.1f dB at 10 s against its first second (the prototype: -16.5, -36.2, -44.8)"),
			SbDbOf(At4 / First), SbDbOf(At8 / First), SbDbOf(At10 / First)));
		Check(R.Sound() && First > 1e-5 && SbDbOf(At4 / First) < -10 && SbDbOf(At4 / First) > -24 && SbDbOf(At8 / First) < -26 && SbDbOf(At8 / First) > -46 && At10 < At8,
			TEXT("the room's tail dies as designed (about 16 dB down after 4 s, 36 dB after 8 s) and never grows"));
		Again->Disconnect();
	}
	{
		// the hit and the duck, on the clock
		FRig R(Music);
		R.Bus->Set({ 0, .5, 0, 0 }); R.Go(1.);
		const double T0 = R.C->CurrentTime() + .02;
		R.Bus->Hit(T0, true); R.Bus->MomentDuck(T0);
		FS47AudioParam* Hp = R.Bus->BrokenAims[0].P;
		const double Rest = R.Bus->BrokenAims[0].Fn(.5);
		Check(Near(Hp->ValueAt(T0 + .01), 320, 1.) && Near(Hp->ValueAt(T0 + .2), 320, 1.) && Hp->ValueAt(T0 + .5) < 320 && Near(Hp->ValueAt(T0 + 3.), Rest, 1.) && Rest > 70 && Rest < 90,
			TEXT("the hit pushes BROKEN to full in 8 ms, holds a quarter second and lets go"));
		Check(Near(R.Bus->HitDrive->Gain.ValueAt(T0 + .01), FMath::Pow(10., .5), .01) && Near(R.Bus->HitDrive->Gain.ValueAt(T0 + .6), 1, .01) && Near(R.Bus->HitHole->Gain.ValueAt(T0 + .19), .2, .01)
			&& Near(R.Bus->HitHole->Gain.ValueAt(T0 + .4), 1, 1e-3) && R.Bus->HitStatic->Gain.ValueAt(T0 + .02) > 0 && R.Bus->HitStatic->Gain.ValueAt(T0 + 1.) < 1e-3,
			TEXT("with its burst of drive (+10 dB for a tenth of a second), its static, and a soft dip to a fifth (no hole)"));
		Check(Near(R.Bus->Moment->Gain.ValueAt(T0 + .1), 1) && Near(R.Bus->Moment->Gain.ValueAt(T0 + .31), FMath::Pow(10., -1.8), 1e-4) && Near(R.Bus->Moment->Gain.ValueAt(T0 + 3.2), FMath::Pow(10., -1.8), 1e-4)
			&& R.Bus->Moment->Gain.ValueAt(T0 + 5.) < .6 && Near(R.Bus->Moment->Gain.ValueAt(T0 + 7.6), 1), TEXT("the duck: 0.2 s of music under the hit, 36 dB down for three seconds, back over four"));
		{
			// SIDEBAND-20261113 (d69: -10.4/-9.9 dB at 2-4/4-6 s against the prototype's -12.6/-16.5): the way back is the prototype's,
			// a glide in dB (-33 dB at +4 s, -10.4 dB at +6 s), not a straight line in gain (-15 and -3.7 dB there)
			bool bCurve = true;
			for (const double T : { 3.5, 4., 5., 6., 7. }) bCurve &= FMath::Abs(SbDbOf(FMath::Max(1e-9, double(R.Bus->Moment->Gain.ValueAt(T0 + T)))) - S47Sideband::DuckDbAt(T)) < .6;
			Check(bCurve && S47Sideband::DuckDbAt(4.) < -32 && S47Sideband::DuckDbAt(6.) < -9.5 && S47Sideband::DuckDbAt(6.) > -11.5,
				TEXT("the duck comes back as the prototype's does: in dB over 4.2 s (-33 dB 4 s after, -10 dB 6 s after), not sooner"));
		}
		R.Go(3.);
		const double Before = R.Rms(.4, .9), Ducked = R.Rms(1.02 + 1., 1.02 + 3.);
		Check(R.Sound() && SbDbOf(Ducked / Before) < -20, TEXT("rendered: the three seconds after a close sight are more than 20 dB under what was before"));
		FRig Plain(Music); Plain.Bus->Set({ 0, .5, 0, 0 }); Plain.Go(.5);
		const double T1 = Plain.C->CurrentTime() + .02; Plain.Bus->Hit(T1, false);
		Check(Near(Plain.Bus->HitDrive->Gain.ValueAt(T1 + .01), 1) && Near(Plain.Bus->HitLevel->Gain.ValueAt(T1 + .01), 1) && Plain.Bus->HitStatic->Gain.ValueAt(T1 + .02) > 0,
			TEXT("under a stinger the hit keeps its crunch but not its bursts of drive and level"));
		// the noise layers close under a voice; the room's return closes under a hold
		Plain.Bus->NoiseOpen(false); Plain.Bus->Hold(true);
		const double T2 = Plain.C->CurrentTime();
		Check(Plain.Bus->WornNoise->Gain.ValueAt(T2 + 1.5) < .001 && Plain.Bus->BrokenNoise->Gain.ValueAt(T2 + 1.5) < .001 && Plain.Bus->Return->Gain.ValueAt(T2 + 1.) < .001,
			TEXT("the hiss, the crackle and the static close while a voice or a signal is audible, and the room's return closes under a hold"));
		Plain.Bus->NoiseOpen(true); Plain.Bus->Hold(false);
		Check(Plain.Bus->WornNoise->Gain.ValueAt(Plain.C->CurrentTime() + 5.) > .99 && Plain.Bus->Return->Gain.ValueAt(Plain.C->CurrentTime() + 5.) > .99, TEXT("and they open again"));
		// the score standing back is on the score alone, also with the chain out
		FRig Stand(Music); Stand.Bus->StandBack(.05); Stand.Go(3.);
		FRig Dry(Music, false); Dry.Go(3.);
		Check(!Stand.Bus->Engaged() && Near(Stand.Rms(2, 3) / Dry.Rms(2, 3), .05, .002), TEXT("the score stands back at the bus's head, with or without the chain"));
	}
	{
		// FLARES' voices with stand-in buffers: a stinger at the full tier over silence, the bed, and their ends
		auto Tone = MakeShared<FS47AudioBuffer>(2, Rate, float(Rate));
		for (int32 I = 0; I < Rate; ++I) { const float V = float(.1 * FMath::Sin(2. * UE_DOUBLE_PI * 440. * I / Rate)); Tone->GetChannelData(0)[I] = V; Tone->GetChannelData(1)[I] = V; }
		FRig R(nullptr);
		R.Bus->TestFlares(Tone, Tone);
		Check(R.Bus->FlaresLoaded() == 12 && R.Bus->HasStinger(0) && R.Bus->HasBed(2) && !R.Bus->HasStinger(9) && !R.Bus->HasBed(-1), TEXT("FLARES' twelve sounds, by index"));
		const bool bStarted = R.Bus->Stinger(0, ETier::Full, false, 0.);
		const bool bSecond = R.Bus->Stinger(1, ETier::Full, false, 0.);
		{
			// SIDEBAND-20261113 (click ratio 11.7 against the prototype's 7.0): the stinger's gain comes up over the prototype's 3 ms
			const double S0 = R.C->CurrentTime(), Top = FMath::Pow(10., S47Sideband::TierMix(ETier::Full, false, S47Sideband::Stingers()[0].TrimDb).LevelDb / 20.);
			FS47AudioParam& G = R.Bus->StingGain->Gain;
			Check(bStarted && G.ValueAt(S0) < Top * .02 && Near(G.ValueAt(S0 + .0015), Top * .5, Top * .1) && Near(G.ValueAt(S0 + .004), Top, Top * .01),
				TEXT("a full stinger's gain rises over 3 ms (raised cosine), as the prototype's front, never from its level at the first sample"));
		}
		R.Go(1.);
		// 0.1 at +9.5 dB, direct at 1, back to the decks' level: an RMS of 0.091 (the prototype's chain on the same tone)
		const double Loud = R.Rms(.3, .8);
		// (the little of it that goes to the room adds to that)
		Lines.Add(FString::Printf(TEXT("INFO sideband: a 0.1 tone as a full stinger leaves the bus at an RMS of %.4f (direct alone: 0.0908)"), Loud));
		Check(bStarted && !bSecond && R.Bus->Engaged() && Loud > .075 && Loud < .16 && R.Sound(), TEXT("a full stinger plays through its tier's chain at about its level (+9.5 dB on the baked sound), one at a time"));
		R.Go(.5);
		Check(!R.Bus->StingerSounding(), TEXT("and its voice is gone when it has played"));
		FRig Soft(nullptr); Soft.Bus->TestFlares(Tone, Tone);
		Soft.Bus->Stinger(0, ETier::Full, true, 0.); Soft.Go(1.);
		const double SoftLoud = Soft.Rms(.3, .8), Front = Soft.Rms(0, .03);
		Check(SoftLoud < Loud * .4 && SoftLoud > Loud * .25 && Front < SoftLoud * .35, TEXT("a soft stinger is 9.5 dB lower and comes in over a tenth of a second"));
		FRig Glimpse(nullptr); Glimpse.Bus->TestFlares(Tone, Tone);
		Glimpse.Bus->Stinger(4, ETier::Glimpse, false, 0.); Glimpse.Go(3.);
		Check(Glimpse.Bus->RoomEngaged() && Glimpse.Rms(1.5, 2.5) > 1e-4 && Glimpse.Sound(), TEXT("a glimpse's stinger goes into FAR's room (which comes in for it) and rings on there after its own end"));
		// the bed: only while its event is on
		FRig Bed(nullptr); Bed.Bus->TestFlares(Tone, Tone);
		Bed.Bus->Bed(1, 1., true); Bed.Go(6.);
		const double Up = Bed.Rms(5, 6);
		Check(Bed.Bus->BedSounding() && Near(Bed.Bus->BedNear(), 1) && Up > .005 && Bed.Sound(), TEXT("the bed comes up inside its event"));
		{
			// REVIEW-20261113: another event's bed in its place: the old one fades over 50 ms (no stop mid-waveform), the new one comes in
			FRig Swap(nullptr); Swap.Bus->TestFlares(Tone, Tone);
			Swap.Bus->Bed(1, 1., true); Swap.Go(6.);
			const double Was = Swap.Rms(5.8, 6.);
			Swap.Bus->Bed(0, 1., true); Swap.Go(1.);
			Check(Swap.Rms(6., 6.02) > Was * .3 && Swap.Rms(6.1, 6.3) < Was * .5 && Swap.Bus->BedSounding() && Swap.Sound(),
				TEXT("a bed that gives way to another fades out over 50 ms (still sounding at first, gone after a tenth), and the new one comes in"));
		}
		Bed.Bus->Bed(1, .2, true); Bed.Go(4.);
		Check(Bed.Rms(9, 10) < Up * .5, TEXT("and sinks and darkens as the thing goes off"));
		Bed.Bus->Bed(-1, 0, false); Bed.Go(14.);
		Check(!Bed.Bus->BedSounding() && Bed.Bus->BedNear() == 0 && Bed.Rms(23.2, 24) < 1e-5, TEXT("outside an event the bed fades and its source is stopped: exactly nothing"));
		// missing files: silence, and no error
		FRig None(nullptr); None.Bus->TestFlares(nullptr, nullptr);
		const bool bNoStinger = !None.Bus->Stinger(0, ETier::Full, false, 0.);
		None.Bus->Bed(0, 1., true); None.Bus->BedMoment(0.); None.Go(.5);
		Check(bNoStinger && None.Bus->FlaresLoaded() == 0 && !None.Bus->BedSounding() && !None.Bus->StingerSounding() && !None.Bus->Engaged() && None.Rms(0, .5) == 0,
			TEXT("without the baked files FLARES is silent and the chain stays out: no error"));
	}
	{
		// the session without a game: it cannot own the ground or a moment, its auditions and its scene run, its recording is a file
		const TSharedRef<FS47AudioContext> C = FS47AudioContext::Make();
		{
			FS47Sideband S(nullptr, C);
			S.Bus->Out->Connect(C->Destination);
			TArray<float> Pcm; Pcm.SetNumZeroed(4800 * 2);
			const auto Go = [&S, &C, &Pcm](double Secs) { for (double Done = 0; Done < Secs; Done += .1) { S.Step(.1); C->Render(Pcm.GetData(), 4800, 2); C->Tick(.1); } };
			Check(!S.Ground(1, 1, false) && !S.MomentFor(0, 1, false) && !S.GroundOwned() && Near(S.StandBackFor(false, 1, 0), .34) && Near(S.StandBackFor(true, 1, 0), .05),
				TEXT("a session that cannot take the ground says so: the bought ground and hit stay as they were, and the score stands back for them as before"));
			S.Audition(0, 1., 20); Go(2.);
			const double Rising = S.Bus->Bands().Worn;
			Go(3.5);
			const double Full = S.Bus->Bands().Worn;
			S.Audition(0, -1); Go(7.);
			Check(Rising > .3 && Rising < .5 && Near(Full, 1) && S.Bus->Bands().Worn == 0, TEXT("an auditioned band glides up in its own time and goes back when let go"));
			auto Tone = MakeShared<FS47AudioBuffer>(2, Rate, float(Rate));
			S.Bus->TestFlares(Tone, Tone);
			// (with stand-in sounds the session could own the ground, were there a Freeform game)
			FString What;
			const bool bFlare = S.AuditionFlare(ETier::Full, What);
			Check(bFlare && S.Bus->StingerSounding() && What.StartsWith(TEXT("full_")) && S.FarRise > 0, TEXT("an auditioned full flare plays a full stinger with the hit, and FAR is set to follow"));
			Go(3.);
			Check(S.AfterEncounter == 1 && S.AfterHold > 0, TEXT("2.6 s after a full moment the minute of FAR begins"));
			const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("S47SidebandSelfTest.wav"));
			const bool bArmed = S.Record(1., Path), bTwice = S.Record(1., Path);
			Go(1.3);
			const int64 Size = IFileManager::Get().FileSize(*Path);
			Check(bArmed && !bTwice && Size >= 44 + 48000 * 4 && Size < 44 + 60000 * 4, TEXT("a recording of the bus is a 16-bit stereo WAVE file of the length asked for"));
			IFileManager::Get().Delete(*Path);
			S.StartScene();
			for (int32 I = 0; I < 300; ++I) S.Step(.1);
			const bool bWalk = S.SceneRunning() && Near(S.Bus->Bands().Worn, .3, .02) && S.Bus->Bands().Broken == 0 && S.Bus->BedSounding();
			for (int32 I = 0; I < 165; ++I) S.Step(.1);
			const bool bSeenAt46 = S.bSceneHit && S.Bus->Bands().Broken > .4;
			for (int32 I = 0; I < 345; ++I) S.Step(.1);
			Check(bWalk && bSeenAt46 && !S.SceneRunning(), TEXT("the 80 s scene: WORN on the walk out with the bed under it, the sight at 46 s, and it ends by itself"));
		}
		C->Close();
	}
	return bOk;
}
