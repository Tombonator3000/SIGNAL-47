#include "S47Townsfolk.h"
#include "S47EventPlaces.h"
#include "S47Evening.h"
#include "S47Freeform.h"
#include "S47GameStyle.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47RadioTips.h"
#include "S47Script.h"
#include "S47TownDay.h"
#include "S47TownUse.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Townsfolk, Log, All);

namespace S47TownsfolkLocal
{
	// Whoever is gone in the running game (one game at a time).
	TSet<FString> GAway;
	const TCHAR* const Weekdays[] = { TEXT("Monday"), TEXT("Tuesday"), TEXT("Wednesday"), TEXT("Thursday"), TEXT("Friday"), TEXT("Saturday"), TEXT("Sunday") };
	// Freeform's day 1 is Monday, April 28, 1986; a game begun on the old calendar (Sunday, April 13) keeps it. M7 (C): the day of the
	// week comes from the same date the HUD shows (S47Game::GameDate; EDayOfWeek counts from Monday, as the table does).
	FString WeekdayOf(int32 Day, bool bOldCalendar) { return Weekdays[FMath::Clamp(int32(S47Game::GameDate(true, FMath::Max(1, Day), 0, bOldCalendar).GetDayOfWeek()), 0, 6)]; }
	int32 Index(const FString& Who) { return Who == TEXT("attendant") ? 0 : Who == TEXT("waitress") ? 1 : Who == TEXT("deputy") ? 2 : Who == TEXT("souvenir_man") ? 3 : -1; }
	FString Fill(const TCHAR* Text, const FString& A, const FString& B = FString(), const FString& C = FString())
	{
		const FString Up = A.IsEmpty() ? A : A.Left(1).ToUpper() + A.Mid(1);
		return FString(Text).Replace(TEXT("{A}"), *Up, ESearchCase::CaseSensitive).Replace(TEXT("{a}"), *A, ESearchCase::CaseSensitive).Replace(TEXT("{b}"), *B).Replace(TEXT("{c}"), *C);
	}
	FString Cap(const FString& S) { return S.IsEmpty() ? S : S.Left(1).ToUpper() + S.Mid(1); }
	// How the town speaks of each of the four: the waitress is she.
	FString He(const FString& Who) { return Who == TEXT("waitress") ? TEXT("she") : TEXT("he"); }
	// Each line in four voices: the attendant (dry), the waitress (warm, talks), the deputy (careful, times not stories), the souvenir
	// man (sells it). {a} {b} {c} as each says.
	using FFour = const TCHAR* const[4];
	FFour GoneLines = {
		TEXT("{a} hasn't been in since {b}. Truck's still at the house. Nobody's saying anything, which is how you know."),
		TEXT("Have you heard? {A}. Nobody's seen hide nor hair since {b}. I keep looking at the door."),
		TEXT("{a} has been missing since {b}, last seen near {c}. If you were out that night I want to hear it. Times, not stories."),
		TEXT("{A}. Gone since {b}, out by {c}. You think I don't know what that means? I've been telling people for thirty years.") };
	FFour SelfBackLines = {
		TEXT("Don't ask me where I was. I don't know, and I don't want to."),
		TEXT("I'm fine. Everybody keeps asking. I lost a couple of days, is all. Coffee?"),
		TEXT("I was off duty. That's what goes in the file, and that's all I'm saying."),
		TEXT("Where was I? Socorro. Visiting. ...Don't you put that on the radio.") };
	FFour BackLines = {
		TEXT("{a}'s back. Won't say where. Looks like {he} hasn't slept."),
		TEXT("{a} came back {b}. Sat right there and ate like {he} hadn't in a week, and never said a word about it."),
		TEXT("{a} is accounted for as of {b}. That's the file. I'd leave it there."),
		TEXT("{a}'s back, and doesn't remember a thing. They never do. I've got a book on it.") };
	// What is going round about a night's event, by what kind of thing it was (FS47Townsfolk::Talk). Second-hand: a third of the time
	// it is put at the wrong place (Lines).
	FFour RumorLines = {
		TEXT("Fella fueling up this morning said there were lights out by {a} the other night. Paid cash and didn't wait for change."),
		TEXT("The breakfast crowd's all about {a}. Lights, they say. Low, and no sound at all."),
		TEXT("I've had calls about {a}. Nothing on the ground when I got there. I wrote the times down."),
		TEXT("{A}. The other night. I'll have a postcard of it by Friday.") };
	FFour DoorRumorLines = {
		TEXT("Somebody was trying doors out by {a} the other night, they say. Nobody anybody knows. Didn't take a thing."),
		TEXT("They're saying somebody was at the windows out by {a} the other night. Just looking in. I'd have died."),
		TEXT("Call came in about {a}. Somebody at a door in the night, nobody there when I got out there. I wrote the time down."),
		TEXT("{A}. Somebody at the door in the night, and no tracks in the morning. That's how it starts.") };
	FFour FigureRumorLines = {
		TEXT("Trucker said he saw somebody walking out past {a} the other night. No coat, no car. He didn't stop."),
		TEXT("A man was walking out by {a} in the middle of the night, they say. Just walking. Who does that?"),
		TEXT("Had a report of somebody on foot out by {a} after midnight. Nobody when I drove by. If it was you, don't."),
		TEXT("Somebody walking out by {a} at night, and not one of ours. I'd stay in, if I were you.") };
	FFour StockRumorLines = {
		TEXT("Rancher in this morning says his cattle were acting up out by {a}. Wouldn't say how."),
		TEXT("There's talk about the cattle out by {a}. Something about the way they were standing. Eat your eggs."),
		TEXT("Livestock complaint out by {a}. I don't do livestock. I wrote it down anyway."),
		TEXT("{A}. The cattle know first. Always have.") };
	FFour TipLines = {
		TEXT("Heard Earl read your note about {a}. Everybody did. Half of them are going out there to look."),
		TEXT("You're the one called Earl about {a}? My sister heard it. She hasn't slept."),
		TEXT("I heard what you gave the radio about {a}. Next time, give it to me first."),
		TEXT("Your bit on Earl about {a}. Good. Keep them talking. It's good for business.") };
	FFour FoundLines = {
		TEXT("Somebody saw your truck out by {a}. Not much out there but what you go looking for."),
		TEXT("Somebody said they saw you out at {a}. All by yourself? You be careful."),
		TEXT("You were out at {a}. Somebody always sees. Just so you know that."),
		TEXT("You've been out to {a}. Find anything? No, you wouldn't say. They never do.") };
	FFour HelpLines = {
		TEXT("County says you called in that blue pickup out on the flats. That was decent of you."),
		TEXT("You called in that poor man's truck. His wife was in here crying. Thank you."),
		TEXT("Your call on that pickup went in the file with your name on it. Appreciated."),
		TEXT("You found the missing man's truck. See? It's not just me.") };
	FFour TakenLines = {
		TEXT("You look like you lost a night somewhere. Get some sleep."),
		TEXT("Honey, you look like you haven't slept since Easter. Sit down."),
		TEXT("You're not right. If something happened to you out there, I want it on paper."),
		TEXT("You've got the look. Lost some time, have you? Come in, I won't charge you to talk.") };
	FFour HearLines = {
		TEXT("Fella at the elevator was going on about {a}. I don't know what about. Nobody does."),
		TEXT("A trucker at the counter wouldn't stop talking about {a}. Wouldn't say why, either."),
		TEXT("If you're out driving, there's been talk about {a}. I haven't been out there. I'd like to know."),
		TEXT("{A}. That's the one nobody puts on a postcard. Ask yourself why.") };
	FFour SpacemanLines = {
		TEXT("Saw that rubber spaceman on your dash. Suction cup won't hold in the heat."),
		TEXT("I saw your little green man on the dash! That's cute. Is that allowed?"),
		TEXT("Take that thing off your dash before you drive into my county."),
		TEXT("How's the spaceman riding? Tell your friends at the dishes. I've got a whole bin.") };
	FFour LureLines = {
		TEXT("You're the one setting salt out on open range? The ranchers have noticed."),
		TEXT("Somebody said you're feeding the coyotes out there. Is that science?"),
		TEXT("Salt blocks off the lease draw cattle onto the highway. Mind where you put them."),
		TEXT("Salt on open ground, to see what comes. Smart. Animals know first. Always have.") };
	FFour TourLines = {
		TEXT("Took his tour, did you? Five dollars to look at a rock."),
		TEXT("You went on his tour! Did he do the voices? He does the voices."),
		TEXT("That tour goes onto private land at the fence line. I don't stop him. Yet."),
		TEXT("Good tour, wasn't it. Better with the second telling. Come back in summer.") };
	FFour PrintLines = {
		TEXT("Your picture's in his window. Half the town's been in to look at it."),
		TEXT("I saw your picture in the shop window. It gave me the shivers, I'll tell you."),
		TEXT("That picture in the shop window. I'd like the negative, if it's what he says it is."),
		TEXT("Your picture's in my window. Best seller since the comet. Bring me more.") };
	const TCHAR* const Chatter[4][2] = {
		{ TEXT("Regular's down to eighty-nine nine. It's been coming down all spring."), TEXT("Wind'll pick up after four. It always does.") },
		{ TEXT("Pie's peach today. Tomorrow it's peach too. The cook's got a lot of peaches."), TEXT("You work too much. All you people out there do.") },
		{ TEXT("Keep it under thirty-five through town. I mean it."), TEXT("Quiet week, mostly. I'd like to keep it that way.") },
		{ TEXT("Hats are half off. They've been half off since Christmas."), TEXT("Tourists don't come till June. Then they all come at once.") } };
	FFour GoneMinding = {
		TEXT("His sister-in-law is minding the counter. \"He didn't come in. His truck's at the house. Don't ask me.\""),
		TEXT("The cook is working the counter and the grill both. \"She's not in. Don't ask me.\""),
		TEXT("The substation is locked and the patrol car is gone. A note on the door: CALL SOCORRO."),
		TEXT("A girl from the high school is minding the register. \"He hasn't been in. He left the lights on.\"") };
	FFour GonePost = {
		TEXT("The night window is shut. A card in it: BACK SOON. Under it, in another hand: SINCE {b}."),
		TEXT("The cook is working the counter as well as the grill. \"She's not in. Don't ask me.\""),
		TEXT("The substation is locked. The patrol car isn't out front. A note on the door: CALL SOCORRO."),
		TEXT("Nobody on the stoop. The ashtray is full and nobody has emptied it. The lights are on inside.") };
	// M4 (CHURCH, 13 November): the San Isidro cemetery behind the church. The waitress's kitchen looks out on it (her house is the next
	// lot south): from day 2 she tells, once, of a broom out there after dark and a ring swept round one stone by morning. The look is
	// the player's own (StepGrave): standing in it keeps what is there as a field record, and she has a word for that after. Nothing in
	// it is more than somebody could have done with a broom; whatever else comes to the cemetery comes as to any place on the catalogue.
	const TCHAR* const GraveKey = TEXT("grave:cemetery");
	const TCHAR* const GraveSeenKey = TEXT("graveseen:cemetery");
	const TCHAR* const GraveFind = TEXT("cemetery_swept");
	const TCHAR* const GravePlace = TEXT("cemetery");
	constexpr double GraveWithinM = 11; // inside its fence, about (the plot is 22 m by 21)
	const TCHAR* const GraveLine = TEXT("Somebody's out in San Isidro with a broom after dark. I hear it from my kitchen. By morning there's a ring swept round one old stone. Go look. In daylight.");
	const TCHAR* const GraveSeenLine = TEXT("You went out to San Isidro? Then it's not just me. Don't tell the deputy I sent you.");
	const TCHAR* const GraveDay = TEXT("At the back of the cemetery behind the church: a ring of ground ten feet across swept smooth round one old stone, every stroke of the broom going the same way round. A jar of fresh flowers on the stone. Prints come in at the north gap and stop at the edge of the ring. None go in.");
	const TCHAR* const GraveDark = TEXT("At the back of the cemetery behind the church, by flashlight: a ring of ground ten feet across swept smooth round one old stone, every stroke going the same way round. Flowers in a jar on the stone, the water still cold. Prints come in at the north gap and stop at the edge of the ring.");
	const TCHAR* const NotSaid[] = { TEXT("chevy"), TEXT("ford"), TEXT("dodge"), TEXT("jeep"), TEXT("coke"), TEXT("pepsi"), TEXT("alien"), TEXT("ufo"), TEXT("saucer"), TEXT("tyre"), TEXT("grey"), TEXT("colour"), TEXT("petrol") };
}

