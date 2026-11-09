// TV-SCHEDULE-20261109: the set in Dale's office on the late station (see S47OfficeTv.h).
#include "S47OfficeTv.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RHI.h"
#include "S47Audio.h"
#include "S47Freeform.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47MotelTv.h"
#include "S47Room6.h"
#include "S47Story.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47WebAudio.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47OfficeTv, Log, All);

namespace
{
	// the tube (4:3), the card's size (S47MotelTv), how far the picture is drawn and heard
	const int32 TW = 192, TH = 144, CW = 320, CH = 240;
	const double SEEN = 25, HEARD = 14, NEAR_WALL = 3;
	// the programme from the set's speaker, as loud as the motel office's (S47MotelTv PROGRAMME); the card's thin tone, the snow's
	// hiss and the tube's whine as the motel's set and room 6's (S47Audio TvHiss) have them
	const double PROGRAMME = 0.3, TONE = 0.03, CARD_HISS = 0.006, SNOW_HISS = 0.09, WHINE = 0.0016;
	// CRIT6-20261109, critic round 7: in the story the set is on the room's circuit, as Dale's ceiling and desk lamps are (S47Office):
	// dark in the dark minute and while the feeder is out (AS47Story::PowerOut folds FeederOutage into bDark), not waiting on RX
	// bank 3 (AS47Story::bPowered, the receiver's), which comes up only after the handover.
	bool StoryLit(bool bDark) { return !bDark; }
	// through the office's wall and door: duller and quieter
	const double WALL_LP = 650, OPEN_LP = 18000, THROUGH = 0.4;
	// the set's light on the room in front of it (lumens, as the office's own lamps are given), following the picture
	const double GLOW_LM = 0.7;
	void Blend(FColor& C, const FColor& Ink, double A)
	{
		C.R = uint8(FMath::Clamp(FMath::RoundToInt(C.R * (1 - A) + Ink.R * A), 0, 255));
		C.G = uint8(FMath::Clamp(FMath::RoundToInt(C.G * (1 - A) + Ink.G * A), 0, 255));
		C.B = uint8(FMath::Clamp(FMath::RoundToInt(C.B * (1 - A) + Ink.B * A), 0, 255)); C.A = 255;
	}
	double Mean(const TArray<FColor>& P)
	{
		double Sum = 0; int32 N = 0;
		for (int32 I = 0; I < P.Num(); I += 7) { Sum += 0.299 * P[I].R + 0.587 * P[I].G + 0.114 * P[I].B; ++N; }
		return N ? Sum / (255.0 * N) : 0;
	}
	const TCHAR* Name(FS47OfficeTv::EShow S)
	{
		return S == FS47OfficeTv::EShow::Programme ? TEXT("programme") : S == FS47OfficeTv::EShow::Card ? TEXT("card") : S == FS47OfficeTv::EShow::Snow ? TEXT("snow") : TEXT("off");
	}
}

