#include "S47Keys.h"
#include "S47Gamepad.h"

#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Dom/JsonValue.h"
#include "GameFramework/PlayerController.h"

namespace S47KeysLocal
{
	TArray<S47Keys::FAction> MakeActions()
	{
		// ui/UI.ts CONTROLS (desk): the same keys, one action a row
		auto A = [](const TCHAR* Id, const TCHAR* Label, const TCHAR* Group, const TCHAR* Help, FKey K0, FKey K1 = EKeys::Invalid, bool bFixed = false)
		{
			S47Keys::FAction X;
			X.Id = Id; X.Label = Label; X.Group = Group; X.Help = Help;
			X.Default[0] = K0; X.Default[1] = K1; X.bFixedFirst = bFixed;
			return X;
		};
		const TCHAR* Foot = TEXT("On foot");
		const TCHAR* Panels = TEXT("Notebook and pause");
		const TCHAR* Truck = TEXT("In the truck");
		return {
			A(TEXT("forward"), TEXT("Walk forward"), Foot, TEXT("Walk forward. In the truck: the throttle."), EKeys::W, EKeys::Up),
			A(TEXT("back"), TEXT("Walk back"), Foot, TEXT("Walk back. In the truck: the brake, and reverse once it has stopped."), EKeys::S, EKeys::Down),
			A(TEXT("left"), TEXT("Step left"), Foot, TEXT("Step to the left. In the truck: steer left."), EKeys::A, EKeys::Left),
			A(TEXT("right"), TEXT("Step right"), Foot, TEXT("Step to the right. In the truck: steer right."), EKeys::D, EKeys::Right),
			A(TEXT("run"), TEXT("Run"), Foot, TEXT("Hold to run."), EKeys::LeftShift, EKeys::RightShift),
			// (Tom 22 October 2026: a jump on foot; the web version has none. Space, as the shutter has it:
			// the two are never wanted at once, S47Keys::MayShare)
			A(TEXT("jump"), TEXT("Jump"), Foot, TEXT("Jump. With the camera raised the same key is the shutter."), EKeys::SpaceBar),
			A(TEXT("use"), TEXT("Use"), Foot, TEXT("Use what the crosshair is on. Put a paper back. With the camera raised it is the shutter; in the truck it turns the key, works the radio or gets you out. A left click does the same."), EKeys::E, EKeys::Enter),
			A(TEXT("alt"), TEXT("Second choice / takeaway"), Foot, TEXT("Manage takeaway on foot: set down, stow, take out or discard. At a prompt, choose the other action; in the truck, get out while Use turns the key."), EKeys::Q),
			A(TEXT("zoom"), TEXT("Closer look"), Foot, TEXT("Hold for a closer look at what the crosshair is on."), EKeys::RightMouseButton),
			A(TEXT("describe"), TEXT("Read description"), Foot, TEXT("Read the current or last description. Press again to stop."), EKeys::V),
			A(TEXT("light"), TEXT("Flashlight"), Foot, TEXT("The flashlight, once you have it."), EKeys::F),
			A(TEXT("camera"), TEXT("Field camera"), Foot, TEXT("Raise or lower the field camera."), EKeys::C),
			A(TEXT("shutter"), TEXT("Shutter"), Foot, TEXT("Take the picture, with the camera raised."), EKeys::SpaceBar),
			A(TEXT("radio"), TEXT("Truck radio"), Truck, TEXT("The radio in the cab: on, the next station, off."), EKeys::R),
			A(TEXT("notebook"), TEXT("Notebook"), Panels, TEXT("The notebook: tonight's tasks, notes, papers and photographs."), EKeys::Tab, EKeys::N),
			A(TEXT("map"), TEXT("Notebook map"), Panels, TEXT("The notebook, open at its map."), EKeys::M),
			A(TEXT("kit"), TEXT("Notebook kit"), Panels, TEXT("The notebook, open at the kit you carry."), EKeys::I),
			A(TEXT("controls"), TEXT("Notebook controls"), Panels, TEXT("The notebook, open at its page of controls."), EKeys::H),
			A(TEXT("board"), TEXT("Case board"), Panels, TEXT("The case board, once there is one."), EKeys::B),
			A(TEXT("pause"), TEXT("Pause"), Panels, TEXT("Pause: settings, save and load. Escape always pauses, and lowers the camera first when it is raised."), EKeys::Escape, EKeys::P, true),
		};
	}

