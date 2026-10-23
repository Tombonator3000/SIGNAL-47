#include "S47Truck.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/DrawElements.h"
#include "RenderingThread.h"
#include "S47Figures.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Web.h"
#include "SS47Widgets.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/SLeafWidget.h"

#include <cmath>

using namespace S47Three;

namespace
{
	constexpr double WHEELBASE = 3.3;
	constexpr double FRONT_AXLE = -1.8, REAR_AXLE = FRONT_AXLE + WHEELBASE, WHEEL_R = 0.37, TRACK = 0.84;
	const double WHEELS[4][2] = { { -TRACK, FRONT_AXLE }, { TRACK, FRONT_AXLE }, { -TRACK, REAR_AXLE }, { TRACK, REAR_AXLE } };
	// Legacy story slots retain their moving positions and state. Native headlight spots own
	// their illumination; these slots are excluded from the area light conversion.
	struct FPool { FVector P; double W; };
	const FPool FLOODS[3] = { { FVector(0, 0.9, -7.8), 5.5 }, { FVector(0, 1.2, -14.6), 10 }, { FVector(0, 1.8, -28.6), 14 } };
	const FLinearColor HEAD(1.0f, 0.9f, 0.72f), TAIL(1.f, 0.05f, 0.03f), AMBER(1.f, 0.55f, 0.12f);
	/** Degrees -180..180. */
	double Wrap180(double D) { return std::fmod(std::fmod(D + 180, 360.0) + 360, 360.0) - 180; }
	/** The compass ball on the instrument hood: where it sits, truck-local. */
	const FVector COMPASS(-0.28, 1.343, -0.5);
	const TCHAR* WARN[4] = { TEXT("BRAKE"), TEXT("OIL"), TEXT("VOLTS"), TEXT("BELTS") };

	// an unlit lens: its glass colour when off, plus the lamp colour times k
	FLinearColor Lens(uint32 Off, const FLinearColor& C, double K)
	{
		FLinearColor O = Hex(Off);
		return FLinearColor(O.R + C.R * float(K), O.G + C.G * float(K), O.B + C.B * float(K));
	}

	// three's Euler.setFromQuaternion in the YXZ order: (x, y, z) in radians
	FVector EulerYXZ(const FQuat& Q)
	{
		double M[16];
		Compose(FVector::ZeroVector, Q, FVector::OneVector, M);
		const double m11 = M[0], m13 = M[8], m21 = M[1], m22 = M[5], m23 = M[9], m31 = M[2], m33 = M[10];
		const double X = FMath::Asin(-FMath::Clamp(m23, -1.0, 1.0));
		if (FMath::Abs(m23) < 0.9999999) return FVector(X, FMath::Atan2(m13, m33), FMath::Atan2(m21, m22));
		return FVector(X, FMath::Atan2(-m31, m11), 0);
	}
	FLinearColor Css(uint8 R, uint8 G, uint8 B, float A = 1.f) { FLinearColor C = FLinearColor::FromSRGBColor(FColor(R, G, B)); C.A = A; return C; }
	FLinearColor CssHex(uint32 Rgb, float A = 1.f) { return Css((Rgb >> 16) & 255, (Rgb >> 8) & 255, Rgb & 255, A); }
}

