#include "S47Breather.h"
#include "S47EventScript.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47Rhythm.h"
#include "S47Script.h"
#include "S47TownSites.h"
#include "S47TownUse.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47WebAudio.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Breather, Log, All);

// N1110: see the header.
namespace S47BreatherLocal
{
	const TCHAR* const BufKey = TEXT("made:breather_draft");
	const TCHAR* const LoopKey = TEXT("breatherDraft");
	// The uses (S47TownSites' "breather" row names them on its parts).
	const TCHAR* const UseGate = TEXT("ff:breather:gate");
	const TCHAR* const UseSign = TEXT("ff:breather:sign");
	const TCHAR* const UsePlaque = TEXT("ff:breather:plaque");
	const TCHAR* const UseHut = TEXT("ff:breather:hut");
	// What the player reads and is told. Plain 1986 English; the leaflet at the gas station door has the tours' prices and rules.
	const TCHAR* const SignText = TEXT("THE BREATHER. CAVE TOURS. CLOSED FOR THE SEASON. Under it, painted smaller: Saturdays and Sundays, 10 to 3, from Memorial Day. A constant 54 degrees. Ask at the Trujillo house.");
	const TCHAR* const PlaqueText = TEXT("A bronze plaque set in a stone by the gate. THE BREATHER. In 1923 a well driller on Main Street lost his bit into open air sixty feet down, and a cold wind came up the hole. The limestone cave he had found runs under much of the town. It draws air in and lets it out again with the hour and the weather, and that is how it got its name. Opened to visitors in 1931 by the Trujillo family. Caliche Chamber of Commerce, 1976.");
	const TCHAR* const GateText = TEXT("A chain through the gate and a padlock on it, and a card wired to the mesh: CLOSED FOR THE SEASON. Through the fence, cold air comes up out of the dark in the rock, slow, and then draws back the other way. It smells of wet stone.");
	const TCHAR* const GateLook = TEXT("Past the wire, a concrete landing with a pipe rail along it, up to a dark opening in the rock. The draft reaches you through the fence, and goes back.");
	const TCHAR* const HutText = TEXT("TICKETS. Plywood over the window, and a price card still in the frame of the door: ADULTS $3, CHILDREN $1, LANTERNS FURNISHED. Under it, in marker: BRING A WRAP. IT IS COLD BELOW.");
	const TCHAR* const FindingText = TEXT("The cave draft at the Breather. The tour gate west of Main Street is chained, closed for the season. Cold air comes up out of the rock in a slow cycle, about twelve seconds out and back. Caves breathe with the weather and the hour, the leaflet says.");

	double Smooth(double A, double B, double X) { const double T = FMath::Clamp((X - A) / (B - A), 0.0, 1.0); return T * T * (3 - 2 * T); }
	// 0 by day, 1 in the dark: up from 19 to 21, down from 5 to 7.
	double NightWeight(double Hour)
	{
		const double H = FMath::Fmod(FMath::Fmod(Hour, 24.0) + 24.0, 24.0);
		return H >= 12 ? Smooth(19, 21, H) : 1 - Smooth(5, 7, H);
	}
	// One-pole filters, run in place.
	void Low(TArray<float>& S, float Hz, float Rate)
	{
		const float A = 1.f - FMath::Exp(-2.f * PI * Hz / Rate); float Y = 0;
		for (float& V : S) { Y += A * (V - Y); V = Y; }
	}
	void High(TArray<float>& S, float Hz, float Rate)
	{
		TArray<float> L = S; Low(L, Hz, Rate);
		for (int32 I = 0; I < S.Num(); ++I) S[I] -= L[I];
	}
	// A breath's shape over its twelve seconds: in (rising, then easing), a held moment, a longer and stronger out, a little floor of
	// draft all the time so that a stop is heard. Depth and Lean make the second breath of the loop a little unlike the first.
	float Shape(double T, double Depth, double Lean)
	{
		const double In0 = 0.2, In1 = 4.9 + Lean, Out0 = 5.9 + Lean, Out1 = 11.6;
		double E = 0.22;
		if (T > In0 && T < In1) E += 0.55 * Depth * FMath::Sin(PI * (T - In0) / (In1 - In0));
		if (T > Out0 && T < Out1) { const double U = (T - Out0) / (Out1 - Out0); E += 0.78 * Depth * FMath::Sin(PI * FMath::Pow(U, 0.8)); }
		return float(E);
	}
	// Whether the player is on the ground by the Breather, and where (road-local).
	bool PlayerRoad(US47Main* M, FVector2D& Out)
	{
		if (!M || !M->Player || !M->Web) return false;
		const FString Area = M->Web->Area();
		if (Area != TEXT("road") && Area != TEXT("diner")) return false;
		double X = 0, Z = 0;
		if (!S47OpenLand::RoadOf(Area, M->Player->Pos.X, M->Player->Pos.Z, X, Z)) return false;
		Out = FVector2D(X, Z);
		return true;
	}
}