	struct FState
	{
		TArray<S47Keys::FAction> Actions = MakeActions();
		TMap<FName, TStaticArray<FKey, 2>> Map;
		uint32 Changes = 0;     // counted up whenever a key changes (S47Keys::Changes)
		FState() { for (const S47Keys::FAction& A : Actions) { TStaticArray<FKey, 2> K; K[0] = A.Default[0]; K[1] = A.Default[1]; Map.Add(A.Id, K); } }
	};
	FState& St() { static FState S; return S; }

	TStaticArray<FKey, 2>* Slots(FName Action) { return St().Map.Find(Action); }

	// no key twice: a later duplicate goes (the actions' order), unless every action before it with
	// that key may share it; a fixed first key stays as it is
	void Normalize()
	{
		TMap<FKey, TArray<FName>> Seen;
		for (const S47Keys::FAction& A : St().Actions)
		{
			TStaticArray<FKey, 2>& K = St().Map.FindChecked(A.Id);
			if (A.bFixedFirst) K[0] = A.Default[0];
			for (int32 S = 0; S < 2; ++S)
			{
				if (!K[S].IsValid()) continue;
				const TArray<FName>* Had = Seen.Find(K[S]);
				const bool bTaken = Had && Had->ContainsByPredicate([&A](FName B) { return !S47Keys::MayShare(A.Id, B); });
				if (bTaken || (!(A.bFixedFirst && S == 0) && !S47Keys::Bindable(K[S]))) K[S] = EKeys::Invalid;
				else Seen.FindOrAdd(K[S]).AddUnique(A.Id);
			}
		}
	}
}

bool S47Keys::MayShare(FName A, FName B)
{
	// the shutter is asked only while the camera is raised, a jump only while it is not
	static const FName Shutter(TEXT("shutter")), Jump(TEXT("jump"));
	return (A == Shutter && B == Jump) || (A == Jump && B == Shutter);
}

const TArray<S47Keys::FAction>& S47Keys::Actions() { return S47KeysLocal::St().Actions; }

const S47Keys::FAction* S47Keys::Find(FName Id) { return S47KeysLocal::St().Actions.FindByPredicate([Id](const FAction& A) { return A.Id == Id; }); }

FKey S47Keys::Key(FName Action, int32 Slot)
{
	const TStaticArray<FKey, 2>* K = S47KeysLocal::Slots(Action);
	return K && Slot >= 0 && Slot < 2 ? (*K)[Slot] : EKeys::Invalid;
}

TArray<FKey, TInlineAllocator<2>> S47Keys::KeysOf(FName Action)
{
	TArray<FKey, TInlineAllocator<2>> Out;
	if (const TStaticArray<FKey, 2>* K = S47KeysLocal::Slots(Action)) for (const FKey& X : *K) if (X.IsValid()) Out.Add(X);
	const FKey Pad = S47Gamepad::Key(Action);
	if (Pad.IsValid()) Out.Add(Pad);
	if (Action == TEXT("notebook")) Out.Add(EKeys::Gamepad_LeftShoulder);
	return Out;
}

bool S47Keys::Is(FName Action, const FKey& K)
{
	const TStaticArray<FKey, 2>* S = S47KeysLocal::Slots(Action);
	return K.IsValid() && ((S && ((*S)[0] == K || (*S)[1] == K)) || S47Gamepad::Key(Action) == K || (Action == TEXT("notebook") && K == EKeys::Gamepad_LeftShoulder));
}

bool S47Keys::Down(const APlayerController* PC, FName Action)
{
	if (!PC) return false;
	for (const FKey& K : KeysOf(Action)) if (K.IsGamepadKey() ? S47Gamepad::Down(K) : PC->IsInputKeyDown(K)) return true;
	return false;
}

bool S47Keys::Pressed(const APlayerController* PC, FName Action)
{
	if (!PC) return false;
	for (const FKey& K : KeysOf(Action)) if (WasPressed(PC, K)) return true;
	return false;
}