FS47OfficeTv::FS47OfficeTv(US47Main* InM, UStaticMeshComponent* InSet, const FS47Obj& InObj, TFunction<bool()> InInside)
	: M(InM), Set(InSet), Obj(InObj), Inside(MoveTemp(InInside))
{
	Card = FS47MotelTv::CardPicture();
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!InSet || !W) return;
	// KELVORA's slots: 1 the screen (unlit, its picture through EmissiveMap/EmU/EmV, as S47RetroProps binds the motel's), 2 the pilot.
	if (InSet->GetNumMaterials() > 1 && InSet->GetMaterial(1))
	{
		UMaterialInstanceDynamic* S = UMaterialInstanceDynamic::Create(InSet->GetMaterial(1), W);
		S->SetVectorParameterValue(TEXT("EmU"), FLinearColor(1, 0, 0, 0));
		S->SetVectorParameterValue(TEXT("EmV"), FLinearColor(0, -1, 1, 0));
		S->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::White);
		S->SetScalarParameterValue(TEXT("UseEmissiveMap"), 0);
		InSet->SetMaterial(1, S); Screen = S;
	}
	if (InSet->GetNumMaterials() > 2 && InSet->GetMaterial(2))
	{
		UMaterialInstanceDynamic* P = UMaterialInstanceDynamic::Create(InSet->GetMaterial(2), W);
		P->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
		InSet->SetMaterial(2, P); Pilot = P;
	}
	// Its glow: a small lamp a hand in front of the glass (KELVORA faces its own +X), no shadow, only while the picture is drawn.
	UPointLightComponent* L = NewObject<UPointLightComponent>(W);
	L->SetMobility(EComponentMobility::Movable); L->SetupAttachment(InSet);
	L->SetRelativeLocation(FVector(40, 0, 28)); L->SetIntensityUnits(ELightUnits::Lumens); L->SetIntensity(0);
	L->SetLightColor(FLinearColor(0.66f, 0.72f, 0.92f)); L->SetAttenuationRadius(260); L->SetCastShadows(false); L->SetVisibility(false);
	L->RegisterComponent(); Glow = L;
	// A closer look (the zoom), never in the way of the papers beside it.
	if (M->Inter && Obj)
	{
		InSet->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		FS47Interactable It; It.Id = TEXT("office:tv"); It.Object = Obj; It.Label = [] { return FString(); }; It.Use = [] {}; It.bLook = true; It.ExamineRange = 4;
		It.Examine = [this]() -> FString
		{
			if (Inside && !Inside()) return FString();
			switch (Show)
			{
			case EShow::Programme: return TEXT("Dale's set, left on with the sound low. A late broadcast: robes, painted spacecraft and a promise of visitors from the stars.");
			case EShow::Card: return TEXT("Dale's set, left on. The station's test card, and a thin tone.");
			case EShow::Snow: return TEXT("Dale's set, left on. Snow and a hiss: nothing on the air at this hour.");
			default: return TEXT("Dale's portable set, dark. No power.");
			}
		};
		M->Inter->Add(It);
	}
	SetScreen(false);
	UE_LOG(LogS47OfficeTv, Display, TEXT("S47: Operations television on the late station (programme %s, %02d:%02d to %02d:%02d)"), Programme.Available() ? TEXT("staged") : TEXT("not staged: test card"),
		int32(FS47TvProgramme::ON_AIR / 3600), int32(FMath::Fmod(FS47TvProgramme::ON_AIR, 3600.0) / 60), int32(FS47TvProgramme::OFF_AIR / 3600), int32(FMath::Fmod(FS47TvProgramme::OFF_AIR, 3600.0) / 60));
}

FS47OfficeTv::~FS47OfficeTv()
{
	// UObject teardown may already have destroyed M/Audio; the graph owns its stop closure.
	if (Snd && Snd->Stop) Snd->Stop();
	Snd.Reset();
	if (Glow.IsValid()) Glow->DestroyComponent();
}

double FS47OfficeTv::Clock() const
{
	if (!M) return 0;
	if (M->IsFreeform() && M->Freeform) return M->Freeform->Clock();
	return M->Story ? M->Story->Clock() : 0;
}

bool FS47OfficeTv::Powered() const
{
	// Freeform: SARO's own supply (the feeder or the generator). The story: the room's power, as its lamps have it (StoryLit).
	if (!M) return false;
	if (M->IsFreeform() && M->Freeform) return M->Freeform->HasPowerForReactiveLight();
	return M->Story && StoryLit(M->Story->bDark);
}

FVector FS47OfficeTv::Speaker() const { return Obj ? Obj.LocalToWorld(FVector(0, 0.26, 0)) : FVector::ZeroVector; }

FS47OfficeTv::EShow FS47OfficeTv::ShowAt(double InClock, bool bPowered, bool bHaveProgramme, bool bForced)
{
	if (!bPowered) return EShow::Off;
	if (bForced || FS47TvProgramme::OnAir(InClock)) return bHaveProgramme ? EShow::Programme : EShow::Card;
	return EShow::Snow;
}

