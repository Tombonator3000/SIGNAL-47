// main.ts: the phones, the archive and the records (archivePhone, archiveDishes, officePhone, roofSteps,
// fieldBook, sameThread, looks, the closer look), in the order main.ts builds and steps them
#include "S47Phones.h"

#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Internationalization/Regex.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "S47Annex.h"
#include "S47ArchiveDishes.h"
#include "S47Audio.h"
#include "S47Canvas.h"
#include "S47CaseBoard.h"
#include "S47Chapter2.h"
#include "S47Curve.h"
#include "S47FieldBook.h"
#include "S47FieldBookView.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Geo.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Looks.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47SameThread.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47Board.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Phones, Log, All);

namespace
{
	using EMode = US47Main::EMode;
	int32 ChapterNo(const FString& Key) { return Key.StartsWith(TEXT("ch")) ? FCString::Atoi(*Key.Mid(2)) : 0; }
	/** the tired tube in the records room (Annex.flickerTube) */
	const FVector TUBE_AT(-4.6, 2.7, 8.0);
	/** the array's first rows, for the seen log (main.ts arrayFront) */
	const FVector ARRAY_FRONT(-1, 9, -80);
	/** world/Crossing.ts atMotel */
	bool AtMotel(const FVector& P) { const S47Geo::FRect& R = S47Geo::MOTEL_RECT(); return P.X >= R.MinX && P.X <= R.MaxX + 1 && P.Z >= R.MinZ - 4 && P.Z <= R.MaxZ + 4; }
	bool Is(const TFunction<bool()>& F) { return F && F(); }
	US47Chapter2* Ch2(const US47Main* M) { return M && M->Story ? Cast<US47Chapter2>(M->Story->Chapter(TEXT("ch2"))) : nullptr; }
	bool AnyRead(const US47Chapter2* C) { return C && (C->S.Read.bOriginal || C->S.Read.bAmended || C->S.Read.bLineage || C->S.Read.bIndex); }
}

// ---------- boot ----------