bool S47Keys::WasPressed(const APlayerController* PC, const FKey& K)
{
	return K.IsGamepadKey() ? S47Gamepad::Pressed(K) : PC && PC->WasInputKeyJustPressed(K);
}

FString S47Keys::Short(const FKey& K)
{
	if (!K.IsValid()) return FString();
	static const TMap<FKey, FString> Names = {
		{ EKeys::LeftShift, TEXT("SHIFT") }, { EKeys::RightShift, TEXT("R SHIFT") }, { EKeys::LeftControl, TEXT("CTRL") }, { EKeys::RightControl, TEXT("R CTRL") },
		{ EKeys::LeftAlt, TEXT("ALT") }, { EKeys::RightAlt, TEXT("R ALT") }, { EKeys::SpaceBar, TEXT("SPACE") }, { EKeys::Enter, TEXT("ENTER") },
		{ EKeys::Escape, TEXT("ESC") }, { EKeys::Tab, TEXT("TAB") }, { EKeys::BackSpace, TEXT("BKSP") }, { EKeys::Delete, TEXT("DEL") },
		{ EKeys::Up, TEXT("UP") }, { EKeys::Down, TEXT("DOWN") }, { EKeys::Left, TEXT("LEFT") }, { EKeys::Right, TEXT("RIGHT") },
		{ EKeys::LeftMouseButton, TEXT("CLICK") }, { EKeys::RightMouseButton, TEXT("RMB") }, { EKeys::MiddleMouseButton, TEXT("MMB") },
		{ EKeys::ThumbMouseButton, TEXT("MOUSE 4") }, { EKeys::ThumbMouseButton2, TEXT("MOUSE 5") }, { EKeys::CapsLock, TEXT("CAPS") },
		{ EKeys::PageUp, TEXT("PG UP") }, { EKeys::PageDown, TEXT("PG DN") }, { EKeys::Insert, TEXT("INS") },
	};
	if (const FString* N = Names.Find(K)) return *N;
	return K.GetDisplayName(false).ToString().ToUpper();
}

FString S47Keys::Name(const FKey& K)
{
	if (!K.IsValid()) return FString();
	static const TMap<FKey, FString> Names = {
		{ EKeys::LeftShift, TEXT("Shift") }, { EKeys::RightShift, TEXT("right Shift") }, { EKeys::SpaceBar, TEXT("Space") },
		{ EKeys::LeftMouseButton, TEXT("a click") }, { EKeys::RightMouseButton, TEXT("right mouse") }, { EKeys::MiddleMouseButton, TEXT("the middle mouse button") },
		{ EKeys::Up, TEXT("Up") }, { EKeys::Down, TEXT("Down") }, { EKeys::Left, TEXT("Left") }, { EKeys::Right, TEXT("Right") },
	};
	if (const FString* N = Names.Find(K)) return *N;
	return K.GetDisplayName(false).ToString();
}

FString S47Keys::Label(FName Action)
{
	if (S47Gamepad::UsingGamepad()) { const FString Pad = S47Gamepad::Label(Action); if (!Pad.IsEmpty()) return Pad; }
	// KeysOf also contains fixed controller aliases; keyboard hints use only the saved slots.
	for (int32 Slot = 0; Slot < 2; ++Slot)
	{
		const FKey K = Key(Action, Slot);
		if (K.IsValid()) return Short(K);
	}
	return FString();
}

FString S47Keys::Said(FName Action)
{
	if (S47Gamepad::UsingGamepad()) { const FString Pad = S47Gamepad::Label(Action); if (!Pad.IsEmpty()) return Pad; }
	for (int32 Slot = 0; Slot < 2; ++Slot)
	{
		const FKey K = Key(Action, Slot);
		if (K.IsValid()) return Name(K);
	}
	return FString();
}