void FS47OfficeTv::Step(double Dt)
{
	if (!M || !M->Web || !Set.IsValid()) return;
	const bool bPlay = M->Mode == US47Main::EMode::Play && M->Player && M->Web->Area() == TEXT("saro");
	const EShow Was = Show;
	Show = ShowAt(Clock(), Powered(), Programme.Available(), FS47TvProgramme::ForcedOnAir());
	if (Show != Was) { UE_LOG(LogS47OfficeTv, Display, TEXT("S47: Operations television %s"), Name(Show)); SetScreen(Show != EShow::Off); PaintedFrame = -1; PaintT = 1; }
	const double Dist = bPlay ? FVector::Distance(M->Player->Pos, Speaker()) : TNumericLimits<double>::Max();
	const bool bThrough = !(Inside && Inside());
	// The station's programme: its clock is the station's when it comes on (every set on the same moment of it), then it goes on
	// with its own sound; heard only near, and stepping back under a phone, a call or a voice.
	const bool bProgramme = Show == EShow::Programme;
	if (bProgramme && !bShowing) Programme.Time = FMath::Fmod(FMath::Fmod(Clock() - FS47TvProgramme::ON_AIR, 86400.0) + 86400.0, 86400.0);
	bShowing = bProgramme;
	if (bProgramme)
	{
		const double Duck = M->Story ? M->Story->VoicesDuck() : 1.0;
		Programme.Update(Dt, M->Audio.Get(), Speaker(), Dist <= HEARD ? PROGRAMME * Duck : 0.0, bThrough);
	}
	else Programme.Silence();
	// The picture: drawn while it can be seen, the programme at its own rate, the card and the snow at 15 a second.
	PaintT += Dt;
	if (Show != EShow::Off && Dist < SEEN && (bProgramme ? Programme.FrameIndex() != PaintedFrame : PaintT >= 1.0 / 15)) { PaintT = 0; Paint(); }
	const bool bLit = Show != EShow::Off && Dist < SEEN;
	Light += ((bLit ? FMath::Clamp(FMath::Sqrt(FMath::Max(0.0, Lum) / 0.25), 0.35, 1.35) * (0.94 + 0.06 * FMath::FRand()) : 0.0) - Light) * FMath::Clamp(Dt * 12, 0.0, 1.0);
	if (Glow.IsValid()) { Glow->SetVisibility(Light > 0.01); Glow->SetIntensity(float(GLOW_LM * Light)); }
	Sound(bPlay ? Dist : TNumericLimits<double>::Max(), bThrough);
}

void FS47OfficeTv::SetScreen(bool bOn)
{
	if (Screen.IsValid())
	{
		Screen->SetScalarParameterValue(TEXT("UseEmissiveMap"), bOn && Tube ? 1.f : 0.f);
		if (Tube) Screen->SetTextureParameterValue(TEXT("EmissiveMap"), Tube.Get());
	}
	if (Pilot.IsValid()) Pilot->SetVectorParameterValue(TEXT("Emissive"), bOn ? FLinearColor(.8f, .002f, .001f) : FLinearColor::Black);
	if (!bOn) { Lum = 0; Silence(); }
}