void US47Phones::Init(US47Main* InM)
{
	M = InM;
	AS47Web* W = M->Web;
	TWeakObjectPtr<US47Phones> Self = this;
	if (!W || !M->Story || !M->Night) { UE_LOG(LogS47Phones, Warning, TEXT("S47: the phones and the archive have no world")); return; }

	// ---------- a thread on the case board the player did not tie (story/sameThread.ts) ----------
	FS47SameDeps Sd;
	Sd.Seen = &M->TownSeen;
	Sd.Pins = [Self]
	{
		TArray<TPair<FString, FString>> Out;
		if (Self.IsValid() && Self->M->CaseBoard) for (const US47CaseBoard::FPin& P : Self->M->CaseBoard->Pins()) Out.Add({ P.Pin, P.Card });
		return Out;
	};
	Sd.Rests = [Self](const FString& P) { return Self.IsValid() && Self->M->CaseBoard && Self->M->CaseBoard->Rests(P); };
	Sd.Clock = [Self] { return Self.IsValid() ? Self->Clock() : 0.0; };
	Sd.Note = [Self](const FString& T) { if (Self.IsValid()) Self->M->Story->Note(T); };
	Sd.Saw = [Self] { if (!Self.IsValid()) return; FS47SawOpts O; O.bLookingSet = true; O.Looking = true; O.bNoWatch = true; Self->M->Night->Saw(TEXT("scare"), TEXT("same thread"), O); };
	Sd.Save = [Self] { if (Self.IsValid() && Self->M->Mode == EMode::Play) Self->RequestAutosave(); };
	SameThread = MakeShared<FS47SameThread>(Sd);
	SameThread->OnBook = [Self] { if (Self.IsValid()) Self->SyncBooks(); };
	if (US47CaseBoard* CB = M->CaseBoard)
	{
		// game.board.onOpen = (view) => sameThread.opened(view)
		TFunction<void(TSharedRef<SS47Board>)> Before = CB->OnOpen;
		CB->OnOpen = [Self, Before](TSharedRef<SS47Board> V)
		{
			if (Before) Before(V);
			if (!Self.IsValid()) return;
			TWeakPtr<SS47Board> Wv = V;
			Self->SameThread->Opened(V, [Self, Wv] { return Self.IsValid() && Self->M->CaseBoard && Self->M->CaseBoard->View.IsValid() && Self->M->CaseBoard->View == Wv.Pin(); });
		};
	}

	// ---------- the crank telephone in the motel office (story/officePhone.ts) ----------
	FS47OfficePhoneDeps Od;
	Od.Audio = M->Audio; Od.Web = W; Od.Seen = &M->TownSeen;
	Od.Notes = [Self] { return Self.IsValid() ? Self->M->Story->Notes() : TArray<FString>(); };
	Od.Note = [Self](const FString& T) { if (Self.IsValid()) Self->M->Story->Note(T); };
	Od.Toast = [Self](const FString& T, float S) { if (Self.IsValid()) Self->M->UI->Toast(T, S); };
	Od.Frames = [Self] { return Self.IsValid() && Self->Frames ? Self->Frames() : TPair<TOptional<double>, TOptional<double>>(); };
	Od.Save = [Self] { if (Self.IsValid() && Self->M->Mode == EMode::Play) Self->RequestAutosave(); };
	Od.Refresh = [Self] { if (Self.IsValid() && Self->M->Board.Refresh) Self->M->Board.Refresh(); };
	OfficePhone = MakeShared<FS47OfficePhone>(Od);
	// (ch4.officePhone = { label: () => officePhone.label(), use: () => officePhone.use() }: chapter four's
	// port binds the office's telephone, and asks this first)

	// ---------- T. Vega's field book, and the envelope for plate 4 (story/fieldBook.ts) ----------
	FS47FieldBookDeps Fd;
	Fd.UI = M->UI; Fd.Audio = M->Audio; Fd.Seen = &M->TownSeen;
	Fd.Notes = [Self] { return Self.IsValid() ? Self->M->Story->Notes() : TArray<FString>(); };
	Fd.Note = [Self](const FString& T) { if (Self.IsValid()) Self->M->Story->Note(T); };
	Fd.Toast = [Self](const FString& T, float S) { if (Self.IsValid()) Self->M->UI->Toast(T, S); };
	Fd.Clock = [Self] { return Self.IsValid() ? Self->Clock() : 0.0; };
	Fd.After = [Self](double S, TFunction<void()> Fn) { if (Self.IsValid()) Self->M->Story->After(S, MoveTemp(Fn)); };
	Fd.State.Get = [Self] { return !Self.IsValid() ? TSharedPtr<FJsonValue>() : Self->FbGet ? Self->FbGet() : Self->FbOwn; };
	Fd.State.Set = [Self](const TSharedPtr<FJsonValue>& V) { if (!Self.IsValid()) return; if (Self->FbSet) Self->FbSet(V); else Self->FbOwn = V; };
	Fd.File = [Self](const FS47Doc& D) { if (Self.IsValid()) Self->M->Story->AddDoc(D); };
	Fd.Save = [Self] { if (Self.IsValid() && Self->M->Mode == EMode::Play) Self->RequestAutosave(); };
	Fd.Refresh = [Self] { if (Self.IsValid() && Self->M->Board.Refresh) Self->M->Board.Refresh(); };
	Fd.Drawer = [Self](bool bOpen, bool bNow) { if (!Self.IsValid()) return; if (Self->StationDrawer) Self->StationDrawer(bOpen, bNow); else Self->SetDrawerFallback(bOpen, bNow); };
	Fd.Foretell = [Self] { if (Self.IsValid() && Self->Foretell) Self->Foretell(); };
	FieldBook = MakeShared<FS47FieldBook>(Fd);
	DrawerZ = W->Obj(TEXT("world.site.objs.drawer")).Position().Z;
	{
		// the drawer under the bench in the hut (main.ts world.onStationLoaded)
		FS47Interactable I;
		I.Id = TEXT("s1drawer"); I.Object = W->Obj(TEXT("world.site.proxies.drawer")); I.Range = 2.2; I.ExamineRange = 3;
		I.Label = [Self] { return Self.IsValid() && Self->M->World && Self->M->World->Area == TEXT("station01") && Self->At(TEXT("ch3")) ? Self->FieldBook->DrawerLabel() : FString(); };
		I.Use = [Self] { if (Self.IsValid()) Self->FieldBook->UseDrawer(); };
		I.Examine = [Self] { return Self.IsValid() && Self->M->World && Self->M->World->Area == TEXT("station01") && Self->At(TEXT("ch3")) ? Self->FieldBook->DrawerExamine() : FString(); };
		M->Inter->Add(I);
		FieldBook->DrawerNow();
	}
	{
		// the envelope on the motel office's counter
		FS47Interactable I;
		I.Id = TEXT("court:envelope"); I.Object = W->Obj(TEXT("world.motel.proxies.envelope")); I.Range = 2.2;
		I.Label = [Self] { return Self.IsValid() && Self->M->Mode == EMode::Play && Self->M->World && Self->M->World->Area == TEXT("saro") && !Self->Driving() && Self->At(TEXT("ch1")) ? Self->FieldBook->EnvelopeLabel() : FString(); };
		I.Use = [Self] { if (Self.IsValid()) Self->FieldBook->UseEnvelope(); };
		M->Inter->Add(I);
	}

	// ---------- light steps on the records room roof, once in chapter two (story/roofSteps.ts) ----------
	FS47RoofDeps Rd;
	Rd.Audio = M->Audio; Rd.Seen = &M->TownSeen; Rd.Inter = M->Inter;
	Rd.Note = [Self](const FString& T) { if (!Self.IsValid()) return; Self->M->Story->Note(T); Self->RequestAutosave(); };
	Rd.Toast = [Self](const FString& T, float S) { if (Self.IsValid()) Self->M->UI->Toast(T, S); };
	Rd.Frost = [](const FString& C) { if (US47Annex* A = US47Annex::Get()) A->SetRoofFrost(C); };
	Rd.Tubes = [](const FString& Mode) { if (US47Annex* A = US47Annex::Get()) A->HoldTubes(Mode); };
	// (a little in front of the face, so it falls through the view whether the player looks up or not)
	Rd.Dust = [Self](double X, double Z)
	{
		US47Annex* A = US47Annex::Get();
		if (!Self.IsValid() || !A) return;
		FVector F = FS47Cam::Of(Self->M->Player).Forward; F.Y = 0;
		if (F.SizeSquared() < 1e-6) F = FVector(0, 0, -1);
		F.Normalize();
		A->Sift(X + F.X * 0.7, Z + F.Z * 0.7);
	};
	Rd.Puff = [](double X, double Z, int32 N) { if (US47Annex* A = US47Annex::Get()) A->Sift(X, Z, N, 0.16, 0.3); };
	Rd.TubeAt = TUBE_AT;
	Rd.Hit = { W->Obj(TEXT("annex.frost.hit.unit")), W->Obj(TEXT("annex.frost.hit.sw")), W->Obj(TEXT("annex.frost.hit.se")) };
	Rd.Outside = [Self] { return Self.IsValid() && Self->M->World && Self->M->World->Area == TEXT("saro") && !Self->Driving() && Self->M->Space == TEXT("yard"); };
	Rd.Clock = [Self] { return Self.IsValid() ? Self->Clock() : 0.0; };
	RoofSteps = MakeShared<FS47RoofSteps>(Rd);

	// ---------- the archive's phone: chapter two's peak, the phone from the future a second time ----------
	// (story/archivePhone.ts; the dishes it tips, story/archiveDishes.ts; reel 047 on the deck, story/reel047.ts)
	FS47ArrayTipDeps Td;
	Td.Web = W; Td.Audio = M->Audio;
	Td.Rogue = [Self]
	{
		if (!Self.IsValid() || !Self->DishRogue || !Self->M->Web) return false;
		for (int32 I = 0; I < Self->M->Web->DishCount(); ++I) if (Self->DishRogue(I)) return true;
		return false;
	};
	ArrayTip = MakeShared<FS47ArrayTip>(Td);
	FS47ArcDeps Ad;
	Ad.Audio = M->Audio; Ad.Annex = US47Annex::Get(); Ad.Inter = M->Inter; Ad.Seen = &M->TownSeen; Ad.Dishes = ArrayTip.Get();
	Ad.Deck.Playing = [Self] { return Self.IsValid() && Is(Self->Reel047.Playing); };
	Ad.Deck.Finished = [Self] { return Self.IsValid() && Is(Self->Reel047.Finished); };
	Ad.Deck.Update = [Self](double Dt) { if (Self.IsValid() && Self->Reel047.Update) Self->Reel047.Update(Dt); };
	Ad.Deck.Play = [Self](TFunction<void(const FString&)> Cue)
	{
		if (Self.IsValid() && Self->Reel047.Play) Self->Reel047.Play(MoveTemp(Cue));
		else { UE_LOG(LogS47Phones, Warning, TEXT("S47: reel 047's deck (story/reel047.ts) is not hooked to the archive's phone")); }
	};
	Ad.Deck.Reset = [Self] { if (Self.IsValid() && Self->Reel047.Reset) Self->Reel047.Reset(); };
	Ad.Colliders = [Self]() -> TArray<FS47Rect>* { return Self.IsValid() && Self->M->Web ? &Self->M->Web->CollidersMutable(TEXT("saro")) : nullptr; };
	Ad.Note = [Self](const FString& T) { if (!Self.IsValid()) return; Self->M->Story->Note(T); Self->RequestAutosave(); };
	Ad.Toast = [Self](const FString& T, float S) { if (Self.IsValid()) Self->M->UI->Toast(T, S); };
	Ad.Save = [Self] { if (Self.IsValid() && Self->M->Mode == EMode::Play) Self->RequestAutosave(); };
	Ad.Log = [Self](const FString& T, double At) { if (Self.IsValid()) Self->M->Story->ConsoleLog(T, At); };
	Ad.Relay = [Self] { return Self.IsValid() && Self->M->Story->RelayClack(); };
	Ad.At.Rack = [Self] { return Self.IsValid() ? Self->M->Story->RackAt() : FVector::ZeroVector; };
	Ad.At.Desk = [Self] { return Self.IsValid() && Self->M->Web ? Self->M->Web->Obj(TEXT("room.objs.phone")).WorldPosition() : FVector::ZeroVector; };
	Ad.At.Printer = [Self] { FVector P = Self.IsValid() && Self->M->Web ? Self->M->Web->Obj(TEXT("room.objs.printer")).WorldPosition() : FVector::ZeroVector; P.Y = 0.95; return P; };
	Ad.At.RecPhone = [Self] { return Self.IsValid() && Self->M->Web ? Self->M->Web->Obj(TEXT("annex.objs.recPhone")).WorldPosition() : FVector::ZeroVector; };
	Ad.At.Tube = TUBE_AT;
	// (game.cinematic = on; if (on) input.reset(): the walker reads its keys afresh each frame)
	Ad.Cinematic = [Self](bool bOn) { if (Self.IsValid()) Self->M->Story->SetCinematic(bOn); };
	Ad.Shake = [Self](double K) { if (Self.IsValid() && Self->M->Player) Self->M->Player->Shake = FMath::Max(Self->M->Player->Shake, K); };
	Ad.Calm = [Self] { return Self.IsValid() && Self->M->Flow && Self->M->Flow->bCalmFlash; };
	Ad.File = [Self](const FS47Doc& D) { if (Self.IsValid()) Self->M->Story->AddDoc(D); };
	Ad.Open = [Self](const FS47Doc& D, TFunction<void()> OnClose) { if (Self.IsValid()) Self->M->UI->Document(D, MoveTemp(OnClose)); };
	Ad.SolveAt = [Self] { return Self.IsValid() ? Self->M->Story->Night().SolveAt : TOptional<double>(); };
	// the prologue's call: its line went dead at the first WWVB loss in the console log
	Ad.FirstCall = [Self]() -> TOptional<double>
	{
		if (Self.IsValid()) for (const FS47LogLine& L : Self->M->Story->Night().Log) if (L.Text == TEXT("STATION CLOCK  WWVB REFERENCE LOST")) return L.Clock - 5;
		return {};
	};
	Ad.WardCall = [Self]() -> TOptional<double>
	{
		if (!Self.IsValid()) return {};
		for (const FString& N : Self->M->Story->Notes())
		{
			FRegexMatcher R(FRegexPattern(TEXT("^(\\d\\d):(\\d\\d)\\. Dr\\. Ward read the B-12 report")), N);
			if (R.FindNext()) return FCString::Atoi(*R.GetCaptureGroup(1)) * 3600.0 + FCString::Atoi(*R.GetCaptureGroup(2)) * 60.0 - 20;
		}
		return {};
	};
	Ad.Paper = [Self](const FString& Kind, double K) { if (Self.IsValid()) Self->Paper(Kind, K); };
	Ad.Refresh = [Self] { if (Self.IsValid() && Self->M->Board.Refresh) Self->M->Board.Refresh(); };
	Ad.DeckReady = [Self]
	{
		if (!Self.IsValid()) return FString();
		return Is(Self->Dark) ? FString(TEXT("Nothing. The power is out.")) : !Self->M->Story->BankIn() ? FString(TEXT("No power to the processor. It runs off RX bank 3.")) : FString();
	};
	Ad.RoofNow = [Self] { return Self.IsValid() && Self->RoofSteps->Now(Self->RoofIn(true)); };
	Archive = MakeShared<FS47ArchivePhone>(Ad);
	if (US47Chapter2* C2 = Ch2(M))
	{
		// ch2.phoneHook: while it rings, the phones answer it; while it is still to come, the wall phone's line is open
		C2->PhoneHook.Label = [Self] { return Self.IsValid() ? Self->Archive->PhoneLabel() : FString(); };
		C2->PhoneHook.Use = [Self](const FString& Where) { return Self.IsValid() && Self->Archive->PhoneUse(Where, Self->ArcIn()); };
		// main.ts ch2.onEnd: the chapter waits for the steps on the roof
		Ch2End = C2->OnEnd;
		C2->OnEnd = [Self] { if (Self.IsValid()) Self->Ch2EndWhenQuiet(); };
	}
	// game.printerHook: the printer by the rack has a sheet of the archive's phone's in it
	M->Story->PrinterHook.Label = [Self] { return Self.IsValid() ? Self->Archive->PrinterLabel() : FString(); };
	M->Story->PrinterHook.Use = [Self] { return Self.IsValid() && Self->Archive->PrinterUse(); };
	// game.decoder.reel: (the reel waits while something else of the night's is going: one thing at a
	// time). story/Decoder.ts, the desk with tape B, is not in the Unreal version yet: the signal
	// processor here is the reel's deck, and nothing else.
	if (const FS47WebItem* It = W->Item(TEXT("decoder")))
	{
		FS47Interactable I;
		I.Id = TEXT("decoder"); I.Object = W->ObjAt(It->Node); I.Range = 2.4;
		I.Label = [Self]
		{
			if (!Self.IsValid()) return FString();
			return Is(Self->Reel047.Playing) || !Self->M->OneAtATime() ? Self->Archive->DeckLabel() : FString();
		};
		I.Use = [Self]
		{
			if (!Self.IsValid()) return;
			const bool bPlaying = Is(Self->Reel047.Playing);
			if (!bPlaying && Self->M->OneAtATime()) return;
			if (Self->Archive->Reel() == TEXT("kit") && !bPlaying && Is(Self->DecoderPlaying) && Self->DecoderStop) Self->DecoderStop();
			Self->Archive->DeckUse();
		};
		M->Inter->Add(I);
	}

	// ---------- the night's hooks (S47Night: oneAtATime, the curve) ----------
	FS47NightHooks& H = M->Night->Hooks;
	H.RoofStepsBusy = [Self] { return Self.IsValid() && Self->RoofSteps->Busy(); };
	H.OfficePhoneBusy = [Self] { return Self.IsValid() && Self->OfficePhone->Busy(); };
	H.OfficePhoneRinging = [Self] { return Self.IsValid() && Self->OfficePhone->State != TEXT("idle"); };
	H.ArchiveBusy = [Self] { return Self.IsValid() && Self->Archive->Busy(); };
	H.ArchivePeaking = [Self] { return Self.IsValid() && Self->Archive->Peaking(); };
	// and chapter two's 47 is the archive's phone until it has come
	H.ArchiveKeep = [Self] { const US47Chapter2* C2 = Self.IsValid() ? Ch2(Self->M) : nullptr; return C2 && C2->bActive && C2->S.Stage != TEXT("complete") && Self->Archive->Pending(); };

	// ---------- a closer look at what had nothing to say (story/looks.ts) ----------
	FS47LooksDeps Ld;
	Ld.Inter = M->Inter; Ld.Web = W;
	// (rogue: tipped by the archive's phone, or come down to follow a warning)
	Ld.Rogue = [Self](int32 I) { return Self.IsValid() && (Self->ArrayTip->Holds() || (Self->DishRogue && Self->DishRogue(I))); };
	Ld.Back = [Self](int32 I) { return Self.IsValid() && Self->DishBack && Self->DishBack(I); };
	S47Looks::BindLooks(Ld);

	SyncBooks();
	TestFlags();
}

