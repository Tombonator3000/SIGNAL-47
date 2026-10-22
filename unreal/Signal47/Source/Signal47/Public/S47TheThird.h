#pragma once

#include "CoreMinimal.h"
#include "S47Seen.h"

class AS47Web;
class FS47Group;
struct FS47Node;
class UMaterialInstanceDynamic;

// web/src/story/theThird.ts. The third one before the credits (FORSLAG.md, 20 October, Valg for Tom 4, a
// and b; Tom: "Spenning og skummelt slår faste regler"). HISTORIE.md has three at the survey equipment in
// the whole 1947 photograph after the credits: Nora, Tomás and a third, blurred, with Reyes's
// silhouette, the SARO jacket with the reflective band across the back. Tom bends that rule here, twice,
// and the ending stays the one ending:
//
//   FRAME 03  At STATION 01 in chapter three the field camera takes the empty footing and the marked
//             post. When the shutter goes, out in the dark past the fence on C's ground, someone in a SARO
//             jacket stands with their back to the camera, and the band gives the flash back: a short pale
//             line at chest height on something darker than the dark, in the print and nowhere else (R4).
//             Out on the C stakes' own line when the frame has them; else on C's ground beside it, as near
//             the stakes as the picture allows. Small and faint.
//   05:29:10  On the old road, when the headlights wake in the wrong colours, someone in a SARO jacket
//             stands 9 m in front of the truck with their back to it, looking east along C. The band across
//             the back takes the headlights' colours as they change. It does not move. It is gone in the
//             FLASH (chapter six's: ThirdFigure).
//
// Both are the same figure as the one standing where the player was taken (story/recording.ts, Walker),
// baked into one draw in the web version, lit by a shader of its own: the jacket and the jeans in the
// light from behind them (how much of it each face gets, by which way it faces), the tape giving the
// light back in the light's own colour. Here the figure is the walker's boxes standing as the bake has
// them, each face a plane of its own with that face's colour worked out the same way (S47Figures); the
// head, seen only from behind, is the hair.

/** theThird.ts ThirdFigure: someone in a SARO jacket, lit by what the caller says. Built facing -z from
 *  the feet, like the player at yaw 0. */
class SIGNAL47_API FS47ThirdFigure
{
public:
	FS47ThirdFigure(AS47Web* Web, const FString& Name);
	~FS47ThirdFigure();
	TUniquePtr<FS47Group> G;
	/** mesh.visible (scene.add / removeFromParent: in the scene or not). */
	void SetVisible(bool bOn);
	void AddToScene();
	void RemoveFromScene();
	/** Where it stands (the feet) and which way it faces (yaw: facing -sin, -cos). */
	void Place(const FVector& At, double Yaw);
	/** lit: the light on its back, amb: the light all round, band: the tape (linear colours). */
	void Light(const FLinearColor& Lit, const FLinearColor& Amb, const FLinearColor& Band);
	/** 1 all there, 0 gone (in between it is let go by a dither in the web version, never blended: here it
	 *  is there from a half up, and gone below). */
	void SetFade(double K);
	double Fade() const { return FadeK; }
	/** The band's colour now (the tests). */
	FLinearColor BandColor() const { return BandC; }
	/** World points along the band across the back and at the two sleeves, a little towards `eye`: five
	 *  along the chest, then the left and right sleeve. */
	TArray<FVector> BandPoints(const FVector& Eye) const;
	/** Where its head is (world). */
	FVector Head() const;

private:
	struct FFace { UMaterialInstanceDynamic* M = nullptr; FLinearColor Base; double Shade = 0; bool bBand = false; };
	TArray<FFace> Faces;
	FLinearColor LitC = FLinearColor::Black, AmbC = FLinearColor::Black, BandC = FLinearColor::Black;
	double FadeK = 1;
	bool bShown = false, bInScene = false;
	FVector Pos = FVector::ZeroVector;
	double YawV = 0;
	void Recolor();
	void Show();
	void Box(FS47Node* Parent, double W, double H, double Dp, const FLinearColor& Base, bool bBand, const FVector& P, const FVector& R = FVector::ZeroVector);
};

/** Where it stands for FRAME 03: how far from the transit, and the way C's stakes run from it, in degrees
 *  north of them it may stand to be in the picture at all. */
struct FS47FilmSpec { double From, To, Step, NorthMax, SouthMax, OnC; };

struct FS47FilmIn
{
	FS47Cam Cam;
	/** The transit's pier (world) and the way C's stakes run from it (world, flat). */
	FVector Transit, CDir;
	TFunction<double(double, double)> Ground;
	/** Nothing in the way between two world points (the hut, the shed, the footing, the land). */
	TFunction<bool(const FVector&, const FVector&)> Clear;
};

struct FS47FilmShot
{
	/** Where the band is in the photograph (u, v from the top left), and whether it stood on the C
	 *  stakes' own line (else on C's ground beside it). */
	double U = 0, V = 0;
	bool bOnC = false;
	/** Switch it into the scene for the exposure, and out again. */
	TFunction<void()> Before, After;
	/** Draw the light the tape gives back onto the raw frame, before the film goes over it. */
	TFunction<void(TArray<FColor>&, int32, int32)> Post;
};

/** The third one on FRAME 03: the spot in the frame nearest C's stakes (58 to 76 m out from the transit,
 *  past the fence), or unset when none of C's ground is in the picture clear of everything. */
class SIGNAL47_API FS47FilmThird
{
public:
	explicit FS47FilmThird(AS47Web* InWeb) : Web(InWeb) {}
	static constexpr FS47FilmSpec FILM = { 58, 76, 3, 60, 8, 7 };
	TOptional<FS47FilmShot> Shot(const FS47FilmIn& O);
	static bool SelfTest(TArray<FString>& Lines);

private:
	TWeakObjectPtr<AS47Web> Web;
	TSharedPtr<FS47ThirdFigure> Fig;
	static void Glow(TArray<FColor>& Raw, int32 W, int32 H, const FS47Cam& Cam, const FVector& At, double Yaw);
};