// ---------- who they are ----------

const TArray<FString>& FS47Townsfolk::People()
{
	static const TArray<FString> P = { TEXT("attendant"), TEXT("waitress"), TEXT("deputy"), TEXT("souvenir_man") };
	return P;
}
FString FS47Townsfolk::Called(const FString& Who)
{
	return Who == TEXT("attendant") ? TEXT("the attendant at the gas station") : Who == TEXT("waitress") ? TEXT("the waitress at the Mesa") : Who == TEXT("deputy") ? TEXT("the deputy") : Who == TEXT("souvenir_man") ? TEXT("the souvenir man") : TEXT("somebody");
}
FString FS47Townsfolk::Speaker(const FString& Who)
{
	return Who == TEXT("attendant") ? TEXT("THE ATTENDANT") : Who == TEXT("waitress") ? TEXT("THE WAITRESS") : Who == TEXT("deputy") ? TEXT("THE DEPUTY") : Who == TEXT("souvenir_man") ? TEXT("THE SHOP MAN") : FString();
}
FString FS47Townsfolk::WorksAt(const FString& Who)
{
	return Who == TEXT("attendant") ? TEXT("gas") : Who == TEXT("waitress") ? TEXT("diner") : Who == TEXT("deputy") ? TEXT("sheriff") : Who == TEXT("souvenir_man") ? TEXT("souvenir") : FString();
}
FString FS47Townsfolk::NightPlaceOf(const FString& Who)
{
	// Critics, round 8: a night's event at the substation took the deputy, who by then was asleep in his house (S47TownDay); one at
	// his house took nobody. At night they are where the day has them after dark.
	static const bool bNoDay = FParse::Param(FCommandLine::Get(), TEXT("S47TownDayOff"));
	if (bNoDay) return WorksAt(Who);
	return Who == TEXT("deputy") ? TEXT("h02") : Who == TEXT("souvenir_man") ? TEXT("h03") : WorksAt(Who);
}
bool FS47Townsfolk::TakenBy(int32 Seed, int32 Night, const FString& Who)
{
	// About one night in three that something comes to their place, by the game's seed: never the same answer twice for a night.
	return FS47NightRhythm::Roll(Seed, Night, FCrc::StrCrc32(*Who) ^ 0x544f574eu) < .34f;
}
int32 FS47Townsfolk::DaysAway(int32 Seed, const FString& Who, int32 Day)
{
	return 2 + int32(FS47NightRhythm::Roll(Seed, Day, FCrc::StrCrc32(*Who) ^ 0x4241434bu) * 3.f) % 3; // two to four days
}
FString FS47Townsfolk::TakenAfter(int32 Seed, int32 Night, TFunctionRef<int32(const FString&)> LastNightOf)
{
	for (const FString& Who : People()) if (LastNightOf(NightPlaceOf(Who)) == Night && TakenBy(Seed, Night, Who)) return Who;
	return FString();
}
FString FS47Townsfolk::Talk(const FString& Kind)
{
	const auto Any = [&Kind](std::initializer_list<const TCHAR*> Words) { for (const TCHAR* W : Words) if (Kind.Contains(W)) return true; return false; };
	if (Kind.IsEmpty() || Kind.Contains(TEXT("motel"))) return FString();
	if (Any({ TEXT("lights"), TEXT("landing"), TEXT("site_visit"), TEXT("field_visit"), TEXT("flyover"), TEXT("close_pass"), TEXT("burst"), TEXT("crash"), TEXT("marks") })) return TEXT("lights");
	if (Any({ TEXT("door"), TEXT("peek"), TEXT("lit_room"), TEXT("house_lit"), TEXT("knock") })) return TEXT("doors");
	if (Any({ TEXT("figure"), TEXT("walker"), TEXT("hitch"), TEXT("missing"), TEXT("taken"), TEXT("observer") })) return TEXT("figure");
	if (Any({ TEXT("cattle"), TEXT("carcass") })) return TEXT("stock");
	return FString();
}