void US47Phones::Ch2EndWhenQuiet()
{
	if (RoofSteps->Busy() && M->Story && M->Story->ChapterKey == TEXT("ch2"))
	{
		TWeakObjectPtr<US47Phones> Self = this;
		M->Story->After(0.25, [Self] { if (Self.IsValid()) Self->Ch2EndWhenQuiet(); });
		return;
	}
	if (Ch2End) Ch2End();
}

// ---------- the deps main.ts reads ----------

bool US47Phones::Modal() const { return M->ModalOpen(); }
bool US47Phones::Cinematic() const { return M->Story && M->Story->IsCinematic(); }
bool US47Phones::Driving() const { return M->World && M->World->bDriving; }
bool US47Phones::InSaro(const FString& Zone) const { return M->Web && M->Player && M->Web->Rect(Zone).Contains(M->Player->Pos); }
FString US47Phones::Phase() const { return M->Story ? M->Story->ChapterKey : FString(TEXT("prologue")); }
bool US47Phones::At(const FString& Chapter) const { return ChapterNo(Phase()) >= ChapterNo(Chapter); }
double US47Phones::Clock() const { return M->Story ? M->Story->Clock() : 0; }
void US47Phones::RequestAutosave() const { M->RequestAutosave(false); }

FString US47Phones::ArcWhere() const
{
	// where the player is for it: the records room, the rest of the annex, the control room, out at SARO
	const FString Area = M->World ? M->World->Area : FString(TEXT("saro"));
	if (Area != TEXT("saro") || Driving()) return TEXT("away");
	if (InSaro(TEXT("annex.zone.records")) && !InSaro(TEXT("annex.zone.recordsDoor"))) return TEXT("records");
	for (const TCHAR* Z : { TEXT("annex.zone.southDoor"), TEXT("annex.zone.corridor"), TEXT("annex.zone.recordsDoor"), TEXT("annex.zone.records") }) if (InSaro(Z)) return TEXT("annex");
	if (InSaro(TEXT("room.bounds"))) return TEXT("room");
	return M->Space == TEXT("yard") ? TEXT("out") : TEXT("away");
}