// ---------- the picture tube ----------
void FS47OfficeTv::Paint()
{
	TArray<FColor> Px;
	const bool bFilm = Show == EShow::Programme && Programme.Frame(Px, TW, TH, 4.0 / 3);
	if (bFilm) { PaintedFrame = Programme.FrameIndex(); Lum = Programme.Brightness(); }
	else if (Show == EShow::Snow)
	{
		// room 6's snow (96 x 72, its rolling band), drawn up to the tube
		TArray<FColor> Snow;
		US47Room6::DrawSnow(Snow, M ? M->T : 0.0, [] { return double(FMath::FRand()); });
		Px.SetNumUninitialized(TW * TH);
		for (int32 Y = 0; Y < TH; ++Y) for (int32 X = 0; X < TW; ++X) Px[Y * TW + X] = Snow[(Y * US47Room6::TV_H / TH) * US47Room6::TV_W + X * US47Room6::TV_W / TW];
		Lum = Mean(Px);
	}
	else
	{
		// the card with the motel set's overscan (drawImage(src, 6, 5, W - 12, H - 10))
		Px.SetNumUninitialized(TW * TH);
		const int32 J = FMath::FRand() < 0.08 ? 1 : 0;
		for (int32 Y = 0; Y < TH; ++Y) for (int32 X = 0; X < TW; ++X)
		{
			const int32 Sx = FMath::Clamp(6 + X * (CW - 12) / TW, 0, CW - 1), Sy = FMath::Clamp(5 + (FMath::Max(0, Y - J)) * (CH - 10) / TH, 0, CH - 1);
			Px[Y * TW + X] = Card.Num() == CW * CH ? Card[Sy * CW + Sx] : FColor::Black;
		}
		Lum = Mean(Px);
	}
	// the tube: every other line darker, a few specks, its corners falling off and a breath of the whole picture (S47MotelTv::Paint)
	for (int32 Y = 0; Y < TH; Y += 2) for (int32 X = 0; X < TW; ++X) Blend(Px[Y * TW + X], FColor::Black, 0.24);
	if (Show != EShow::Snow) for (int32 I = 0; I < 60; ++I)
	{
		const int32 X = FMath::RandRange(0, TW - 1), Y = FMath::RandRange(0, TH - 1);
		Blend(Px[Y * TW + X], FColor::White, FMath::FRand() * 0.12);
	}
	const double Breath = 0.04 + 0.04 * FMath::FRand();
	for (int32 Y = 0; Y < TH; ++Y) for (int32 X = 0; X < TW; ++X)
	{
		const double Radius = FMath::Sqrt(FMath::Square(X + 0.5 - TW / 2.0) + FMath::Square(Y + 0.5 - TH / 2.0));
		const double A = FMath::Clamp((Radius - TH * 0.32) / (TW * 0.66 - TH * 0.32), 0.0, 1.0) * 0.6;
		Blend(Px[Y * TW + X], FColor::Black, A); Blend(Px[Y * TW + X], FColor::Black, Breath);
	}
	Upload(Px);
}

void FS47OfficeTv::Upload(const TArray<FColor>& Pixels)
{
	if (Pixels.Num() != TW * TH) return;
	if (!Tube)
	{
		Tube.Reset(S47Image::FromPixels(Pixels, TW, TH));
		SetScreen(Show != EShow::Off);
		return;
	}
	// One persistent canvas texture, its rows top first (as the motel office's tube and its EmV).
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, TW, TH);
	uint8* Data = static_cast<uint8*>(FMemory::Malloc(Pixels.Num() * sizeof(FColor)));
	FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	Tube->UpdateTextureRegions(0, 1, Region, TW * sizeof(FColor), sizeof(FColor), Data, [](uint8* Src, const FUpdateTextureRegion2D* R) { FMemory::Free(Src); delete R; });
}