// ---------- the session ----------

void FS47Townsfolk::Begin(const S47J::FObj& Saved)
{
	Gone.Reset(); Returned.Reset(); Told.Reset(); SaidDay.Reset(); LastSaid.Reset(); LastVanishDay = -100; CheckedNight = 0; StepIn = 0;
	if (Saved.IsValid())
	{
		const auto ReadList = [](const S47J::FObj& O, const TCHAR* Key, TArray<FGone>& Out)
		{
			for (const auto& V : S47J::Arr(O, Key))
			{
				if (!V || V->Type != EJson::Object || Out.Num() >= 16) continue;
				const auto X = V->AsObject(); FGone G;
				G.Who = S47J::Str(X, TEXT("who")); G.Place = S47J::Str(X, TEXT("place")).Left(64);
				G.Day = FMath::Clamp(S47J::Int(X, TEXT("day")), 0, 1000000); G.Back = FMath::Clamp(S47J::Int(X, TEXT("back")), 0, 1000000);
				if (People().Contains(G.Who) && G.Day > 0) Out.Add(G);
			}
		};
		ReadList(Saved, TEXT("gone"), Gone); ReadList(Saved, TEXT("returned"), Returned);
		for (const FString& T : S47J::Strs(Saved, TEXT("told"))) if (!T.IsEmpty() && T.Len() < 96 && Told.Num() < 2000) Told.Add(T);
		if (const auto D = S47J::Obj(Saved, TEXT("saidDay"))) for (const auto& Pair : D->Values) { double N = 0; if (Pair.Value && Pair.Value->TryGetNumber(N)) SaidDay.Add(FString(*Pair.Key), FMath::Clamp(int32(N), 0, 1000000)); }
		if (const auto L = S47J::Obj(Saved, TEXT("lastSaid"))) for (const auto& Pair : L->Values) { FString S; if (Pair.Value && Pair.Value->TryGetString(S)) LastSaid.Add(FString(*Pair.Key), S.Left(400)); }
		LastVanishDay = FMath::Clamp(S47J::Int(Saved, TEXT("lastGoneDay"), -100), -100, 1000000);
		CheckedNight = FMath::Clamp(S47J::Int(Saved, TEXT("checkedNight")), 0, 1000000);
	}
	Publish();
}

void FS47Townsfolk::Stop()
{
	S47TownsfolkLocal::GAway.Reset();
	// At process exit the object array may already be gone (as in S47Happenings::Stop): the town's figures are not touched then.
	if (IsEngineExitRequested() || GExitPurge || IsGarbageCollecting()) return;
	S47TownDay::Stop();
	ShowPosts(true);
}

void FS47Townsfolk::ResetAway() { S47TownsfolkLocal::GAway.Reset(); }

void FS47Townsfolk::ShowPosts(bool bAll)
{
	// Freeform does not run the town's hours (US47TownLayer::WorldStep is Story's), so these stay as they are set here.
	if (!IsValid(M) || M->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed) || !IsValid(M->TownLayer.Get()) || !M->TownLayer->Town) return;
	FS47Town& Town = *M->TownLayer->Town;
	const bool bDeputy = bAll || !Away(TEXT("deputy")), bSmoker = bAll || !Away(TEXT("souvenir_man"));
	if (Town.bDeputyIn != bDeputy) Town.SetDeputy(bDeputy);
	if (Town.bSmokerIn != bSmoker) Town.SetPeople(Town.bWomanIn, bSmoker);
}

S47J::FObj FS47Townsfolk::Save() const
{
	auto O = S47J::New();
	const auto WriteList = [&O](const TCHAR* Key, const TArray<FGone>& List)
	{
		if (List.IsEmpty()) return;
		TArray<TSharedPtr<FJsonValue>> A;
		for (const FGone& G : List) { auto X = S47J::New(); S47J::Set(X, TEXT("who"), G.Who); if (!G.Place.IsEmpty()) S47J::Set(X, TEXT("place"), G.Place); S47J::Set(X, TEXT("day"), G.Day); S47J::Set(X, TEXT("back"), G.Back); A.Add(MakeShared<FJsonValueObject>(X)); }
		O->SetArrayField(Key, A);
	};
	WriteList(TEXT("gone"), Gone); WriteList(TEXT("returned"), Returned);
	if (!Told.IsEmpty()) { TArray<FString> T = Told.Array(); T.Sort(); S47J::Set(O, TEXT("told"), T); }
	if (!SaidDay.IsEmpty()) { auto D = S47J::New(); TArray<FString> K; SaidDay.GetKeys(K); K.Sort(); for (const FString& Key : K) S47J::Set(D, *Key, SaidDay[Key]); S47J::Set(O, TEXT("saidDay"), D); }
	if (!LastSaid.IsEmpty()) { auto D = S47J::New(); TArray<FString> K; LastSaid.GetKeys(K); K.Sort(); for (const FString& Key : K) S47J::Set(D, *Key, LastSaid[Key]); S47J::Set(O, TEXT("lastSaid"), D); }
	if (LastVanishDay > -100) S47J::Set(O, TEXT("lastGoneDay"), LastVanishDay);
	if (CheckedNight) S47J::Set(O, TEXT("checkedNight"), CheckedNight);
	return O;
}