FS47ArcIn US47Phones::ArcIn() const
{
	const US47Chapter2* C2 = Ch2(M);
	const bool bOn = Phase() == TEXT("ch2") && C2 && C2->bActive;
	FS47ArcIn S;
	S.Stage = bOn ? C2->S.Stage : FString();
	S.bOk = bOn && (S.Stage == TEXT("records") || S.Stage == TEXT("call-nora")) && !Archive->bOff;
	S.bRead = AnyRead(C2);
	S.bQuiet = !Modal() && !Cinematic() && !(M->Audio && M->Audio->PhonePlaying());
	// (and the dishes nobody else's: story/dishesFollow.ts has them while a warning's are out)
	// (nor while Ward's call-back rings at the desk, story/Chapter2.ts)
	const bool bRinging = C2 && C2->bRinging;
	S.bFree = !M->OneAtATime() && !Is(DecoderPlaying) && !Is(CbBusy) && !ArrayTip->Taken() && !bRinging;
	S.bOthers = M->OneAtATime() || ArrayTip->Taken() || bRinging;
	S.bRoofBusy = RoofSteps->Busy(); S.bRoofDone = RoofSteps->Phase == TEXT("done");
	S.Where = ArcWhere();
	S.bHold = Modal() || M->bPausedByMenu;
	S.Clock = Clock();
	S.Cam = FS47Cam::Of(M->Player);
	S.bSaro = M->World && M->World->Area == TEXT("saro");
	S.bView = S.Where == TEXT("out") || S.Where == TEXT("room");
	S.bBankIn = M->Story && M->Story->BankIn();
	S.bCalled = bOn && C2->S.bCalled;
	return S;
}