// ---------- the dash canvas (truckTextures.ts dashAtlas) ----------
// One 512 x 256 canvas: the cluster (speedometer, fuel, temperature) in the top 160 rows, the
// radio and the clock below. Drawn whole by Slate into the truck's render target whenever the
// clock, the radio's dial or the odometer changes. (The canvas's soft shadows are a faint glow
// drawn under the marks here.)
class SS47Dash : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Dash) {}
	SLATE_END_ARGS()
	void Construct(const FArguments&)
	{
		White = MakeUnique<FSlateColorBrush>(FLinearColor::White);
		Knob = MakeUnique<FSlateRoundedBoxBrush>(FLinearColor::White, 20.f);
	}
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(512, 256); }
	FString Clock;
	double RadioX = 214;
	bool bRadioOn = false;
	double Miles = S47_ODOMETER_OUT;

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		const FLinearColor MARK = CssHex(0xd6eadc);
		const FLinearColor Glow = Css(160, 255, 200, 0.45f);
		Box(G, Out, Layer, 0, 0, 512, 256, CssHex(0x060707));
		// the dials, with their glow
		Dial(G, Out, Layer + 1, 256, 88, 70, -120, 120, 34, TEXT("MPH"), { { 0, TEXT("0") }, { 10 / 85.0, TEXT("10") }, { 20 / 85.0, TEXT("20") }, { 30 / 85.0, TEXT("30") }, { 40 / 85.0, TEXT("40") }, { 50 / 85.0, TEXT("50") }, { 60 / 85.0, TEXT("60") }, { 70 / 85.0, TEXT("70") }, { 80 / 85.0, TEXT("80") } }, MARK, Glow);
		Dial(G, Out, Layer + 1, 88, 98, 40, -60, 60, 4, TEXT("FUEL"), { { 0, TEXT("E") }, { 0.5, TEXT("1/2") }, { 1, TEXT("F") } }, MARK, Glow);
		Dial(G, Out, Layer + 1, 424, 98, 40, -60, 60, 4, TEXT("TEMP"), { { 0, TEXT("C") }, { 1, TEXT("H") } }, MARK, Glow);
		// the gear selector and the dark warning lamps
		Text(G, Out, Layer + 3, TEXT("P   R   N   D   2   1"), 256, 152, S47Style::Display(11), MARK);
		const int32 At[4][2] = { { 157, 30 }, { 355, 30 }, { 157, 46 }, { 355, 46 } };
		for (int32 i = 0; i < 4; ++i)
		{
			Box(G, Out, Layer + 2, At[i][0] - 20, At[i][1] - 6, 40, 12, CssHex(i < 2 ? 0x4a1712 : 0x4a3512));
			Text(G, Out, Layer + 3, WARN[i], At[i][0], At[i][1], S47Style::Display(10), CssHex(0x0b0b0b));
		}
		// the turn signal arrows
		Arrow(G, Out, Layer + 2, 196, -1, CssHex(0x123d22));
		Arrow(G, Out, Layer + 2, 316, 1, CssHex(0x123d22));
		// radio faceplate: AM and FM dial behind glass, two knobs
		Box(G, Out, Layer, 0, 160, 512, 96, CssHex(0x16181b));
		Box(G, Out, Layer + 1, 0, 160, 512, 4, CssHex(0x2b2e32));
		// the dial behind its glass: lit amber while the radio plays, the red needle at the station
		Box(G, Out, Layer + 1, 70, 176, 260, 64, CssHex(bRadioOn ? 0x2b1b07 : 0x0e1210));
		const FLinearColor Ink = bRadioOn ? Css(255, 214, 140, 0.95f) : Css(214, 234, 220, 0.75f);
		const TCHAR* Fm[6] = { TEXT("88"), TEXT("92"), TEXT("96"), TEXT("100"), TEXT("104"), TEXT("108") };
		const TCHAR* Am[7] = { TEXT("54"), TEXT("60"), TEXT("70"), TEXT("80"), TEXT("100"), TEXT("120"), TEXT("160") };
		const FLinearColor RadioGlow = bRadioOn ? Css(255, 190, 90, 0.7f) : FLinearColor::Transparent;
		for (int32 i = 0; i < 6; ++i) Text(G, Out, Layer + 2, Fm[i], 92 + i * 44, 192, S47Style::Mono(12), Ink, RadioGlow);
		for (int32 i = 0; i < 7; ++i) Text(G, Out, Layer + 2, Am[i], 88 + i * 37, 226, S47Style::Mono(12), Ink, RadioGlow);
		Text(G, Out, Layer + 2, TEXT("FM"), 80, 208, S47Style::Mono(12), Ink, RadioGlow);
		Text(G, Out, Layer + 2, TEXT("AM"), 318, 208, S47Style::Mono(12), Ink, RadioGlow);
		Box(G, Out, Layer + 3, FMath::RoundToDouble(RadioX) - 1, 180, 3, 56, CssHex(0xc4361e));
		for (const double X : { 36.0, 357.0 })
		{
			FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(40.f, 40.f), FSlateLayoutTransform(FVector2f(float(X - 20), 188.f))), Knob.Get(), ESlateDrawEffect::None, CssHex(0x3a3d42));
		}
		// the clock
		Box(G, Out, Layer + 1, 384, 178, 116, 60, CssHex(0x020403));
		if (!Clock.IsEmpty()) Text(G, Out, Layer + 2, Clock, 442, 210, S47Style::Mono(46), CssHex(0x86f2cf), Css(120, 255, 210, 0.8f));
		// The odometer in the face of the speedometer, above the needle's hub: five drums of whole
		// miles, light on black, and the tenths on a white drum
		{
			const double W = 54, H = 16, X0 = 256 - W / 2, Y0 = 88 - 32;
			Box(G, Out, Layer + 4, X0, Y0, W, H, FLinearColor::Black);
			const FLinearColor Edge = Css(214, 234, 220, 0.5f);
			FSlateDrawElement::MakeLines(Out, Layer + 5, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(X0, Y0), FVector2f(X0 + W, Y0), FVector2f(X0 + W, Y0 + H), FVector2f(X0, Y0 + H), FVector2f(X0, Y0) }, ESlateDrawEffect::None, Edge, true, 1.f);
			const int64 Tenths = int64(FMath::Floor(Miles * 10 + 1e-6)), Whole = (Tenths / 10) % 100000, Tenth = Tenths % 10;
			const FString Digits = FString::Printf(TEXT("%05lld"), (long long)Whole);
			for (int32 i = 0; i < Digits.Len(); ++i) Text(G, Out, Layer + 5, Digits.Mid(i, 1), X0 + 5.5 + i * 8.2, Y0 + 8.5, S47Style::Mono(16), MARK);
			Box(G, Out, Layer + 5, X0 + W - 12, Y0 + 2, 10, H - 4, CssHex(0xe8e4d8));
			Text(G, Out, Layer + 6, FString::Printf(TEXT("%lld"), (long long)Tenth), X0 + W - 7, Y0 + 8.5, S47Style::Mono(16), CssHex(0x141414));
		}
		return Layer + 7;
	}

