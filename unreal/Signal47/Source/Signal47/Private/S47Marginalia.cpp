#include "S47Marginalia.h"
#include "Misc/PackageName.h"
#include "S47Docs.h"
#include "S47Freeform.h"
#include "S47RadioTips.h"
#include "S47RoadGlimpse.h"
#include "S47Script.h"
#include "S47Townsfolk.h"
#include "S47UI.h"
#include "S47Visitors.h"

// MARGINALIA-20261109: the first version, about twenty scribbles. The table is the whole of it: what unlocks a line, the most it may
// hold, the line, the find that strikes it through when it was a wrong guess, and where it points on to.
namespace S47Marginalia
{
namespace
{
 constexpr ES47Allowed Hint = ES47Allowed::Hint, Glimpse = ES47Allowed::Glimpse, Full = ES47Allowed::Full;
 constexpr ESide People = ESide::People, Them = ESide::Them;
 constexpr EFollow NoFollow = EFollow::None, ToMap = EFollow::Map; // D46-20261110: no pinboard in Freeform; a scribble points on to the map or nowhere
 bool Starts(const FString& S, const TCHAR* Prefix) { return S.StartsWith(Prefix, ESearchCase::CaseSensitive); }
 // The rows the facts unlock, by index in Table(), and whether each is struck through.
 TArray<int32> Shown(const FFacts& Facts, TArray<bool>* Struck = nullptr)
 {
  TArray<int32> Out;
  const TArray<FRow>& T = Table();
  for (int32 I = 0; I < T.Num(); ++I)
  {
   const FRow& R = T[I];
   if (!Facts.Has(R.Trigger)) continue;
   if (uint8(R.Tier) > uint8(TierOf(R.Trigger))) continue; // never more than the find itself showed
   // Someone gone is only gone: none of their lines but the one that says so.
   const bool bGoneLine = Starts(R.Trigger, TEXT("gone:"));
   if (R.Who && *R.Who && !bGoneLine && Facts.Keys.Contains(FString(TEXT("gone:")) + R.Who)) continue;
   Out.Add(I);
   if (Struck) Struck->Add(R.StruckBy && *R.StruckBy && Facts.Has(R.StruckBy));
  }
  return Out;
 }
 FString Whole(const FRow& R) { return FString(R.Box) + TEXT(" / ") + R.Role + TEXT(" / ") + R.Text + TEXT(" / ") + R.FollowText; }
}

const TArray<FRow>& Table()
{
 // Box, role (people), whose, trigger, tier, text, struck by, follow
 static const TArray<FRow> Rows = {
  // ---------- PEOPLE ----------
  { TEXT("dale"), People, TEXT("DALE"), TEXT("Field maintenance. A voice on the phone"), TEXT("dale"), TEXT("dale:talk"), Hint,
    TEXT("Never seen him. Only the voice. Yells about the logs like someone reads them. WHO reads them?"), TEXT(""), NoFollow, TEXT("") },
  { TEXT("earl"), People, TEXT("EARL, 1420 AM"), TEXT("The night show out of Caliche"), TEXT("earl"), TEXT("earl:call"), Hint,
    TEXT("Takes calls about lights like they're the weather. Never laughs at one. Why not?"), TEXT(""), NoFollow, TEXT("") },
  { TEXT("deputy"), People, TEXT("THE DEPUTY"), TEXT("Deputy at the substation"), TEXT("deputy"), TEXT("told:deputy:"), Hint,
    TEXT("Writes everything down. Every car, every time. Who reads HIS notebook?"), TEXT(""), NoFollow, TEXT("") },
  { TEXT("waitress"), People, TEXT("THE WAITRESS"), TEXT("Waitress at the Mesa Diner"), TEXT("waitress"), TEXT("told:waitress:"), Hint,
    TEXT("Asked what I do at night. Said: radio. She didn't buy it."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("shop"), People, TEXT("THE SHOP MAN"), TEXT("Runs the souvenir shop"), TEXT("souvenir_man"), TEXT("told:souvenir_man:"), Hint,
    TEXT("Sells little gray men for four dollars. Laughed too long when I looked at the shelf."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("shop_guess"), People, TEXT("THE SHOP MAN"), TEXT("Runs the souvenir shop"), TEXT("souvenir_man"), TEXT("told:souvenir_man:"), Hint,
    TEXT("Tourist junk. Nobody out here believes any of it."), TEXT("humor_pump_d"), NoFollow, TEXT("") },
  { TEXT("baca"), People, TEXT("ELOY BACA"), TEXT("Ranch under the east rim"), TEXT("baca"), TEXT("baca:told"), Hint,
    TEXT("Waited four years for one of us to come out. Sure of every story. Was sure of the last one too."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("baca_look"), People, TEXT("ELOY BACA"), TEXT("Ranch under the east rim"), TEXT("baca"), TEXT("baca_look_"), Hint,
    TEXT("Says go and look, and when. Says the sky keeps a schedule. Old men and schedules."), TEXT(""), ToMap, TEXT("Where he said to look: on my map with a ?") },
  { TEXT("baca_gone"), People, TEXT("ELOY BACA"), TEXT("Ranch under the east rim"), TEXT("baca"), TEXT("gone:baca"), Hint,
    TEXT("Door open at the Baca place. Nobody home. Nobody asks."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("ward"), People, TEXT("DR. WARD"), TEXT("The district office. Office hours only"), TEXT("ward"), TEXT("ward:talk"), Hint,
    TEXT("Signs for the paper. Not for what's on it."), TEXT(""), NoFollow, TEXT("") },
  // ---------- THEM: only what the player would call them ----------
  { TEXT("lights"), Them, TEXT("THE LIGHTS"), TEXT(""), TEXT(""), TEXT("landing_seen_d"), Hint,
    TEXT("Lights came down out on the flat and went OUT on the ground. Planes don't land out there in the dark."), TEXT(""), ToMap, TEXT("Where did they come down? Take a bearing next time.") },
  { TEXT("balloon"), Them, TEXT("THE LIGHTS"), TEXT(""), TEXT(""), TEXT("landing_seen_d"), Hint,
    TEXT("Weather balloon. Has to be."), TEXT("lights_trace_landing_d"), NoFollow, TEXT("") },
  { TEXT("ring"), Them, TEXT("THE LIGHTS"), TEXT(""), TEXT(""), TEXT("lights_trace_landing_d"), Hint,
    TEXT("A balloon doesn't burn a ring in the caliche. Four marks pressed in. Something with WEIGHT."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("shade"), Them, TEXT("SHADOWS"), TEXT(""), TEXT(""), TEXT("rise:shade"), Glimpse,
    TEXT("Something tall on the ridge. Dark. Gone when I looked twice."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("steps"), Them, TEXT("SHADOWS"), TEXT(""), TEXT(""), TEXT("near:steps"), Full,
    TEXT("Steps in the gravel behind me. Turned round. Only my own shadow. Was it mine?"), TEXT(""), NoFollow, TEXT("") },
  { TEXT("hiker"), Them, TEXT("THE SILVER MAN"), TEXT(""), TEXT(""), TEXT("rise:nordic"), Glimpse,
    TEXT("A man on the rise, too still. Silver, or only the moon on him. Hiker? Lost?"), TEXT("near:nordic"), NoFollow, TEXT("") },
  { TEXT("silver"), Them, TEXT("THE SILVER MAN"), TEXT(""), TEXT(""), TEXT("near:nordic"), Full,
    TEXT("Close this time. A man's face. Nothing in it. Didn't blink."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("shone"), Them, TEXT("THE ONE MADE OF STARS"), TEXT(""), TEXT(""), TEXT("rise:cosmic"), Glimpse,
    TEXT("Something on the rise that SHONE. Not the moon on it. From inside."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("stars"), Them, TEXT("THE ONE MADE OF STARS"), TEXT(""), TEXT(""), TEXT("near:cosmic"), Full,
    TEXT("Stars INSIDE. Not reflecting. Inside."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("tall"), Them, TEXT("THE TALL PALE ONE"), TEXT(""), TEXT(""), TEXT("glimpse:tall"), Glimpse,
    TEXT("Crossed the highway way out in my lights. Long strides. Pale, or the lights made it pale. Gone before I got there."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("low"), Them, TEXT("THE LOW ONE"), TEXT(""), TEXT(""), TEXT("glimpse:low"), Glimpse,
    TEXT("Something low crossed the road far ahead. On four. Too long for a dog. Too many knees?"), TEXT(""), NoFollow, TEXT("") },
  { TEXT("coyote"), Them, TEXT("THE LOW ONE"), TEXT(""), TEXT(""), TEXT("glimpse:low"), Glimpse,
    TEXT("Coyote. A big one."), TEXT("tracks:low"), NoFollow, TEXT("") },
  { TEXT("low_prints"), Them, TEXT("THE LOW ONE"), TEXT(""), TEXT(""), TEXT("tracks:low"), Hint,
    TEXT("Prints in fours, close together, off the edge at a run. None on the asphalt. No coyote does that."), TEXT(""), NoFollow, TEXT("") },
  { TEXT("hat"), Them, TEXT("THE LITTLE GRAY ONE WITH THE HAT"), TEXT(""), TEXT(""), TEXT("humor_pump_d"), Full,
    TEXT("Wanted water at the pump. Short sleeves. A HAT. Very polite. Walked off behind the station like his car was back there."), TEXT(""), NoFollow, TEXT("") },
 };
 return Rows;
}

bool FFacts::Has(const FString& Prefix) const
{
 if (Prefix.IsEmpty()) return false;
 for (const FString& K : Keys) if (K.StartsWith(Prefix, ESearchCase::CaseSensitive)) return true;
 return false;
}

void AddFinding(FFacts& Out, const FString& Key, const FString& Text, int32 Seed, TFunctionRef<bool(const TCHAR*)> Have)
{
 if (Key.IsEmpty()) return;
 Out.Keys.Add(Key);
 const auto NightIn = [&Key](const TCHAR* Prefix) { return FMath::Max(1, FCString::Atoi(*Key.Mid(FCString::Strlen(Prefix)))); };
 // What crossed the road, and its prints: by the record's own words (S47RoadGlimpse::Record, TracksRecord).
 if (Starts(Key, TEXT("road_glimpse_d"))) Out.Keys.Add(Text.Contains(TEXT("something low")) ? TEXT("glimpse:low") : TEXT("glimpse:tall"));
 else if (Starts(Key, TEXT("road_tracks_d"))) Out.Keys.Add(Text.Contains(TEXT("prints in fours")) ? TEXT("tracks:low") : TEXT("tracks:tall"));
 // The figure on the rise and on foot: the body the seed drew for that night (as FS47Happenings draws it), the shade without one.
 else if (Starts(Key, TEXT("horizon_figure_d")))
 {
  const FString Body = S47RoadGlimpse::FigureFor(Seed, NightIn(TEXT("horizon_figure_d")), 0x484f525au, Have);
  Out.Keys.Add(TEXT("rise:") + (Body.IsEmpty() ? FString(TEXT("shade")) : Body));
 }
 else if (Starts(Key, TEXT("figure_near_d")))
 {
  if (Text.Contains(TEXT("I saw nothing"))) Out.Keys.Add(TEXT("near:steps"));
  else { const FString Body = S47RoadGlimpse::FigureFor(Seed, NightIn(TEXT("figure_near_d")), 0x46494731u, Have); Out.Keys.Add(TEXT("near:") + (Body.IsEmpty() ? FString(TEXT("shade")) : Body)); }
 }
 // Baca's tales (S47Visitors: "baca_" and the tale's key), the phone's talks (FS47Predecessor::Mark: "reyes_talk_<who>_<topic>").
 else if (Starts(Key, TEXT("baca_")) && !Starts(Key, TEXT("baca_gone_"))) Out.Keys.Add(TEXT("baca:told"));
 else if (Starts(Key, TEXT("reyes_talk_dale_"))) Out.Keys.Add(TEXT("dale:talk"));
 else if (Starts(Key, TEXT("reyes_talk_ward_"))) Out.Keys.Add(TEXT("ward:talk"));
}

FFacts FactsOf(const FS47FreeformSession& F, const TArray<FS47Doc>& Docs)
{
 FFacts Out;
 const auto Have = [](const TCHAR* Path) { return Path && *Path && FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path))); };
 // Every field record has its paper in the notebook ("freeform-" and its key, FS47FreeformSession::RecordEvidence).
 for (const FS47Doc& D : Docs)
  if (D.Id.StartsWith(TEXT("freeform-"), ESearchCase::CaseSensitive))
  {
   const FString Key = D.Id.Mid(9);
   if (F.HasFinding(Key)) AddFinding(Out, Key, F.FindingText(Key), F.Seed(), Have);
  }
 if (F.Townsfolk)
 {
  for (const FString& T : F.Townsfolk->ToldKeys()) Out.Keys.Add(TEXT("told:") + T);
  for (const FString& Who : FS47Townsfolk::People()) if (F.Townsfolk->Away(Who)) Out.Keys.Add(TEXT("gone:") + Who);
 }
 if (F.Visitors && F.Visitors->BacaGone()) Out.Keys.Add(TEXT("gone:baca"));
 if (F.Tips)
 {
  for (const FS47RadioTips::FCall& C : FS47RadioTips::AmCalls()) if (C.Id && F.Tips->Said().Contains(FString(C.Id))) { Out.Keys.Add(TEXT("earl:call")); break; }
  if (!F.Tips->Given().IsEmpty()) Out.Keys.Add(TEXT("earl:tip"));
 }
 return Out;
}

ES47Allowed TierOf(const FString& Fact)
{
 if (Starts(Fact, TEXT("near:")) || Starts(Fact, TEXT("figure_near_d")) || Starts(Fact, TEXT("humor_pump_d"))) return Full;
 if (Starts(Fact, TEXT("rise:")) || Starts(Fact, TEXT("glimpse:")) || Starts(Fact, TEXT("horizon_figure_d")) || Starts(Fact, TEXT("road_glimpse_d"))) return Glimpse;
 return Hint;
}

bool NamesKind(const FString& Text)
{
 static const TArray<FString> Kinds = { TEXT("alien"), TEXT("aliens"), TEXT("zet"), TEXT("zeta"), TEXT("zetas"), TEXT("reticulan"), TEXT("nordic"), TEXT("nordics"), TEXT("cosmic"),
  TEXT("sasquatch"), TEXT("bigfoot"), TEXT("yeti"), TEXT("crawler"), TEXT("reptilian"), TEXT("reptoid"), TEXT("mantis"), TEXT("humanoid"), TEXT("extraterrestrial"),
  TEXT("martian"), TEXT("martians"), TEXT("ufo"), TEXT("ufos"), TEXT("greys"), TEXT("grays"), TEXT("saucer"), TEXT("visitor"), TEXT("visitors") };
 FString Word;
 const auto Check = [&Word]() { const bool b = Kinds.Contains(Word); Word.Reset(); return b; };
 for (const TCHAR C : Text.ToLower())
 {
  if (FChar::IsAlpha(C)) Word.AppendChar(C);
  else if (!Word.IsEmpty() && Check()) return true;
 }
 return !Word.IsEmpty() && Check();
}

void Fill(const FFacts& Facts, TArray<FS47NbScribble>& Out)
{
 TArray<bool> Struck;
 const TArray<int32> Rows = Shown(Facts, &Struck);
 for (int32 N = 0; N < Rows.Num(); ++N)
 {
  const FRow& R = Table()[Rows[N]];
  // The spoiler limits, and no word for what they are on THEM's page: a line that fails is left out, never shown.
  if (!S47Script::LintText(Whole(R)).IsEmpty() || (R.Side == Them && NamesKind(Whole(R)))) continue;
  FS47NbScribble S;
  S.Side = R.Side == People ? TEXT("people") : TEXT("them");
  S.Box = R.Box; S.Role = R.Role; S.Text = R.Text; S.bStruck = Struck[N];
  // A guess proved wrong points nowhere any more.
  if (!S.bStruck && R.Follow != NoFollow) { S.Follow = TEXT("map"); S.FollowText = R.FollowText; }
  Out.Add(S);
 }
}

bool SelfTest(TArray<FString>& Lines)
{
 bool bAll = true;
 const auto Check = [&](bool bOk, const FString& What) { bAll &= bOk; Lines.Add(FString(bOk ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("marginalia: ") + What); };
 const TArray<FRow>& T = Table();
 const auto Only = [](std::initializer_list<const TCHAR*> Keys) { FFacts F; for (const TCHAR* K : Keys) F.Keys.Add(K); return F; };
 const auto Ids = [&T](const FFacts& F) { TArray<FString> Out; for (const int32 I : Shown(F)) Out.Add(T[I].Id); return Out; };
 int32 NPeople = 0, NThem = 0;
 for (const FRow& R : T) (R.Side == People ? NPeople : NThem) += 1;
 Check(T.Num() >= 18 && T.Num() <= 26 && NPeople >= 8 && NThem >= 10, FString::Printf(TEXT("about twenty scribbles, both pages (%d people, %d them)"), NPeople, NThem));
 // Nothing without its trigger: no facts, nothing; every other find there but this row's, and this row is not shown; with its own
 // find too, it is.
 TArray<FS47NbScribble> None; Fill(FFacts(), None);
 bool bOwn = None.IsEmpty();
 FFacts Every;
 for (const FRow& R : T) if (!Starts(R.Trigger, TEXT("gone:"))) Every.Keys.Add(FString(R.Trigger) + TEXT("1"));
 for (int32 I = 0; I < T.Num(); ++I)
 {
  if (Starts(T[I].Trigger, TEXT("gone:"))) continue;
  FFacts Without = Every;
  for (auto It = Without.Keys.CreateIterator(); It; ++It) if (It->StartsWith(T[I].Trigger, ESearchCase::CaseSensitive)) It.RemoveCurrent();
  bOwn &= !Shown(Without).Contains(I) && Shown(Every).Contains(I);
 }
 Check(bOwn, TEXT("no scribble shows before what unlocks it has happened in this game"));
 // The tiers: no row holds more than its find showed, and finds of lights and sounds unlock nothing of a shape or of near.
 bool bTier = true;
 for (const FRow& R : T) bTier &= uint8(R.Tier) <= uint8(TierOf(R.Trigger));
 FFacts Hints = Only({ TEXT("landing_seen_d3"), TEXT("lights_trace_landing_d3"), TEXT("clock_ahead_d2"), TEXT("tracks:low"), TEXT("dale:talk"), TEXT("told:deputy:chat:1") });
 for (const int32 I : Shown(Hints)) bTier &= T[I].Tier == Hint;
 bTier &= Ids(Only({ TEXT("rise:nordic") })) == TArray<FString>{ TEXT("hiker") } && !Ids(Only({ TEXT("rise:nordic") })).Contains(TEXT("silver"));
 Check(bTier, TEXT("each scribble keeps to its tier: lights and sounds, a shape far off, near only after near"));
 // The spoiler limits, and nothing said of how anyone went.
 bool bLint = true;
 for (const FRow& R : T)
 {
  bLint &= S47Script::LintText(Whole(R)).IsEmpty();
  for (const TCHAR* W : { TEXT("taken"), TEXT("took"), TEXT("abduct"), TEXT("lifted"), TEXT("beam"), TEXT("Tomas"), TEXT("1947"), TEXT("cable") }) bLint &= !Whole(R).Contains(W);
 }
 Check(bLint, TEXT("every scribble keeps to Freeform's spoiler limits (S47Script::LintText), and none says how anyone went"));
 // Wrong guesses: there until a find says otherwise, then struck through, pointing nowhere.
 {
  TArray<FS47NbScribble> A, B;
  Fill(Only({ TEXT("rise:nordic") }), A);
  Fill(Only({ TEXT("rise:nordic"), TEXT("near:nordic") }), B);
  const FS47NbScribble* Guess = B.FindByPredicate([](const FS47NbScribble& S) { return S.Text.Contains(TEXT("Hiker")); });
  TArray<FS47NbScribble> L1, L2;
  Fill(Only({ TEXT("landing_seen_d2") }), L1);
  Fill(Only({ TEXT("landing_seen_d2"), TEXT("lights_trace_landing_d2") }), L2);
  const auto Balloon = [](const TArray<FS47NbScribble>& L) { const FS47NbScribble* S = L.FindByPredicate([](const FS47NbScribble& X) { return X.Text.Contains(TEXT("balloon. Has")); }); return S; };
  Check(A.Num() == 1 && !A[0].bStruck && Guess && Guess->bStruck && B.Num() == 2 && Balloon(L1) && !Balloon(L1)->bStruck && Balloon(L2) && Balloon(L2)->bStruck && Balloon(L2)->Follow.IsEmpty(),
   TEXT("a wrong guess stays until a later find says otherwise, then it is struck through"));
 }
 // THEM by the player's own names only.
 bool bNames = NamesKind(TEXT("a Nordic on the rise")) && NamesKind(TEXT("Zet at the pump")) && !NamesKind(TEXT("THE SILVER MAN"));
 for (const FRow& R : T) if (R.Side == Them) bNames &= !NamesKind(Whole(R));
 Check(bNames, TEXT("no word for what they are on THEM's page, only the player's names for them"));
 // Someone gone is only gone.
 Check(Ids(Only({ TEXT("told:deputy:chat:1"), TEXT("deputy-round-day3"), TEXT("gone:deputy") })).IsEmpty()
  && Ids(Only({ TEXT("baca:told"), TEXT("baca_look_windmill"), TEXT("gone:baca") })) == TArray<FString>{ TEXT("baca_gone") },
  TEXT("someone gone has only the line that says so"));
 // The finds into facts: the road's two bodies, the steps, the shade on the rise without Tom's figures in the build.
 {
  FFacts F;
  const auto NoAsset = [](const TCHAR*) { return false; };
  AddFinding(F, TEXT("road_glimpse_d5"), S47RoadGlimpse::Record(S47RoadGlimpse::FPlan{ TEXT("crawler"), S47RoadGlimpse::EBody::Low }, TEXT("00:40")), 7, NoAsset);
  AddFinding(F, TEXT("road_tracks_d6"), S47RoadGlimpse::TracksRecord(true, TEXT("09:10"), true), 7, NoAsset);
  AddFinding(F, TEXT("figure_near_d4"), TEXT("At 02:30, out on the flat: I saw nothing. There were steps in the gravel, and then one close behind me."), 7, NoAsset);
  AddFinding(F, TEXT("horizon_figure_d3"), TEXT("At 00:20 a figure stood on the rise to the east, a long way off, dead still."), 7, NoAsset);
  AddFinding(F, TEXT("baca_look_mine"), TEXT("Eloy Baca says (unconfirmed): ..."), 7, NoAsset);
  AddFinding(F, TEXT("baca_gone_4"), TEXT("The Baca place"), 7, NoAsset);
  Check(F.Keys.Contains(TEXT("glimpse:low")) && F.Keys.Contains(TEXT("tracks:low")) && F.Keys.Contains(TEXT("near:steps")) && F.Keys.Contains(TEXT("rise:shade")) && F.Keys.Contains(TEXT("baca:told"))
   && !F.Keys.Contains(TEXT("glimpse:tall")) && !F.Has(TEXT("gone:")) && Ids(F).Contains(TEXT("coyote")),
   TEXT("the finds read as they were kept: what crossed, its prints, the steps, the shade on the rise, Baca's tales"));
 }
 return bAll;
}
}
