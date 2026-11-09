#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
struct FS47Sound;

// TAPE-20261108 (Tom, 8 November: "kassettspiller og opptak ... noen ganger er det noe på båndet du ikke hørte"). The portable recorder on
// the desk by the array's PC is the station's field recorder. Take it off the desk, record with U (S47Keys "tape") out in the night, bring it back and
// play the cassette on the desk.
//
// What is on the tape is what really sounded near the microphone while it ran: every recording the game played there, at its
// distance, under cassette hiss and the wind. Sometimes there is more. When something was out there (FS47Omen: a real source, never a
// roll) within what a microphone picks up, but too far or too faint for the player to notice anything (no flicker, no silence), the
// tape has it, at the moment it was nearest: a low hum, a pulse, something like voices too far off to make out. Nothing says what it
// is. Played back past that moment on the desk, it becomes a field record: the night log, the case file and Dr. Ward's assessment read
// it (FS47Predecessor counts the tapes with something on them). Freeform only; saved under "tape".
class FS47Tape
{
public:
 explicit FS47Tape(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;

 struct FSound{float At=0;FString Name;float Gain=1,Rate=1,Lp=0;};
 struct FTake
 {
  int32 N=0,Day=0;double Clock=0;FString Where;bool bOutdoors=true;float Seconds=0;
  double RoadX=0,RoadZ=0;bool bPlaced=false; // where it ran, road-local metres (the case board's place for it)
  TArray<FSound> Sounds;
  FString Odd;float OddAt=-1,OddGain=0; // what is on it that nobody heard: its sound, where on the tape, how loud
  bool bPlayed=false,bOddHeard=false;
 };
 // One side of a C-60: about two minutes as the player lives them, half an hour of the night's clock.
 static constexpr float SideSeconds=120;
 static constexpr int32 MostTakes=12,MostSounds=400;
 bool Carried() const{return bCarried;}
 bool Recording() const{return bRec;}
 bool Playing() const{return PlayIndex!=INDEX_NONE;}
 const TArray<FTake>& Takes() const{return Kept;}
 // Tapes played back with something on them nobody heard out there: what Dr. Ward's assessment counts.
 int32 OddHeardCount() const;
 // The key (S47Keys "tape"): start or stop recording. False: nothing to record with.
 bool ToggleRecord();
 // The desk: take the recorder, put it back, play a cassette, stop it.
 void UseDesk();
 bool PlayTake(int32 Index);
 void StopPlay();
 // The microphone: US47Audio calls this for every recording it plays while the tape runs (US47Audio::Tap).
 void Heard(const FString& Name,const FS47Sound& O);

 // ---------- the pure rules (tested) ----------
 // How loud a sound this far from the microphone is on the tape (0 to 1 of its own gain): close is as loud, far is faint, past 70 m nothing.
 static float GainAt(double Metres);
 // The tape has what the player did not notice: something within the microphone's reach (Sense, widened) while the player's own
 // reading (Level, what makes the lamp flicker) stayed low.
 static bool Unheard(double Sense,double Level);
 // What it sounds like on the tape, from the kind of thing it was: a buffer the game has. Never words.
 static FString OddSound(const FString& SourceId);
 static FString OddWords(const FString& Sound);
 // F10 and tests.
 void DevGive();
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 bool bCarried=false,bRec=false,bHooked=false,bKeyed=false;
 TArray<FTake> Kept;
 int32 Next=1;
 // The take being recorded.
 FTake Rec;
 double RecT=0,SenseTry=0,BestSense=0;
 FString BestSource;
 // The take being played on the desk.
 int32 PlayIndex=INDEX_NONE,PlayNext=0;
 double PlayT=0;
 bool bOddPlayed=false;
 TSharedPtr<uint8> Life;
 bool Hook();
 void Show();
 FVector Desk() const;
 FVector Ear() const;
 void Finish();
 void OpenDesk();
 FString PlaceTitle() const;
};
