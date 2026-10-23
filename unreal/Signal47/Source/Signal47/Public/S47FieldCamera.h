#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "S47Obj.h"
#include "UObject/Object.h"
#include "S47FieldCamera.generated.h"

class US47Main;
class ASceneCapture2D;
class UTextureRenderTarget2D;

// core/FieldCamera.ts Photo: a print, as the saves keep it
struct SIGNAL47_API FS47Photo
{
	FString Id;           // 'frame01' | 'frame02' | 'frame03' | 'frame04'
	FString Subject;
	FString Method;       // 'baseline' | 'passive' | 'active' | 'field'
	double Clock = 0;     // game clock (seconds) when exposed
	FString Url;          // the print as a JPEG data URL
	TArray<TPair<FString, FVector2D>> Targets; // features in photo UV, origin top left

	TSharedPtr<FJsonObject> ToJson() const;
	static TOptional<FS47Photo> FromJson(const TSharedPtr<FJsonObject>& O);
};

// FieldCamera.frameRect: the viewfinder frame on screen (the HUD's units), and its share of the height
struct FS47FrameRect { double X = 0, Y = 0, W = 0, H = 0, Frac = 1; };
// FieldCamera.project: where a world point lands in the photo, from the top left, and whether it is in front
struct FS47Projection { double U = 0, V = 0; bool bFront = false; };

// FieldCamera.expose's options. Before/After switch film-only objects on and off; Restore puts the
// screen back (a no-op here: the photograph is a scene capture of its own, the screen is never
// touched); Post may draw on the frame before the film goes over it (story/theThird.ts, FRAME 03).
struct FS47ExposeOptions
{
	TFunction<void()> Before, After, Restore;
	FString Caption, Time;
	double Seed = 0;
	TFunction<void(TArray<FColor>& Raw, int32 W, int32 H)> Post;
};
// FieldCamera.snap's options (a free frame for the album, W pixels wide, without the paper)
struct FS47SnapOptions
{
	TFunction<void()> Before, After, Restore;
	double Seed = 0;
	int32 W = 640;
	TFunction<void(TArray<FColor>& Raw, int32 W, int32 H)> Post;
};
struct FS47Snap { TArray<FColor> Px; int32 W = 0, H = 0; double Mean = 0; bool bRendered = false; };

// core/FieldCamera.ts: the SARO field camera. Raising it narrows the view to a 16:10 frame; what
// is inside the frame is exactly what the photograph records. An exposure renders the scene from
// the same viewpoint at 960x600 (a long exposure, brighter than the eye sees), with any film-only
// objects switched on for that one render, then gets a film look, a paper border and a
// handwritten caption. Held by US47Main (main.ts makes it and hands it to the chapters).
UCLASS()
class SIGNAL47_API US47FieldCamera : public UObject
{
	GENERATED_BODY()

public:
	static constexpr int32 PHOTO_W = 960, PHOTO_H = 600;
	static constexpr int32 BORDER = 28, CAPTION = 92;
	// PRINT = { w: PHOTO_W + BORDER * 2, h: PHOTO_H + BORDER + CAPTION, x: BORDER, y: BORDER }
	static constexpr int32 PRINT_W = PHOTO_W + BORDER * 2, PRINT_H = PHOTO_H + BORDER + CAPTION, PRINT_X = BORDER, PRINT_Y = BORDER;
	static constexpr double PHOTO_FOV = 44.6; // vertical, degrees: a standard lens

	void Init(US47Main* InM) { M = InM; }

	bool bHave = false;
	bool bRaised = false;

	FS47FrameRect FrameRect() const;
	void Raise(bool bOn);
	// Called every frame while raised: the screen camera gets the fov that makes the frame show
	// exactly the photo's field of view, and the photo camera follows it.
	void Sync();
	FS47Projection Project(const FVector& P) const;
	bool InFrame(const FVector& P, double X0, double X1, double Y0, double Y1) const;
	// Is anything in Occluders (or under them) between the camera and P?
	bool ClearLine(const FVector& P, const TArray<FS47Obj>& Occluders) const;
	// Renders the photograph and makes the print (PRINT_W x PRINT_H pixels). False when nothing
	// could be rendered (no world): the print then holds an unexposed frame.
	bool Expose(const FS47ExposeOptions& O, TArray<FColor>& OutPrint);
	FS47Snap Snap(const FS47SnapOptions& O);

	// the photo camera (photoCam): where it stands and looks, in Unreal's terms (the player's eye)
	FVector PhotoLoc = FVector::ZeroVector;
	FRotator PhotoRot = FRotator::ZeroRotator;

	// filmLook (in place): lifted blacks, warm cast, grain, vignette; the mean brightness
	static double FilmLook(TArray<FColor>& Raw, int32 W, int32 H, double Seed);
	// makePrint: the film look, then the paper round it
	static void MakePrint(TArray<FColor>& Raw, const FString& Caption, const FString& Time, double Seed, TArray<FColor>& OutPrint);
	// JavaScript's x >>> 0
	static uint32 ToUint32(double V);

	static bool SelfTest(TArray<FString>& Lines);

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
	TWeakObjectPtr<ASceneCapture2D> Capture;
	float SavedFov = 70.f;
	bool Render(int32 RW, int32 RH, TArray<FColor>& Out);
};
