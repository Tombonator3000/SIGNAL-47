// world/ServiceYard.ts (the behaviour; the geometry comes from the web version itself)
#include "S47Yard.h"

#include "Components/MaterialBillboardComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "RenderingThread.h"
#include "S47Docs.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Slate/WidgetRenderer.h"
#include "Brushes/SlateColorBrush.h"
#include "SS47Widgets.h"
#include "Widgets/SLeafWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Yard, Log, All);

// ServiceYard.drawBus: the 256 x 160 canvas behind the motor bus's inspection glass, painted as
// the control room's CRTs are (SS47Crt), into a render target on the screen's material.
class SS47Bus : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Bus) {}
	SLATE_END_ARGS()
	void Construct(const FArguments&) { White = MakeUnique<FSlateColorBrush>(FLinearColor::White); }
	double Az = 26, T = 0;
	int32 Commands = 0;
	bool bWarn = true;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(256, 160); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		// the canvas's colours, linear: drawn without Slate's own gamma into an sRGB target, the
		// target holds the canvas's bytes, and the screen's material reads them back as three does
		auto Css = [](uint32 C) { return FLinearColor::FromSRGBColor(FColor((C >> 16) & 255, (C >> 8) & 255, C & 255)); };
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), White.Get(), ESlateDrawEffect::None, Css(0x0b0d08));
		// fillText puts the baseline at y; Slate the top of the line, the font's ascent above it
		auto Txt = [&](const FString& S, float X, float Y, float Px, const FLinearColor& C)
		{
			const FSlateFontInfo F = S47Style::Mono(Px);
			TSharedRef<FSlateFontMeasure> Me = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const float Ascent = float(Me->GetMaxCharacterHeight(F) + Me->GetBaseline(F));
			FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(260.f, Px * 1.6f), FSlateLayoutTransform(FVector2f(X, Y - Ascent))), S, F, ESlateDrawEffect::None, C);
		};
		const FLinearColor Amber = Css(0xffb052);
		Txt(TEXT("MOTOR BUS S-03"), 12, 26, 22, Amber);
		Txt(TEXT("AZIMUTH"), 12, 58, 18, Amber);
		Txt(S47Text::PadStart(FString::FromInt(FMath::FloorToInt(Az + 0.5)), 3, TEXT('0')) + TEXT("°"), 12, 104, 52, Amber);
		Txt(FString::Printf(TEXT("SCHED 042   CMD %d"), Commands), 120, 58, 18, Amber);
		if (!bWarn || FMath::FloorToInt(T * 2) % 2 == 0)
		{
			Txt(TEXT("NO CONTROL COMMAND"), 12, 134, 18, Css(0xffd27a));
			Txt(TEXT("RECORDED"), 12, 152, 18, Css(0xffd27a));
		}
		return Layer + 2;
	}

private:
	TUniquePtr<FSlateBrush> White;
};

FS47Obj US47Yard::Obj(const FString& Name) const
{
	return M && M->Web ? M->Web->Obj(TEXT("yard.objs.") + Name) : FS47Obj();
}