private:
	TUniquePtr<FSlateBrush> White, Knob;
	void Box(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, double X, double Y, double W, double H, const FLinearColor& C) const
	{
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(float(W), float(H)), FSlateLayoutTransform(FVector2f(float(X), float(Y)))), White.Get(), ESlateDrawEffect::None, C);
	}
	// canvas fillText with textAlign centre and textBaseline middle
	void Text(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, const FString& S, double X, double Y, const FSlateFontInfo& F, const FLinearColor& C, const FLinearColor& GlowC = FLinearColor::Transparent) const
	{
		FVector2D Size(S.Len() * F.Size, F.Size * 1.3);
		if (FSlateApplication::IsInitialized()) Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, F);
		const FVector2f At(float(X - Size.X / 2), float(Y - Size.Y / 2));
		if (GlowC.A > 0)
		{
			const FLinearColor Soft(GlowC.R, GlowC.G, GlowC.B, GlowC.A * 0.35f);
			for (const FVector2f D : { FVector2f(-1, 0), FVector2f(1, 0), FVector2f(0, -1), FVector2f(0, 1) })
			{
				FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(FVector2f(Size) + FVector2f(4, 4), FSlateLayoutTransform(At + D)), S, F, ESlateDrawEffect::None, Soft);
			}
		}
		FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(Size) + FVector2f(4, 4), FSlateLayoutTransform(At)), S, F, ESlateDrawEffect::None, C);
	}
	void Line(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, double X0, double Y0, double X1, double Y1, double W, const FLinearColor& C) const
	{
		FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(X0, Y0), FVector2f(X1, Y1) }, ESlateDrawEffect::None, C, true, float(W));
	}
	// truckTextures.ts dial: ticks, labels and the name
	void Dial(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, double Dx, double Dy, double R, double From, double To, int32 Ticks, const TCHAR* Name,
		std::initializer_list<TPair<double, const TCHAR*>> Labels, const FLinearColor& MARK, const FLinearColor& Glow) const
	{
		auto At = [&](double F, double Rad) { const double A = (From + (To - From) * F) * UE_DOUBLE_PI / 180; return FVector2D(Dx + FMath::Sin(A) * Rad, Dy - FMath::Cos(A) * Rad); };
		const FLinearColor Soft(Glow.R, Glow.G, Glow.B, Glow.A * 0.5f);
		for (int32 i = 0; i <= Ticks; ++i)
		{
			const bool bMajor = i % 2 == 0;
			const FVector2D P0 = At(double(i) / Ticks, R), P1 = At(double(i) / Ticks, R - (bMajor ? 9 : 5));
			Line(G, Out, Layer, P0.X, P0.Y, P1.X, P1.Y, (bMajor ? 2.4 : 1.3) + 2, Soft);
			Line(G, Out, Layer + 1, P0.X, P0.Y, P1.X, P1.Y, bMajor ? 2.4 : 1.3, MARK);
		}
		const FSlateFontInfo F = S47Style::Display(R > 50 ? 15 : 13);
		for (const TPair<double, const TCHAR*>& L : Labels) { const FVector2D P = At(L.Key, R - (R > 50 ? 21 : 16)); Text(G, Out, Layer + 1, L.Value, P.X, P.Y, F, MARK, Glow); }
		Text(G, Out, Layer + 1, Name, Dx, Dy + (R > 50 ? 26 : 16), S47Style::Display(11), MARK, Glow);
	}
	// a filled triangle (x + dir * 12, 14), (x, 6), (x, 22), as lines across it
	void Arrow(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, double X, double Dir, const FLinearColor& C) const
	{
		for (double K = 0; K <= 12; K += 0.75)
		{
			const double Half = 8 * (1 - K / 12);
			Line(G, Out, Layer, X + Dir * K, 14 - Half, X + Dir * K, 14 + Half, 1.0, C);
		}
	}
};