FVector2D S47Breather::Mouth()
{
	const S47TownSites::FSite* S = S47TownSites::Find(TEXT("breather"));
	return S ? S47TownSites::ToRoad(*S, -0.5, 0.3) : FVector2D(-62.5, 2146.3);
}

FVector2D S47Breather::Gate()
{
	const S47TownSites::FSite* S = S47TownSites::Find(TEXT("breather"));
	return S ? S47TownSites::ToRoad(*S, 4.5, 0.3) : FVector2D(-57.5, 2146.3);
}

TArray<float> S47Breather::MakeDraft(float Hz)
{
	using namespace S47BreatherLocal;
	const float R = FMath::Max(8000.f, Hz);
	const int32 N = int32(R * LoopSecs), Fade = int32(R * 1.5), Pre = int32(R * 0.5);
	// Made a little longer than the loop: half a second for the filters to settle (dropped), and 1.5 s past the end that is laid
	// over the start, so that the end runs into the start without a seam.
	const int32 All = Pre + N + Fade;
	FRandomStream Rand(0x47b4ea7);
	TArray<float> Deep, Air;
	Deep.SetNumUninitialized(All); Air.SetNumUninitialized(All);
	float Brown = 0;
	for (int32 I = 0; I < All; ++I)
	{
		const float W = Rand.FRandRange(-1.f, 1.f);
		Brown = Brown * 0.995f + W * 0.08f;
		Deep[I] = Brown; Air[I] = Rand.FRandRange(-1.f, 1.f);
	}
	// The body of it: a deep rush, well under 300 Hz, as from a big room under the ground. Over it the air through the opening and
	// the wire, a soft band from 300 Hz to 1.4 kHz that opens more on the stronger out-breath.
	Low(Deep, 260, R); Low(Deep, 260, R); High(Deep, 25, R);
	High(Air, 300, R); Low(Air, 1400, R); Low(Air, 1400, R);
	float DeepTop = 0, AirTop = 0;
	for (int32 I = Pre; I < All; ++I) { DeepTop = FMath::Max(DeepTop, FMath::Abs(Deep[I])); AirTop = FMath::Max(AirTop, FMath::Abs(Air[I])); }
	TArray<float> S; S.SetNumZeroed(N + Fade);
	for (int32 I = 0; I < N + Fade; ++I)
	{
		const double T = FMath::Fmod(double(I) / R, LoopSecs);
		const bool bSecond = T >= Breath;
		const double Local = bSecond ? T - Breath : T;
		const float E = Shape(Local, bSecond ? 0.86 : 1.0, bSecond ? 0.35 : 0.0);
		const float Open = FMath::Clamp((E - 0.22f) / 0.8f, 0.f, 1.f);
		const float D = DeepTop > 1e-6f ? Deep[Pre + I] / DeepTop : 0.f, A = AirTop > 1e-6f ? Air[Pre + I] / AirTop : 0.f;
		S[I] = E * (0.8f * D + (0.12f + 0.28f * Open) * A);
	}
	for (int32 I = 0; I < Fade; ++I) { const float K = float(I) / Fade; S[I] = S[I] * K + S[N + I] * (1.f - K); }
	S.SetNum(N);
	float Top = 0; for (const float V : S) Top = FMath::Max(Top, FMath::Abs(V));
	if (Top > 1e-6f) for (float& V : S) V *= 0.6f / Top;
	return S;
}