FS47RoofIn US47Phones::RoofIn(bool bOk) const
{
	const US47Chapter2* C2 = Ch2(M);
	const FString Stage = C2 ? C2->S.Stage : FString();
	const FS47NightHooks& H = M->Night->Hooks;
	const bool bTown = H.TownEventsNow && H.TownEventsNow() != TEXT("none");
	FS47RoofIn R;
	R.bOk = bOk && Phase() == TEXT("ch2") && C2 && C2->bActive && (Stage == TEXT("records") || Stage == TEXT("call-nora")) && !Is(DecoderPlaying)
		&& !Is(H.AbductionBusy) && !(M->Night->NightEvents && M->Night->NightEvents->Busy()) && !Is(H.RoadsideSilent) && !bTown && !Is(H.ArrayBackBusy) && !(C2 && C2->bRinging)
		&& !Archive->Busy() && !Archive->Resting()
		// (they are the archive's phone's build: never after it, story/archivePhone.ts)
		&& !M->TownSeen.Contains(TEXT("arc:call"));
	R.bQuiet = !Modal() && !Cinematic() && !(M->Audio && M->Audio->PhonePlaying());
	R.bHold = Cinematic() || (M->Audio && M->Audio->PhonePlaying());
	const bool bSaro = M->World && M->World->Area == TEXT("saro");
	R.bInRoom = bSaro && !Driving() && InSaro(TEXT("annex.zone.records"));
	R.bMuffled = bSaro && M->Space != TEXT("yard");
	R.Pos = M->Player ? M->Player->Pos : FVector::ZeroVector;
	R.Clock = Clock();
	R.bRead = AnyRead(C2);
	R.bLastQuiet = Stage == TEXT("call-nora");
	R.bChapter = Phase() == TEXT("ch2");
	return R;
}

FS47OfficePhoneIn US47Phones::OfficeIn(bool bModal) const
{
	// on foot in the office, on the motel's court or within earshot of the office's open door (22 m,
	// the shoulder beside it too), chapter four before room 6 is over, nothing else going
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	const bool bFoot = M->World && M->World->Area == TEXT("saro") && !Driving();
	const S47Geo::FAt& Mt = S47Geo::MOTEL_AT();
	const FVector OfficeDoor(-41.1 + Mt.X, 0, 24.1 + Mt.Z);
	const double Door = FMath::Sqrt(FMath::Square(P.X - OfficeDoor.X) + FMath::Square(P.Z - OfficeDoor.Z));
	const bool bCourt = bFoot && AtMotel(P);
	FS47OfficePhoneIn S;
	S.bOk = !bModal && !Cinematic() && !M->OneAtATime() && !(M->Audio && M->Audio->PhonePlaying()) && !Is(CbBusy);
	// (ch4.s.stage: read off chapter four's saved state, its own field names)
	const US47Chapter* C4 = Phase() == TEXT("ch4") && M->Story ? M->Story->Chapter(TEXT("ch4")) : nullptr;
	const FString Stage = C4 && C4->bActive ? S47J::Str(C4->SaveState(), TEXT("stage")) : FString();
	S.bOpen = Stage == TEXT("to-motel") || Stage == TEXT("room");
	S.bNear = bFoot && (InSaro(TEXT("world.motel.zones[2]")) || Door < 22 || bCourt);
	S.bHere = bFoot && (Door < 45 || bCourt);
	S.Pos = P; S.Clock = Clock();
	return S;
}

// ---------- the closer look (main.ts step) ----------

void US47Phones::LookStep(double Dt)
{
	// the lens narrows while right mouse (or Zoom) is held, on foot
	const bool bRaised = M->FieldCamera && M->FieldCamera->bRaised;
	const bool bCanZoom = M->Mode == EMode::Play && !Modal() && !Cinematic() && !bRaised && !Driving();
	bool bHeld = bForceZoom;
	if (const APlayerController* PC = M->Player ? Cast<APlayerController>(M->Player->GetController()) : nullptr) bHeld |= M->Player->IsActive() && PC->IsInputKeyDown(EKeys::RightMouseButton);
	const double Z0 = M->ZoomK;
	M->ZoomK += ((bHeld && bCanZoom ? 1 : 0) - M->ZoomK) * FMath::Min(1.0, Dt * 9);
	if (M->ZoomK < 0.002) M->ZoomK = 0;
	if (M->Player)
	{
		if (!bRaised && (M->ZoomK > 0 || Z0 > 0)) M->Player->Fov = float((M->Flow ? M->Flow->BaseFov : 70.f) * (1 - 0.55 * M->ZoomK));
		// finer aim through the closer look
		M->Player->LookFine = 1 - 0.6 * M->ZoomK;
	}
}

void US47Phones::ExamineStep()
{
	FString Text;
	const bool bRaised = M->FieldCamera && M->FieldCamera->bRaised;
	if (M->Mode == EMode::Play && !Modal() && !Cinematic() && !bRaised && !Driving())
	{
		// a closer look: what the thing under the crosshair is, once the zoom is most of the way in
		// (and up where the dishes point, nothing to name: what there is to see in the patch with no stars)
		if (M->ZoomK > 0.6 && M->Player && M->Inter)
		{
			FString Id;
			if (!M->Inter->ExamineAt(M->Player->EyeUE(), M->Player->ForwardUE(), Id, Text) && StarHoleExamine) Text = StarHoleExamine();
		}
	}
	if (M->UI) M->UI->Examine(Text);
	if (Text != ExamineLogged)
	{
		ExamineLogged = Text;
		if (!Text.IsEmpty()) { UE_LOG(LogS47Phones, Display, TEXT("S47: closer look: %s"), *Text); }
	}
}

// ---------- step() ----------