// ---------- the truck ----------

FS47Truck::~FS47Truck()
{
	FWorldDelegates::OnWorldPostActorTick.Remove(HeadlightTick);
	for (const TWeakObjectPtr<USpotLightComponent>& L : Headlights) if (L.IsValid()) L->DestroyComponent();
	if (Renderer) { BeginCleanup(Renderer); Renderer = nullptr; }
}

void FS47Truck::Bind(US47Main* InM, const FString& InPath, int32 InCabSet, UObject* Outer)
{
	M = InM;
	Web = M ? M->Web.Get() : nullptr;
	Path = InPath;
	CabSet = InCabSet;
	// The second cab slot was sky fill through the glass, not a fixture. Native sky and
	// exterior lamps now light the cab through its actual windows.
	if (Web && CabSet >= 0) Web->SuppressFloodLight(CabSet, 1);
	HeadColor = HEAD;
	OdoShown = int32(FMath::Floor(S47_ODOMETER_OUT * 10));
	if (!Web) return;
	auto O = [&](const TCHAR* K) { return Web->Obj(Path + TEXT(".") + K); };
	auto Mt = [&](const TCHAR* K) { return Web->Material(Path + TEXT(".m.") + K); };
	Group = O(TEXT("group")); Body = O(TEXT("body")); Shell = O(TEXT("shell")); Cab = O(TEXT("cab"));
	Wheels = O(TEXT("wheels")); Needles = O(TEXT("needles")); Keys = O(TEXT("keys")); WheelSpin = O(TEXT("wheelSpin"));
	CompassCard = O(TEXT("compass.card")); GlowPoints = O(TEXT("glow.points"));
	for (int32 i = 0; i < 4; ++i) Lamps[i] = O(*(FString(TEXT("lamps.")) + WARN[i]));
	MHead = Mt(TEXT("head")); MPark = Mt(TEXT("park")); MTail = Mt(TEXT("tail"));
	MGauge = Mt(TEXT("gauge")); MNeedle = Mt(TEXT("needle")); MCompass = Mt(TEXT("compass"));
	DriverEye = Web->Vec(Path + TEXT(".driverEye"), DriverEye);
	// the group's pose as the export has it, its rotation read back in the YXZ order
	Position = Group.Position();
	Rotation = EulerYXZ(Group.Quaternion());
	if (const FS47WebNode* N = Group.IsValid() ? Web->Node(Group.I) : nullptr)
	{
		for (int32 I = 0; I < 2; ++I)
		{
			// One soft source for each pair of the original sealed beams, just in front of its glass.
			USpotLightComponent* L = NewObject<USpotLightComponent>(Web);
			L->SetupAttachment(N->Comp.Get());
			L->SetMobility(EComponentMobility::Movable);
			L->SetIntensityUnits(ELightUnits::Candelas);
			L->SetIntensity(0);
			L->SetAttenuationRadius(4500);
			L->SetOuterConeAngle(21);
			L->SetInnerConeAngle(10);
			L->SetSourceRadius(6);
			L->SetSoftSourceRadius(8);
			L->SetCastShadows(true);
			L->SetCastVolumetricShadow(true);
			L->ContactShadowLengthInWS = true; L->ContactShadowLength = 18.f;
			L->ContactShadowCastingIntensity = 0.7f;
			L->SetVolumetricScatteringIntensity(0.8f);
			L->SetVisibility(false);
			L->RegisterComponent();
			Headlights[I] = L;
		}
		// Parked trucks do not run Update. Follow area visibility every frame too.
		const TWeakObjectPtr<AS47Web> WeakWeb = Web;
		HeadlightTick = FWorldDelegates::OnWorldPostActorTick.AddLambda([this, WeakWeb](UWorld* World, ELevelTick, float)
		{
			if (WeakWeb.IsValid() && WeakWeb->GetWorld() == World) PlaceHeadlights();
		});
	}
	// the glow points (GlowPoints.add in the constructor: for each side the two headlamps, the
	// parking lamp, the tail lamp), each a sprite of its own in the Unreal scene
	for (int32 S = 0; S < 2; ++S)
	{
		GiHead.Add(S * 4); GiHead.Add(S * 4 + 1); GiPark.Add(S * 4 + 2); GiTail.Add(S * 4 + 3);
	}
	if (const FS47WebNode* N = GlowPoints.IsValid() ? Web->Node(GlowPoints.I) : nullptr)
	{
		if (USceneComponent* C = N->Comp.Get())
		{
			for (USceneComponent* Ch : C->GetAttachChildren())
			{
				const UMaterialBillboardComponent* B = Cast<UMaterialBillboardComponent>(Ch);
				if (B && B->Elements.Num() > 0) Glows.Add(Cast<UMaterialInstanceDynamic>(B->Elements[0].Material));
			}
		}
	}
	// the dash canvas, drawn here and put on the gauge material (its cluster and its radio)
	if (MGauge.IsValid())
	{
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(Outer ? Outer : static_cast<UObject*>(Web));
		RT->ClearColor = FLinearColor::Black;
		RT->InitCustomFormat(512, 256, PF_B8G8R8A8, false);
		RT->UpdateResourceImmediate(true);
		DashRT = RT;
		Dash = SNew(SS47Dash);
		Renderer = new FWidgetRenderer(true);
		// (the canvas's rows run down as a render target's do: the other way up from the exported pictures)
		MGauge.SetMap(RT, true);
		DrawDash();
	}
	SetDriving(false);
	Update(0, 0);
	ApplyLights();
}