void FS47Townsfolk::Publish()
{
	S47TownsfolkLocal::GAway.Reset();
	for (const FGone& G : Gone) S47TownsfolkLocal::GAway.Add(G.Who);
	ShowPosts(false);
}

bool FS47Townsfolk::AwayNow(const FString& Who) { return S47TownsfolkLocal::GAway.Contains(Who); }
bool FS47Townsfolk::Away(const FString& Who) const { return Gone.ContainsByPredicate([&Who](const FGone& G) { return G.Who == Who; }); }

bool FS47Townsfolk::Has(const FString& Key) const
{
	const bool bAway = Key.StartsWith(TEXT("away_")), bBack = Key.StartsWith(TEXT("back_"));
	if (!bAway && !bBack) return false;
	const FString Who = Key.Mid(5);
	// Away only while they are: the radio does not ask after somebody standing at their post again.
	if (bAway || Away(Who)) return bAway && Away(Who);
	const int32 Today = M && M->Freeform ? M->Freeform->Day() : 0;
	return Returned.ContainsByPredicate([&](const FGone& G) { return G.Who == Who && Today <= G.Back + 3; });
}

bool FS47Townsfolk::Vanish(const FString& Who, const FString& Place)
{
	if (!People().Contains(Who) || !Gone.IsEmpty() || !M || !M->Freeform) return false;
	FS47FreeformSession& F = *M->Freeform;
	// After 08:00 it counts from the day it was noticed; before, from the day the night belongs to.
	FGone G; G.Who = Who; G.Place = Place.Left(64); G.Day = F.Day(); G.Back = G.Day + DaysAway(F.Seed(), Who, G.Day);
	Returned.RemoveAll([&Who](const FGone& R) { return R.Who == Who; });
	Gone.Add(G);
	LastVanishDay = G.Day;
	Publish();
	UE_LOG(LogS47Townsfolk, Display, TEXT("S47: townsfolk: %s gone on day %d, last seen at %s; back on day %d"), *Who, G.Day, Place.IsEmpty() ? TEXT("nowhere known") : *Place, G.Back);
	M->RequestAutosave();
	return true;
}

void FS47Townsfolk::DevReturnAll()
{
	const int32 Today = M && M->Freeform ? M->Freeform->Day() : 1;
	for (FGone& G : Gone) { G.Back = Today; Returned.Add(G); }
	Gone.Reset();
	Publish();
}

void FS47Townsfolk::Step(double Dt)
{
	if (!M || !M->Freeform) return;
	// NPC-DAY: their days, every frame (where they are, walking, the sheriff's window, door and car, the houses' lamps).
	S47TownDay::Step(M, Dt);
	StepIn -= Dt;
	if (StepIn > 0) return;
	StepIn = 5;
	FS47FreeformSession& F = *M->Freeform;
	const int32 Today = F.Day();
	// Back after their days away, in the morning.
	const double Hour = FMath::Fmod(F.Clock(), 86400.) / 3600.;
	for (int32 I = Gone.Num() - 1; I >= 0; --I)
		if (Today > Gone[I].Back || (Today == Gone[I].Back && Hour >= 8))
		{
			UE_LOG(LogS47Townsfolk, Display, TEXT("S47: townsfolk: %s back on day %d"), *Gone[I].Who, Today);
			Returned.Add(Gone[I]); Gone.RemoveAt(I); Publish(); M->RequestAutosave();
		}
	StepGrave();
	// Once a night is over (from four in the morning after it): did something come to one of their places that night? Then perhaps
	// it took them. Every night over since the last one settled, so that a rest straight to 08:00 does not sleep through it (critics,
	// round 3: the morning check used to look only between four and eight, and a rest or a motel room jumps past it).
	if (!F.EventPlaces || !F.RulesLive()) return;
	const int32 Done = FS47FreeformSession::NightsOver(F.Clock());
	if (Done <= CheckedNight) return;
	const int32 From = FMath::Max(CheckedNight + 1, Done - 2);
	CheckedNight = Done;
	for (int32 N = From; N <= Done; ++N)
	{
		if (!Gone.IsEmpty() || Today - LastVanishDay < RestDays) break;
		const FString Who = TakenAfter(F.Seed(), N, [&F](const FString& Id) { return F.EventPlaces->LastNight(Id); });
		if (Who.IsEmpty()) continue;
		UE_LOG(LogS47Townsfolk, Display, TEXT("S47: townsfolk: night %d settled in the morning: %s taken"), N, *Who);
		Vanish(Who, NightPlaceOf(Who));
		break;
	}
}

// M4: the player standing in the cemetery once the waitress has told of it: what is there, by day or by flashlight, kept once.
void FS47Townsfolk::StepGrave()
{
	using namespace S47TownsfolkLocal;
	FS47FreeformSession& F = *M->Freeform;
	if (!F.EventPlaces || F.HasFinding(GraveFind) || !Told.Contains(FString(TEXT("waitress:")) + GraveKey)) return;
	const FS47EventPlace* P = F.EventPlaces->Find(GravePlace);
	const double Dist = P ? F.EventPlaces->DistanceFromPlayer(*P) : -1;
	if (Dist < 0 || Dist > GraveWithinM) return;
	const double Hour = FMath::Fmod(F.Clock(), 86400.) / 3600.;
	const bool bDay = Hour >= 6.5 && Hour < 18.5;
	F.RegisterFinding(GraveFind, bDay ? GraveDay : GraveDark);
	UE_LOG(LogS47Townsfolk, Display, TEXT("S47: townsfolk: the swept ring in the cemetery found (%s, %.0f m in)"), bDay ? TEXT("by day") : TEXT("after dark"), Dist);
	if (M->UI) M->UI->Toast(TEXT("A ring swept round one old stone at the back. Kept as a field record."), 6.f);
	M->RequestAutosave();
}

// ---------- what they say ----------