// ---------- the set's little speaker: the card's tone, the snow's hiss, the tube's whine ----------
void FS47OfficeTv::Sound(double Dist, bool bThrough)
{
	US47Audio* A = M ? M->Audio.Get() : nullptr;
	const auto Ctx = A ? A->Ctx : nullptr;
	// (the hiss and the tone stay in the room: from the corridor and the control room only the programme comes through the wall)
	if (!Ctx || Show == EShow::Off || Dist > HEARD || (bThrough && Dist > NEAR_WALL)) { Hush(); return; }
	FS47AudioContext::FBatch B(*Ctx);
	if (!Snd)
	{
		const auto Spk = A->Spatial(Speaker(), 0.9f); const auto Noise = A->NoiseBuffer();
		if (!Spk || !Noise) return;
		const auto Wall = Ctx->CreateBiquadFilter(); Wall->SetType(TEXT("lowpass")); Wall->Frequency.SetValue(bThrough ? WALL_LP : OPEN_LP); Wall->Connect(Spk);
		auto Gain = [&] { const auto G = Ctx->CreateGain(); G->Gain.SetValue(0); G->Connect(Wall); return G; };
		const auto Tone = Gain(); const auto TOsc = Ctx->CreateOscillator(); TOsc->Frequency.SetValue(1000);
		const auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1300); Bp->Q.SetValue(0.5);
		TOsc->Connect(Bp); Bp->Connect(Tone);
		const auto Hiss = Gain(); const auto N = Ctx->CreateBufferSource(); N->Buffer = Noise; N->bLoop = true;
		const auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(450);
		const auto Bh = Ctx->CreateBiquadFilter(); Bh->SetType(TEXT("bandpass")); Bh->Frequency.SetValue(2600); Bh->Q.SetValue(0.5);
		N->Connect(Hp); Hp->Connect(Bh); Bh->Connect(Hiss);
		const auto Whine = Gain(); const auto WOsc = Ctx->CreateOscillator(); WOsc->Frequency.SetValue(15734); WOsc->Connect(Whine);
		TOsc->Start(); N->Start(); WOsc->Start();
		Snd = FSnd{ Tone, Hiss, Whine, Wall, [TOsc, N, WOsc, Spk] { TOsc->Stop(); N->Stop(); WOsc->Stop(); Spk->Disconnect(); } };
	}
	const double Now = Ctx->CurrentTime(), K = bThrough ? THROUGH : 1.0;
	auto Ramp = [Now](const TSharedPtr<FS47GainNode>& G, double V) { G->Gain.SetTargetAtTime(V, Now, 0.04); };
	Ramp(Snd->Tone, Show == EShow::Card ? TONE * K : 0);
	Ramp(Snd->Hiss, (Show == EShow::Snow ? SNOW_HISS : Show == EShow::Card ? CARD_HISS : 0) * K);
	Ramp(Snd->Whine, WHINE * K);
	Snd->Wall->Frequency.SetTargetAtTime(bThrough ? WALL_LP : OPEN_LP, Now, 0.12);
}

void FS47OfficeTv::Silence()
{
	Programme.Silence();
	Hush();
}

void FS47OfficeTv::Hush()
{
	if (!Snd) return;
	const FSnd S = *Snd; Snd.Reset();
	const auto Ctx = M && M->Audio ? M->Audio->Ctx : nullptr;
	if (Ctx)
	{
		FS47AudioContext::FBatch B(*Ctx);
		for (const auto& G : { S.Tone, S.Hiss, S.Whine }) G->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.03);
		Ctx->SetTimeout(0.3, S.Stop);
	}
	else if (S.Stop) S.Stop();
}

FString FS47OfficeTv::State() const
{
	return FString::Printf(TEXT("%s%s"), Name(Show), Show == EShow::Programme ? *(TEXT(", ") + Programme.Describe()) : TEXT(""));
}

bool FS47OfficeTv::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s officeTv: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	Check(ShowAt(1 * 3600, true, true, false) == EShow::Programme && ShowAt(23.75 * 3600, true, true, false) == EShow::Programme, TEXT("the late programme in the station's hours"));
	Check(ShowAt(1 * 3600, true, false, false) == EShow::Card && ShowAt(12 * 3600, true, false, true) == EShow::Card, TEXT("without the local clip: the test card in its place"));
	Check(ShowAt(12 * 3600, true, true, false) == EShow::Snow && ShowAt(4 * 3600, true, true, false) == EShow::Snow, TEXT("snow the rest of the day"));
	Check(ShowAt(1 * 3600, false, true, true) == EShow::Off, TEXT("dark without power"));
	Check(StoryLit(false) && !StoryLit(true) && ShowAt(1 * 3600, StoryLit(true), true, false) == EShow::Off && ShowAt(1 * 3600, StoryLit(false), true, false) == EShow::Programme,
		TEXT("in the story the set follows the room's power (its lamps' !bDark, the feeder in it), not RX bank 3"));
	return bOk;
}