double S47Breather::Level(double Metres, double Hour)
{
	if (!FMath::IsFinite(Metres) || Metres >= Gone) return 0;
	const double Near = 1 - S47BreatherLocal::Smooth(Gone * 0.4, Gone, FMath::Max(0.0, Metres));
	return (0.2 + 0.18 * S47BreatherLocal::NightWeight(Hour)) * Near;
}

double S47Breather::Held(double Since, double Secs)
{
	if (Since < 0) return 1;
	constexpr double Down = 0.4, Back = 3.0;
	if (Since < Down) return 0.5 + 0.5 * FMath::Cos(PI * Since / Down);
	if (Since < Secs) return 0;
	if (Since < Secs + Back) return 0.5 - 0.5 * FMath::Cos(PI * (Since - Secs) / Back);
	return 1;
}

int32 S47Breather::WeekOf(int32 Night) { return FMath::Max(0, Night - 1) / 7; }
FString S47Breather::WeekFlag(int32 Week) { return FString::Printf(TEXT("breather_w%d"), FMath::Max(0, Week)); }

double S47Breather::HushSecs()
{
	if (const FS47EventScript* S = S47EventScript::Find(HushScript))
		for (const FS47Beat& B : S->Beats) if (B.Do == ES47Beat::Silence) return FMath::Clamp(B.Secs, 1.0, 15.0);
	return 5;
}

FS47Play S47Breather::StartLoop(US47Audio* A, const FString& Key, TOptional<FVector> AtThree, float Gain)
{
	using namespace S47BreatherLocal;
	if (!A || !A->Ctx.IsValid() || (AtThree && AtThree->ContainsNaN())) return {};
	if (!A->Buf.Contains(BufKey))
	{
		const float Hz = A->Ctx->SampleRate;
		const TArray<float> S = MakeDraft(Hz);
		if (S.IsEmpty()) return {};
		TSharedRef<FS47AudioBuffer> B = A->Ctx->CreateBuffer(1, S.Num(), Hz);
		FMemory::Memcpy(B->GetChannelData(0), S.GetData(), S.Num() * sizeof(float));
		A->Buf.Add(BufKey, B);
		UE_LOG(LogS47Breather, Display, TEXT("S47: the Breather's draft made, %.1f s at %.0f Hz"), S.Num() / Hz, Hz);
	}
	FS47Sound O; O.Gain = Gain; O.Lp = 900.f;
	// A wide source: the whole opening breathes, and it carries (the panner's reference distance, 3 m).
	if (AtThree) O.DestNode = A->Spatial(*AtThree, 3.f);
	return A->LoopSfx(Key, BufKey, O);
}

// ---------- in the world ----------