TArray<FS47Townsfolk::FLine> FS47Townsfolk::Lines(const FString& Who) const
{
	using namespace S47TownsfolkLocal;
	TArray<FLine> Out;
	const int32 I = Index(Who);
	if (I < 0 || !M || !M->Freeform) return Out;
	const FS47FreeformSession& F = *M->Freeform;
	const int32 Today = F.Day(), Night = FS47FreeformSession::NightOf(F.Clock());
	const auto TitleOf = [&F](const FString& Id) { return F.EventPlaces ? F.EventPlaces->TitleOf(Id) : FString(TEXT("somewhere out there")); };
	const auto Add = [&Out](const FString& Key, const FString& Text, int32 Rank) { FLine L; L.Key = Key; L.Text = Text; L.Rank = Rank; Out.Add(L); };
	// Someone gone: nothing else.
	for (const FGone& G : Gone)
		if (G.Who != Who) Add(FString::Printf(TEXT("gone:%s:%d"), *G.Who, G.Day), Cap(Fill(GoneLines[I], Called(G.Who), WeekdayOf(G.Day, F.OldCalendar()), G.Place.IsEmpty() ? FString(TEXT("the edge of town")) : TitleOf(G.Place))), 100);
	for (const FGone& G : Returned)
	{
		if (Today > G.Back + 6) continue;
		if (G.Who == Who) Add(FString::Printf(TEXT("selfback:%d"), G.Day), SelfBackLines[I], 90);
		else Add(FString::Printf(TEXT("back:%s:%d"), *G.Who, G.Day), Cap(Fill(BackLines[I], Called(G.Who), WeekdayOf(G.Back, F.OldCalendar())).Replace(TEXT("{he}"), *He(G.Who), ESearchCase::CaseSensitive)), 80);
	}
	// What is going round about places on the catalogue, the last two nights: only what somebody could have seen (lights from far off;
	// at a door, a window or out walking only where people are), told as the kind of thing it was, and second-hand: a third of the time
	// at the wrong place. Never the motel, whose nights are its own (MOTEL-SLEEP-20261108), nor Baca's (he tells his own).
	if (F.EventPlaces)
		for (const FS47EventPlace& P : F.EventPlaces->All())
		{
			const int32 Last = F.EventPlaces->LastNight(P.Id);
			if (Last <= 0 || Last < Night - 2 || P.Id == TEXT("motel") || P.Id == TEXT("baca")) continue;
			const FString Kind = Talk(F.EventPlaces->LastKind(P.Id));
			if (Kind.IsEmpty() || (Kind != TEXT("lights") && !P.HasAny(ES47PlaceTrait::People | ES47PlaceTrait::Town | ES47PlaceTrait::Road | ES47PlaceTrait::Farm))) continue;
			FString Where = P.Title;
			if (FS47NightRhythm::Roll(F.Seed(), Last, FCrc::StrCrc32(*P.Id) ^ 0x52554d52u) < .33f)
			{
				const FS47EventPlace* Near = nullptr; double D = 1e9;
				for (const FS47EventPlace& Q : F.EventPlaces->All())
				{
					const double E = FVector2D::Distance(FVector2D(Q.X, Q.Z), FVector2D(P.X, P.Z));
					if (Q.Id != P.Id && Q.Id != TEXT("motel") && Q.Id != TEXT("baca") && E < D) { D = E; Near = &Q; }
				}
				if (Near) Where = Near->Title;
			}
			const TCHAR* Said = Kind == TEXT("doors") ? DoorRumorLines[I] : Kind == TEXT("figure") ? FigureRumorLines[I] : Kind == TEXT("stock") ? StockRumorLines[I] : RumorLines[I];
			const FString Text = Fill(Said, Where).Replace(TEXT("{A}"), *Cap(Where));
			Add(FString::Printf(TEXT("rumor:%s:%d"), *P.Id, Last), Cap(Text), 60 + Last);
		}
	// M4: the cemetery behind the church, from the waitress only, from day 2 (see GraveLine).
	if (Who == TEXT("waitress") && Today >= 2 && F.EventPlaces && F.EventPlaces->Find(GravePlace))
	{
		if (F.HasFinding(GraveFind)) Add(GraveSeenKey, GraveSeenLine, 37);
		else Add(GraveKey, GraveLine, 36);
	}
	// What the player has done.
	if (F.Tips) for (const FS47RadioTips::FGiven& G : F.Tips->Given()) if (G.Stage >= 1) Add(TEXT("tip:") + G.Place, Cap(Fill(TipLines[I], TitleOf(G.Place))), 55);
	for (const FString& Kept : F.FindingIdsFrom(0))
	{
		if (Kept.StartsWith(TEXT("missing_reported_"))) Add(TEXT("help:") + Kept, HelpLines[I], 58);
		else if (Kept.StartsWith(TEXT("place_")) && !Kept.EndsWith(TEXT("_needle")))
		{
			const FString Id = Kept.Mid(6);
			if (FS47Places::Def(Id)) Add(TEXT("found:") + Id, Cap(Fill(FoundLines[I], TitleOf(Id))), 50 + FMath::Min(F.FindingDay(Kept), 40) / 10);
		}
	}
	if (F.TimesTaken() > 0) Add(FString::Printf(TEXT("taken:%d"), F.TimesTaken()), TakenLines[I], 57);
	if (const FS47TownUse* T = F.TownUse.Get())
	{
		if (T->Seen(TEXT("spaceman"))) Add(TEXT("shop:spaceman"), SpacemanLines[I], 45);
		if (T->Seen(TEXT("lure"))) Add(TEXT("shop:lure"), LureLines[I], 46);
		if (T->Seen(TEXT("tour")) && Who != TEXT("souvenir_man")) Add(TEXT("shop:tour"), TourLines[I], 44);
		if (T->Seen(TEXT("sold_odd"))) Add(TEXT("shop:sold_odd"), PrintLines[I], 52);
	}
	// A place nobody has told the player of: said once by somebody, and then it is on the map with a question mark.
	if (F.Places)
	{
		const FString New = F.Places->NextUnknown(Today * 13 + I);
		if (!New.IsEmpty()) Add(TEXT("hear:") + New, Cap(Fill(HearLines[I], TitleOf(New)).Replace(TEXT("{A}"), *Cap(TitleOf(New)))), 30);
	}
	Add(FString::Printf(TEXT("chat:%d"), Today % 2), Chatter[I][Today % 2], 1);
	Out.Sort([](const FLine& A, const FLine& B) { return A.Rank > B.Rank; });
	return Out;
}

// ---------- the notebook's WHO & WHAT (PT-NOTEBOOK-20261109) ----------

FString FS47Townsfolk::RoleOf(const FString& Who)
{
	return Who == TEXT("attendant") ? TEXT("Night man at the gas station") : Who == TEXT("waitress") ? TEXT("Waitress at the Mesa Diner") : Who == TEXT("deputy") ? TEXT("Deputy at the substation") : Who == TEXT("souvenir_man") ? TEXT("Runs the souvenir shop") : FString();
}