void US47Phones::StepNight(double Dt, bool bNightOk)
{
	US47Night* N = M->Night;
	const FS47NightHooks& H = N->Hooks;
	// T. Vega's field book: what it has, seen after it was read; the knocks on the roof it has, on a
	// drive after it, when nothing else of the night's is going on (one thing at a time), and when the
	// night's curve lets an event come (its build from half way, or its peak; never in a rest)
	const bool bFbOk = bNightOk && !(N->NightEvents && N->NightEvents->Busy()) && !Is(H.LineLightsOn) && !Is(H.MotelTvBusy) && !Is(CbBusy) && !(M->Audio && M->Audio->PhonePlaying())
		&& N->FaceQuiet <= 0 && !(M->World && M->World->Trouble.Kind() != ES47Trouble::None) && N->Curve->May(TEXT("event"));
	FieldBook->Update(Dt, bFbOk, Driving(), [N] { return N->NightEvents && N->NightEvents->Now(TEXT("knocks"), N->NightIn(true)); });
	// the steps on the roof are chapter two's peak (the curve's build or peak); the room's last quiet
	// moment, before the call to the key holder, is theirs whatever the curve says
	FS47RoofIn Ri = RoofIn(true);
	if (!N->Curve->May(TEXT("scare")) && !Ri.bLastQuiet) Ri.bOk = false;
	RoofSteps->Update(Dt, Ri);
	// the archive's phone after them (story/archivePhone.ts): the build was the steps
	const FS47ArcIn Ai = ArcIn();
	Archive->Update(Dt, Ai);
	if (Ai.bCalled) Archive->CalledOut(Clock());
	// the log, for the tests and the screenshot runs
	if (Archive->Phase != LoggedArc) { LoggedArc = Archive->Phase; UE_LOG(LogS47Phones, Display, TEXT("S47: archive phase %s (%s, %s)"), *LoggedArc, *Ai.Where, M->Player ? *M->Player->Pos.ToString() : TEXT("")); }
	if (RoofSteps->Phase != LoggedRoof) { LoggedRoof = RoofSteps->Phase; UE_LOG(LogS47Phones, Display, TEXT("S47: roof steps %s"), *LoggedRoof); }
	if (ArrayTip->Mode != LoggedTip) { LoggedTip = ArrayTip->Mode; UE_LOG(LogS47Phones, Display, TEXT("S47: array %s"), *LoggedTip); }
}

void US47Phones::StepLate(double Dt, bool bModal)
{
	// (the diner's cooler and the morning: story/morningHolds.ts, its own port) and the thread
	SameThread->Update(AbductionCount ? AbductionCount() : 0);
	OfficePhone->Update(Dt, OfficeIn(bModal));
	if (OfficePhone->State != LoggedOffice) { LoggedOffice = OfficePhone->State; UE_LOG(LogS47Phones, Display, TEXT("S47: office telephone %s"), *LoggedOffice); }
	// the corner clock in seconds while the archive's 47 counts (main.ts: ui.clockSeconds(abduction.warnClock
	// ?? saroDark.clockShow ?? archive.clockShow ?? game.ch6.clockShow); the others are their ports')
	if (Archive->ClockShow) { M->UI->ClockSeconds(Archive->ClockShow->Hm + TEXT(":") + Archive->ClockShow->Ss); bClockShown = true; }
	else if (bClockShown) { M->UI->ClockSeconds(FString()); bClockShown = false; }
	SyncBooks();
}

void US47Phones::SyncBooks()
{
	// game.extraBooks: [...fieldBook.books(), ...officePhone.books(), ...archiveBooks(), sameThread.book]
	if (!M || !FieldBook || !OfficePhone || !Archive || !SameThread) return;
	TArray<FS47RuleBook*> Want;
	Want.Append(FieldBook->Books());
	Want.Append(OfficePhone->Books());
	Want.Append(Archive->Books());
	Want.Add(SameThread->Book.Get());
	for (FS47RuleBook* B : Mine) if (!Want.Contains(B)) M->Books.Remove(B);
	for (FS47RuleBook* B : Want) M->Books.AddUnique(B);
	Mine = Want;
}

// ---------- what the night did, and whether it was seen (main.ts seenStep) ----------

void US47Phones::SeenStep()
{
	US47Night* N = M->Night;
	// the steps on the roof
	if (RoofSteps->Busy() != Was.bRoof)
	{
		if (RoofSteps->Busy())
		{
			TWeakPtr<FS47RoofSteps> R = RoofSteps;
			FS47SawOpts O; O.At = RoofSteps->Overhead();
			FS47SeenWatch Wt; Wt.Pos = [R] { const TSharedPtr<FS47RoofSteps> P = R.Pin(); return P && P->Busy() ? TOptional<FVector>(P->Overhead()) : TOptional<FVector>(); }; Wt.Secs = 90; Wt.Deg = 35; Wt.Range = 8;
			O.Watch = Wt;
			N->Saw(TEXT("roof"), TEXT("steps"), O);
		}
		Was.bRoof = RoofSteps->Busy();
	}
	// the crank telephone in the motel office (story/officePhone.ts): written down when it rings, only
	// heard (down the line); how it ended: heard to the end, put down or walked off, or let ring
	if (OfficePhone->State != Was.Office)
	{
		if (Was.Office == TEXT("idle") && OfficePhone->State == TEXT("ringing"))
		{
			const FS47Obj P = M->Web ? M->Web->Obj(TEXT("world.motel.proxies.officePhone")) : FS47Obj();
			FS47SawOpts O; if (P) O.At = P.WorldPosition(); O.bLookingSet = true; O.bNoWatch = true;
			OfficeEntry = N->Saw(TEXT("event"), TEXT("officephone"), O);
		}
		else if (OfficePhone->State == TEXT("idle") && OfficeEntry) { OfficeEntry->End = OfficePhone->Result.IsEmpty() ? FString(TEXT("away")) : OfficePhone->Result; OfficeEntry.Reset(); }
		Was.Office = OfficePhone->State;
	}
	// the archive's phone (story/archivePhone.ts): written when its line opens, only heard; how it began
	// (answered when it rang, or the handset lifted to dial); the array tipped down after it, watched
	// while it is down; reel 047 on the deck, only heard
	if (Archive->Phase != Was.Arc)
	{
		if (Archive->Phase == TEXT("line"))
		{
			FS47SawOpts O; O.At = M->Web ? M->Web->Obj(TEXT("annex.objs.recPhone")).WorldPosition() : FVector::ZeroVector; O.Line = FString(); O.bLookingSet = true; O.bNoWatch = true;
			ArcEntry = N->Saw(TEXT("event"), TEXT("archive"), O);
		}
		else if (Archive->Phase == TEXT("done") && ArcEntry) { ArcEntry->End = Archive->How == TEXT("lift") ? TEXT("lifted") : TEXT("answered"); ArcEntry.Reset(); }
		Was.Arc = Archive->Phase;
	}
	if (ArrayTip->Mode != Was.Tip)
	{
		if (Was.Tip == TEXT("idle") && ArrayTip->Mode == TEXT("tipping"))
		{
			TWeakPtr<FS47ArrayTip> T = ArrayTip;
			FS47SawOpts O; O.At = ARRAY_FRONT; O.Line = FString();
			FS47SeenWatch Wt;
			Wt.Pos = [T] { const TSharedPtr<FS47ArrayTip> P = T.Pin(); return P && P->Holds() ? TOptional<FVector>(ARRAY_FRONT) : TOptional<FVector>(); };
			Wt.Secs = 900; Wt.Range = 420; Wt.Deg = 40;
			Wt.Until = [T] { const TSharedPtr<FS47ArrayTip> P = T.Pin(); return !P || !P->Holds(); };
			O.Watch = Wt;
			N->Saw(TEXT("dishes"), TEXT("archive"), O);
		}
		Was.Tip = ArrayTip->Mode;
	}
	const bool bR47 = Is(Reel047.Playing);
	if (bR47 != Was.bR47)
	{
		if (bR47) { FS47SawOpts O; O.Line = FString(); O.bLookingSet = true; O.bNoWatch = true; R47Entry = N->Saw(TEXT("event"), TEXT("reel047"), O); }
		else if (R47Entry) { R47Entry->End = Is(Reel047.Finished) ? TEXT("heard") : TEXT("away"); R47Entry.Reset(); }
		Was.bR47 = bR47;
	}
}