void FS47Breather::Begin(AS47Web* W, const TMap<FString, int32>& Nodes)
{
	using namespace S47BreatherLocal;
	bLive = M && M->IsFreeform();
	if (!M || !IsValid(W) || !IsValid(M->Inter)) return;
	const TWeakPtr<uint8> Guard = Alive;
	const auto Add = [&](const TCHAR* Id, const TCHAR* Name, TFunction<void()> Use, const TCHAR* Look, double Range)
	{
		const int32* Node = Nodes.Find(Id);
		if (!Node) return;
		FS47Interactable I; I.Id = Id; I.Object = W->ObjAt(*Node); I.Range = Range; I.ExamineRange = 10;
		I.Label = [this, Guard, Label = FString(Name)] { return Guard.IsValid() && bLive && M && M->IsFreeform() ? Label : FString(); };
		I.Use = [this, Guard, Use = MoveTemp(Use)] { if (Guard.IsValid() && bLive && Use) Use(); };
		I.Examine = [this, Guard, Text = FString(Look)] { return Guard.IsValid() && bLive ? Text : FString(); };
		M->Inter->Add(I);
	};
	const auto Say = [this](const TCHAR* Text, float Secs) { if (M && M->UI) M->UI->Toast(Text, Secs); };
	Add(UseGate, TEXT("The Breather / the gate"), [this, Say] { Say(GateText, 7.f); Record(); }, GateLook, 3.0);
	Add(UseSign, TEXT("Sign"), [Say] { Say(SignText, 8.f); }, TEXT("THE BREATHER / CAVE TOURS / CLOSED FOR THE SEASON, in white on green, and the hours painted smaller."), 4.5);
	Add(UsePlaque, TEXT("Plaque"), [this, Say] { Say(PlaqueText, 16.f); Record(); }, TEXT("Bronze, gone green at the edges. THE BREATHER, and a date: 1923."), 3.0);
	Add(UseHut, TEXT("Ticket hut"), [Say] { Say(HutText, 7.f); }, TEXT("A little stucco hut, the window boarded over for the winter."), 3.0);
	UE_LOG(LogS47Breather, Display, TEXT("S47: the Breather is up at road %.1f %.1f (gate %.1f %.1f), %d uses"), S47Breather::Mouth().X, S47Breather::Mouth().Y, S47Breather::Gate().X, S47Breather::Gate().Y, Nodes.Num());
}

void FS47Breather::Record()
{
	FS47FreeformSession* F = M ? M->Freeform.Get() : nullptr;
	if (!F || F->HasFinding(S47Breather::Finding)) return;
	F->RegisterFinding(S47Breather::Finding, S47BreatherLocal::FindingText);
	UE_LOG(LogS47Breather, Display, TEXT("S47: the Breather: field record kept"));
}

void FS47Breather::Stop()
{
	bLive = false; AtGate = 0; HushAt = -1;
	if (bSound && M && IsValid(M->Audio)) M->Audio->Stop(S47BreatherLocal::LoopKey, 0.6f);
	bSound = false;
}

void FS47Breather::Step(double Dt)
{
	FS47FreeformSession* F = M ? M->Freeform.Get() : nullptr;
	if (!bLive || !F || !M->IsFreeform()) { if (bSound) Stop(); return; }
	FVector2D Me;
	const bool bHere = S47BreatherLocal::PlayerRoad(M, Me);
	const double Metres = bHere ? FVector2D::Distance(Me, S47Breather::Mouth()) : 1e9;
	const double Hour = FMath::Fmod(F->Clock(), 86400.) / 3600.;
	const bool bAtGate = bHere && FVector2D::Distance(Me, S47Breather::Gate()) <= S47Breather::GateReach && M->World && !M->World->bDriving;
	StepHush(Dt, bAtGate, Hour);
	StepSound(Dt, Metres, Hour, bHere);
}

void FS47Breather::StepSound(double Dt, double Metres, double Hour, bool bHere)
{
	(void)Dt;
	using namespace S47BreatherLocal;
	US47Audio* A = IsValid(M->Audio) ? M->Audio.Get() : nullptr;
	if (!A || !A->Ctx.IsValid()) { bSound = false; return; }
	const bool bWant = bHere && Metres < S47Breather::Gone + 5;
	if (!bWant) { if (bSound) { A->Stop(LoopKey, 0.8f); bSound = false; } return; }
	// (a new audio context, or the loop let go by someone else: started again)
	const TOptional<FS47LoopState> Now = A->InspectLoop(LoopKey);
	if (!bSound || !Now || !Now->bPlaying)
	{
		const FVector2D At = S47Breather::Mouth();
		const FVector Three(8000 + At.X, S47FarmWorld::FloorRoad(At.X, At.Y) + 0.9, At.Y);
		bSound = S47Breather::StartLoop(A, LoopKey, Three, 0.f).IsValid();
		if (!bSound) return;
	}
	const double Held = S47Breather::Held(HushAt, HushSecsNow);
	const float Lp = float(650 + 550 * NightWeight(Hour));
	A->LoopLevel(LoopKey, float(S47Breather::Level(Metres, Hour) * Held), 1.f, Lp);
}