double FS47Truck::Now() const { return FPlatformTime::Seconds() * 1000.0; }

void FS47Truck::SetPose(const FVector& P, const FVector& R)
{
	Position = P;
	Rotation = R;
	Group.SetPosition(P);
	Group.SetQuaternion(Euler(R.X, R.Y, R.Z, TEXT("YXZ")));
	PlaceHeadlights();
}

FQuat FS47Truck::Quat() const { return Euler(Rotation.X, Rotation.Y, Rotation.Z, TEXT("YXZ")); }
FVector FS47Truck::LocalToWorld(const FVector& L) const { return Quat().RotateVector(L) + Position; }
FVector FS47Truck::WorldToLocal(const FVector& W) const { return Quat().UnrotateVector(W - Position); }

int32 FS47Truck::CompassReads() const
{
	const int32 A = int32(FMath::RoundToDouble(Compass.A)) % 360;
	return (A + 360) % 360;
}

FVector FS47Truck::CompassAt() const { return LocalToWorld(COMPASS); }

double FS47Truck::Bearing() const
{
	return std::fmod(std::fmod(Wrap180((CompassNorth - Rotation.Y) * 180 / UE_DOUBLE_PI), 360.0) + 360, 360.0);
}

void FS47Truck::CompassStep(double Dt)
{
	double Target = Bearing();
	if (CompassPull && CompassPull->K > 0) Target += Wrap180(CompassPull->Deg - Target) * FMath::Min(1.0, CompassPull->K);
	if (!Compass.bSet || Dt <= 0) { if (!Compass.bSet) { Compass.A = Target; Compass.V = 0; Compass.bSet = true; } }
	else
	{
		// a ball compass: the card swings after the heading through its liquid and settles, all
		// but without overshooting
		const double W = 4.2, Z = 0.8, Err = Wrap180(Target - Compass.A);
		Compass.V += (W * W * Err - 2 * Z * W * Compass.V) * FMath::Min(Dt, 0.05);
		Compass.A += Compass.V * FMath::Min(Dt, 0.05);
	}
	CompassCard.SetRotation(FVector(0, Compass.A * UE_DOUBLE_PI / 180, 0));
}

void FS47Truck::SetDriving(bool bOn)
{
	// (parked, the card stands where the heading is: it is found there the next time, not swinging)
	if (!bOn) { Compass.bSet = false; CompassPull.Reset(); }
	bDriving = bOn;
	Cab.SetVisible(bOn);
	Shell.SetVisible(!bOn);
	// from the driver's seat the lamps face away: their glows only poked over the hood
	GlowPoints.SetVisible(!bOn);
}

void FS47Truck::ViewFrom(const FVector& CamThree)
{
	if (!bDriving) return;
	const FVector EyeW = LocalToWorld(DriverEye);
	if (FVector::Dist(CamThree, EyeW) > 0.9) SetDriving(false);
}