void US47Yard::Init(US47Main* InM)
{
	M = InM;
	if (!M || !M->Web) return;
	AS47Web* W = M->Web;
	VaneObj = W->Obj(TEXT("yard.vane"));
	VaneStripe = W->Obj(TEXT("yard.vaneStripe"));
	Echo = W->Obj(TEXT("yard.echo"));
	WetPrint = W->Obj(TEXT("yard.wetPrint"));
	LabDoorHinge = W->Obj(TEXT("yard.labDoorHinge"));
	DryPrints.Reset();
	for (int32 i = 0; i < 4; ++i) DryPrints.Add(W->Obj(FString::Printf(TEXT("yard.dryPrints[%d]"), i)));
	Occluders.Reset();
	for (const FString& P : W->PathsUnder(TEXT("yard.occluders["))) Occluders.Add(W->Obj(P));
	// the motor bus screen: a render target drawn here in place of the exported canvas
	if (FApp::CanEverRender())
	{
		Bus = NewObject<UTextureRenderTarget2D>(this);
		Bus->ClearColor = FLinearColor(0.004f, 0.004f, 0.003f);
		Bus->InitCustomFormat(256, 160, PF_B8G8R8A8, false);
		Bus->UpdateResourceImmediate(true);
		Renderer = new FWidgetRenderer(false);
		BusWidget = SNew(SS47Bus);
		// (rows: a render target runs the other way from three's flipped canvas, as the CRTs)
		W->Material(TEXT("yard.screens[0]")).SetMap(Bus, true);
		// The screen lies in the plane of the cabinet's face (faced() puts the face 2 mm out, the
		// screen is placed at the same 0.232); three draws it over the face, Unreal's depth pass
		// lets the two fight. The screen comes 2 mm further out.
		const int32 ScreenMat = W->HandleMat(TEXT("yard.screens[0]"));
		Obj(TEXT("motorBus")).Traverse([ScreenMat](FS47Obj O)
		{
			const FS47WebNode* N = O.W->Node(O.I);
			if (N && N->Mats.Contains(ScreenMat)) O.SetPosition(O.Position() + FVector(0, 0, 0.002));
		});
	}
	FindWorkLamp();
}

void US47Yard::BeginDestroy()
{
	if (Renderer) { BeginCleanup(Renderer); Renderer = nullptr; }
	BusWidget.Reset();
	Super::BeginDestroy();
}

TArray<TSharedPtr<FJsonValue>> US47Yard::ManifestSection(const FString& Key)
{
	// "key": [ ... ] at the top of the manifest, cut out by its brackets and parsed alone
	static TMap<FString, TArray<TSharedPtr<FJsonValue>>> Cache;
	if (const TArray<TSharedPtr<FJsonValue>>* Have = Cache.Find(Key)) return *Have;
	TArray<TSharedPtr<FJsonValue>>& Out = Cache.Add(Key);
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("WebData/manifest.json")))) return Out;
	const FString Tag = TEXT("\"") + Key + TEXT("\":");
	int32 I = Text.Find(Tag, ESearchCase::CaseSensitive);
	if (I < 0) return Out;
	I += Tag.Len();
	while (I < Text.Len() && FChar::IsWhitespace(Text[I])) ++I;
	if (I >= Text.Len() || Text[I] != TEXT('[')) return Out;
	int32 Depth = 0, E = I;
	bool bStr = false;
	for (; E < Text.Len(); ++E)
	{
		const TCHAR C = Text[E];
		if (bStr) { if (C == TEXT('\\')) ++E; else if (C == TEXT('"')) bStr = false; continue; }
		if (C == TEXT('"')) bStr = true;
		else if (C == TEXT('[') || C == TEXT('{')) ++Depth;
		else if (C == TEXT(']') || C == TEXT('}')) { if (--Depth == 0) break; }
	}
	const S47J::FObj O = S47J::Read(TEXT("{\"a\":") + Text.Mid(I, E - I + 1) + TEXT("}"));
	Out = S47J::Arr(O, TEXT("a"));
	return Out;
}