FString FS47Townsfolk::CardLine(const FString& Who, const TArray<FString>& Keys, int32 Today, int32 GoneDay, TFunctionRef<FString(const FString&)> TitleOf)
{
	using namespace S47TownsfolkLocal;
	const int32 I = Index(Who);
	if (I < 0) return FString();
	// Gone: only that the place is empty, and for how long. Never where they went or why.
	if (GoneDay > 0)
	{
		const int32 N = FMath::Max(1, Today - GoneDay + 1);
		static const TCHAR* const Empty[4] = { TEXT("Night window shut"), TEXT("Not at the counter"), TEXT("Window dark"), TEXT("Nobody on the stoop") };
		return FString::Printf(TEXT("%s %d %s. Nobody asks."), Empty[I], N, N == 1 ? (I == 1 ? TEXT("day") : TEXT("night")) : (I == 1 ? TEXT("days") : TEXT("nights")));
	}
	// What they have told, the weightiest first (as Lines ranks it): one plain line in the player's pencil.
	FString Best; int32 Rank = -1;
	const auto Part = [](const FString& Key, int32 N) { TArray<FString> P; Key.ParseIntoArray(P, TEXT(":"), false); return P.IsValidIndex(N) ? P[N] : FString(); };
	for (const FString& K : Keys)
	{
		FString Say; int32 R = 0;
		if (K.StartsWith(TEXT("gone:"))) { Say = FString::Printf(TEXT("Talks about %s being gone."), *Called(Part(K, 1))); R = 100; }
		else if (K.StartsWith(TEXT("selfback:"))) { Say = FString::Printf(TEXT("Back at work. Won't say where %s was."), *He(Who)); R = 90; }
		else if (K.StartsWith(TEXT("back:"))) { Say = Cap(FString::Printf(TEXT("%s is back, %s says. Nothing more."), *Called(Part(K, 1)), *He(Who))); R = 80; }
		else if (K.StartsWith(TEXT("rumor:"))) { Say = FString::Printf(TEXT("Passed on talk about %s."), *TitleOf(Part(K, 1))); R = 60 + FMath::Min(FCString::Atoi(*Part(K, 2)), 30); }
		else if (K.StartsWith(TEXT("help:"))) { Say = TEXT("Knows I called in the blue pickup."); R = 58; }
		else if (K.StartsWith(TEXT("taken:"))) { Say = TEXT("Says I look like I lost a night."); R = 57; }
		else if (K.StartsWith(TEXT("tip:"))) { Say = FString::Printf(TEXT("Heard my call to the radio about %s."), *TitleOf(Part(K, 1))); R = 55; }
		else if (K.StartsWith(TEXT("found:"))) { Say = FString::Printf(TEXT("Knew I had been out to %s."), *TitleOf(Part(K, 1))); R = 50; }
		else if (K.StartsWith(TEXT("shop:"))) { Say = TEXT("Noticed what I picked up in town."); R = 45; }
		else if (K.StartsWith(TEXT("graveseen:"))) { Say = TEXT("Sent me to the swept graves. Not a word to the deputy."); R = 37; }
		else if (K.StartsWith(TEXT("grave:"))) { Say = TEXT("Hears a broom in the cemetery after dark. Said to look in daylight."); R = 36; }
		else if (K.StartsWith(TEXT("hear:"))) { Say = FString::Printf(TEXT("Told me about %s. On my map with a question mark."), *TitleOf(Part(K, 1))); R = 30; }
		else if (K.StartsWith(TEXT("chat:")))
		{
			static const TCHAR* const Small[4] = { TEXT("Talks gas prices and the wind."), TEXT("Talks pie, and how much I work."), TEXT("Talks speed limits. Times, not stories."), TEXT("Talks hats and tourists.") };
			Say = Small[I]; R = 1;
		}
		if (!Say.IsEmpty() && R > Rank) { Rank = R; Best = Say; }
	}
	return Best;
}

TArray<FS47Townsfolk::FCard> FS47Townsfolk::Cards() const
{
	TArray<FCard> Out;
	if (!M || !M->Freeform) return Out;
	const FS47FreeformSession& F = *M->Freeform;
	const auto TitleOf = [&F](const FString& Id) { const FString T = F.EventPlaces ? F.EventPlaces->TitleOf(Id) : FString(); return T.IsEmpty() ? FString(TEXT("a place out there")) : T; };
	for (const FString& Who : People())
	{
		TArray<FString> Keys;
		const FString Mine = Who + TEXT(":");
		for (const FString& T : Told) if (T.StartsWith(Mine)) Keys.Add(T.Mid(Mine.Len()));
		Keys.Sort();
		const FGone* G = Gone.FindByPredicate([&Who](const FGone& X) { return X.Who == Who; });
		if (Keys.IsEmpty() && !G) continue; // not met, and not missed
		FCard Cd; Cd.Who = Who; Cd.Name = Speaker(Who); Cd.Role = RoleOf(Who); Cd.bGone = G != nullptr;
		Cd.Line = CardLine(Who, Keys, F.Day(), G ? FMath::Max(1, G->Day) : 0, TitleOf);
		Out.Add(Cd);
	}
	return Out;
}

TArray<FString> FS47Townsfolk::Candidates(const FString& Who) const
{
	TArray<FString> Out;
	for (const FLine& L : Lines(Who)) Out.Add(L.Key + TEXT(" / ") + L.Text);
	return Out;
}

FString FS47Townsfolk::Gossip(const FString& Who)
{
	if (!M || !M->Freeform || Away(Who) || S47TownsfolkLocal::Index(Who) < 0) return FString();
	const int32 Today = M->Freeform->Day();
	// One new thing a day each; ask again the same day and they say it again.
	if (SaidDay.FindRef(Who) == Today && LastSaid.Contains(Who)) return Speaker(Who) + TEXT(": ") + LastSaid[Who];
	for (const FLine& L : Lines(Who))
	{
		// Somebody gone is said again every day; the rest once by each.
		if (!L.Key.StartsWith(TEXT("gone:")) && !L.Key.StartsWith(TEXT("chat:")) && Told.Contains(Who + TEXT(":") + L.Key)) continue;
		Told.Add(Who + TEXT(":") + L.Key);
		SaidDay.Add(Who, Today); LastSaid.Add(Who, L.Text);
		if (L.Key.StartsWith(TEXT("hear:")) && M->Freeform->Places) M->Freeform->Places->Hear(L.Key.Mid(5));
		// Asking about someone gone keeps what the town says as a field record.
		if (L.Key.StartsWith(TEXT("gone:")))
		{
			const FString Id = FString::Printf(TEXT("townsfolk_%s"), *L.Key.Replace(TEXT(":"), TEXT("_")));
			if (!M->Freeform->HasFinding(Id)) M->Freeform->RegisterFinding(Id, FString::Printf(TEXT("Caliche, %s: \"%s\""), *Called(Who), *L.Text));
		}
		UE_LOG(LogS47Townsfolk, Display, TEXT("S47: townsfolk: %s says %s"), *Who, *L.Key);
		M->RequestAutosave();
		return Speaker(Who) + TEXT(": ") + L.Text;
	}
	return FString();
}

FString FS47Townsfolk::CounterLine(const FString& Who)
{
	const int32 I = S47TownsfolkLocal::Index(Who);
	if (I < 0) return FString();
	if (Away(Who)) return S47TownsfolkLocal::GoneMinding[I];
	return Gossip(Who);
}

