#include "S47Chapter5.h"

#include "Components/MeshComponent.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "S47ArrayBack.h"
#include "S47Audio.h"
#include "S47Away.h"
#include "S47Chapter3.h"
#include "S47Chapter4.h"
#include "S47Chapter6.h"
#include "S47Diner.h"
#include "S47Drawings.h"
#include "S47Flow.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47LostNights.h"
#include "S47Main.h"
#include "S47SaroNight.h"
#include "S47StationTaken.h"
#include "S47Story.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47MapOverlay.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Ch5, Log, All);

namespace
{
	constexpr double DAWN = 5 * 3600; // 05:00, the chapter's start; civil dawn is 05:05, sunrise 05:30
	const TCHAR* SUPPORTED = TEXT("P13 SUPPORTED / With E09A traced and laid on STATION 01, north to north, C runs due east and meets the old Roswell road just past the eight-mile post, at a survey bolt in the shoulder. It fits N. Vega's account: east from the station, across the old highway, a bolt where it crosses.");

	using FLine = S47Panels::FTalkLine;
	FLine Aside(const FString& Text) { return { FString(), Text }; }
	FLine You(const FString& Text) { return { TEXT("YOU"), Text }; }
	FLine Waitress(const FString& Text) { return { TEXT("THE WAITRESS"), Text }; } // W
	FLine Driver(const FString& Text) { return { TEXT("THE DRIVER"), Text }; }     // D
	FLine Ward(const FString& Text) { return { TEXT("E. WARD"), Text }; }          // E

	FS47Sound Snd(float Gain, float Rate = 1.f, const TOptional<FVector>& At = {})
	{
		FS47Sound O;
		O.Gain = Gain; O.Rate = Rate; O.At = At;
		return O;
	}

	// this.lines.slice(-2)
	TArray<FLine> LastTwo(const TArray<FLine>& L)
	{
		TArray<FLine> Out;
		for (int32 i = FMath::Max(0, L.Num() - 2); i < L.Num(); ++i) Out.Add(L[i]);
		return Out;
	}
}

// ---------- the saved state ----------

S47J::FObj FS47Ch5State::Save() const
{
	S47J::FObj O = Extra.IsValid() ? S47J::Clone(Extra) : S47J::New();
	S47J::Set(O, TEXT("v"), 1);
	S47J::Set(O, TEXT("stage"), Stage);
	S47J::Set(O, TEXT("bell"), bBell);
	S47J::Set(O, TEXT("file"), bFile);
	S47J::Set(O, TEXT("arrived"), bArrived);
	S47J::Set(O, TEXT("coffee"), bCoffee);
	S47J::Set(O, TEXT("lights"), bLights);
	S47J::Set(O, TEXT("clipped"), bClipped);
	S47J::Set(O, TEXT("aunt"), bAunt);
	S47J::Set(O, TEXT("radio"), bRadio);
	S47J::Set(O, TEXT("road"), bRoad);
	S47J::Set(O, TEXT("october"), bOctober);
	S47J::Set(O, TEXT("confirmed"), bConfirmed);
	S47J::Set(O, TEXT("ward"), bWard);
	S47J::Set(O, TEXT("regular"), bRegular);
	if (Tracing) S47J::Set(O, TEXT("tracing"), TArray<double>{ Tracing->X, Tracing->Y });
	else S47J::SetNull(O, TEXT("tracing"));
	S47J::Set(O, TEXT("p13"), bP13);
	S47J::Set(O, TEXT("wrong13"), Wrong13);
	if (ArrayTold) S47J::Set(O, TEXT("arrayTold"), *ArrayTold);
	return O;
}

FS47Ch5State FS47Ch5State::Load(const S47J::FObj& Saved)
{
	// { ...fresh(), ...saved }
	FS47Ch5State C;
	if (!Saved.IsValid()) return C;
	C.Extra = S47J::Clone(Saved);
	C.Stage = S47J::Str(Saved, TEXT("stage"), C.Stage);
	C.bBell = S47J::Bool(Saved, TEXT("bell"));
	C.bFile = S47J::Bool(Saved, TEXT("file"));
	C.bArrived = S47J::Bool(Saved, TEXT("arrived"));
	C.bCoffee = S47J::Bool(Saved, TEXT("coffee"));
	C.bLights = S47J::Bool(Saved, TEXT("lights"));
	C.bClipped = S47J::Bool(Saved, TEXT("clipped"));
	C.bAunt = S47J::Bool(Saved, TEXT("aunt"));
	C.bRadio = S47J::Bool(Saved, TEXT("radio"));
	C.bRoad = S47J::Bool(Saved, TEXT("road"));
	C.bOctober = S47J::Bool(Saved, TEXT("october"));
	C.bConfirmed = S47J::Bool(Saved, TEXT("confirmed"));
	C.bWard = S47J::Bool(Saved, TEXT("ward"));
	C.bRegular = S47J::Bool(Saved, TEXT("regular"));
	if (S47J::Has(Saved, TEXT("arrayTold"))) C.ArrayTold = S47J::Bool(Saved, TEXT("arrayTold"));
	C.Tracing = S47J::Uv(Saved, TEXT("tracing"));
	C.bP13 = S47J::Bool(Saved, TEXT("p13"));
	C.Wrong13 = S47J::Int(Saved, TEXT("wrong13"));
	return C;
}

// ---------- registration (the constructor, and main.ts's part) ----------

US47Chapter5* US47Chapter5::Register()
{
	BuildBook();
	if (!M || !Host) return this;
	// (one chapter five at a time: a book left by an earlier start of the game goes)
	M->Books.RemoveAll([](const FS47RuleBook* B) { return B && B->Key() == TEXT("ch5"); });
	M->Books.Add(Book.Get());
	TWeakObjectPtr<US47Chapter5> W = this;
	US47Main* Mn = M;
	if (M->World)
	{
		// main.ts truckChoice: not before chapter one, and the station's truck without its keys stays
		// chapter three's (both come first); then ch3.truckTrip(place) ?? ch4.truckTrip(world.truckAt) ??
		// ch5.truckTrip(place) ?? the player's own drive. Chapter three's and four's choices are behind
		// this one (Prev); neither has a drive while chapter five runs, so asking this one first is the same.
		TFunction<US47World::FChoice(const FString&)> Prev = M->World->TruckChoice;
		M->World->TruckChoice = [W, Prev, Mn](const FString& Place) -> US47World::FChoice
		{
			const US47Chapter3* C3 = Mn->Story ? Cast<US47Chapter3>(Mn->Story->Chapter(TEXT("ch3"))) : nullptr;
			const bool bKeysGone = Place == TEXT("station01") && C3 && C3->StationTaken && C3->StationTaken->bKeysGone;
			if (W.IsValid() && Mn->Story && Mn->Story->At(ES47Phase::Ch1) && !bKeysGone)
			{
				if (const TOptional<FTrip> T = W->TruckTrip(Place)) return { T->Label, T->Use };
			}
			return Prev ? Prev(Place) : US47World::FChoice();
		};
	}
	// world.onDinerTruck = (px) => ch5.bindDinerTruck(px): the box World puts round the truck on the
	// diner's lot (export.js has it from 22 October). An older export has no box: the road's truck
	// itself stands in for it, only while it is on the lot (World moves the box away when it is not).
	if (M->Web)
	{
		const FS47Obj Px = M->Web->Obj(TEXT("world.dinerTruckProxy"));
		if (Px) BindDinerTruck(Px);
		else if (const FS47Obj Truck = M->Web->Obj(TEXT("world.legs.road.drive.truck.group")))
		{
			UE_LOG(LogS47Ch5, Warning, TEXT("S47: the export has no world.dinerTruckProxy; the truck on the diner's lot is used by its own meshes (re-run Tools/webexport/import.sh)"));
			BindDinerTruck(Truck);
			if (FS47Interactable* It = M->Inter ? M->Inter->Get(TEXT("dinerTruck")) : nullptr)
			{
				const TFunction<FString()> Label = It->Label, Examine = It->Examine;
				auto OnLot = [Mn] { return Mn->World && Mn->World->TruckAt == TEXT("diner") && !Mn->World->bDriving; };
				It->Label = [Label, OnLot] { return OnLot() && Label ? Label() : FString(); };
				It->Examine = [Examine, OnLot] { return OnLot() && Examine ? Examine() : FString(); };
			}
		}
		// world.onDinerLoaded = (d) => ch5.bindDiner(d): the diner is in the export from the start, but
		// main.ts's town layer (M->TownLayer->Diner) is made after the chapters: bound on the next frame
		M->Web->GetWorldTimerManager().SetTimerForNextTick([W]
		{
			if (W.IsValid() && W->Diner()) W->BindDiner(*W->Diner());
		});
	}
	// main.ts: ch5.driverMore (story/lostNights.ts) and ch5.arrayCount (story/arrayBack.ts); both are
	// made after the chapters, so they are reached when asked
	FDriverMore Dm;
	auto Lost = [Mn]() -> FS47LostNights* { return Mn->Away && Mn->Away->LostNights ? Mn->Away->LostNights.Get() : nullptr; };
	auto Talk = [](const TArray<FS47LostLine>& L) { TArray<FLine> Out; for (const FS47LostLine& X : L) Out.Add({ X.Key, X.Value }); return Out; };
	Dm.Examine = [Lost] { FS47LostNights* L = Lost(); return L ? L->DriverExamine() : FString(); };
	Dm.Open = [Lost, Talk] { FS47LostNights* L = Lost(); return L ? Talk(L->DriverOpen()) : TArray<FLine>(); };
	Dm.Choices = [Lost](bool bOctober)
	{
		TArray<S47Panels::FTalkChoice> Out;
		if (FS47LostNights* L = Lost()) for (const FS47LostNights::FChoice& C : L->DriverChoices(bOctober)) Out.Add({ C.Id, C.Label });
		return Out;
	};
	Dm.Choose = [Lost, Talk](const FString& Id) -> TOptional<TArray<FLine>>
	{
		FS47LostNights* L = Lost();
		const TOptional<TArray<FS47LostLine>> R = L ? L->DriverChoose(Id) : TOptional<TArray<FS47LostLine>>();
		return R ? TOptional<TArray<FLine>>(Talk(*R)) : TOptional<TArray<FLine>>();
	};
	DriverMore = Dm;
	ArrayCount = [Mn] { return Mn->SaroNight && Mn->SaroNight->ArrayBack ? Mn->SaroNight->ArrayBack->Count() : 0; };
	// Prologue.jump('chapter5')
	Host->Jumps.Add(TEXT("chapter5"), [W] { if (W.IsValid()) W->JumpChapter5(); });
	// main.ts ch5.onEnd = () => game.beginChapter6(): out of the diner and onto the old road, no cut
	// (Prologue.beginChapter6: ch5.active = false, amRadio(null), ch6.begin(); the case was saved in
	// the diner just before, by finish)
	OnEnd = [W]
	{
		if (!W.IsValid() || !W->Host) return;
		if (US47Chapter6* C6 = Cast<US47Chapter6>(W->Host->Chapter(TEXT("ch6")))) C6->BeginChapter6(nullptr);
	};
	return this;
}

