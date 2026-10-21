#pragma once

#include "CoreMinimal.h"
#include "S47Docs.h"
#include "UObject/Object.h"
#include "S47UI.generated.h"

class AS47HUD;
class SWidget;

// UI.viewfinder's options
struct FS47Viewfinder
{
	float X = 0, Y = 0, W = 0, H = 0; // the frame in screen pixels
	FString Frame, Status;
	bool bOk = false;
	bool bTouch = false;
	bool bHasFree = false;
	FString FreeText;
	bool bFreeOk = false;
};

// ui/UI.ts: what the game asks of the screen. The names follow the web version, so a call ports
// as it reads there (ui.toast('...', 4) is UI->Toast(TEXT("..."), 4)). One modal at a time:
// a paper, a panel, the notebook or the board; game time stops while one is open.
UCLASS()
class SIGNAL47_API US47UI : public UObject
{
	GENERATED_BODY()

public:
	void Init(AS47HUD* InHud) { Hud = InHud; }

	void Toast(const FString& Text, float Secs = 3.2f);
	void ClearToasts();
	FString LastDescription() const;                 // the last toast that was not a caption (V)
	void Objective(const FString& Text);             // empty hides it
	void Examine(const FString& Text);               // a closer look; empty hides it
	void Fade(bool bOn, const FString& Text = FString());
	bool Faded() const;
	void ClockSeconds(const FString& Text);          // the amber seconds clock (chapter six); empty: the plain clock

	// a paper (UI.document)
	void Document(const FS47Doc& Doc, TFunction<void()> OnClose = nullptr);
	// any panel (UI.open / UI.close / UI.modal)
	void Open(TSharedRef<SWidget> Widget, TFunction<void()> OnClose = nullptr);
	void Close(bool bSilent = false);
	bool Modal() const;

	// the field camera (UI.viewfinder, UI.shutterFlash, UI.cameraButton)
	using FViewfinder = FS47Viewfinder;
	void Viewfinder(bool bOn, const FS47Viewfinder& V = FS47Viewfinder());
	void ShutterFlash();
	void CameraButton(bool bOn, bool bRaised = false) { bCameraButton = bOn; bCameraRaised = bRaised; }
	bool bCameraButton = false, bCameraRaised = false;

	// what is offered at the crosshair (UI.prompt): set by the main step from the interaction
	void Prompt(const FString& Label, const FString& Alt = FString());

private:
	UPROPERTY() TObjectPtr<AS47HUD> Hud;
};