void FS47Breather::StepHush(double Dt, bool bAtGate, double Hour)
{
	FS47FreeformSession* F = M->Freeform.Get();
	if (HushAt >= 0)
	{
		HushAt += Dt;
		if (HushAt > HushSecsNow + 3.5) HushAt = -1;
		return;
	}
	const double H = FMath::Fmod(FMath::Fmod(Hour, 24.0) + 24.0, 24.0);
	AtGate = bAtGate && H >= 2 && H < 5.5 ? AtGate + Dt : 0;
	if (AtGate < 3) return;
	const int32 Night = FS47FreeformSession::NightOf(F->Clock());
	const FString Flag = S47Breather::WeekFlag(S47Breather::WeekOf(Night));
	if (AskedNight == Night || !F->TownUse || F->TownUse->Seen(Flag) || F->InTutorial() || !F->RulesLive()) return;
	// Not on top of anything else the night is doing.
	if (M->Rhythm && !M->Rhythm->ReadNow().Busy.IsNone()) return;
	AskedNight = Night;
	FString Why;
	if (!S47EventScript::Allows(M, S47Breather::HushScript, Why)) { UE_LOG(LogS47Breather, Display, TEXT("S47: the Breather: no hush tonight (%s)"), *Why); return; }
	FS47EventCtx Ctx;
	if (!S47EventScript::Play(M, S47Breather::HushScript, Ctx)) return;
	HushAt = 0; HushSecsNow = S47Breather::HushSecs();
	F->TownUse->Note(Flag);
	UE_LOG(LogS47Breather, Display, TEXT("S47: the Breather holds its breath for %.1f s, night %d"), HushSecsNow, Night);
}

// ---------- tests ----------