bool S47Keys::Bindable(const FKey& K)
{
	// (the console's key, the wheel and the mouse's axes, the gamepad and touch are not offered)
	if (!K.IsValid() || K == EKeys::Escape || K == EKeys::LeftMouseButton || K == EKeys::Tilde || K == EKeys::AnyKey) return false;
	if (K.IsGamepadKey() || K.IsTouch() || K.IsAxis1D() || K.IsAxis2D() || K.IsAxis3D()) return false;
	if (K == EKeys::MouseScrollUp || K == EKeys::MouseScrollDown || K == EKeys::MouseWheelAxis) return false;
	return true;
}

S47Keys::EBind S47Keys::Bind(FName Action, int32 Slot, const FKey& K, FName* Other)
{
	const FAction* A = Find(Action);
	TStaticArray<FKey, 2>* S = S47KeysLocal::Slots(Action);
	if (!A || !S || Slot < 0 || Slot > 1) return EBind::Reserved;
	if (Slot == 0 && A->bFixedFirst) return EBind::Fixed;
	if (!Bindable(K)) return EBind::Reserved;
	const FKey Old = (*S)[Slot];
	if (Old == K) return EBind::Same;
	// the action's other key: the two change places (not a fixed one)
	if ((*S)[1 - Slot] == K)
	{
		if (1 - Slot == 0 && A->bFixedFirst) return EBind::Fixed;
		(*S)[1 - Slot] = Old; (*S)[Slot] = K;
		++S47KeysLocal::St().Changes;
		if (Other) *Other = Action;
		return EBind::Swapped;
	}
	// the actions that have K now: those that may share it with this one keep it (Shared); from the
	// others it is taken, the first getting this one's old key in exchange (when no action it cannot
	// share a key with has that key), the rest none
	struct FHolder { FName Id; int32 Slot; };
	TArray<FHolder> Takes;
	FName Sharer;
	for (const FAction& B : S47KeysLocal::St().Actions)
	{
		if (B.Id == Action) continue;
		const TStaticArray<FKey, 2>& T = S47KeysLocal::St().Map.FindChecked(B.Id);
		for (int32 t = 0; t < 2; ++t)
		{
			if (T[t] != K) continue;
			if (MayShare(Action, B.Id)) { if (Sharer.IsNone()) Sharer = B.Id; continue; }
			if (t == 0 && B.bFixedFirst) return EBind::Fixed;
			Takes.Add({ B.Id, t });
		}
	}
	(*S)[Slot] = K;
	++S47KeysLocal::St().Changes;
	if (Takes.IsEmpty())
	{
		if (Other) *Other = Sharer;
		return Sharer.IsNone() ? EBind::Done : EBind::Shared;
	}
	bool bGave = false;
	for (int32 i = 0; i < Takes.Num(); ++i)
	{
		TStaticArray<FKey, 2>& T = S47KeysLocal::St().Map.FindChecked(Takes[i].Id);
		bool bFree = i == 0 && Old.IsValid();
		if (bFree)
		{
			for (const TPair<FName, TStaticArray<FKey, 2>>& P : S47KeysLocal::St().Map)
			{
				if (P.Key == Takes[i].Id || MayShare(P.Key, Takes[i].Id)) continue;
				if (P.Value[0] == Old || P.Value[1] == Old) { bFree = false; break; }
			}
		}
		T[Takes[i].Slot] = bFree ? Old : FKey(EKeys::Invalid);
		bGave |= bFree;
	}
	if (Other) *Other = Takes[0].Id;
	return bGave ? EBind::Swapped : EBind::Moved;
}

void S47Keys::Clear(FName Action, int32 Slot)
{
	const FAction* A = Find(Action);
	TStaticArray<FKey, 2>* S = S47KeysLocal::Slots(Action);
	if (!A || !S || Slot < 0 || Slot > 1 || (Slot == 0 && A->bFixedFirst)) return;
	(*S)[Slot] = EKeys::Invalid;
	++S47KeysLocal::St().Changes;
}

void S47Keys::Reset(FName Action)
{
	const FAction* A = Find(Action);
	TStaticArray<FKey, 2>* S = S47KeysLocal::Slots(Action);
	if (!A || !S) return;
	for (int32 i = 0; i < 2; ++i)
	{
		const FKey D = A->Default[i];
		if (D.IsValid())
		{
			// taken from whichever action has it now (not one that may share it)
			for (TPair<FName, TStaticArray<FKey, 2>>& P : S47KeysLocal::St().Map)
			{
				if (P.Key == Action || MayShare(P.Key, Action)) continue;
				for (FKey& X : P.Value) if (X == D) X = EKeys::Invalid;
			}
		}
		(*S)[i] = D;
	}
	++S47KeysLocal::St().Changes;
}

