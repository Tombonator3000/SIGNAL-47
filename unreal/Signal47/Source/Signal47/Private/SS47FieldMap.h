#pragma once

#include "CoreMinimal.h"
#include "S47UI.h"
#include "Widgets/SLeafWidget.h"

class US47Main;

// MAP-20261108 (Tom, 8 November: "Kart må bli bedre og mer nøyaktig (med zoomfunksjon) og må vise hvor spiller er nøyaktig").
// Freeform's map in the notebook: the whole land as a 1986 topographic quadrangle shows it, to scale in road-local metres
// (S47OpenLand: x east, z south, north up), with the player's own pencil on top.
//
//   printed   contours from the land's own heights (brown, every 20 feet, an index line every 100 with its height), the
//             highway (red, cased), the service and ranch roads, the survey track and the old road stub (dashed), the dry wash
//             (blue), section lines (red, a mile apart, numbered), buildings in black from what stops a walker (the areas'
//             colliders, the farm's plots, SARO's own walls), the 27 dishes, windmill, tank and mine, names, ticks every km.
//   pencil    places found or heard of, bearings from SARO, CB and radio pins (FillMap), the player's own numbered marks, the
//             truck where it was left.
//   red       where the player is and which way they face. It goes through the same RoadOf as every place on the land, so it is
//             right in SARO's terms, the road's, the diner's and STATION 01's turned ones.
//
// Zoom: the wheel (about the cursor), + and -, Page Up and Down, LT and RT. Move: drag, the arrows or W A S D, the left stick,
// the D-pad. Home, Enter, Space, A or L3: back to where you are. X, R3 or a right click: a pencil mark of your own, or rubbed out.
namespace S47FieldMap
{
	// A point in an area's own terms on the land, and a heading there (the walker's or the truck's yaw), as road-local metres and
	// degrees from north, clockwise. room6 is the motel's room 6 (its own interior, far off): the motel. False: not on the land.
	bool OnSheet(const FString& Area, double X, double Z, double Yaw, double& OutX, double& OutZ, double& OutHeading);
	// Where the player is now: the cab when driving. False: nowhere on this sheet (no player, Roswell).
	bool Me(const US47Main* M, double& OutX, double& OutZ, double& OutHeading, FString& OutArea, bool& bOutDriving);
	// Where the truck stands, when the player is not in it.
	bool Truck(const US47Main* M, double& OutX, double& OutZ, double& OutHeading);
	// The work the player has now, by its place: a dish that is down, the feeder, the round's stops, the truck in trouble, the tray,
	// the mailbox's flag, the jobs waiting at OPS. Written on the sheet in ink with a line to the place.
	void GatherJobs(US47Main* M, FS47MapView& V);
	// "north-northeast"
	FString Compass(double Deg);
	// What V reads on the sheet.
	TArray<FString> Description(const FS47MapView& V);
	// The next sheet opens at this scale (page units a metre), on the player. A developer run's pictures.
	void DevScale(double Scale);
	double Scale();
	// The land's height at a road-local point, as the sheet's contours have it (metres, the road's datum), and in feet above sea level.
	double LandHeight(double X, double Z);
	double Feet(double Height);
	bool SelfTest(TArray<FString>& Lines);
}

using FS47OnMapNote = TFunction<TArray<FS47MapNote>(double, double, double)>;

class SS47FieldMap : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47FieldMap) : _Width(900.f) {}
		SLATE_ARGUMENT(FS47MapView, View)
		SLATE_ARGUMENT(float, Width)
		SLATE_ARGUMENT(FS47OnMapNote, OnNote)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Width, Width * 800.0 / 1200.0); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual FReply OnMouseWheel(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override;
	virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& G, const FPointerEvent& E) const override;

	// From the notebook, which keeps the keyboard: a key on the map page. True: it was the map's.
	bool Key(const FKey& K);
	// The left stick (Gamepad_LeftX, Gamepad_LeftY): the sheet moves while it is held.
	void Stick(const FKey& Axis, float Value);
	// The wheel, or LT and RT, from outside the sheet: about its middle.
	void Wheel(float Delta);
	void Step(float Dt);
	// Where the middle of the sheet is (road-local) and its scale (page units a metre), for the tests.
	FVector2D Centre() const { return Mid; }
	double ScaleNow() const { return Sc; }
	void CentreOnMe();

private:
	FS47MapView View;
	float Width = 900.f;
	FS47OnMapNote OnNote;
	FVector2D Mid = FVector2D::ZeroVector; // road-local metres at the middle of the frame
	double Sc = .5;                         // page units per metre
	FVector2D Pad = FVector2D::ZeroVector;  // the left stick
	bool bDrag = false;
	FVector2D DragAt = FVector2D::ZeroVector; // page units
	float DragMoved = 0;
	// What the areas have built, read when the sheet is made: black rectangles, road-local.
	struct FBlock { FBox2D Box; bool bChurch = false; };
	mutable TArray<FBlock> Blocks;
	mutable TArray<FVector2D> Dishes;
	mutable TArray<FString> DishIds; // S-01 to S-27, as the array's PC and the jobs name them
	mutable TArray<TPair<FVector2D, FVector2D>> Paths; // the array's walks, end to end
	struct FFeature { FVector2D At; FString Kind; };
	mutable TArray<FFeature> Features; // windmill, tank, mine, shack, feeder
	mutable FVector2D SaroAt = FVector2D(33, -300);
	mutable double SaroZ = -300;
	// Const: it only fills the drawing caches above, called from painting when the world has changed.
	void SurveyWorld() const;
	void ZoomAbout(double Factor, const FVector2D& Page);
	void Clamp();
	void Note(const FVector2D& Road);
	FVector2D ToPage(double X, double Z) const;
	FVector2D ToRoad(const FVector2D& Page) const;
	double PageScale(const FGeometry& G) const { return G.GetLocalSize().X / 1200.0; }
};
