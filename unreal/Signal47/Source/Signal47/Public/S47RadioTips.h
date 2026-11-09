#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// Freeform only: what comes over the air in the truck. The CB under the dash, and callers on 1420. Mostly nothing; a few of them
// point at a place on the land, or at what the player has been doing. They never explain anything. Text as the rest of the
// radio is, no recorded voices yet. More people tell more once the county and Caliche know the station (SheriffTrust).
class FS47RadioTips
{
public:
 explicit FS47RadioTips(US47Main* Main):M(Main){}
 struct FLine{const TCHAR* Id;const TCHAR* Text;const TCHAR* Place;const TCHAR* Needs;int32 Trust;};
 // Two speakers at most: a caller and Earl. Empty answer: he lets it go.
 struct FCall{const TCHAR* Id;const TCHAR* Who;const TCHAR* Text;const TCHAR* Answer;const TCHAR* Place;const TCHAR* Needs;int32 Trust;};
 static const TArray<FLine>& CbLines();
 static const TArray<FCall>& AmCalls();
 void Begin(const S47J::FObj& Saved);
 void Step(double Dt);
 S47J::FObj Save() const;
 static bool SelfTest(TArray<FString>& Lines);
 const TSet<FString>& Said() const{return Used;}
 // Developer menu: the next one now, whatever the waiting. False: there is none left that fits.
 bool DevCb();
 bool DevAm();
 // A CB line as said: once, and what it points at heard of. Called when its first word comes over the CB (CB-WALKIE-20261108).
 void MarkSaid(const FString& Id);

 // TOWN-USE-20261108: a tip the player gives 1420 AM, in the station's lobby or on the phone from SARO (S47TownUse). Earl reads it
 // on the air in his own turn on the programme, and a day or more after that somebody calls back about it: as often as not about
 // somewhere else, a place the player has not been told of, which is then heard of (a pencil question mark on the map). Heard on
 // the truck's radio when it is on 1420; otherwise Earl leaves a note at the lobby. One tip a day, each place once.
 struct FGiven{FString Place;int32 Day=0;int32 Stage=0;int32 ReadDay=0;FString Back,BackPlace;};
 bool CanGive() const;
 bool Gave(const FString& Place) const{return Tipped.ContainsByPredicate([&Place](const FGiven& G){return G.Place==Place;});}
 bool Give(const FString& Place);
 const TArray<FGiven>& Given() const{return Tipped;}
 // What came back while nobody was listening, as Earl's note at the lobby; the place it points at is heard of as it is read. Empty: none.
 FString TakeNotes();
 // Earl's words for a tip, and what a caller says back (a place the player has not heard of, by the game's seed; else nobody saw a thing).
 static FString OnAirLine(const FString& Title);
 // OutVoice: whose voice says it ("woman", "man", "old"), bound to the words (a woman's husband, the baby, "ma'am").
 FString CallBack(const FGiven& G,FString& OutPlace,FString& OutAnswer,FString* OutVoice=nullptr) const;
 // F10: the next waiting tip read now, or its call-back now, whatever the hour and the radio. What was said; empty: nothing waiting.
 FString DevNext();
private:
 US47Main* M=nullptr;
 TSet<FString> Used;
 TArray<FGiven> Tipped;
 // The next tip's turn on 1420 (StepAm), when there is one waiting: true if it took the slot.
 bool StepTipped(class FS47Station& Station);
 FString TitleOf(const FString& Place) const;
 // Its turn has come, said on the air or not: the call-back's place heard of.
 void Back(FGiven& G);
 double CbIn=40,AmIn=60;
 bool Ready(const TCHAR* Needs,int32 Trust) const;
 void StepCb();
 void StepAm();
};