void S47Keys::ResetAll()
{
	for (const FAction& A : S47KeysLocal::St().Actions) { TStaticArray<FKey, 2>& K = S47KeysLocal::St().Map.FindChecked(A.Id); K[0] = A.Default[0]; K[1] = A.Default[1]; }
	++S47KeysLocal::St().Changes;
}

bool S47Keys::IsDefault(FName Action)
{
	const FAction* A = Find(Action);
	const TStaticArray<FKey, 2>* S = S47KeysLocal::Slots(Action);
	return A && S && (*S)[0] == A->Default[0] && (*S)[1] == A->Default[1];
}

TSharedPtr<FJsonObject> S47Keys::Save()
{
	TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
	for (const FAction& A : S47KeysLocal::St().Actions)
	{
		const TStaticArray<FKey, 2>& K = S47KeysLocal::St().Map.FindChecked(A.Id);
		TArray<TSharedPtr<FJsonValue>> V;
		for (const FKey& X : K) V.Add(MakeShared<FJsonValueString>(X.IsValid() ? X.GetFName().ToString() : FString()));
		O->SetArrayField(A.Id.ToString(), V);
	}
	return O;
}

void S47Keys::Load(const TSharedPtr<FJsonObject>& O)
{
	ResetAll();
	if (!O.IsValid()) return;
	for (const FAction& A : S47KeysLocal::St().Actions)
	{
		const TArray<TSharedPtr<FJsonValue>>* V = nullptr;
		if (!O->TryGetArrayField(A.Id.ToString(), V) || !V || V->Num() != 2) continue;
		TStaticArray<FKey, 2>& K = S47KeysLocal::St().Map.FindChecked(A.Id);
		for (int32 i = 0; i < 2; ++i)
		{
			FString Name;
			if (!(*V)[i].IsValid() || !(*V)[i]->TryGetString(Name)) continue;
			if (Name.IsEmpty()) { K[i] = EKeys::Invalid; continue; }
			const FKey X(*Name);
			if (X.IsValid()) K[i] = X;
		}
	}
	S47KeysLocal::Normalize();
	++S47KeysLocal::St().Changes;
}

uint32 S47Keys::Changes() { return S47KeysLocal::St().Changes + S47Gamepad::Changes(); }

