#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47EveningPack.h"
class US47Main;
class FS47DaleVoice;

// STORY-ON-FREEFORM-01, step 5 (Docs/Design/Story/STORY_PA_FREEFORM_2026-11-11.md): the story's scene beat. A story script's
// {"do":"scene","scene":"D01_HANDOVER"} plays one scene of the authored EveningShift pack (FS47EveningPack, Narrative/EveningShift) on a
// Story night, on Freeform's own telephone, walkie or radio: a phone scene rings at OPS and opens when the player picks up (the "phone"
// prop, before Freeform's own use of it); a walkie or radio scene opens as soon as no other panel is up. Lines are shown one by one with
// Continue, choices as buttons; the pack's own rules (phase, requires, next, on_select, on_complete) decide what follows.
//
// Only one scene at a time. The scene's phase is set from the scene itself while the pack is idle, so a script may play scenes in any
// order the pack's requirements allow; a world scene (no lines, no choices: e.g. I11_SARO_ARRIVAL) completes at once and only sets its
// flags. When a scene is over, every pack flag that is set and is a valid story flag id becomes a story flag, and so does
// "scene_<id in lower case>": later story scripts wait on them with story_flag.
//
// STORY-DRIVE-20261112: the car radio and the face-to-face talk. A radio scene of the drive (channel radio, phase commute, no choices:
// I01_DRIVE, I10_BACK_ON_ROAD) is the truck's radio: its lines come as captions one after another while the player is in the truck and
// no panel is up, no panel of its own; out of the truck the rest plays on unheard and the scene is over (its effects hold). A physical
// scene (the diner's counter, I02..I09) ends where it stands when the player drives off (FS47EveningPack::Abandon); its flags so far
// and scene_<id> become story flags as usual.
//
// Dale is heard with Freeform's own voice (FS47DaleVoice, the "dale" bank = dale_de_design, never a clone); a line with no take is text.
// Everyone else is text. The save is {pack:<FS47EveningPack::Save>, ring:bool}; an active scene is taken up again (a phone scene rings).
class FS47StoryScene
{
public:
 explicit FS47StoryScene(US47Main* Main);
 ~FS47StoryScene();
 // The pack read and valid.
 bool Ready() const{return bReady;}
 // The pack has a scene by this id (for the lint; read once).
 static bool Known(const FString& Id);
 // Starts a scene. False with why: no pack, one is under way already, not a trigger scene, or the pack's requirements say no.
 bool Begin(const FString& Id,FString& Why);
 // A scene rings, is open, or waits to open.
 bool Busy() const;
 const FString& SceneId() const{return Active;}
 // The player used the OPS telephone: true when the story's call took it (picked up, or taken up again).
 bool UsePhone();
 void Tick(double Dt);
 void Stop();
 S47J::FObj Save() const;
 void Load(const S47J::FObj& Saved);
 // Step 6: the pack's external (ctx_*) flags from Freeform's world, told by the director each tick. Unknown ids are ignored.
 void SetContext(const TMap<FString,bool>& Ctx);
 // Called with each story flag a finished scene sets (the director's SetFlag).
 TFunction<void(const FString&)> OnFlag;

 // The panel's two actions, also the self-test's (no panel needed).
 bool Continue();
 bool Choose(const FString& ChoiceId);
 static bool SelfTest(TArray<FString>& Lines);
 // Supper at the Mesa on the Story drive, from Freeform's cash (Tom 12 Nov).
 static constexpr int32 MealPrice=4;
 // The scene under way is the truck's radio on the drive (see above).
 bool CarRadio() const;
private:
 US47Main* M=nullptr;
 FS47EveningPack Pack;
 TSharedPtr<FS47DaleVoice> Voice;
 FString Active,Channel,SpokenLine;
 bool bReady=false,bRing=false,bOpen=false,bWantOpen=false;
 double ReopenIn=0;
 double RadioHold=0;   // seconds the car radio's current caption has left
 int8 TestCab=-1;      // the self-test's "in the truck" (1 or 0) in place of US47World::bDriving; -1: the world's
 bool InCab() const;
 void RadioStep(double Dt);
 TSharedPtr<uint8> Life=MakeShared<uint8>(0);
 void StartRing();
 void StopRing();
 void Show();
 void Close();
 void AfterStep();
 // Step 6: the pack's papers a scene hands over (give_documents) go into Freeform's notebook as field records, once each.
 void Deliver();
 TSet<FString> Delivered;
 void Finish();
};