void US47Phones::SeenSync()
{
	Was.bRoof = RoofSteps->Busy(); Was.Office = OfficePhone->State; Was.Arc = Archive->Phase; Was.Tip = ArrayTip->Mode; Was.bR47 = Is(Reel047.Playing);
	OfficeEntry.Reset(); ArcEntry.Reset(); R47Entry.Reset();
}

// ---------- the saves (main.ts startGame, leavePlay, toTitle, saveBlock) ----------

void US47Phones::Restore()
{
	// (the book's state is chapter three's; until its port keeps it, a night opened starts without it)
	if (!FbGet) FbOwn.Reset();
	RoofSteps->Reset(); OfficePhone->Reset(); SameThread->Reset(AbductionCount ? AbductionCount() : 0);
}

void US47Phones::Restored()
{
	Archive->Restore();   // (after the dishes' own: the archive's drawer, box, strip, paper and array)
	FieldBook->Restore();   // after chapter three's state is back (game.start)
	SyncBooks();
}

void US47Phones::Leave(bool bTitle)
{
	Archive->Halt(); ArrayTip->Reset();
	if (!bTitle) { RoofSteps->Reset(); OfficePhone->Reset(); }
}

FString US47Phones::SaveBlock() const
{
	// (nor from the archive's phone ringing until the dishes are down, nor while reel 047 runs)
	return Archive && Archive->Racing() ? TEXT("Not right now. Let the moment finish first.") : FString();
}

bool US47Phones::OpenDoc(const FS47Doc& Doc)
{
	if (Doc.Id != TEXT("fieldbook") || !FieldBook || !FieldBook->Read()) return false;
	FieldBook->OpenBook();
	return true;
}

// ---------- the printer's paper and the drawer ----------

void US47Phones::Paper(const FString& Kind, double K)
{
	// main.ts paper(kind, k): the control room's printer, the paper feeding out with the solve or blank
	AS47Web* W = M->Web;
	const FS47Obj Pp = W ? W->Obj(TEXT("room.printerPaper")) : FS47Obj();
	if (!Pp) return;
	if (!SolvePaper.Num())
	{
		// the solve's own sheet as the export has it (room.printerPaper's map)
		for (UMaterialInstanceDynamic* D : W->NodeMids(Pp.I))
		{
			FPaperMap P;
			UTexture* T = nullptr;
			D->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Map")), T);
			P.Map = T;
			D->GetVectorParameterValue(FMaterialParameterInfo(TEXT("MapU")), P.U);
			D->GetVectorParameterValue(FMaterialParameterInfo(TEXT("MapV")), P.V);
			D->GetScalarParameterValue(FMaterialParameterInfo(TEXT("UseMap")), P.Use);
			P.bHave = true;
			SolvePaper.Add(P);
		}
	}
	auto Want = [&](bool bBlank)
	{
		if (bBlank == bPaperBlank) return;
		bPaperBlank = bBlank;
		if (bBlank)
		{
			if (!BlankPaper)
			{
				// greenbarPaper([]): fan-fold with its green bands and the tractor holes, nothing printed
				FS47Canvas G(512, 768);
				G.SetFill(0xf1efe6); G.FillRect(0, 0, 512, 768);
				G.SetFill(0x96c8a0, 0.45);
				for (int32 Y = 0; Y < 768; Y += 64) G.FillRect(0, Y, 512, 32);
				G.SetFill(0x000000, 0.45);
				for (int32 Y = 16; Y < 768; Y += 32) { G.BeginPath(); G.Arc(14, Y, 5, 0, 7); G.Fill(); G.BeginPath(); G.Arc(512 - 14, Y, 5, 0, 7); G.Fill(); }
				BlankPaper = S47Image::FromPixels(G.Px, 512, 768);
			}
			Pp.SetMap(BlankPaper);
			return;
		}
		const TArray<UMaterialInstanceDynamic*> Ds = W->NodeMids(Pp.I);
		for (int32 i = 0; i < Ds.Num() && i < SolvePaper.Num(); ++i)
		{
			const FPaperMap& P = SolvePaper[i];
			if (P.Map.IsValid()) Ds[i]->SetTextureParameterValue(TEXT("Map"), P.Map.Get());
			Ds[i]->SetVectorParameterValue(TEXT("MapU"), P.U);
			Ds[i]->SetVectorParameterValue(TEXT("MapV"), P.V);
			Ds[i]->SetScalarParameterValue(TEXT("UseMap"), P.Use);
		}
	};
	if (Kind.IsEmpty()) { Want(false); if (K >= 0) W->SetPrinterPaper(-1.f); return; }
	Want(Kind == TEXT("blank"));
	W->SetPrinterPaper(float(FMath::Max(0.001, K)));
}