void FS47Truck::CopyCab(const FS47Truck& O)
{
	Gauge = O.Gauge;
	Swing = O.Swing;
	// (the card goes on swinging where it was: the heading is the map's in every area)
	Compass.A = O.Compass.A; Compass.V = O.Compass.V; Compass.bSet = O.Compass.bSet;
	CompassPull = O.CompassPull;
	Keys.SetRotation(FVector(O.Swing.A * 0.4, 0, O.Swing.A));
	SetOdometer(O.Odometer() - S47_ODOMETER_OUT);
	SetFrost(O.Frosted());
	Update(0, 0, O.WheelAngle);   // the needles drawn where they are now, in this frame
}

void FS47Truck::SetDash(double Mph, double Rpm, const FString& Clock, double Fuel)
{
	Gauge.TSpeed = FMath::Clamp(Mph / 85, 0.0, 1.0);
	Gauge.TFuel = FMath::Clamp(Fuel, 0.0, 1.0);
	Gauge.TTemp = Rpm > 100 ? 0.5 + FMath::Min(0.06, Rpm / 60000) : 0.15;
	ClockWanted = Clock;
	FlushClock();
}

void FS47Truck::FlushClock()
{
	const double T = Now();
	if (ClockWanted == ClockShown || T - ClockAt < 100) return;
	ClockShown = ClockWanted; ClockAt = T;
	if (Dash.IsValid()) Dash->Clock = ClockShown;
	DrawDash();
}

void FS47Truck::SetRadio(double X, bool bOn)
{
	const FString K = FString::Printf(TEXT("%lld:%s"), (long long)FMath::RoundToDouble(X), bOn ? TEXT("true") : TEXT("false"));
	if (K == RadioShown) return;
	RadioShown = K;
	if (Dash.IsValid()) { Dash->RadioX = X; Dash->bRadioOn = bOn; }
	DrawDash();
}

void FS47Truck::SetOdometer(double Tonight)
{
	const double Miles = S47_ODOMETER_OUT + FMath::Max(0.0, Tonight);
	const int32 K = int32(FMath::Floor(Miles * 10 + 1e-6));
	if (K == OdoShown) return;
	OdoShown = K;
	if (Dash.IsValid()) Dash->Miles = Miles;
	DrawDash();
}

void FS47Truck::SetFrost(double K)
{
	if (K <= 0.001 && !FrostDecal) return;
	Frost = K > 0.001 ? FMath::Clamp(K, 0.0, 1.0) : 0;
	if (!FrostDecal && Web && Cab)
	{
		// (no light of its own: the night's on it, a pale rime against the dark, not a glow; on the cab's
		// inside of the windshield: decal(cab, 1.74, 0.7, mat, [0, 0, 1, 1], 0, 1.452, -0.604, 0, 0.552))
		const FS47WebNode* CabN = Web->Node(Cab.I);
		FrostDecal = MakeShared<FS47Group>(Web, TEXT("truck:frost"), CabN ? CabN->Comp.Get() : nullptr);
		FrostMat = FrostDecal->Basic(Hex(0x77808b), TEXT("M_Web_Unlit_Trans_2S"));
		S47Figures::SetMap(FrostMat, S47Figures::FrostGlass());
		FrostDecal->Plane(&FrostDecal->Root(), 1.74, 0.7, FrostMat, FVector(0, 1.452, -0.604), FVector(0.552, 0, 0));
		FrostDecal->RenderOrder(4);
		FrostDecal->AddToScene();
	}
	if (!FrostDecal) return;
	S47Figures::SetOpacity(FrostMat, Frost);
	FrostDecal->SetVisible(K > 0.001);
}

void FS47Truck::LightFromAbove(double K) { Above = FMath::Clamp(K, 0.0, 1.0); PlaceFloods(); }

void FS47Truck::SetSteer(double V) { WheelSpin.SetRotation(FVector(0, 0, -FMath::Clamp(V, -1.0, 1.0) * 2.4)); }
void FS47Truck::SetHeadlights(bool bOn) { bHeadOn = bOn; ApplyLights(); }
void FS47Truck::SetLightLevel(double Level) { LightLevel = FMath::Clamp(Level, 0.0, 1.0); ApplyLights(); }
void FS47Truck::SetBrake(bool bOn) { if (bOn != bBrakeOn) { bBrakeOn = bOn; ApplyLights(); } }
void FS47Truck::SetHeadColor(TOptional<FLinearColor> C) { HeadColor = C ? *C : HEAD; ApplyLights(); }
void FS47Truck::SetDashLevel(TOptional<double> K) { DashLevel = K; ApplyLights(); }