void US47Chapter5::BuildBook()
{
	// P13 on the case board. It is found with the maps on a table in the diner, not with threads; the
	// board shows it, tied to the map and to C on the transit record.
	FS47BookSpec O;
	O.Key = TEXT("ch5");
	O.Owns = [](const FString& N) { return N == TEXT("P13"); };
	FS47BookQuestion P13;
	P13.Id = TEXT("P13"); P13.Stamp = TEXT("P13"); P13.Text = TEXT("Where does C cross the old Roswell road?"); P13.Slots = 0; P13.X = 14; P13.Y = 14; P13.W = 300;
	P13.Hints = { TEXT("C is a direction from the transit, not a place on a map."), TEXT("Lay a tracing of E09A on the service map, transit on STATION 01, north to north."), TEXT("Spread E14 on a table in the diner, lay the tracing on STATION 01 and follow C east to the road.") };
	P13.Elsewhere = TEXT("Worked out on a table in the diner, with the service map and a tracing of E09A.");
	P13.Open = [this] { return bStarted && S.bArrived; };
	P13.Done = [this] { return S.bP13 ? FString(SUPPORTED) : FString(); };
	O.Questions = { P13 };
	O.Links = [this]() -> TArray<FString>& { NoLinks.Reset(); return NoLinks; }; // () => []
	O.Wrong = [](const FString&, const FS47BookRule&) -> TOptional<FS47Result> { return {}; };
	O.Save = [this] { Save(); };
	O.Cards = [] { return TArray<FS47BoardCard>(); };
	O.Auto = [this]() -> TArray<TPair<FString, FString>>
	{
		if (!S.bP13) return {};
		return { { TEXT("P13"), TEXT("e14") }, { TEXT("P13"), TEXT("e09a.c") } };
	};
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

void US47Chapter5::BeginDestroy()
{
	if (M && Book) M->Books.Remove(Book.Get());
	Super::BeginDestroy();
}

// ---------- lifecycle ----------

void US47Chapter5::Begin(const S47J::FObj& Saved)
{
	Reset();
	bActive = true; bStarted = true;
	S = Saved.IsValid() ? FS47Ch5State::Load(Saved) : FS47Ch5State();
	ApplyWorld();
	if (!Saved.IsValid())
	{
		Note(FString::Printf(TEXT("%s. Out of room 6. %s C crosses the old road to Roswell; the road leaves the highway at the diner south of here, the one that is open all night."), *Time(), *TruckWhere()));
		Toast(TruckWhere(), 3.4f);
	}
	Save();
	static bool bDriven = false;
	FString Stop;
	if (!bDriven && FParse::Value(FCommandLine::Get(), TEXT("S47Ch5Drive="), Stop)) { bDriven = true; TestDrive(Stop); }
}

void US47Chapter5::Reset()
{
	bActive = false; bStarted = false;
	S = FS47Ch5State(); Lines.Reset(); Who.Reset(); Panel.Reset(); bLeaving = false;
	if (M && M->Audio) M->Audio->AmRadioThree({});
	ApplyWorld();
}

S47J::FObj US47Chapter5::SaveState() const
{
	return bStarted ? S.Save() : nullptr;
}

void US47Chapter5::Save() { if (OnSave) OnSave(); }

FString US47Chapter5::Time() const { return S47Text::ClockText(Host ? Host->Clock() : 0, false); }
FString US47Chapter5::Area() const { return M && M->World ? M->World->Area : FString(TEXT("saro")); }
bool US47Chapter5::InDiner() const { return Area() == TEXT("diner"); }
bool US47Chapter5::Driving() const { return M && M->World && M->World->bDriving; }
FString US47Chapter5::TruckAt() const { return M && M->World ? M->World->TruckAt : FString(); }
FS47Diner* US47Chapter5::Diner() const { return M && M->TownLayer && M->TownLayer->Diner ? M->TownLayer->Diner.Get() : nullptr; }

FString US47Chapter5::TruckWhere() const
{
	const FString At = TruckAt();
	return At == TEXT("motel") ? TEXT("The truck is on the motel's lot.") : At == TEXT("diner") ? TEXT("The truck is out on the highway, where I left it.") : TEXT("The truck is on its pad at SARO, up the highway.");
}

bool US47Chapter5::File(const FS47Doc& Doc)
{
	// file(doc): replaces the one with the same id (false), or adds it (true)
	if (!Host) { if (TestDocs.Contains(Doc.Id)) return false; TestDocs.Add(Doc.Id); return true; }
	const bool bNew = !Host->Docs().ContainsByPredicate([&](const FS47Doc& D) { return D.Id == Doc.Id; });
	Host->AddDoc(Doc);
	return bNew;
}

void US47Chapter5::Note(const FString& Text) { if (Host) Host->Note(Text); else if (!TestNotes.Contains(Text)) TestNotes.Add(Text); }
void US47Chapter5::Toast(const FString& Text, float Secs) const { if (M && M->UI) M->UI->Toast(Text, Secs); }
void US47Chapter5::Milestone() const { if (Host && Host->OnCheckpoint) Host->OnCheckpoint(TEXT("chapter5")); }

// The diner as the state has it (after begin, and when the diner has loaded).
void US47Chapter5::ApplyWorld()
{
	if (FS47Diner* Dn = Diner())
	{
		// the cup is on the counter once she has poured, and stays there when you go
		FS47Obj Cup = Dn->Obj(TEXT("cup"));
		if (Cup) Cup.SetVisible(!bActive || S.bCoffee);
		FS47Obj His = RegularCup();
		if (His) His.SetVisible(bActive && S.bRegular);
		Dn->SetSignLit(true);
	}
	if (!bActive) return;
	if (S.bArrived) File(DocMap());
	if (S.bClipped) File(DocClipping());
	if (S.bP13) File(DocFinding());
}

// ---------- objective and journal ----------

FString US47Chapter5::Objective() const
{
	if (S.Stage == TEXT("to-truck"))
	{
		const FString At = TruckAt();
		return Driving() ? TEXT("MESA DINER // SOUTH ON THE HIGHWAY, ON THE RIGHT AFTER TWO KILOMETRES")
			: At == TEXT("motel") ? TEXT("ALL NIGHT // THE TRUCK IS ON THE MOTEL'S LOT") : At == TEXT("diner") ? TEXT("ALL NIGHT // THE TRUCK IS OUT ON THE HIGHWAY, WHERE I LEFT IT") : TEXT("ALL NIGHT // THE TRUCK IS ON ITS PAD AT SARO");
	}
	if (S.Stage == TEXT("diner"))
	{
		if (!InDiner()) return TEXT("ALL NIGHT // MESA DINER");
		return TEXT("MESA DINER // WHERE DOES C CROSS THE OLD ROAD? SPREAD THE MAPS ON A TABLE (P13)");
	}
	if (S.Stage == TEXT("road")) return TEXT("MESA DINER // C CROSSES JUST PAST THE EIGHT-MILE POST. THE TRUCK IS OUTSIDE");
	if (S.Stage == TEXT("complete")) return TEXT("ROSWELL ROAD // FOLLOW C TO THE BOLT JUST PAST THE EIGHT-MILE POST");
	return FString();
}

TArray<FS47Task> US47Chapter5::Tasks() const
{
	TArray<FS47Task> T = { { TEXT("Take the truck to the diner"), S.bArrived } };
	if (S.bBell || S.bFile) T.Add({ TEXT("The new file on the operations terminal (optional)"), S.bFile });
	if (S.bArrived) T.Append({
		{ TEXT("Coffee (optional)"), S.bCoffee },
		{ TEXT("Ask the waitress about the lights over the mesa"), S.bLights },
		{ TEXT("The clipping on the wall"), S.bClipped },
		{ TEXT("Find where C crosses the old Roswell road (P13)"), S.bP13 },
		{ TEXT("Ask the driver about the old road"), S.bOctober },
		{ TEXT("Call Ward from the payphone (optional)"), S.bWard },
		{ TEXT("Drive the old road"), S.Stage == TEXT("complete") },
	});
	return T;
}

// ---------- SARO: the terminal and the truck ----------

void US47Chapter5::TerminalCommand(const FString& Cmd)
{
	// (/TYPE RUN860414_0529/.test(cmd): the terminal hands it on upper case)
	if (!bActive || S.bFile || !Cmd.Contains(TEXT("TYPE RUN860414_0529"), ESearchCase::CaseSensitive)) return;
	S.bFile = true;
	Note(FString::Printf(TEXT("%s. A new file in SURVEY.RAW on the operations terminal: RUN860414_0529.DAT. 0 blocks, operator Reyes, opened at 05:29. It is %s now. \"File incomplete. Record not closed.\""), *Time(), *Time()));
	Save();
}

TOptional<US47Chapter5::FTrip> US47Chapter5::TruckTrip(const FString& Place)
{
	if (!bActive) return {};
	TWeakObjectPtr<US47Chapter5> W = this;
	if (S.Stage == TEXT("road") && Place == TEXT("diner")) return FTrip{ TEXT("Drive the old road"), [W] { if (W.IsValid()) W->Finish(); } };
	if (S.Stage == TEXT("to-truck") || (S.Stage == TEXT("road") && Place != TEXT("diner")))
	{
		return FTrip{ TEXT("Drive to Mesa Diner"), [W]
		{
			if (W.IsValid() && W->M && W->M->World) W->M->World->DriveToDiner([W] { if (W.IsValid()) W->ArrivedDiner(); });
		} };
	}
	return {};
}

void US47Chapter5::ArrivedDiner()
{
	if (!S.bArrived)
	{
		S.bArrived = true;
		if (S.Stage == TEXT("to-truck")) S.Stage = TEXT("diner");
		File(DocMap());
		Note(FString::Printf(TEXT("%s. Mesa Diner, where the old road to Roswell leaves the highway. Open all night. Grey light in the east. The service map from the truck's door pocket came along (E14)."), *Time()));
		Toast(TEXT("Mesa Diner. The service map from the door pocket is in your jacket."), 4);
		Milestone();
	}
	ApplyWorld();
	Save();
}

// ---------- the diner's things ----------
// Every hit area in the diner has a label whenever the player is there. Before or after the chapter
// they are just a diner; while it runs they carry it.

void US47Chapter5::BindDiner(FS47Diner& Dn)
{
	if (!bDinerBound && M && M->Inter)
	{
		bDinerBound = true;
		US47Inter* Inter = M->Inter;
		TWeakObjectPtr<US47Chapter5> W = this;
		auto Here = [W] { return W.IsValid() && W->InDiner(); };
		auto Add = [&](const TCHAR* Id, TFunction<FString()> Label, TFunction<void()> Use, double Range = 2.4, TFunction<FString()> Examine = nullptr)
		{
			const FS47Obj Px = Dn.Proxy(Id);
			if (!Px) return;
			FS47Interactable It;
			It.Id = FString(TEXT("diner:")) + Id; It.Object = Px; It.Range = Range;
			It.Label = [Here, Label] { return Here() ? Label() : FString(); };
			It.Use = MoveTemp(Use);
			if (Examine) It.Examine = [Here, Examine] { return Here() ? Examine() : FString(); };
			Inter->Add(It);
		};
		auto On = [W] { return W.IsValid() && W->bActive && (W->S.Stage == TEXT("diner") || W->S.Stage == TEXT("road")); };
		auto Str = [](const TCHAR* S) { return [S] { return FString(S); }; };
		auto Say = [W](const TCHAR* Text, float Secs) { return [W, Text, Secs] { if (W.IsValid()) W->Toast(Text, Secs); }; };
		Add(TEXT("waitress"), [On] { return FString(On() ? TEXT("Talk to the waitress") : TEXT("The waitress")); },
			[W, On] { if (!W.IsValid()) return; if (On()) W->OpenTalk(TEXT("waitress")); else W->Toast(TEXT("She is wiping the same stretch of counter."), 2.8f); }, 2.8);
		Add(TEXT("driver"), [On] { return FString(On() ? TEXT("Talk to the driver") : TEXT("A driver at the counter")); },
			[W, On] { if (!W.IsValid()) return; if (On()) W->OpenTalk(TEXT("driver")); else W->Toast(TEXT("He is eating eggs and not looking up."), 2.8f); }, 2.6,
			[W] { return W.IsValid() && W->DriverMore && W->DriverMore->Examine ? W->DriverMore->Examine() : FString(); });
		Add(TEXT("coffee"), [W, On] { return FString(On() && !W->S.bCoffee ? TEXT("Coffee") : TEXT("The coffee pot")); }, [W] { if (W.IsValid()) W->UseCoffee(); });
		Add(TEXT("counter"), Str(TEXT("The radio behind the counter")), [W] { if (W.IsValid()) W->UseRadio(); });
		Add(TEXT("clipping"), Str(TEXT("The clipping by the door")), [W] { if (W.IsValid()) W->ReadClipping(); });
		Add(TEXT("payphone"), [W, On] { return FString(On() && !W->S.bWard ? TEXT("Payphone") : TEXT("Payphone")); }, [W] { if (W.IsValid()) W->UsePayphone(); });
		Add(TEXT("booth"), [W, On] { return FString(On() ? (W->S.bP13 ? TEXT("The maps on the table") : TEXT("Spread the maps on the table")) : TEXT("A booth by the window")); },
			[W, On] { if (!W.IsValid()) return; if (On()) W->OpenMap(); else W->Toast(TEXT("A booth by the window. The vinyl is cold."), 2.6f); });
		Add(TEXT("menu"), Str(TEXT("Menu board")), Say(TEXT("Coffee .35. Two eggs any style 1.95. Green chile stew 2.95. Pie .95, under a glass dome on the counter."), 4.2f));
		Add(TEXT("jukebox"), Str(TEXT("Jukebox")), Say(TEXT("A card in the glass, in ballpoint: OUT OF ORDER. It has been there a while."), 3.2f));
		Add(TEXT("window"), Str(TEXT("Window")), [W]
		{
			if (W.IsValid()) W->Toast(W->bActive && W->S.bArrived ? TEXT("Grey behind the mesa to the east. The truck on the lot, and the old road going off past it.") : TEXT("The lot, the highway, the dark."), 3.6f);
		});
		Add(TEXT("door"), Str(TEXT("Door")), Say(TEXT("The door sticks in the cold and lets it in."), 2.6f));
		Add(TEXT("sign"), Str(TEXT("Sign")), Say(TEXT("MESA DINER / OPEN ALL NIGHT. One letter of the neon buzzes."), 3));
		Add(TEXT("rig"), Str(TEXT("Rig")), Say(TEXT("A tractor and a refrigerated trailer, the unit running. The driver's."), 3), 4.5);
	}
	ApplyWorld();
}

void US47Chapter5::BindDinerTruck(const FS47Obj& Object)
{
	if (!M || !M->Inter || !Object) return;
	TWeakObjectPtr<US47Chapter5> W = this;
	FS47Interactable It;
	It.Id = TEXT("dinerTruck"); It.Object = Object; It.Range = 3.2;
	It.Label = [W]
	{
		if (!W.IsValid() || !W->InDiner() || !W->M->World || !W->M->World->TruckChoice) return FString();
		return W->M->World->TruckChoice(TEXT("diner")).Label;
	};
	It.Use = [W]
	{
		if (!W.IsValid() || !W->M->World || !W->M->World->TruckChoice) return;
		const US47World::FChoice C = W->M->World->TruckChoice(TEXT("diner"));
		if (C.Use) C.Use();
	};
	It.Examine = [W] { return W.IsValid() && W->InDiner() ? FString(TEXT("SARO 07: a white pickup with tired paint and a toolbox in the bed. The keys are in it, like always.")) : FString(); };
	M->Inter->Add(It);
}

void US47Chapter5::UseCoffee()
{
	if (!bActive || !S.bArrived) { Toast(TEXT("Fresh, or near enough."), 2.4f); return; }
	if (S.bCoffee) { Toast(TEXT("Hot, and strong enough to stand a spoon in."), 2.6f); return; }
	Pour();
	Toast(TEXT("She pours without asking how you take it. \"Thirty-five cents. Pay when you go.\""), 4);
	if (M && M->Audio) M->Audio->Sfx(TEXT("thudSoft"), Snd(0.12f, 2.2f));
}

// The regular's cup, at the last stool along the counter (a copy of the player's). The web version
// clones the cup the first time it is asked for, with the diner; the export has that copy already
// (world.diner.objs.regularCup, at z -4.8 under the cup's parent).
FS47Obj US47Chapter5::RegularCup() const
{
	FS47Diner* Dn = Diner();
	const FS47Obj Cup = Dn ? Dn->Obj(TEXT("cup")) : FS47Obj();
	if (!Dn || !Cup || !Cup.Parent()) return FS47Obj();
	return Dn->Obj(TEXT("regularCup"));
}

void US47Chapter5::Pour()
{
	S.bCoffee = true;
	FS47Diner* Dn = Diner();
	FS47Obj Cup = Dn ? Dn->Obj(TEXT("cup")) : FS47Obj();
	if (Cup) Cup.SetVisible(true);
	// the pour into the cup, heard only with the bought sounds (Audio.ts)
	if (Cup && M && M->Audio) M->Audio->Sfx(M->Audio->Or(TEXT("pour")), Snd(0.33f, 1.f, Cup.WorldPosition()));
	Note(FString::Printf(TEXT("%s. Coffee at the counter. Thirty-five cents."), *Time()));
	Save();
}

// The radio is on low behind the counter (core/radio.ts: Roswell's morning programme, main.ts plays
// it); turned up, the early news is the real news of the morning of 14 April 1986.
void US47Chapter5::UseRadio()
{
	if (bRadioBusy) return;
	bRadioBusy = true;
	FString Now = Time();
	if (Now.StartsWith(TEXT("0"))) Now.RightChopInline(1); // time().replace(/^0/, '')
	const TArray<FString> Parts = {
		TEXT("RADIO: \"...clearing after last night's dry storms west of the Magdalenas. Cold this morning, high around seventy.\""),
		TEXT("RADIO: \"Jack Nicklaus won the Masters yesterday. Forty-six years old, thirty on the back nine. Nobody saw it coming.\""),
		TEXT("RADIO: \"And if you are up at this hour, Halley's comet is low in the south around midnight this week. Get away from town lights.\""),
		FString::Printf(TEXT("RADIO: \"It is %s.\""), *Now),
	};
	TWeakObjectPtr<US47Chapter5> W = this;
	if (Host)
	{
		for (int32 i = 0; i < Parts.Num(); ++i) { const FString P = Parts[i]; Host->After(i * 4.4, [W, P] { if (W.IsValid()) W->Toast(P, 4.2f); }); }
		Host->After(Parts.Num() * 4.4, [W] { if (W.IsValid()) W->bRadioBusy = false; });
	}
	if (bActive && !S.bRadio)
	{
		S.bRadio = true;
		Note(FString::Printf(TEXT("%s. The radio behind the counter: the weather after last night's dry storms, Nicklaus won the Masters at forty-six, Halley's comet low in the south around midnight this week."), *Time()));
		Save();
	}
}

void US47Chapter5::ReadClipping()
{
	const FS47Doc Doc = DocClipping();
	if (bActive && !S.bClipped)
	{
		S.bClipped = true;
		File(Doc);
		Note(FString::Printf(TEXT("%s. The clipping by the diner door (E15): the Pecos Valley Sentinel, July 1947. \"LIGHT HELD OVER MESA FOR AN HOUR.\" The mesa, a steady light high over it, two ranchers at a fence. Low to the right of the mesa, a smudge with a short tail."), *Time()));
		Save();
	}
	if (M && M->UI) M->UI->Document(Doc, [] {});
}

void US47Chapter5::UsePayphone()
{
	if (!bActive || !S.bArrived) { Toast(TEXT("A payphone in a niche. A dime for a local call."), 2.8f); return; }
	if (S.bWard) { Toast(TEXT("She said what she had to say. Six o'clock."), 2.8f); return; }
	OpenTalk(TEXT("ward"));
}

// ---------- conversations ----------

void US47Chapter5::OpenTalk(const FString& InWho)
{
	Who = InWho;
	if (Who == TEXT("waitress"))
	{
		Lines = { Waitress(S.bCoffee ? TEXT("More? No? Suit yourself.") : TEXT("Morning. Or whatever this is.")) };
		if (!S.bCoffee) Lines.Add(Waitress(TEXT("Coffee?")));
	}
	// event 11: once she has told you about the lights, she pours for the regular
	if (Who == TEXT("waitress") && S.bLights && S.bCoffee && !S.bRegular)
	{
		S.bRegular = true;
		Lines = {
			Aside(TEXT("She fills a second cup and sets it down at the last stool along the counter. The stool is empty.")),
			Waitress(TEXT("He's a regular. Comes in when the lights are up.")),
			Aside(TEXT("The cup steams. Nobody comes to drink it.")),
		};
		FS47Obj His = RegularCup();
		if (His) His.SetVisible(true);
		Note(FString::Printf(TEXT("%s. Coffee poured for an empty stool at the end of the counter. \"He's a regular. Comes in when the lights are up.\""), *Time()));
		Save();
	}
	if (Who == TEXT("driver"))
	{
		Lines = { Driver(TEXT("Morning.")), Aside(TEXT("He does not look up from his eggs.")) };
		if (DriverMore && DriverMore->Open) Lines.Append(DriverMore->Open());
	}
	if (Who == TEXT("ward"))
	{
		Lines = {
			Aside(TEXT("A dime in the slot, and her number from the card in your wallet.")),
			Ward(TEXT("Ward.")),
			You(TEXT("It is Reyes. I am at the diner on the old road.")),
			Ward(TEXT("I read the telex. All of it.")),
			Ward(TEXT("Hondo called. So did Site 11.")),
		};
		if (M && M->Audio) M->Audio->Sfx(TEXT("click"), Snd(0.4f));
	}
	if (!M || !M->UI) return;
	TWeakObjectPtr<US47Chapter5> W = this;
	const FString Talker = Who;
	TSharedRef<TArray<FLine>> Spoken = MakeShared<TArray<FLine>>();
	S47Panels::FTalk O;
	O.Title = Who == TEXT("ward") ? TEXT("PAYPHONE / E. WARD") : Who == TEXT("waitress") ? TEXT("MESA DINER / THE WAITRESS") : TEXT("MESA DINER / THE DRIVER");
	O.View = [W] { return W.IsValid() ? W->View() : S47Panels::FTalkView(); };
	O.OnChip = [](const FString&) {};
	O.OnTable = [] {};
	O.OnChoice = [W](const FString& Id) { if (W.IsValid()) W->Choose(Id); };
	O.OnRead = [](const FString&) {};
	O.OnLines = [W, Talker, Spoken](const TArray<FLine>& L)
	{
		if (Talker != TEXT("ward") || !W.IsValid()) return;
		// The panel retains the last two lines when another question is asked.
		// Read only the newly appended speech, never the retained history or stage directions.
		int32 Overlap = FMath::Min(Spoken->Num(), L.Num());
		auto Same = [&](int32 K)
		{
			for (int32 i = 0; i < K; ++i)
			{
				const FLine& A = (*Spoken)[Spoken->Num() - K + i];
				if (A.Who != L[i].Who || A.Text != L[i].Text) return false;
			}
			return true;
		};
		while (Overlap && !Same(Overlap)) Overlap--;
		TArray<FS47PhoneCue> Fresh;
		for (int32 i = Overlap; i < L.Num(); ++i) if (!L[i].Who.IsEmpty()) { FS47PhoneCue C; C.By = L[i].Who; C.Text = L[i].Text; Fresh.Add(C); }
		*Spoken = L;
		if (US47Audio* A = W->M ? W->M->Audio.Get() : nullptr)
		{
			A->StopPhoneCalls();
			if (Fresh.Num()) A->PhoneCall(Fresh);
		}
	};
	O.OnClose = [W]
	{
		if (!W.IsValid()) return;
		if (W->M && W->M->Audio) W->M->Audio->StopPhoneCalls();
		W->Panel.Reset(); W->Who.Reset();
	};
	Panel = S47Panels::Talk(M->UI, O);
}

S47Panels::FTalkView US47Chapter5::View() const
{
	S47Panels::FTalkView V;
	V.Status = Objective(); V.Lines = Lines;
	if (Who == TEXT("waitress"))
	{
		if (!S.bCoffee) V.Choices.Add({ TEXT("coffee"), TEXT("Coffee, please") });
		if (!S.bLights) V.Choices.Add({ TEXT("lights"), TEXT("Ask about the lights over the mesa") });
		if (!S.bAunt) V.Choices.Add({ TEXT("clipping"), TEXT("Ask about the clipping by the door") });
		V.Choices.Add({ TEXT("bye"), TEXT("Thank her") });
	}
	if (Who == TEXT("driver"))
	{
		if (!S.bRoad) V.Choices.Add({ TEXT("road"), TEXT("Ask about the old road") });
		if (!S.bOctober) V.Choices.Add({ TEXT("october"), TEXT("Ask if anything ever happens out there") });
		if (S.bP13 && !S.bConfirmed) V.Choices.Add({ TEXT("bolt"), TEXT("Ask about the survey bolt past the eight-mile post") });
		if (DriverMore && DriverMore->Choices) V.Choices.Append(DriverMore->Choices(S.bOctober));
		V.Choices.Add({ TEXT("bye"), TEXT("Let him eat") });
	}
	if (Who == TEXT("ward") && !S.bWard)
	{
		V.Choices.Add({ TEXT("what"), TEXT("Ask what they said") });
		// only when it was tried (the console log is hers to read): the count, not a word before
		if (!S.ArrayTold.Get(false) && (ArrayCount ? ArrayCount() : 0) > 0) V.Choices.Add({ TEXT("array"), TEXT("Tell her the array would not stay on the schedule") });
		V.Choices.Add({ TEXT("road"), TEXT("Tell her you are checking the old road first") });
	}
	if (Who == TEXT("ward") && S.bWard) V.Choices.Add({ TEXT("bye"), TEXT("Hang up") });
	return V;
}

void US47Chapter5::Choose(const FString& Id)
{
	if (Id == TEXT("bye")) { if (M && M->UI) M->UI->Close(); return; }
	if (Who == TEXT("waitress"))
	{
		if (Id == TEXT("coffee")) { Lines = { You(TEXT("Coffee, please.")), Aside(TEXT("She pours without asking how you take it.")), Waitress(TEXT("Thirty-five cents. Pay when you go.")) }; Pour(); }
		if (Id == TEXT("lights"))
		{
			S.bLights = true;
			Lines = {
				You(TEXT("The lights over the mesa. You have seen them?")),
				Waitress(TEXT("You are from the observatory. You people never ask about anything else.")),
				Waitress(TEXT("First time I was eleven. Over the mesa, steady, no sound. An hour, maybe.")),
				Waitress(TEXT("My daddy said it was the Army. The Army said it was weather.")),
				Waitress(TEXT("Every few years since. Last October was the last time.")),
				Waitress(TEXT("And before you ask: no, I do not think it is little green men. I think it is the mesa.")),
			};
			Note(FString::Printf(TEXT("%s. The waitress has seen the light over the mesa since she was eleven: steady, no sound, an hour. Every few years. Last October was the last time. \"I do not think it is little green men. I think it is the mesa.\""), *Time()));
			Milestone();
		}
		if (Id == TEXT("clipping"))
		{
			S.bAunt = true;
			Lines = {
				You(TEXT("The clipping by the door.")),
				Waitress(TEXT("My aunt cut that out of the Sentinel. The Kesslers had the ranch on the old road then.")),
				Waitress(TEXT("The paper ran it bigger. She trimmed it to fit the frame.")),
			};
			Note(TEXT("The waitress: her aunt cut the clipping out of the Sentinel. The paper ran the picture bigger; it was trimmed to fit the frame."));
		}
	}
	if (Who == TEXT("driver"))
	{
		const TOptional<TArray<FLine>> More = DriverMore && DriverMore->Choose ? DriverMore->Choose(Id) : TOptional<TArray<FLine>>();
		if (More) { Lines = *More; return; }
		if (Id == TEXT("road"))
		{
			S.bRoad = true;
			Lines = {
				You(TEXT("You take the old road to Roswell?")),
				Driver(TEXT("Nobody takes it since they put the new one through. I take it. Saves me eleven miles.")),
				Driver(TEXT("Nothing out there but fence and the mesa.")),
			};
		}
		if (Id == TEXT("october"))
		{
			S.bOctober = true;
			Lines = {
				You(TEXT("Anything ever happen out there?")),
				Driver(TEXT("Last October she quit on me out there. Right past the eight-mile post.")),
				Driver(TEXT("Lights, radio, engine, all at once. Coasted to a stop. Then it all came back by itself.")),
				Driver(TEXT("I figured battery. Put a new one in the next day. Never did it again.")),
			};
			Note(FString::Printf(TEXT("%s. The driver: last October his rig died on the old road, right past the eight-mile post. Lights, radio and engine at once, then all of it came back. He put in a new battery. It has not happened since."), *Time()));
			Milestone();
		}
		if (Id == TEXT("bolt"))
		{
			S.bConfirmed = true;
			Lines = {
				You(TEXT("There is a survey bolt in the shoulder just past the eight-mile post.")),
				Driver(TEXT("The brass thing? Sure.")),
				Driver(TEXT("That is right where she quit on me. I remember, because I sat there looking at it.")),
			};
			Note(FString::Printf(TEXT("%s. The driver knows the bolt past the eight-mile post. That is where his rig died last October."), *Time()));
			Milestone();
		}
	}
	if (Who == TEXT("ward"))
	{
		if (Id == TEXT("what")) { TArray<FLine> L = LastTwo(Lines); L.Append({ You(TEXT("What did they say?")), Ward(TEXT("Enough. I am not doing this on a pay phone.")) }); Lines = L; }
		// (new lines: no recording, so text on the panel only, as with every reply that has none)
		if (Id == TEXT("array") && !S.ArrayTold.Get(false))
		{
			const int32 N = ArrayCount ? ArrayCount() : 0;
			const FString K = S47ArrayBack::Times(N);
			S.ArrayTold = true;
			TArray<FLine> L = LastTwo(Lines);
			L.Append({ You(TEXT("I put the array back on the schedule. It would not stay there.")),
				Ward(N == 1 ? FString(TEXT("You tried it once. It went back once.")) : FString::Printf(TEXT("You tried it %s. It went back %s."), *K, *K)), Aside(TEXT("A pause on the line.")), Ward(TEXT("Leave it where it is.")) });
			Lines = L;
			Note(FString::Printf(TEXT("%s. Ward already had it from the console log: \"You tried it %s. It went back %s.\" Then: \"Leave it where it is.\""), *Time(), *K, *K));
		}
		if (Id == TEXT("road"))
		{
			S.bWard = true;
			Lines = {
				You(TEXT("I am checking the old road first.")),
				Ward(TEXT("Then check it, and come back. I want you on the console before the morning series.")),
				Ward(TEXT("Six o'clock, Reyes.")),
				Aside(TEXT("She hangs up.")),
			};
			Note(FString::Printf(TEXT("%s. Ward, from the payphone: \"Hondo called. So did Site 11.\" She wants me back on the console before the morning series at six."), *Time()));
			if (M && M->Audio) M->Audio->Sfx(TEXT("click"), Snd(0.4f));
			Milestone();
		}
	}
	Save();
}

// ---------- P13: the maps on the booth table ----------

void US47Chapter5::OpenMap()
{
	if (!S.Tracing) S.Tracing = FVector2D(300, 690);
	if (!M || !M->UI) return;
	TWeakObjectPtr<US47Chapter5> W = this;
	S47MapOverlay::FOpts O;
	O.At = *S.Tracing;
	O.Solved = S.bP13 ? SUPPORTED : TEXT("");
	O.Question = TEXT("P13 / Where does C cross the old Roswell road?");
	O.Choices = {
		{ TEXT("bm8"), TEXT("At the survey bolt just past the eight-mile post") },
		{ TEXT("bm6"), TEXT("At the bolt by the six-mile post") },
		{ TEXT("bm10"), TEXT("At the bolt past the ten-mile post") },
		{ TEXT("none"), TEXT("Nowhere. C runs clear of the road") },
	};
	O.OnMove = [W](double X, double Y, bool) { if (!W.IsValid()) return; W->S.Tracing = FVector2D(X, Y); W->Save(); };
	O.OnChoose = [W](const FString& Id) { return W.IsValid() ? W->Answer(Id) : FS47Result(); };
	S47MapOverlay::MapOverlay(M->UI, O);
}

bool US47Chapter5::Aligned() const
{
	return S.Tracing && FVector2D::Distance(*S.Tracing, S47Drawings::SERVICE_MAP().Station) < 0.5;
}

FS47Result US47Chapter5::Answer(const FString& Id)
{
	// Unreal: record only an investigation actually performed on the map, including a correct guess.
	if (!Aligned()) return { false, TEXT("Lay the tracing on STATION 01 first, transit on the station, north to north. Then follow C.") };
	if (Id != TEXT("bm8"))
	{
		S.Wrong13++;
		Save();
		return { false, Id == TEXT("none") ? TEXT("Follow C east from the station. It does meet the road.") : Id == TEXT("bm6") ? TEXT("That bolt is south of the line. C meets the road further on.") : TEXT("C meets the road before that.") };
	}
	S.bP13 = true;
	if (S.Stage == TEXT("diner")) S.Stage = TEXT("road");
	File(DocFinding());
	Note(FString::Printf(TEXT("%s. P13: with E09A on STATION 01, C runs due east and crosses the old Roswell road just past the eight-mile post, at a survey bolt in the shoulder. The truck is outside."), *Time()));
	if (M && M->Audio) M->Audio->Sfx(TEXT("click"), Snd(0.5f));
	Milestone();
	Save();
	return { true, SUPPORTED };
}

// ---------- out ----------
// Out to the truck: the case is saved here first, with the truck ready, so the end of the night can
// be played again from the diner. Then chapter six takes the road.

void US47Chapter5::Finish()
{
	if (S.Stage != TEXT("road") || bLeaving) return;
	bLeaving = true;
	// the autosave is written after this frame; the state must still say 'road' then
	Milestone();
	Save();
	TWeakObjectPtr<US47Chapter5> W = this;
	if (Host) Host->After(0.4, [W]
	{
		if (!W.IsValid()) return;
		W->bLeaving = false;
		W->S.Stage = TEXT("complete");
		W->Note(FString::Printf(TEXT("%s. Out to the truck. %s The old road goes off east into the grey."), *W->Time(),
			W->S.bCoffee ? TEXT("The coffee stays on the counter, half drunk.") : TEXT("The waitress is wiping the counter.")));
		if (W->OnEnd) W->OnEnd();
	});
}

// ---------- documents ----------

FS47Doc US47Chapter5::DocMap()
{
	if (MapUrl.IsEmpty()) MapUrl = S47Drawings::ServiceMap();
	FS47Doc D;
	D.Id = TEXT("e14"); D.Title = TEXT("E14 / SARO 07 service map"); D.Kind = TEXT("map"); D.Stamp = TEXT("E14"); D.Page = TEXT(""); D.Image = MapUrl;
	D.Transcript = TEXT("From the door pocket of the service truck. The old Roswell road leaves the highway at the diner, and its mileposts count from there. Survey bolts in the shoulder are marked BM.\n\nC is not on it. E09A gives C as a direction from the transit, not a place.");
	return D;
}

FS47Doc US47Chapter5::DocClipping()
{
	if (ClippingUrl.IsEmpty())
	{
		// the clipping as it hangs on the wall (world/Diner.ts draws it on a canvas; the export has
		// that canvas as the texture of the frame's paper, the one of its pictures 1000 or more high)
		UTexture* C = nullptr;
		if (FS47Diner* Dn = Diner())
		{
			Dn->Obj(TEXT("clipping")).Traverse([&C](FS47Obj O)
			{
				const FS47WebNode* N = O.W ? O.W->Node(O.I) : nullptr;
				const UMeshComponent* Mc = N ? Cast<UMeshComponent>(N->Comp.Get()) : nullptr;
				if (!Mc) return;
				for (int32 k = 0; k < Mc->GetNumMaterials(); ++k)
				{
					UTexture* T = nullptr;
					const UMaterialInterface* Mi = Mc->GetMaterial(k);
					if (Mi && Mi->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Map")), T) && T && T->GetSurfaceHeight() >= 1000) C = T;
				}
			});
		}
		if (C) ClippingUrl = S47Drawings::Clipping(C);
	}
	FS47Doc D;
	D.Id = TEXT("e15"); D.Title = TEXT("E15 / The Sentinel clipping"); D.Kind = TEXT("photo"); D.Page = TEXT(""); D.Image = ClippingUrl; D.Stamp = TEXT("E15");
	D.Transcript = TEXT("The Pecos Valley Sentinel, July 1947, cut out and framed by the diner door. LIGHT HELD OVER MESA FOR AN HOUR. Ranchers on the old survey road watched a steady glow; the Army field office cites weather equipment.\n\nThe photograph: a flat mesa, a steady light high over it, two ranchers at a fence. Low to the right of the mesa, a smudge with a short tail. The right-hand edge of the picture is trimmed close.");
	return D;
}

FS47Doc US47Chapter5::DocFinding() const
{
	FS47Doc D;
	D.Id = TEXT("p13"); D.Title = TEXT("P13 / Where C crosses"); D.Kind = TEXT("typed"); D.Stamp = TEXT("P13"); D.Page = SUPPORTED;
	D.Transcript = TEXT("The service map from the truck with E09A traced over it. C crosses the old road just past the eight-mile post.");
	return D;
}

// ---------- per frame ----------

void US47Chapter5::Update(double Dt, double T)
{
	(void)Dt; (void)T;
	if (!bActive || !M) return;
	// the terminal chimes once, the first time the player comes past the control room
	if (S.Stage == TEXT("to-truck") && !S.bBell && Area() == TEXT("saro") && M->Player && M->Web
		&& FVector::Distance(M->Player->Pos, M->Web->Obj(TEXT("room.objs.opsTerminal")).WorldPosition()) < 22)
	{
		S.bBell = true;
		if (M->Audio) { M->Audio->Beep(880, 0.12f, 0.08f); M->Audio->Beep(880, 0.12f, 0.08f, 0.22f); }
		Toast(TEXT("Inside, the operations terminal chimes once."), 3);
		Save();
	}
	// the grey comes up in the east while the player is in the diner
	FS47Diner* Dn = Diner();
	if (Dn && InDiner() && Host)
	{
		Dn->SetDawn(FMath::Clamp((FMath::Fmod(Host->Clock(), 86400.0) - (Host->bEveningTimeline ? 4 * 3600 + 40 * 60 : DAWN)) / (Host->bEveningTimeline ? 50 * 60 : 25 * 60), Host->bEveningTimeline ? 0.0 : 0.15, 0.85));
	}
	if (M->UI) M->UI->Objective(Objective());
}

// ---------- the host (Prologue.ts) ----------

void US47Chapter5::BeginChapter5(const S47J::FObj& SavedCase)
{
	// Prologue.beginChapter5: out of room 6 at five, back to the truck, and out to the diner on the old
	// road. (ch4.active = false: she has said what she had to say; room 6 stays as it is. The host's
	// BeginChapter makes the other chapters stop leading; setPhase('ch5') is the host's chapter key.)
	if (!Host) return;
	Host->SetCinematic(false);
	Host->ClearTimers();
	const S47J::FObj Ch5 = S47J::Obj(SavedCase, TEXT("ch5"));
	if (Ch5.IsValid())
	{
		Host->NotesMutable() = S47J::Strs(SavedCase, TEXT("notes"));
		Host->SetClock(S47J::Num(SavedCase, TEXT("clock"), Host->Clock()));
	}
	else Host->SetClock(FMath::Max(Host->Clock(), S47Timeline::Chapter5(Host->bEveningTimeline)));
	if (M && M->UI) M->UI->Fade(false);
	Host->BeginChapter(TEXT("ch5"), Ch5); // (ch5.begin, then onCheckpoint('chapter5'))
}

void US47Chapter5::JumpChapter5()
{
	// Prologue.jumpChapter5: chapter five from a checkpoint, or for testing straight after room 6 when
	// there is no saved case (chapter four complete, without photographs).
	if (!Host) return;
	const S47J::FObj Saved = Host->LoadCase ? Host->LoadCase() : S47J::FObj();
	US47Chapter4* C4 = Cast<US47Chapter4>(Host->Chapter(TEXT("ch4")));
	if (C4) C4->JumpChapter4();
	if (S47J::Obj(Saved, TEXT("ch5")).IsValid()) { BeginChapter5(Saved); return; }
	const S47J::FObj J4 = S47J::Read(TEXT("{\"v\":1,\"stage\":\"complete\",\"exitOpen\":true,\"arrived\":true,\"greeted\":true,")
		TEXT("\"placed\":[\"frame01\",\"frame02\",\"e07\"],\"shown\":true,\"p10\":true,\"wrong10\":0,")
		TEXT("\"topics\":{\"record\":true,\"tomas\":true,\"cable\":true},\"tone\":\"mild\",\"p11\":true,\"wrong11\":0,")
		TEXT("\"line\":true,\"p12\":true,\"wrong12\":0,\"told\":true,")
		TEXT("\"read\":{\"card\":true,\"letter\":true,\"correction\":true},\"seen\":{\"photo\":false,\"window\":false}}"));
	if (C4 && C4->S.Stage != TEXT("complete"))
	{
		// beginChapter4({ ...composeCase(ch1.s), ch4, clock: 04:58 })
		S47J::FObj Case = M && M->Flow ? M->Flow->ComposeCase() : S47J::FObj();
		if (!Case.IsValid()) { Case = S47J::New(); S47J::Set(Case, TEXT("notes"), Host->Notes()); }
		S47J::Set(Case, TEXT("ch4"), J4);
		S47J::Set(Case, TEXT("clock"), (Host->bEveningTimeline ? 4 * 3600.0 + 8 * 60 : 4 * 3600.0 + 58 * 60));
		C4->BeginChapter4(Case);
		Host->SetClock((Host->bEveningTimeline ? 4 * 3600.0 + 8 * 60 : 4 * 3600.0 + 58 * 60));
	}
	BeginChapter5(nullptr);
}

// ---------- -S47Ch5Drive ----------

void US47Chapter5::TestDrive(const FString& Stop)
{
	// tools/chapter5.py's moves in the diner, done from here: the truck left on the lot (arrivedDiner),
	// into the diner, and then at the counter (the radio behind it), the waitress (coffee, the lights),
	// the clipping by the door (E15), the maps on the booth table (laid out, on the station, P13), or
	// back out by the truck once C is found. Each stop holds the game there.
	if (!M || !M->Web || !M->Player || !M->World) return;
	TWeakObjectPtr<US47Chapter5> W = this;
	auto Say = [](const FString& T) { UE_LOG(LogS47Ch5, Display, TEXT("S47: ch5 drive: %s"), *T); };
	auto Later = [W](float Secs, TFunction<void()> Fn)
	{
		if (!W.IsValid()) return;
		FTimerHandle H;
		W->M->Web->GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([W, Fn] { if (W.IsValid()) Fn(); }), Secs, false);
	};
	// eyes on a point (chapter5.py face): yaw and pitch from where the player stands
	auto Face = [W](const FVector& P)
	{
		AS47Walker* Pl = W->M->Player;
		const FVector& At = Pl->Pos;
		Pl->Yaw = FMath::Atan2(-(P.X - At.X), -(P.Z - At.Z));
		Pl->Pitch = FMath::Atan2(P.Y - (Pl->FloorY + Pl->Eye), FMath::Sqrt(FMath::Square(P.X - At.X) + FMath::Square(P.Z - At.Z)));
	};
	const bool bTruck = Stop == TEXT("truck");
	Later(0.6f, [W, Say, bTruck]
	{
		// the truck left on the diner's lot: there, and the player at the door
		W->M->World->PlaceTruck(TEXT("diner"));
		W->ArrivedDiner();
		W->M->World->GoDiner(bTruck ? TEXT("outside") : TEXT("stool"));
		Say(FString::Printf(TEXT("arrived: stage %s, docs e14 %d"), *W->S.Stage, W->Host && W->Host->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("e14"); })));
	});
	Later(1.8f, [W, Say, Face, Stop]
	{
		FS47Diner* Dn = W->Diner();
		if (!Dn) { Say(TEXT("no diner")); return; }
		auto Px = [Dn](const TCHAR* K) { return Dn->Proxy(K).WorldPosition(); };
		const FVector O = Dn->Group.WorldPosition();
		auto Label = [W](const FString& Id) { return W->M->Inter ? W->M->Inter->LabelOf(Id) : FString(); };
		if (Stop == TEXT("counter"))
		{
			Face(Px(TEXT("counter")));
			Say(FString::Printf(TEXT("at the counter: area %s, '%s', '%s'"), *W->Area(), *Label(TEXT("diner:counter")), *Label(TEXT("diner:waitress"))));
			return;
		}
		if (Stop == TEXT("waitress"))
		{
			Face(Px(TEXT("waitress")));
			W->OpenTalk(TEXT("waitress"));
			W->Choose(TEXT("coffee")); W->Choose(TEXT("lights"));
			if (W->Panel && W->Panel->Refresh) W->Panel->Refresh();
			Say(FString::Printf(TEXT("waitress: coffee %d, lights %d, lines %d"), W->S.bCoffee, W->S.bLights, W->Lines.Num()));
			return;
		}
		if (Stop == TEXT("clipping"))
		{
			W->M->Player->Place(O.X + 1.0, O.Z + 1.35, 0);
			Face(Px(TEXT("clipping")));
			W->ReadClipping();
			int32 Iw = 0, Ih = 0;
			TArray<FColor> Img;
			const bool bImg = S47Image::DecodePixels(W->ClippingUrl, Img, Iw, Ih);
			Say(FString::Printf(TEXT("clipping: E15 %d, image %d x %d, paper %d %d %d"), W->S.bClipped, Iw, Ih, bImg && Img.Num() ? Img[10 * Iw + 10].R : -1, bImg && Img.Num() ? Img[10 * Iw + 10].G : -1, bImg && Img.Num() ? Img[10 * Iw + 10].B : -1));
			return;
		}
		if (Stop == TEXT("map") || Stop == TEXT("aligned") || Stop == TEXT("p13"))
		{
			W->M->Player->Place(O.X - 1.0, O.Z + 3.0, 0);
			Face(Px(TEXT("booth")));
			if (Stop != TEXT("map")) W->S.Tracing = S47Drawings::SERVICE_MAP().Station;
			FString Reply;
			if (Stop == TEXT("p13")) { Reply = W->Answer(TEXT("bm10")).Text + TEXT(" / "); Reply += W->Answer(TEXT("bm8")).Text.Left(24); }
			W->OpenMap();
			Say(FString::Printf(TEXT("map: label '%s', tracing %.0f %.0f, p13 %d, stage %s %s"), *Label(TEXT("diner:booth")), W->S.Tracing->X, W->S.Tracing->Y, W->S.bP13, *W->S.Stage, *Reply));
			return;
		}
		if (Stop == TEXT("truck") || Stop == TEXT("road"))
		{
			W->S.Tracing = S47Drawings::SERVICE_MAP().Station;
			W->Answer(TEXT("bm8"));
			const FS47Obj Truck = W->M->Web->Obj(TEXT("world.dinerTruckProxy"));
			const FVector At = Truck ? Truck.WorldPosition() : W->M->Web->Obj(TEXT("world.legs.road.drive.truck.group")).WorldPosition() + FVector(0, 0.95, 0);
			// out of the door and straight at it, until it is in reach (2.6 m from its middle)
			const FVector From = W->M->Player->Pos;
			FVector2D Back(From.X - At.X, From.Z - At.Z);
			Back = Back.GetSafeNormal() * 2.6;
			W->M->Player->Place(At.X + Back.X, At.Z + Back.Y, 0);
			Face(At);
			Say(FString::Printf(TEXT("truck: stage %s, '%s'"), *W->S.Stage, *Label(TEXT("dinerTruck"))));
			return;
		}
	});
	// road: the truck used, out onto the old road (chapter six)
	if (Stop == TEXT("road")) Later(3.0f, [W, Say]
	{
		if (W->M->Inter) W->M->Inter->UseId(TEXT("dinerTruck"));
		Say(FString::Printf(TEXT("road: used the truck, stage %s"), *W->S.Stage));
	});
}