void US47Phones::SetDrawerFallback(bool bOpen, bool bNow)
{
	// world/Station01.ts setDrawer: the drawer out (what is in it seen) or in. (Station01.update slides
	// it over 0.45 s; without the station's own port it stands where it is put at once.)
	(void)bNow;
	AS47Web* W = M->Web;
	if (!W) return;
	FS47Obj Dw = W->Obj(TEXT("world.site.objs.drawer")), In = W->Obj(TEXT("world.site.objs.drawerInside"));
	if (!Dw) return;
	const double K = bOpen ? 1 : 0, E = K * K * (3 - 2 * K);
	Dw.SetZ(DrawerZ + 0.27 * E);
	In.SetVisible(K > 0.05);
}

// ---------- the developer menu's, for the tests ----------

void US47Phones::TestFlags()
{
	// -S47Archive=ring|lift|tip: the archive's phone rings now, or its line opens (the player in the
	// records room in chapter two), or the array tips; -S47Roof: the steps on the roof now (in the
	// records room); -S47Office[=answer]: the office's telephone rings (and is answered); -S47FieldBook
	// [=envelope]: the field book open, or the envelope held up to the lamp; -S47Zoom: the closer look
	// held; -S47Same: a time away more, and its thread
	const TCHAR* Cmd = FCommandLine::Get();
	FString Arc, Office, Fb;
	const bool bArc = FParse::Value(Cmd, TEXT("S47Archive="), Arc), bRoof = FParse::Param(Cmd, TEXT("S47Roof"));
	const bool bOffice = FParse::Value(Cmd, TEXT("S47Office="), Office) || FParse::Param(Cmd, TEXT("S47Office"));
	const bool bFb = FParse::Value(Cmd, TEXT("S47FieldBook="), Fb) || FParse::Param(Cmd, TEXT("S47FieldBook"));
	const bool bZoom = FParse::Param(Cmd, TEXT("S47Zoom")), bSame = FParse::Param(Cmd, TEXT("S47Same"));
	if (!(bArc || bRoof || bOffice || bFb || bZoom || bSame) || !M->Web) return;
	TWeakObjectPtr<US47Phones> Self = this;
	FTimerHandle H;
	M->Web->GetWorldTimerManager().SetTimer(H, [Self, bArc, Arc, bRoof, bOffice, Office, bFb, Fb, bZoom, bSame]
	{
		if (!Self.IsValid()) return;
		US47Phones* P = Self.Get();
		if (bZoom) { P->bForceZoom = true; UE_LOG(LogS47Phones, Display, TEXT("S47: the closer look held")); }
		if (bArc)
		{
			if (Arc == TEXT("tip")) { UE_LOG(LogS47Phones, Display, TEXT("S47: the array tips: %s"), P->ArrayTip->Tip() ? TEXT("tipping") : TEXT("something else has the dishes")); }
			else { UE_LOG(LogS47Phones, Display, TEXT("S47: the archive's phone, %s: %s"), *Arc, P->Archive->Now(Arc == TEXT("lift") ? TEXT("lift") : TEXT("ring"), P->ArcIn()) ? TEXT("now") : TEXT("only from the records room in chapter two, once")); }
		}
		if (bRoof) UE_LOG(LogS47Phones, Display, TEXT("S47: steps on the roof: %s"), P->RoofSteps->Now(P->RoofIn(true)) ? TEXT("now") : TEXT("only from the records room, once"));
		if (bOffice)
		{
			const bool bRang = P->OfficePhone->RingNow();
			UE_LOG(LogS47Phones, Display, TEXT("S47: the office telephone rings: %s"), bRang ? TEXT("yes") : TEXT("it is ringing already, or off the hook"));
			if (Office == TEXT("answer") && P->OfficePhone->Use()) UE_LOG(LogS47Phones, Display, TEXT("S47: office line open, %d captions to come"), P->OfficePhone->Plan ? P->OfficePhone->Plan->Caps.Num() : 0);
		}
		if (bFb)
		{
			if (Fb == TEXT("envelope"))
			{
				const S47FieldBookView::FEnvelopeView V = P->FieldBook->UseEnvelope();
				if (V.Hold) V.Hold();
				UE_LOG(LogS47Phones, Display, TEXT("S47: envelope %s"), P->M->TownSeen.Contains(TEXT("env:lamp")) ? TEXT("held up to the lamp") : TEXT("not held"));
			}
			else { P->FieldBook->OpenBook(0); UE_LOG(LogS47Phones, Display, TEXT("S47: field book opened at page 0")); }
		}
		if (bSame)
		{
			const int32 N = (P->AbductionCount ? P->AbductionCount() : 0) + 1;
			P->SameThread->Update(N);
			const FS47SameThread::FThread* T = P->SameThread->Threads.FindByPredicate([N](const FS47SameThread::FThread& X) { return X.N == N; });
			UE_LOG(LogS47Phones, Display, TEXT("S47: same thread for time away %d: %s"), N, T ? *FString::Printf(TEXT("%s to %s"), *T->A, *T->B) : TEXT("none, no photograph from above on the board"));
		}
	}, 3.0f, false);
}

bool US47Phones::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	bOk &= FS47ArrayTip::SelfTest(Lines);
	bOk &= FS47ArchivePhone::SelfTest(Lines);
	bOk &= FS47RoofSteps::SelfTest(Lines);
	bOk &= FS47OfficePhone::SelfTest(Lines);
	bOk &= FS47FieldBook::SelfTest(Lines);
	bOk &= S47FieldBookView::SelfTest(Lines);
	bOk &= S47Looks::SelfTest(Lines);
	bOk &= FS47SameThread::SelfTest(Lines);
	return bOk;
}