void FS47Truck::SetWarnings(const FWarn& W)
{
	const bool On[4] = { W.bBrake, W.bOil, W.bVolts, W.bBelts };
	for (int32 i = 0; i < 4; ++i) if (Lamps[i].IsValid() && Lamps[i].Visible() != On[i]) Lamps[i].SetVisible(On[i]);
	WarnOn = W;
}

void FS47Truck::HeadlightFloods(int32 Set, const TArray<int32>& Slots)
{
	FloodSet = Set;
	FloodSlots = Slots;
	if (FloodSlots.Num() > 3) FloodSlots.SetNum(3);
	// The two physical sealed-beam sources replace all three ground-pool proxies.
	if (Web) for (int32 I : FloodSlots) Web->SuppressFloodLight(FloodSet, I);
	PlaceFloods();
}

void FS47Truck::SetWheelDrops(const double D[4]) { for (int32 i = 0; i < 4; ++i) Drops[i] = FMath::Clamp(D[i], -0.09, 0.09); }

void FS47Truck::ApplyLights()
{
	const double L = LightLevel, Head = bHeadOn ? L : 0, D = DashLevel ? *DashLevel : L;
	const double Tail = L * FMath::Max(bHeadOn ? 0.5 : 0, bBrakeOn ? 1.5 : 0);
	MHead.SetColor(Lens(0x3e3e3a, HeadColor, 1.6 * Head));
	MPark.SetColor(Lens(0x4a3a18, AMBER, 0.8 * Head));
	MTail.SetColor(Lens(0x420a08, TAIL, Tail));
	const float G = float(0.03 + 0.85 * D);
	MGauge.SetColor(FLinearColor(G, G, G));
	const float N = float(0.04 + 0.96 * D), Cc = float(0.03 + 0.82 * D);
	MNeedle.SetColor(FLinearColor(1.f * N, 0.42f * N, 0.19f * N));
	MCompass.SetColor(FLinearColor(1.f * Cc, 0.9f * Cc, 0.7f * Cc));
	auto Put = [&](const TArray<int32>& Ids, const FLinearColor& C, double K)
	{
		for (int32 i : Ids)
		{
			if (Glows.IsValidIndex(i) && Glows[i].IsValid()) Glows[i]->SetVectorParameterValue(TEXT("Color"), FLinearColor(C.R * float(K), C.G * float(K), C.B * float(K)));
		}
	};
	Put(GiHead, HeadColor, 0.75 * Head); Put(GiPark, AMBER, 0.3 * Head); Put(GiTail, TAIL, 0.4 * Tail);
	PlaceFloods();
}

void FS47Truck::PlaceFloods()
{
	if (!Web) return;
	PlaceHeadlights();
	const double D = DashLevel ? *DashLevel : LightLevel;
	if (CabSet >= 0)
	{
		// the instruments' glow, and the sky through the glass: a faint cool light from above the cab
		Web->SetFloodPool(CabSet, 0, LocalToWorld(FVector(-0.42, 1.12, -0.28)), D > 0.01 ? 0.9f : 0.f, FLinearColor(0.6f, 0.78f, 0.68f) * float(D), false);
		const double Up = Above;
		Web->SetFloodPool(CabSet, 1, LocalToWorld(FVector(0.1, 2.3, -0.2)), float(0.35 + 1.6 * Up), FLinearColor(float(0.32 + 4.5 * Up), float(0.36 + 4.6 * Up), float(0.46 + 4.6 * Up)), false);
		Web->FlushFlood(CabSet);
	}
	if (FloodSet < 0) return;
	const bool bOn = bHeadOn && LightLevel > 0.01;
	for (int32 j = 0; j < FloodSlots.Num(); ++j)
	{
		const FPool& F = FLOODS[j];
		// dim by colour, not by intensity: the pools fade instead of shrinking
		Web->SetFloodPool(FloodSet, FloodSlots[j], LocalToWorld(F.P), bOn ? float(F.W) : 0.f, HeadColor * float(LightLevel), false);
	}
	Web->FlushFlood(FloodSet);
}

void FS47Truck::PlaceHeadlights()
{
	if (!Web) return;
	const bool bOn = Group.Shown() && bHeadOn && LightLevel > 0.01;
	for (int32 I = 0; I < 2; ++I)
	{
		USpotLightComponent* L = Headlights[I].Get();
		if (!L) continue;
		const double X = I == 0 ? -0.657 : 0.657;
		const FVector P = Group.LocalToWorld(FVector(X, 0.8435, -2.65));
		// A slight downward aim keeps the verge visible without lighting the night sky.
		const FVector Target = Group.LocalToWorld(FVector(X, 0.1, -32.65));
		L->SetWorldLocationAndRotation(ToUE(P), (ToUE(Target) - ToUE(P)).Rotation());
		const float ColorGain = FMath::Max(1.f, FMath::Max3(HeadColor.R, HeadColor.G, HeadColor.B));
		L->SetLightColor(HeadColor / ColorGain);
		L->SetIntensity(bOn ? float(4200 * LightLevel * ColorGain) : 0.f);
		if (L->IsVisible() != bOn) L->SetVisibility(bOn);
	}
}