// ---------- self test ----------

bool US47Chapter5::SelfTest(TArray<FString>& Out)
{
	bool bOk = S47MapOverlay::SelfTest(Out);
	auto Check = [&](bool bCond, const FString& What) { Out.Add(FString::Printf(TEXT("%s chapter five: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	auto Has = [](const TArray<S47Panels::FTalkChoice>& C, const TCHAR* Id) { return C.ContainsByPredicate([Id](const S47Panels::FTalkChoice& X) { return X.Id == Id; }); };

	// tools/chapter5.py's chapter, without the world (no host, no screen, no sound)
	US47Chapter5* C = NewObject<US47Chapter5>(GetTransientPackage());
	C->BuildBook();
	C->bStarted = true; C->bActive = true;
	Check(C->Objective() == TEXT("ALL NIGHT // THE TRUCK IS ON ITS PAD AT SARO") && C->Tasks().Num() == 1, TEXT("to the truck: on its pad at SARO"));
	C->TerminalCommand(TEXT("DIR SURVEY.RAW"));
	Check(!C->S.bFile, TEXT("the terminal: another command is nothing"));
	C->TerminalCommand(TEXT("TYPE RUN860414_0529.DAT"));
	Check(C->S.bFile && C->TestNotes.Num() == 1 && C->TestNotes[0].Contains(TEXT("opened at 05:29")) && C->Tasks().Num() == 2, TEXT("TYPE RUN860414_0529.DAT: the file opened at 05:29, in the journal"));
	Check(C->TruckTrip(TEXT("saro")) && C->TruckTrip(TEXT("saro"))->Label == TEXT("Drive to Mesa Diner") && C->TruckTrip(TEXT("motel"))->Label == TEXT("Drive to Mesa Diner"), TEXT("the truck, on its pad or the motel's lot: to Mesa Diner"));
	C->ArrivedDiner();
	Check(C->S.bArrived && C->S.Stage == TEXT("diner") && C->TestDocs.Contains(TEXT("e14")) && !C->TruckTrip(TEXT("diner")) && C->Tasks().Num() == 9, TEXT("at the diner: the service map came along (E14)"));
	Check(C->Book->Questions().Num() == 1 && C->Book->Questions()[0].Id == TEXT("P13") && C->Book->Questions()[0].Done.IsEmpty() && C->Book->Threads().Num() == 0, TEXT("P13 on the board, open"));

	// the waitress: coffee, the lights, the clipping; then the regular's cup, once
	C->OpenTalk(TEXT("waitress"));
	S47Panels::FTalkView V = C->View();
	Check(C->Lines.Num() == 2 && C->Lines[1].Text == TEXT("Coffee?") && Has(V.Choices, TEXT("coffee")) && Has(V.Choices, TEXT("lights")) && Has(V.Choices, TEXT("clipping")) && V.Choices.Last().Label == TEXT("Thank her"), TEXT("the waitress offers coffee"));
	C->Choose(TEXT("coffee"));
	C->Choose(TEXT("lights"));
	Check(C->S.bCoffee && C->S.bLights && C->Lines[2].Text.Contains(TEXT("I was eleven")) && C->Lines.Last().Text.Contains(TEXT("little green men")), TEXT("coffee, and the lights over the mesa since she was eleven"));
	C->Choose(TEXT("clipping"));
	Check(C->S.bAunt && C->Lines.Last().Text.Contains(TEXT("trimmed it to fit the frame")) && C->View().Choices.Num() == 1, TEXT("her aunt cut the clipping out"));
	auto Stools = [C] { int32 N = 0; for (const FString& X : C->TestNotes) N += X.Contains(TEXT("empty stool")); return N; };
	C->OpenTalk(TEXT("waitress"));
	Check(C->S.bRegular && C->Lines[1].Text == TEXT("He's a regular. Comes in when the lights are up.") && Stools() == 1, TEXT("back to her: a cup poured at the empty end stool (event 11)"));
	C->OpenTalk(TEXT("waitress"));
	Check(C->Lines.Num() == 1 && C->Lines[0].Text == TEXT("More? No? Suit yourself.") && Stools() == 1, TEXT("only the once"));

	// the driver: the old road and last October; the bolt only once P13 is found
	C->OpenTalk(TEXT("driver"));
	V = C->View();
	Check(Has(V.Choices, TEXT("road")) && Has(V.Choices, TEXT("october")) && !Has(V.Choices, TEXT("bolt")) && V.Choices.Last().Label == TEXT("Let him eat"), TEXT("the driver: no question about the bolt yet"));
	C->Choose(TEXT("road")); C->Choose(TEXT("october"));
	Check(C->S.bRoad && C->S.bOctober && C->Lines[1].Text.Contains(TEXT("eight-mile post")) && C->Lines.Last().Text.Contains(TEXT("battery")), TEXT("his rig died last October right past the eight-mile post"));

	// Ward on the payphone: the count of the array's tries only when it was tried
	C->OpenTalk(TEXT("ward"));
	V = C->View();
	Check(C->Lines.Last().Text == TEXT("Hondo called. So did Site 11.") && !Has(V.Choices, TEXT("array")) && Has(V.Choices, TEXT("what")), TEXT("Ward: \"Hondo called. So did Site 11.\""));
	C->ArrayCount = [] { return 2; };
	Check(Has(C->View().Choices, TEXT("array")), TEXT("the array tried: she can be told"));
	C->Choose(TEXT("what"));
	Check(C->Lines.Num() == 4 && C->Lines[0].Text == TEXT("I read the telex. All of it.") && C->Lines[3].Text == TEXT("Enough. I am not doing this on a pay phone."), TEXT("what they said: the last two lines kept, then hers"));
	C->Choose(TEXT("array"));
	Check(C->S.ArrayTold.Get(false) && C->Lines[3].Text == TEXT("You tried it twice. It went back twice.") && !Has(C->View().Choices, TEXT("array")) && C->TestNotes.Last().Contains(TEXT("\"You tried it twice. It went back twice.\"")), TEXT("the array: she has it from the console log"));
	C->Choose(TEXT("road"));
	V = C->View();
	Check(C->S.bWard && C->Lines[1].Text.Contains(TEXT("morning series")) && V.Choices.Num() == 1 && V.Choices[0].Label == TEXT("Hang up"), TEXT("she wants you back before the morning series"));

	// P13 at the booth
	{
		// Guard regressions own their state; the original serialized-case scenario keeps three real wrong answers.
		US47Chapter5* Guard = NewObject<US47Chapter5>(GetTransientPackage());
		Guard->S.Stage = TEXT("diner");
		const FS47Result Guess = Guard->Answer(TEXT("bm8"));
		Check(!Guess.bOk && Guess.Text.Contains(TEXT("STATION 01 first")) && !Guard->S.bP13 && Guard->S.Stage == TEXT("diner") && Guard->TestNotes.IsEmpty() && Guard->TestDocs.IsEmpty(),
			TEXT("a correct guess before map alignment records no finding, note or progress"));
		const FS47Result Unaligned = Guard->Answer(TEXT("bm6"));
		Check(!Unaligned.bOk && Unaligned.Text.Contains(TEXT("STATION 01 first")) && Guard->S.Wrong13 == 0 && !Guard->S.bP13,
			TEXT("an answer before the tracing is on the station: lay it on STATION 01 first"));
		Guard->MarkAsGarbage();
	}
	C->S.Tracing = FVector2D(470, 360);
	FS47Result R = C->Answer(TEXT("bm6"));
	Check(!R.bOk && R.Text.StartsWith(TEXT("That bolt is south")) && C->S.Wrong13 == 1 && !C->S.bP13, TEXT("the aligned six-mile answer is south of C and records one wrong answer"));
	R = C->Answer(TEXT("bm10"));
	Check(!R.bOk && R.Text == TEXT("C meets the road before that."), TEXT("past the ten-mile post: before that"));
	R = C->Answer(TEXT("none"));
	Check(!R.bOk && R.Text.StartsWith(TEXT("Follow C east")) && C->S.Wrong13 == 3, TEXT("nowhere: follow C east"));
	R = C->Answer(TEXT("bm8"));
	Check(R.bOk && R.Text.StartsWith(TEXT("P13 SUPPORTED")) && C->S.bP13 && C->S.Stage == TEXT("road") && C->TestDocs.Contains(TEXT("p13")), TEXT("P13 recorded: just past the eight-mile post"));
	Check(C->Objective() == TEXT("MESA DINER // C CROSSES JUST PAST THE EIGHT-MILE POST. THE TRUCK IS OUTSIDE"), TEXT("objective: the truck is outside"));
	const TArray<FS47BoardThread> Th = C->Book->Threads();
	Check(C->Book->Questions()[0].Done.StartsWith(TEXT("P13 SUPPORTED")) && Th.Num() == 2 && Th[0].bAuto && Th[1].B == TEXT("e09a.c"), TEXT("P13 on the board, resting on the map and C"));
	C->OpenTalk(TEXT("driver"));
	Check(Has(C->View().Choices, TEXT("bolt")), TEXT("now the driver can be asked about the bolt"));
	C->Choose(TEXT("bolt"));
	Check(C->S.bConfirmed && C->Lines.Last().Text.Contains(TEXT("right where she quit on me")), TEXT("the brass bolt is right where his rig died"));
	Check(C->TruckTrip(TEXT("diner")) && C->TruckTrip(TEXT("diner"))->Label == TEXT("Drive the old road") && C->TruckTrip(TEXT("saro"))->Label == TEXT("Drive to Mesa Diner"), TEXT("the truck on the lot: the old road; left elsewhere: back to the diner"));

	// the saved state, with the TypeScript's field names
	const FString Json = S47J::Write(C->S.Save());
	Check(Json == TEXT("{\"v\":1,\"stage\":\"road\",\"bell\":false,\"file\":true,\"arrived\":true,\"coffee\":true,\"lights\":true,\"clipped\":false,\"aunt\":true,\"radio\":false,\"road\":true,\"october\":true,\"confirmed\":true,\"ward\":true,\"regular\":true,\"tracing\":[470,360],\"p13\":true,\"wrong13\":3,\"arrayTold\":true}"),
		TEXT("Ch5State in the web version's JSON: ") + Json);
	const S47J::FObj Old = S47J::Read(TEXT("{\"v\":1,\"stage\":\"diner\",\"bell\":true,\"file\":false,\"arrived\":true,\"coffee\":true,\"lights\":false,\"clipped\":false,\"aunt\":false,\"radio\":false,\"road\":false,\"october\":false,\"confirmed\":false,\"ward\":false,\"tracing\":null,\"p13\":false,\"wrong13\":0,\"later\":3}"));
	const FS47Ch5State L = FS47Ch5State::Load(Old);
	const FString Again = S47J::Write(L.Save());
	Check(L.Stage == TEXT("diner") && L.bBell && L.bCoffee && !L.bRegular && !L.ArrayTold && !L.Tracing && Again.Contains(TEXT("\"regular\":false")) && Again.Contains(TEXT("\"tracing\":null")) && Again.Contains(TEXT("\"later\":3")) && !Again.Contains(TEXT("arrayTold")),
		TEXT("an older save: regular from fresh, no arrayTold, unknown fields kept"));
	// a new chapter is fresh again
	C->Reset();
	Check(!C->bActive && C->S.Stage == TEXT("to-truck") && !C->TruckTrip(TEXT("saro")) && C->Book->Threads().Num() == 0, TEXT("reset: not active, fresh"));
	C->MarkAsGarbage();
	return bOk;
}