bool S47Keys::SelfTest(TArray<FString>& Out)
{
	bool bOk = true;
	auto Check = [&](bool bPass, const TCHAR* What) { Out.Add(FString::Printf(TEXT("%s keys: %s"), bPass ? TEXT("ok  ") : TEXT("FAIL"), What)); bOk &= bPass; };
	const TSharedPtr<FJsonObject> Before = Save();
	ResetAll();
	Check(Key(TEXT("forward"), 0) == EKeys::W && Key(TEXT("forward"), 1) == EKeys::Up && Is(TEXT("use"), EKeys::Enter) && !Is(TEXT("use"), EKeys::LeftMouseButton), TEXT("the web version's keys"));
	Check(Label(TEXT("shutter")) == TEXT("SPACE") && Said(TEXT("shutter")) == TEXT("Space") && Label(TEXT("zoom")) == TEXT("RMB"), TEXT("names"));
	// a key another action has: the two exchange
	FName Other;
	Check(Bind(TEXT("light"), 0, EKeys::C, &Other) == EBind::Swapped && Other == FName(TEXT("camera")) && Key(TEXT("camera"), 0) == EKeys::F && Key(TEXT("light"), 0) == EKeys::C, TEXT("swap with the camera"));
	Check(Bind(TEXT("light"), 1, EKeys::G) == EBind::Done && Is(TEXT("light"), EKeys::G), TEXT("a free key"));
	Check(Bind(TEXT("light"), 1, EKeys::C) == EBind::Swapped && Key(TEXT("light"), 0) == EKeys::G && Key(TEXT("light"), 1) == EKeys::C, TEXT("its own other key"));
	Check(Bind(TEXT("use"), 0, EKeys::Escape) == EBind::Reserved && Bind(TEXT("use"), 0, EKeys::LeftMouseButton) == EBind::Reserved && Bind(TEXT("use"), 0, EKeys::Gamepad_FaceButton_Bottom) == EBind::Reserved, TEXT("reserved keys"));
	Check(Bind(TEXT("pause"), 0, EKeys::K) == EBind::Fixed && Key(TEXT("pause"), 0) == EKeys::Escape, TEXT("Escape stays"));
	// kept and read back; duplicates in a hand-edited file go
	const TSharedPtr<FJsonObject> Saved = Save();
	ResetAll();
	Load(Saved);
	Check(Key(TEXT("light"), 0) == EKeys::G && Key(TEXT("camera"), 0) == EKeys::F, TEXT("saved and read back"));
	TSharedPtr<FJsonObject> Dup = MakeShared<FJsonObject>();
	Dup->SetArrayField(TEXT("map"), { MakeShared<FJsonValueString>(TEXT("W")), MakeShared<FJsonValueString>(TEXT("")) });
	Load(Dup);
	Check(Key(TEXT("forward"), 0) == EKeys::W && !Key(TEXT("map"), 0).IsValid(), TEXT("a duplicate in the file goes"));
	Reset(TEXT("map"));
	Check(Key(TEXT("map"), 0) == EKeys::M && IsDefault(TEXT("map")), TEXT("reset"));
	// a jump and the shutter share Space: the camera decides which it is
	ResetAll();
	Check(Is(TEXT("jump"), EKeys::SpaceBar) && Is(TEXT("shutter"), EKeys::SpaceBar) && Label(TEXT("jump")) == TEXT("SPACE"), TEXT("Space is a jump and the shutter"));
	Check(Bind(TEXT("jump"), 0, EKeys::J) == EBind::Done && Is(TEXT("shutter"), EKeys::SpaceBar) && !Is(TEXT("jump"), EKeys::SpaceBar), TEXT("a jump elsewhere leaves the shutter"));
	Other = NAME_None;
	Check(Bind(TEXT("jump"), 0, EKeys::SpaceBar, &Other) == EBind::Shared && Other == FName(TEXT("shutter")) && Is(TEXT("shutter"), EKeys::SpaceBar), TEXT("back on Space, shared"));
	// (a third takes it from both: the first in the list gets its old key, the other none)
	Check(Bind(TEXT("light"), 0, EKeys::SpaceBar, &Other) == EBind::Swapped && Other == FName(TEXT("jump")) && Key(TEXT("jump"), 0) == EKeys::F && !Key(TEXT("shutter"), 0).IsValid() && Is(TEXT("light"), EKeys::SpaceBar), TEXT("taken from both by a third"));
	Reset(TEXT("jump"));
	Check(Is(TEXT("jump"), EKeys::SpaceBar) && !Is(TEXT("light"), EKeys::SpaceBar) && !Is(TEXT("jump"), EKeys::F), TEXT("a jump's reset takes Space back"));
	ResetAll();
	Bind(TEXT("jump"), 0, EKeys::G);
	Check(Bind(TEXT("light"), 1, EKeys::G, &Other) == EBind::Moved && Other == FName(TEXT("jump")) && !Key(TEXT("jump"), 0).IsValid() && Key(TEXT("light"), 0) == EKeys::F, TEXT("into an empty slot: the other is left without"));
	// (the old key goes only where it can: the shutter's Space cannot go to the flashlight while a jump has it)
	ResetAll();
	Check(Bind(TEXT("shutter"), 0, EKeys::F, &Other) == EBind::Moved && Other == FName(TEXT("light")) && !Key(TEXT("light"), 0).IsValid() && Is(TEXT("jump"), EKeys::SpaceBar), TEXT("the shutter's old Space stays a jump's"));
	{
		TSharedPtr<FJsonObject> Both = MakeShared<FJsonObject>();
		Both->SetArrayField(TEXT("shutter"), { MakeShared<FJsonValueString>(TEXT("SpaceBar")), MakeShared<FJsonValueString>(TEXT("")) });
		Both->SetArrayField(TEXT("jump"), { MakeShared<FJsonValueString>(TEXT("SpaceBar")), MakeShared<FJsonValueString>(TEXT("")) });
		Both->SetArrayField(TEXT("describe"), { MakeShared<FJsonValueString>(TEXT("SpaceBar")), MakeShared<FJsonValueString>(TEXT("")) });
		Load(Both);
		Check(Is(TEXT("shutter"), EKeys::SpaceBar) && Is(TEXT("jump"), EKeys::SpaceBar) && !Is(TEXT("describe"), EKeys::SpaceBar), TEXT("a file with both keeps both, and no third"));
	}
	// A user may deliberately leave both keyboard slots empty while Xbox remains usable.
	Clear(TEXT("camera"), 0); Clear(TEXT("camera"), 1);
	Check(Label(TEXT("camera")).IsEmpty() && Said(TEXT("camera")).IsEmpty(), TEXT("fully unbound keyboard hints stay empty rather than showing an Xbox alias"));
	Check(Is(TEXT("camera"), EKeys::Gamepad_FaceButton_Left) && KeysOf(TEXT("camera")).Contains(EKeys::Gamepad_FaceButton_Left) && S47Gamepad::Label(TEXT("camera")) == TEXT("X"), TEXT("clearing keyboard slots preserves the fixed Xbox camera binding and label"));
	Check(Bind(TEXT("camera"), 1, EKeys::K) == EBind::Done && Label(TEXT("camera")) == TEXT("K") && Said(TEXT("camera")) == TEXT("K"), TEXT("keyboard hints fall through an empty first slot to the second saved key"));
	Load(Before);
	return bOk;
}