void FS47Truck::Update(double Dt, double Speed, double InWheelAngle)
{
	Spin -= Speed / WHEEL_R * Dt;
	WheelAngle = InWheelAngle;
	for (int32 i = 0; i < 4; ++i)
	{
		const bool bFront = i < 2, bLeft = WHEELS[i][0] < 0;
		// the left wheels are the same wheel turned round, so they spin the other way
		const FQuat Q = Euler(bLeft ? -Spin : Spin, (bFront ? -WheelAngle : 0) + (bLeft ? UE_DOUBLE_PI : 0), 0, TEXT("YXZ"));
		Wheels.SetInstance(i, FVector(WHEELS[i][0], WHEEL_R + Drops[i], WHEELS[i][1]), Q, FVector::OneVector, i == 3);
	}
	// needles have a little weight to them
	FGauge& G = Gauge;
	const double K = 1 - FMath::Exp(-Dt * 6);
	const bool bLive = GaugePower > 0.5;
	G.Speed += (G.TSpeed - G.Speed) * K;
	G.Fuel += ((bLive ? G.TFuel : 0) - G.Fuel) * K * (bLive ? 0.3 : 0.5);
	G.Temp += bLive ? (G.TTemp - G.Temp) * Dt * 0.02 : (0 - G.Temp) * K * 0.5;
	const double W = 0.56, H = 0.175;
	auto Needle = [&](int32 I, double Dx, double Dy, double From, double To, double F, double Len)
	{
		const double A = (From + (To - From) * F) * UE_DOUBLE_PI / 180;
		const FQuat Q(FVector(0, 0, 1), -A);
		Needles.SetInstance(I, FVector((Dx / 512 - 0.5) * W, (0.5 - Dy / 160) * H, 0.003), Q, FVector(1, Len, 1), I == 2);
	};
	Needle(0, 256, 88, -120, 120, G.Speed, 0.068);
	Needle(1, 88, 98, -60, 60, G.Fuel, 0.038);
	Needle(2, 424, 98, -60, 60, G.Temp, 0.038);
	CompassStep(Dt);
	// the keys swing on their ring: pushed by the turns and the road
	if (Dt > 0)
	{
		FSwing& Sw = Swing;
		const double Push = Speed * WheelAngle * 0.35 + (Speed - Sw.Last) / Dt * 0.004;
		Sw.Last = Speed;
		Sw.V += (-30 * Sw.A - 2.2 * Sw.V - Push) * Dt;
		Sw.A = FMath::Clamp(Sw.A + Sw.V * Dt, -0.7, 0.7);
		Keys.SetRotation(FVector(Sw.A * 0.4, 0, Sw.A));
	}
	FlushClock();
	PlaceFloods();
}

void FS47Truck::DrawDash()
{
	if (!Renderer || !Dash.IsValid() || !DashRT.IsValid()) return;
	Renderer->DrawWidget(DashRT.Get(), Dash.ToSharedRef(), FVector2D(512, 256), 0.f);
}

bool FS47Truck::RadioHit(const FVector& From, const FVector& Dir, double Far) const
{
	// the faceplate's hit box: 0.46 x 0.14 x 0.09 at (0.14, 1.06, -0.4) in the cab, tipped 0.2 back
	const FQuat Box = Euler(0.2, 0, 0);
	const FVector C(0.14, 1.06, -0.4), Half(0.23, 0.07, 0.045);
	const FVector O = Box.UnrotateVector(WorldToLocal(From) - C);
	const FVector D = Box.UnrotateVector(Quat().UnrotateVector(Dir));
	double T0 = 0, T1 = Far;
	for (int32 a = 0; a < 3; ++a)
	{
		if (FMath::Abs(D[a]) < 1e-9) { if (FMath::Abs(O[a]) > Half[a]) return false; continue; }
		double Ta = (-Half[a] - O[a]) / D[a], Tb = (Half[a] - O[a]) / D[a];
		if (Ta > Tb) Swap(Ta, Tb);
		T0 = FMath::Max(T0, Ta); T1 = FMath::Min(T1, Tb);
		if (T0 > T1) return false;
	}
	return true;
}