bool FS47Townsfolk::ShowAway(const FString& Who)
{
	using namespace S47TownsfolkLocal;
	const int32 I = Index(Who);
	if (I < 0 || !Away(Who) || !M || !M->UI || !M->Freeform) return false;
	const FGone* G = Gone.FindByPredicate([&Who](const FGone& X) { return X.Who == Who; });
	TArray<TSharedRef<SWidget>> Rows;
	Rows.Add(S47Kit::Paper(Fill(GonePost[I], FString(), WeekdayOf(G ? G->Day : 1, M->Freeform->OldCalendar()).ToUpper())));
	// Whoever else is about says the rest.
	for (const FString& Other : People())
	{
		if (Other == Who || Away(Other)) continue;
		const FString Said = Gossip(Other);
		if (!Said.IsEmpty() && Said.Contains(Called(Who))) { Rows.Add(S47Kit::Text(Said)); break; }
	}
	Rows.Add(S47Kit::Button(TEXT("Leave it"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, FString::Printf(TEXT("%s / NOBODY"), *Speaker(Who).Replace(TEXT("THE "), TEXT(""))), S47Kit::Col(Rows), 820));
	return true;
}

bool FS47Townsfolk::Use(const FString& Id)
{
	if (Id != TEXT("diner:waitress") || !M || !M->IsFreeform()) return false;
	if (M->Evening && M->Evening->Commute()) return false; // the evening's own supper scene
	OpenDiner();
	return true;
}

void FS47Townsfolk::OpenDiner()
{
	if (!M || !M->UI || !M->Freeform) return;
	if (ShowAway(TEXT("waitress"))) return;
	TArray<TSharedRef<SWidget>> Rows;
	// NPC-DAY-FOLLOW-20261109: her shift is the night (S47TownDay, about seven at night to five). By day the cook pours through the
	// hatch and has nothing to say; before the kitchen opens, and on the cook's break, nobody is behind the counter and the coffee is cold.
	const FString Behind = S47TownDay::Counter();
	if (Behind != TEXT("waitress"))
	{
		const double Hour = FMath::Fmod(M->Freeform->Clock(), 86400.) / 3600.;
		Rows.Add(S47Kit::Paper(Behind == TEXT("cook") ? DinerLine(TEXT("cook"), Hour) : DinerLine(FString(), Hour)));
		Rows.Add(S47Kit::Button(Behind == TEXT("cook") ? TEXT("Leave a quarter and go") : TEXT("Leave"), [this] { if (M && M->UI) M->UI->Close(); }));
		M->UI->Open(S47Kit::Shell(M->UI, Behind == TEXT("cook") ? TEXT("MESA DINER / THE COOK") : TEXT("MESA DINER"), S47Kit::Col(Rows), 800));
		return;
	}
	Rows.Add(S47Kit::Paper(TEXT("MESA DINER. She pours your coffee without asking and leaves the pot where you can reach it.")));
	const FString Said = Gossip(TEXT("waitress"));
	if (!Said.IsEmpty()) Rows.Add(S47Kit::Text(Said));
	Rows.Add(S47Kit::Button(TEXT("Leave a quarter and go"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("MESA DINER / THE WAITRESS"), S47Kit::Col(Rows), 820));
}

FString FS47Townsfolk::DinerLine(const FString& Behind, double Hour)
{
	if (Behind == TEXT("cook")) return TEXT("MESA DINER. Nobody at the counter. The cook leans out through the hatch, pours you a cup from the kitchen pot and goes back to the grill without a word. The counter is the waitress's from about seven at night.");
	if (Hour < 12) return TEXT("MESA DINER. The chairs are still up on the tables and the coffee machine is cold, the pot upside down on the drainer. Nobody comes out. The kitchen light comes on at six.");
	return TEXT("MESA DINER. Nobody behind the counter. Cigarette smoke drifts in from the back door: the cook is on a break. What is left in the pot has gone cold.");
}

void FS47Townsfolk::FillMap(FS47MapView& V) const
{
	if (!M || !M->Freeform || !M->Freeform->EventPlaces) return;
	double SaroX = 0, SaroZ = 0;
	S47OpenLand::RoadOf(TEXT("saro"), 9, 0, SaroX, SaroZ);
	for (const FGone& G : Gone)
	{
		const FS47EventPlace* P = G.Place.IsEmpty() ? nullptr : M->Freeform->EventPlaces->Find(G.Place);
		if (!P) continue;
		FS47MapPin Pin; Pin.East = P->X; Pin.South = P->Z - SaroZ; Pin.Label = TEXT("last seen: ") + Called(G.Who); Pin.bHeard = true; V.Pins.Add(Pin);
	}
}

// ---------- tests ----------

bool FS47Townsfolk::SelfTest(TArray<FString>& Lines)
{
	using namespace S47TownsfolkLocal;
	bool bAll = true;
	const auto Check = [&](bool bOk, const FString& What) { bAll &= bOk; Lines.Add(FString(bOk ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("townsfolk: ") + What); };
	Check(People().Num() == 4 && WorksAt(TEXT("deputy")) == TEXT("sheriff") && WorksAt(TEXT("waitress")) == TEXT("diner") && Speaker(TEXT("souvenir_man")) == TEXT("THE SHOP MAN"), TEXT("four regulars, each at a place on the catalogue"));
	// PT-NOTEBOOK-20261109: WHO & WHAT's boxes: one plain line each, the weightiest thing told; someone gone is only gone.
	{
		const auto Title = [](const FString& Id) { return Id == TEXT("church") ? FString(TEXT("the church")) : FString(TEXT("the old mine")); };
		const TArray<FString> Told3 = { TEXT("chat:1"), TEXT("rumor:church:3"), TEXT("hear:mine") };
		Check(CardLine(TEXT("attendant"), Told3, 4, 0, Title) == TEXT("Passed on talk about the church.") && CardLine(TEXT("waitress"), { TEXT("chat:0") }, 2, 0, Title) == TEXT("Talks pie, and how much I work."),
			TEXT("WHO & WHAT: the weightiest thing told is the box's one line"));
		Check(CardLine(TEXT("deputy"), Told3, 6, 4, Title) == TEXT("Window dark 3 nights. Nobody asks.") && CardLine(TEXT("waitress"), {}, 5, 5, Title) == TEXT("Not at the counter 1 day. Nobody asks."),
			TEXT("WHO & WHAT: someone gone is only gone, and for how long"));
		bool bClean = true;
		const TArray<FString> Every = { TEXT("gone:deputy:3"), TEXT("selfback:2"), TEXT("back:waitress:2"), TEXT("rumor:church:3"), TEXT("help:missing_reported_x"), TEXT("taken:1"), TEXT("tip:church"), TEXT("found:mine"), TEXT("shop:lure"), TEXT("hear:mine"), TEXT("chat:0") };
		for (const FString& Who : People())
		{
			for (const FString& K : Every) { const FString L = CardLine(Who, { K }, 5, 0, Title); bClean &= !L.IsEmpty() && S47Script::LintText(L).IsEmpty(); }
			bClean &= S47Script::LintText(CardLine(Who, {}, 5, 2, Title) + TEXT(" ") + Speaker(Who) + TEXT(" ") + RoleOf(Who)).IsEmpty();
		}
		Check(bClean, TEXT("WHO & WHAT: every line a box can say keeps to Freeform's spoiler limits (S47Script::LintText)"));
	}
	// The odds: about a third of nights, the same answer each time; two to four days away.
	int32 Taken = 0; bool bSame = true, bDays = true;
	for (int32 N = 1; N <= 300; ++N) { const bool T = TakenBy(47, N, TEXT("deputy")); Taken += T ? 1 : 0; bSame &= T == TakenBy(47, N, TEXT("deputy")); const int32 D = DaysAway(47, TEXT("deputy"), N); bDays &= D >= 2 && D <= 4; }
	Check(bSame && Taken > 60 && Taken < 150 && bDays, FString::Printf(TEXT("a night's event at their place takes them %d nights in 300, the same each time, for two to four days"), Taken));
	// Gone, published, back; a save keeps it.
	{
		FS47Townsfolk T(nullptr);
		FGone G; G.Who = TEXT("deputy"); G.Place = TEXT("sheriff"); G.Day = 9; G.Back = 12; T.Gone.Add(G);
		FGone R; R.Who = TEXT("waitress"); R.Day = 2; R.Back = 4; T.Returned.Add(R);
		T.Told.Add(TEXT("attendant:rumor:church:3")); T.SaidDay.Add(TEXT("attendant"), 9); T.LastSaid.Add(TEXT("attendant"), TEXT("Wind'll pick up.")); T.LastVanishDay = 9; T.CheckedNight = 9;
		FS47Townsfolk B(nullptr); B.Begin(S47J::Read(S47J::Write(T.Save())));
		Check(B.Away(TEXT("deputy")) && AwayNow(TEXT("deputy")) && !AwayNow(TEXT("attendant")) && B.Returned.Num() == 1 && B.Told.Num() == 1 && S47J::Write(B.Save()) == S47J::Write(T.Save()),
			TEXT("who is gone, who came back and what was said survive a Continue, and the town's figures know who is gone"));
		B.Stop();
		Check(!AwayNow(TEXT("deputy")), TEXT("out of Freeform nobody is kept away"));
		FS47Townsfolk None(nullptr); None.Begin(nullptr);
		Check(None.Save()->Values.Num() == 0 && !None.Vanish(TEXT("nobody"), FString()), TEXT("an older save has nobody gone and writes nothing; only the four can go"));
	}
	// Words: each line in all four voices, no brand, no British spelling, nobody says what it was.
	{
		bool bClean = true; FString Bad;
		TArray<const TCHAR*> All;
		for (const auto* Set : { &GoneLines, &SelfBackLines, &BackLines, &RumorLines, &DoorRumorLines, &FigureRumorLines, &StockRumorLines, &TipLines, &FoundLines, &HelpLines, &TakenLines, &HearLines, &SpacemanLines, &LureLines, &TourLines, &PrintLines, &GoneMinding, &GonePost })
			for (int32 I = 0; I < 4; ++I) All.Add((*Set)[I]);
		for (int32 I = 0; I < 4; ++I) { All.Add(Chatter[I][0]); All.Add(Chatter[I][1]); }
		All.Add(GraveLine); All.Add(GraveSeenLine);
		for (const TCHAR* Said : All)
		{
			FString Word;
			const FString Lower = FString(Said).ToLower() + TEXT(" ");
			for (const TCHAR Ch : Lower)
			{
				if (FChar::IsAlpha(Ch)) { Word.AppendChar(Ch); continue; }
				for (const TCHAR* N : NotSaid) if (Word == N) { bClean = false; Bad = Word; }
				Word.Reset();
			}
			bClean &= FCString::Strlen(Said) < 200;
		}
		Check(bClean, bClean ? FString(TEXT("every line short, in American English, with no brand names, and nobody says what it was")) : TEXT("not said here: ") + Bad);
	}
	// M4: the cemetery's lines and what is found there keep to Freeform's limits, say no more than a broom could do, and have a box line.
	{
		bool bGrave = true;
		for (const TCHAR* Said : { GraveLine, GraveSeenLine, GraveDay, GraveDark })
			bGrave &= S47Script::LintText(Said).IsEmpty() && !FString(Said).Contains(TEXT("taken")) && FCString::Strlen(Said) < 320;
		const auto NoTitle = [](const FString&) { return FString(TEXT("a place out there")); };
		bGrave &= CardLine(TEXT("waitress"), { GraveKey }, 3, 0, NoTitle).Contains(TEXT("broom")) && CardLine(TEXT("waitress"), { GraveKey, GraveSeenKey }, 3, 0, NoTitle).Contains(TEXT("swept graves"));
		// (the record names the place by its catalogue title, so the case paper finds it there: S47Mail)
		bGrave &= FString(GraveDay).Contains(TEXT("the cemetery behind the church")) && FString(GraveDark).Contains(TEXT("the cemetery behind the church"));
		bGrave &= FString(GraveKey) != GraveSeenKey && !FString(GraveSeenKey).StartsWith(TEXT("grave:")) && GraveWithinM > 8 && GraveWithinM < 16;
		Check(bGrave, TEXT("M4: the waitress's word about the cemetery, its sequel and the swept ring found there are plain and clean, and the notebook box says them"));
	}
	Check(WeekdayOf(1, false) == TEXT("Monday") && WeekdayOf(7, false) == TEXT("Sunday") && WeekdayOf(9, false) == TEXT("Tuesday"), TEXT("day 1 is Monday, April 28, 1986"));
	Check(WeekdayOf(1, true) == TEXT("Sunday") && WeekdayOf(2, true) == TEXT("Monday") && WeekdayOf(5, true) == TEXT("Thursday"), TEXT("M7: on the old calendar day 1 is Sunday, April 13, 1986, as the HUD has it"));
	// Settled in arrears (critics, round 3): a night's event at the deputy's house at two, the player rests to eight; that morning the
	// night is over and settled, the deputy taken where the roll says so. Before four it is not over yet. Two nights slept through: both.
	// Critics, round 8: at night he is at home (S47TownDay), so an event at the empty substation takes nobody.
	{
		int32 N = 2;
		while (N < 400 && !TakenBy(47, N, TEXT("deputy"))) ++N;
		const double At2 = double(N) * 86400. + 2 * 3600., At8 = double(N) * 86400. + 8 * 3600.;
		const bool bDay = !FParse::Param(FCommandLine::Get(), TEXT("S47TownDayOff"));
		const auto Home = [N, bDay](const FString& Id) { return Id == (bDay ? TEXT("h02") : TEXT("sheriff")) ? N : 0; };
		const auto Office = [N](const FString& Id) { return Id == TEXT("sheriff") ? N : 0; };
		const bool bNotYet = FS47FreeformSession::NightsOver(At2) == N - 1 && FS47FreeformSession::NightOf(At2) == N;
		const bool bSlept = FS47FreeformSession::NightOf(At8) == N + 1 && FS47FreeformSession::NightsOver(At8) == N && FS47FreeformSession::NightsOver(At8 + 86400.) == N + 1;
		Check(N < 400 && bNotYet && bSlept && TakenAfter(47, N, Home) == TEXT("deputy") && TakenAfter(47, N + 1, Home).IsEmpty() && (!bDay || TakenAfter(47, N, Office).IsEmpty()),
			FString::Printf(TEXT("night %d's event at the deputy's house, slept through to 08:00: settled that morning, and the deputy is taken; one at the empty substation takes nobody"), N));
	}
	Check(Talk(TEXT("farm_lights")) == TEXT("lights") && Talk(TEXT("site_visit")) == TEXT("lights") && Talk(TEXT("door_visit")) == TEXT("doors") && Talk(TEXT("figure_near")) == TEXT("figure") && Talk(TEXT("cattle_ring")) == TEXT("stock") && Talk(TEXT("motel_night")).IsEmpty(),
		TEXT("talk follows what kind of thing it was; a night at the motel is not the town's"));
	{
		FS47Townsfolk T(nullptr); FGone R; R.Who = TEXT("deputy"); R.Day = 2; R.Back = 4; T.Returned.Add(R);
		Check(!T.Has(TEXT("away_deputy")) && T.Has(TEXT("back_deputy")), TEXT("back at his post: the radio no longer asks after him, it says he is back"));
	}
	{
		// NPC-DAY-FOLLOW-20261109: the Mesa's counter off her shift: three plain lines, the cold machine only when nobody pours.
		const FString Cook = DinerLine(TEXT("cook"), 10), Early = DinerLine(FString(), 5.5), Break = DinerLine(FString(), 15.1);
		bool bClean = true;
		for (const FString& L : { Cook, Early, Break }) bClean &= S47Script::LintText(L).IsEmpty() && !L.Contains(TEXT("47"));
		Check(bClean && Cook != Early && Early != Break && Early.Contains(TEXT("cold")) && Break.Contains(TEXT("cold")) && !Cook.Contains(TEXT("cold")) && Cook.Contains(TEXT("seven at night")),
			TEXT("off her shift the counter says who pours (the cook through the hatch) or that nobody does and the coffee is cold"));
	}
	return bAll;
}