void S47Keys::TestPresses(const FString& Csv, float Every)
{
	TArray<FString> Names;
	Csv.ParseIntoArray(Names, TEXT(","));
	for (int32 i = 0; i < Names.Num(); ++i)
	{
		const FString Name = Names[i].TrimStartAndEnd();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Name](float)
		{
			const FKey K(*Name);
			if (Name.StartsWith(TEXT("Click:")) && FSlateApplication::IsInitialized() && GEngine && GEngine->GameViewport)
			{
				// the point in the game's picture, on the screen where its window is
				TArray<FString> P;
				Name.ParseIntoArray(P, TEXT(":"));
				const TSharedPtr<SWindow> W = GEngine->GameViewport->GetWindow();
				if (P.Num() == 3 && W.IsValid())
				{
					const FVector2D At = W->GetPositionInScreen() + FVector2D(FCString::Atof(*P[1]), FCString::Atof(*P[2]));
					FSlateApplication& App = FSlateApplication::Get();
					const TSet<FKey> None, Left = { EKeys::LeftMouseButton };
					App.ProcessMouseMoveEvent(FPointerEvent(0, At, At - FVector2D(1, 0), None, EKeys::Invalid, 0.f, App.GetModifierKeys()));
					App.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, At, At, Left, EKeys::LeftMouseButton, 0.f, App.GetModifierKeys()));
					App.ProcessMouseButtonUpEvent(FPointerEvent(0, At, At, None, EKeys::LeftMouseButton, 0.f, App.GetModifierKeys()));
				}
				UE_LOG(LogTemp, Display, TEXT("S47: pressed %s"), *Name);
				return false;
			}
			if (Name != TEXT("Wait") && K.IsValid() && FSlateApplication::IsInitialized())
			{
				FSlateApplication& App = FSlateApplication::Get();
				App.ProcessKeyDownEvent(FKeyEvent(K, App.GetModifierKeys(), 0, false, 0, 0));
				App.ProcessKeyUpEvent(FKeyEvent(K, App.GetModifierKeys(), 0, false, 0, 0));
			}
			UE_LOG(LogTemp, Display, TEXT("S47: pressed %s%s"), *Name, K.IsValid() || Name == TEXT("Wait") ? TEXT("") : TEXT(" (no such key)"));
			return false;
		}), Every * (i + 1));
	}
}