bool S47Breather::SelfTest(TArray<FString>& Lines)
{
	using namespace S47BreatherLocal;
	bool bAll = true;
	const auto Check = [&](bool bOk, const FString& What) { bAll &= bOk; Lines.Add(FString(bOk ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("the Breather: ") + What); };
	// The loop: the same every time, in level, seamless, and it breathes: its loudness a breath apart alike, a quarter breath apart
	// unlike.
	{
		constexpr float Hz = 16000.f;
		const TArray<float> A = MakeDraft(Hz), B = MakeDraft(Hz);
		float Top = 0; bool bFinite = true;
		for (const float V : A) { Top = FMath::Max(Top, FMath::Abs(V)); bFinite &= FMath::IsFinite(V); }
		const int32 Win = int32(Hz * 0.5f);
		TArray<double> Loud;
		for (int32 I = 0; I + Win <= A.Num(); I += Win) { double Sum = 0; for (int32 K = 0; K < Win; ++K) Sum += double(A[I + K]) * A[I + K]; Loud.Add(FMath::Sqrt(Sum / Win)); }
		const auto Lag = [&Loud](int32 By) { double D = 0; for (int32 I = 0; I < Loud.Num(); ++I) D += FMath::Abs(Loud[I] - Loud[(I + By) % Loud.Num()]); return D / FMath::Max(1, Loud.Num()); };
		const double Same = Lag(int32(Breath / 0.5)), Quarter = Lag(int32(Breath / 0.5 / 4));
		const double Seam = FMath::Abs(A.Last() - A[0]);
		double Steps = 0; for (int32 I = 1; I < A.Num(); ++I) Steps = FMath::Max(Steps, double(FMath::Abs(A[I] - A[I - 1])));
		const bool bLen = FMath::IsNearlyEqual(A.Num() / double(Hz), LoopSecs, 0.01);
		Check(A == B && bFinite && Top > 0.5f && Top <= 0.61f && bLen && Seam <= Steps * 3 + 1e-3 && Quarter > Same * 1.5,
			FString::Printf(TEXT("the draft is %.1f s, made the same each time, peak %.2f, no seam at the loop (%.4f), and breathes every %.0f s (loudness a breath apart differs %.3f, a quarter breath %.3f)"), A.Num() / double(Hz), Top, Seam, Breath, Same, Quarter));
	}
	Check(Level(3, 23) > Level(50, 23) && Level(50, 23) > 0 && Level(Gone, 23) == 0 && Level(200, 3) == 0 && Level(3, 2) > Level(3, 13) * 1.6 && FMath::IsNearlyEqual(Level(3, 3), Level(3, 23), 1e-9),
		TEXT("louder near the mouth, nothing past 75 m, stronger at night than by day"));
	Check(Held(-1, 5) == 1 && Held(0, 5) == 1 && Held(0.5, 5) == 0 && Held(4.9, 5) == 0 && Held(6.5, 5) > 0 && Held(6.5, 5) < 1 && Held(8.1, 5) == 1,
		TEXT("a hush stops the draft in under half a second, holds it, and lets it back over three"));
	Check(WeekOf(1) == 0 && WeekOf(7) == 0 && WeekOf(8) == 1 && WeekOf(15) == 2 && WeekFlag(WeekOf(8)).Len() < 24 && WeekFlag(0) != WeekFlag(1),
		TEXT("one hush a week at most: a flag a week, short enough for the town's saved flags"));
	// The script: there, clean, a full-tier night after two, rare, and a silence beat that says how long; nothing kept.
	{
		const FS47EventScript* S = S47EventScript::Find(HushScript);
		bool bGood = S && S->bEnabled && S47EventScript::Lint(*S).Num() == 0 && S->Gates.bTier && S->Gates.Tier == ES47Allowed::Full && S->Gates.HourFrom >= 2 && S->Gates.Odds < 1;
		bool bSilence = false, bKept = false;
		if (S) for (const FS47Beat& B : S->Beats) { bSilence |= B.Do == ES47Beat::Silence; bKept |= B.Do == ES47Beat::Evidence || B.Do == ES47Beat::Map || B.Do == ES47Beat::CameraSnap || B.Do == ES47Beat::Thought || B.Do == ES47Beat::Line; }
		Check(bGood && bSilence && !bKept && HushSecs() >= 3 && HushSecs() <= 8, S ? FString::Printf(TEXT("breather_hush.json: full tier, from %.0f:00, odds %.2f, the draft holds %.1f s; no thought, line, finding or mark"), S->Gates.HourFrom, S->Gates.Odds, HushSecs()) : FString(TEXT("breather_hush.json is not in the library")));
	}
	// Where it stands, and its words.
	{
		const S47TownSites::FSite* S = S47TownSites::Find(TEXT("breather"));
		const FVector2D Mo = Mouth(), Ga = Gate();
		Check(S && S->bLandmark && Mo.X < -42 && Ga.X > Mo.X && FVector2D::Distance(Mo, Ga) < 8 && S47OpenLand::InRim(Mo.X, Mo.Y, 50),
			FString::Printf(TEXT("the mouth at road %.1f %.1f, west of the town's fence, the gate %.1f m in front of it"), Mo.X, Mo.Y, FVector2D::Distance(Mo, Ga)));
		bool bClean = true; FString Bad;
		for (const TCHAR* T : { SignText, PlaqueText, GateText, GateLook, HutText, FindingText })
		{
			const FString Why = S47Script::LintText(T);
			if (!Why.IsEmpty()) { bClean = false; Bad = Why; }
			if (FString(T).ToLower().Contains(TEXT("taken"))) { bClean = false; Bad = TEXT("taken"); }
		}
		Check(bClean, bClean ? FString(TEXT("the sign, the plaque, the gate, the hut and the record say nothing they should not")) : TEXT("says: ") + Bad);
	}
	return bAll;
}