void US47Yard::FindWorkLamp()
{
	// ServiceYard.b12: the work lamp is addFlood(lp.x + 0.45, 2.75, lp.z + 0.1, 7, 0xffb46a) in the
	// site floods and glow.add(lp.x + 0.42, 2.82, lp.z + 0.2, 0.7, 0xffc985), lp = (vane.x - 0.9,
	// vane.z + 0.45). Neither index is a handle the export keeps, so they are found by place.
	const FVector Lp(S47YardPlaces::Vane().X - 0.9, 0, S47YardPlaces::Vane().Z + 0.45);
	const FVector Pool(Lp.X + 0.45, 2.75, Lp.Z + 0.1), Glow(Lp.X + 0.42, 2.82, Lp.Z + 0.2);
	const TArray<TSharedPtr<FJsonValue>> Floods = ManifestSection(TEXT("floods"));
	for (int32 s = 0; s < Floods.Num() && WorkLamp < 0; ++s)
	{
		const S47J::FObj F = Floods[s]->AsObject();
		const TArray<TSharedPtr<FJsonValue>> Pools = S47J::Arr(F, TEXT("pools"));
		for (int32 k = 0; k < Pools.Num(); ++k)
		{
			const S47J::FObj P = Pools[k]->AsObject();
			if (FVector::Dist(FVector(S47J::Num(P, TEXT("x")), S47J::Num(P, TEXT("y")), S47J::Num(P, TEXT("z"))), Pool) < 0.02) { FloodSetIdx = s; WorkLamp = k; break; }
		}
	}
	if (WorkLamp < 0 && M && M->Web)
	{
		// the export of 22 October: the site floods, pool 16
		FloodSetIdx = M->Web->FloodSet(TEXT("site"));
		WorkLamp = 16;
		UE_LOG(LogS47Yard, Warning, TEXT("S47: the work lamp's flood was not found in the manifest; site pool 16"));
	}
	// the glow: the billboard at that place under yard.glow.points (the fifth point, as added)
	WorkGlow = -1;
	const FS47Obj Pts = M && M->Web ? M->Web->Obj(TEXT("yard.glow.points")) : FS47Obj();
	const FS47WebNode* N = Pts ? Pts.W->Node(Pts.I) : nullptr;
	if (N && N->Comp.IsValid())
	{
		int32 k = 0, Fifth = -1;
		const FVector Want = Pts.LocalToWorld(Glow);
		for (USceneComponent* C : N->Comp->GetAttachChildren())
		{
			if (!Cast<UMaterialBillboardComponent>(C)) continue;
			if (k == 4) Fifth = k;
			if (FVector::Dist(S47Three::ToThree(C->GetComponentLocation()), Want) < 0.03) { WorkGlow = k; break; }
			++k;
		}
		if (WorkGlow < 0) WorkGlow = Fifth;
	}
}

void US47Yard::SetVane(double YawDeg)
{
	VaneObj.SetRotY(YawDeg * PI / 180);
}

void US47Yard::SetWorkLamp(double Level)
{
	if (M && M->Web && FloodSetIdx >= 0 && WorkLamp >= 0) M->Web->SetFlood(FloodSetIdx, WorkLamp, float(WorkLampBase * Level));
	// the glow point's colour: 0xffc985 times min(1.4, level)
	const FS47Obj Pts = M && M->Web ? M->Web->Obj(TEXT("yard.glow.points")) : FS47Obj();
	const FS47WebNode* N = Pts ? Pts.W->Node(Pts.I) : nullptr;
	if (!N || !N->Comp.IsValid() || WorkGlow < 0) return;
	const FLinearColor C = S47Three::Hex(0xffc985) * float(FMath::Min(1.4, Level));
	int32 k = 0;
	for (USceneComponent* Ch : N->Comp->GetAttachChildren())
	{
		UMaterialBillboardComponent* B = Cast<UMaterialBillboardComponent>(Ch);
		if (!B) continue;
		if (k++ != WorkGlow) continue;
		if (B->Elements.Num() > 0)
		{
			if (UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(B->Elements[0].Material)) D->SetVectorParameterValue(TEXT("Color"), FLinearColor(C.R, C.G, C.B, 1.f));
		}
		break;
	}
}

void US47Yard::DrawBus(double Az, int32 Commands, bool bWarn, double T)
{
	if (!Renderer || !Bus || !BusWidget.IsValid() || !FSlateApplication::IsInitialized()) return;
	BusWidget->Az = Az;
	BusWidget->Commands = Commands;
	BusWidget->bWarn = bWarn;
	BusWidget->T = T;
	Renderer->DrawWidget(Bus, BusWidget.ToSharedRef(), FVector2D(256, 160), 0.f);
}
