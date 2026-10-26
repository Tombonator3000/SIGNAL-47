// ui/Ending.ts and style.css .ending: the end of the night, kept in the original order.
#include "SS47Ending.h"

#include "Brushes/SlateColorBrush.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "S47Image.h"
#include "SS47Screens.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SOverlay.h"

namespace
{
	// CSS ease: cubic-bezier(.25, .1, .25, 1), used by both opacity transitions.
	double Ease(double X)
	{
		X = FMath::Clamp(X, 0.0, 1.0);
		double U = X;
		for (int32 I = 0; I < 8; ++I)
		{
			const double B = 3 * (1 - U) * (1 - U) * U * .25 + 3 * (1 - U) * U * U * .25 + U * U * U - X;
			const double D = 3 * (1 - U) * (1 - U) * .25 + 3 * U * U * .75;
			if (FMath::Abs(D) < 1e-8) break;
			U = FMath::Clamp(U - B / D, 0.0, 1.0);
		}
		return 3 * (1 - U) * (1 - U) * U * .1 + 3 * (1 - U) * U * U + U * U * U;
	}

	void SetImage(FSlateBrush& B, UTexture2D* Texture)
	{
		B.SetResourceObject(Texture);
		B.ImageSize = Texture ? FVector2D(Texture->GetSizeX(), Texture->GetSizeY()) : FVector2D::ZeroVector;
		B.DrawAs = ESlateBrushDrawType::Image;
	}

	UTexture2D* PhotoFilter(UTexture2D* Src)
	{
		// Slate has no CSS filter; apply grayscale(1) contrast(1.05) to the canvas's sRGB bytes.
		if (!Src || !Src->GetPlatformData() || Src->GetPlatformData()->PixelFormat != PF_B8G8R8A8 || Src->GetPlatformData()->Mips.IsEmpty()) return Src;
		FTexture2DMipMap& Mip = Src->GetPlatformData()->Mips[0];
		const int32 W = Mip.SizeX, H = Mip.SizeY;
		if (W <= 0 || H <= 0 || Mip.BulkData.GetBulkDataSize() < int64(W) * H * int64(sizeof(FColor))) return Src;
		const FColor* Data = static_cast<const FColor*>(Mip.BulkData.LockReadOnly());
		TArray<FColor> Pixels;
		if (Data)
		{
			Pixels.SetNumUninitialized(W * H);
			for (int32 I = 0; I < Pixels.Num(); ++I)
			{
				const FColor C = Data[I];
				const double Grey = .2126 * C.R + .7152 * C.G + .0722 * C.B;
				const uint8 V = uint8(FMath::Clamp(FMath::RoundToInt((Grey - 127.5) * 1.05 + 127.5), 0, 255));
				Pixels[I] = FColor(V, V, V, C.A);
			}
		}
		Mip.BulkData.Unlock();
		return Pixels.IsEmpty() ? Src : S47Image::FromPixels(Pixels, W, H);
	}
}

TArray<TPair<FString, FString>> SS47Ending::CreditsRows(bool bBoughtSfx)
{
	// Ending.ts CREDITS (Slate draws text directly, so HTML esc() is unnecessary).
	TArray<TPair<FString, FString>> Rows = {
		{ TEXT(""), TEXT("A night in New Mexico, 14 April 1986") },
		{ TEXT("Story and direction"), TEXT("Tom") },
		{ TEXT("Code, chapters and sound"), TEXT("Claude (Anthropic)") },
		{ TEXT("Mesa Diner, Sierra Motor Court, room 6, art and textures"), TEXT("Codex and ChatGPT (OpenAI)") },
		{ TEXT("Music"), TEXT("Scott Buckley, CC BY 4.0 (scottbuckley.com.au): \"Signal to Noise\", and from \"Shadows and Dust\", \"Decoherence\", \"The Old Ones\", \"In Search of Solitude\", \"Neon\" and \"Hymn to the Dawn\"") },
		{ TEXT(""), TEXT("Chris Zabriskie, CC BY 4.0 (chriszabriskie.com): \"Cylinder One\", \"Cylinder Two\" and \"Cylinder Eight\" from Cylinders") },
		{ TEXT(""), TEXT("Kevin MacLeod, CC BY 4.0 (incompetech.com): \"Floating Cities\", \"Ossuary 1 - A Beginning\", \"Ossuary 5 - Rest\", \"Long Note Four\", \"Lightless Dawn\", \"Phantom from Space\" and \"Night on the Docks - Sax\"; on the radio \"Drankin Song\", \"Bama Country\", \"Cattails\" and \"Anamalie\"") },
		{ TEXT(""), TEXT("Shane Ivers, CC BY 4.0 (silvermansound.com): \"Signs To Nowhere\"") },
		{ TEXT("National anthem on the radio"), TEXT("The United States Air Force Band, \"The Star Spangled Banner\" (choral), public domain") },
		{ TEXT("Out in the dark"), TEXT("Freesound: resourcemoon (CC BY 3.0), InSintesi (CC BY 4.0), superhalo, szegvari and JarredGibb (CC0)") },
		{ TEXT("Sound effects, CC0"), TEXT("Freesound: inspire153 and craigsmith (the truck engine), viertelnachvier, transitking, geraldfiebig, DarkShroom, FreethinkerAnon, Gamba_Studio, rvandemark, qubodup, sqeeeek, Yoyodaman234, matth3wc04, BlondPanda, GiocoSound, Notarget, cabled_mess, CastIronCarousel, JarredGibb, Lunardrive, wjtaylor, yatoimtop, LilMati. Kenney.") },
	};
	// Not asked for by their licence; named because the others are.
	if (bBoughtSfx) Rows.Emplace(TEXT("Sound effects"), TEXT("Ultimate SFX for Games, Function Loops"));
	// Departs from the web version: the Unreal world also uses the bought Vintage Liminal models and the
	// free ones (CC BY 4.0 asks for title, author, licence and link: the links are in CREDITS.txt).
#include "S47FreeCredits.inl"
	Rows.Emplace(TEXT("Fonts"), TEXT("VT323, Reenie Beanie and Oswald (SIL Open Font License 1.1). Special Elite (Apache License 2.0)."));
	Rows.Emplace(TEXT("Built with"), TEXT("Unreal Engine 5.8.4 (Epic Games)"));
	Rows.Emplace(TEXT(""), TEXT("Everything in this story is invented. The comet was real."));
	return Rows;
}

void SS47Ending::Construct(const FArguments& InArgs)
{
	bBoughtSfx = InArgs._BoughtSfx;
	bLargeText = InArgs._LargeText;
	SetVisibility(EVisibility::SelfHitTestInvisible);
	SetClipping(EWidgetClipping::ClipToBounds);
	SkipButton = S47Css::BoxButton(TEXT("Skip"), S47Css::Term(20), S47Css::C(0x8f8a7c), FLinearColor::Transparent, S47Css::C(0x4a463e), [this] { Skip(); }, FMargin(16 * S47Css::PX, 6 * S47Css::PX));
	SkipButton->SetVisibility(TAttribute<EVisibility>::CreateLambda([this] { return !bGone && Card == ECard::Credits && T >= .06 + 4 ? EVisibility::Visible : EVisibility::Collapsed; }));
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0, 0, 18 * S47Css::PX, 16 * S47Css::PX))
		[
			SkipButton.ToSharedRef()
		]
	];
}

void SS47Ending::Flash(UTexture2D* C, double Opacity)
{
	// Ending.flash
	if (bGone) return;
	if (!C) { Frame.Reset(); FrameOpacity = 0; return; }
	if (Frame.Get() != C) { Frame.Reset(C); SetImage(FrameBrush, C); }
	FrameOpacity = FMath::Clamp(Opacity, 0.0, 1.0);
}

void SS47Ending::Black(bool bOn, double Ms)
{
	// Ending.black
	if (bGone) return;
	const double To = bOn ? 1 : 0;
	// Assigning the same CSS opacity again leaves its existing transition running.
	if (CoverTo == To) return;
	CoverFrom = CoverAlpha();
	CoverTo = To;
	CoverT = 0;
	CoverDuration = FMath::Max(0.0, Ms / 1000);
}

void SS47Ending::BeginCard(ECard Kind, TFunction<void()> Done)
{
	Card = Kind;
	T = 0;
	OnDone = MoveTemp(Done);
	OnPaper = {};
	bPaperPlayed = false;
}

void SS47Ending::Title(TFunction<void()> Done)
{
	// Ending.title: 60 ms; on for 5600 ms; off for 1400 ms; remove.
	if (bGone) return;
	BeginCard(ECard::Title, MoveTemp(Done));
}

void SS47Ending::Credits(TFunction<void()> Done)
{
	// Ending.credits: the roll begins after 60 ms, takes 46 s (here CreditsSeconds), and may be skipped after 4 s.
	if (bGone) return;
	BeginCard(ECard::Credits, MoveTemp(Done));
	CreditsOut = .06 + CreditsSeconds;
}

void SS47Ending::Photo(UTexture2D* Src, TFunction<void()> Paper, TFunction<void()> Done)
{
	// Ending.photo: 1500 ms, paper and card, 60 ms, on for 6000 + 8000 ms, off for 2200 ms,
	// remove, then another 1800 ms before returning to the title.
	if (bGone) return;
	BeginCard(ECard::Photo, MoveTemp(Done));
	Photograph.Reset(PhotoFilter(Src));
	SetImage(PhotoBrush, Photograph.Get());
	OnPaper = MoveTemp(Paper);
}

void SS47Ending::Dispose()
{
	// Ending.dispose: cancel timers and root.remove().
	bGone = true;
	Card = ECard::None;
	OnDone = {};
	OnPaper = {};
	Frame.Reset();
	Photograph.Reset();
	SetVisibility(EVisibility::Collapsed);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(SharedThis(this));
}

void SS47Ending::Complete()
{
	TFunction<void()> Done = MoveTemp(OnDone);
	Card = ECard::None;
	OnDone = {};
	OnPaper = {};
	Photograph.Reset();
	if (Done) Done();
}

void SS47Ending::Advance(double Dt)
{
	if (bGone) return;
	Dt = FMath::Max(0.0, Dt);
	CoverT += Dt;
	if (Card == ECard::None) return;
	T += Dt;
	if (Card == ECard::Photo && T >= 1.5 && !bPaperPlayed)
	{
		bPaperPlayed = true;
		TFunction<void()> Paper = MoveTemp(OnPaper);
		OnPaper = {};
		if (Paper) Paper();
		if (bGone) return;
	}
	const double End = Card == ECard::Title ? .06 + 5.6 + 1.4 : Card == ECard::Credits ? CreditsOut + 1.2 : 1.5 + .06 + 6 + 8 + 2.2 + 1.8;
	if (T >= End) Complete();
}

bool SS47Ending::SkipOffered() const
{
	return !bGone && Card == ECard::Credits && T >= .06 + 4 && T < CreditsOut;
}

bool SS47Ending::Skip()
{
	if (!SkipOffered()) return false;
	CreditsOut = T;
	return true;
}

double SS47Ending::CoverAlpha() const
{
	return CoverDuration <= 0 ? CoverTo : FMath::Lerp(CoverFrom, CoverTo, Ease(CoverT / CoverDuration));
}

double SS47Ending::CardAlpha() const
{
	if (!CardPresent()) return 0;
	const double Start = Card == ECard::Photo ? 1.5 + .06 : .06;
	const double Off = Card == ECard::Title ? .06 + 5.6 : Card == ECard::Credits ? CreditsOut : 1.5 + .06 + 6 + 8;
	const double Duration = Card == ECard::Photo ? 6 : 1.4;
	if (T < Off) return Ease((T - Start) / Duration);
	return 1 - Ease((T - Off) / Duration);
}

bool SS47Ending::CardPresent() const
{
	return Card != ECard::None && (Card != ECard::Photo || (T >= 1.5 && T < 1.5 + .06 + 6 + 8 + 2.2));
}

void SS47Ending::Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(G, InCurrentTime, InDeltaTime);
	// Slate's delta is real time, independent of the stopped 3D game and Host->Gt().
	Advance(InDeltaTime);
	if (SkipButton) SkipButton->SetRenderOpacity(float(CardAlpha()));
}

void SS47Ending::LayoutCredits(const FVector2D& Size) const
{
	if (CreditsSize.Equals(Size, .1)) return;
	CreditsSize = Size;
	CreditLines.Reset();
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const float Width = FMath::Min(560.f * S47Css::PX, float(Size.X) * .88f);
	const float TitlePx = FMath::Clamp(float(Size.X) * .08f, 40 * S47Css::PX, 84 * S47Css::PX);
	const FSlateFontInfo TitleFont = S47Css::Display(TitlePx / S47Css::PX);
	CreditsTitleHeight = float(Measure->Measure(TEXT("SIGNAL/47"), TitleFont).Y);
	float Y = CreditsTitleHeight + float(Size.Y) * .08f;
	FSlateFontInfo KeyFont = S47Css::Term(18);
	KeyFont.LetterSpacing = 100; // .cr-k: letter-spacing .1em
	const FSlateFontInfo ValueFont = S47Css::Type(bLargeText ? 22 : 18);
	auto AddText = [&](const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Color, float LineHeight)
	{
		TArray<FString> Words;
		Text.ParseIntoArrayWS(Words);
		FString Line;
		for (const FString& Word : Words)
		{
			const FString Next = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
			if (!Line.IsEmpty() && Measure->Measure(Next, Font).X > Width)
			{
				CreditLines.Add({ Line, Font, Color, Y, LineHeight });
				Y += LineHeight;
				Line = Word;
			}
			else Line = Next;
		}
		if (!Line.IsEmpty()) { CreditLines.Add({ Line, Font, Color, Y, LineHeight }); Y += LineHeight; }
	};
	for (const TPair<FString, FString>& Row : CreditsRows(bBoughtSfx))
	{
		if (!Row.Key.IsEmpty())
		{
			AddText(Row.Key.ToUpper(), KeyFont, S47Css::C(0x8f8a7c), float(Measure->Measure(TEXT("X"), KeyFont).Y));
			Y += .4f * 18 * S47Css::PX;
		}
		// a value may be a list, one entry a line (the models' credits)
		TArray<FString> Paragraphs;
		Row.Value.ParseIntoArray(Paragraphs, TEXT("\n"), false);
		for (const FString& Paragraph : Paragraphs) AddText(Paragraph, ValueFont, S47Css::C(0xd9d2c0), (bLargeText ? 22.f : 18.f) * S47Css::PX * 1.55f);
		Y += float(Size.Y) * .055f;
	}
	CreditsHeight = Y;
}

int32 SS47Ending::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	if (bGone) return Layer;
	const FVector2f Size(G.GetLocalSize());
	static const FSlateColorBrush White(FLinearColor::White);
	auto BlackBox = [&](int32 At, double Alpha)
	{
		if (Alpha > 0) FSlateDrawElement::MakeBox(Out, At, G.ToPaintGeometry(), &White, ESlateDrawEffect::None, FLinearColor(0, 0, 0, float(Alpha)));
	};
	auto Image = [&](int32 At, const FSlateBrush& Brush, bool bContain, double Alpha)
	{
		if (Alpha <= 0 || Brush.ImageSize.X <= 0 || Brush.ImageSize.Y <= 0) return;
		const float K = bContain ? FMath::Min(Size.X / float(Brush.ImageSize.X), Size.Y / float(Brush.ImageSize.Y)) : FMath::Max(Size.X / float(Brush.ImageSize.X), Size.Y / float(Brush.ImageSize.Y));
		const FVector2f DrawSize(float(Brush.ImageSize.X) * K, float(Brush.ImageSize.Y) * K);
		FSlateDrawElement::MakeBox(Out, At, G.ToPaintGeometry(DrawSize, FSlateLayoutTransform((Size - DrawSize) * .5f)), &Brush, ESlateDrawEffect::None, FLinearColor(1, 1, 1, float(Alpha)));
	};
	// FLASH canvases already contain the frame's black background. Drawing their opacity once
	// preserves CSS's group opacity during slow fades. The cover is above the frame.
	if (Frame.IsValid()) Image(Layer + 1, FrameBrush, false, FrameOpacity);
	BlackBox(Layer + 2, CoverAlpha());
	const double Alpha = CardAlpha();
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	auto Text = [&](const FString& S, const FSlateFontInfo& Font, float X, float Y, const FLinearColor& Color)
	{
		const FVector2f Extent(Measure->Measure(S, Font));
		FSlateDrawElement::MakeText(Out, Layer + 4, G.ToPaintGeometry(Extent, FSlateLayoutTransform(FVector2f(X, Y))), S, Font, ESlateDrawEffect::None, Color.CopyWithNewOpacity(Color.A * float(Alpha)));
	};
	auto Heading = [&](float Px, float Y)
	{
		const FSlateFontInfo Font = S47Css::Display(Px / S47Css::PX);
		const float A = float(Measure->Measure(TEXT("SIGNAL"), Font).X), B = float(Measure->Measure(TEXT("/"), Font).X), C = float(Measure->Measure(TEXT("47"), Font).X);
		const float Gap = Px * .12f, X = (Size.X - A - B - C - 2 * Gap) * .5f;
		Text(TEXT("SIGNAL"), Font, X, Y, S47Css::C(0xf3efe4));
		Text(TEXT("/"), Font, X + A + Gap, Y, S47Css::C(0xff5a45));
		Text(TEXT("47"), Font, X + A + Gap + B + Gap, Y, S47Css::C(0xf3efe4));
	};
	if (Alpha > 0 && Card == ECard::Title)
	{
		const float Px = FMath::Clamp(Size.X * .14f, 64 * S47Css::PX, 180 * S47Css::PX);
		const float H = float(Measure->Measure(TEXT("SIGNAL/47"), S47Css::Display(Px / S47Css::PX)).Y);
		Heading(Px, (Size.Y - H) * .5f);
	}
	else if (Alpha > 0 && Card == ECard::Credits)
	{
		BlackBox(Layer + 3, Alpha);
		LayoutCredits(FVector2D(Size));
		// roll has 100vh top padding, then translateY(calc(-100% - 60vh)) over 46 seconds (CreditsSeconds).
		const float K = float(FMath::Clamp((T - .06) / CreditsSeconds, 0.0, 1.0));
		const float Y = Size.Y - K * (Size.Y * 1.6f + CreditsHeight);
		Heading(FMath::Clamp(Size.X * .08f, 40 * S47Css::PX, 84 * S47Css::PX), Y);
		for (const FCreditLine& Line : CreditLines)
		{
			const float At = Y + Line.Y;
			if (At + Line.Height < 0 || At > Size.Y) continue;
			const float W = float(Measure->Measure(Line.Text, Line.Font).X);
			Text(Line.Text, Line.Font, (Size.X - W) * .5f, At, Line.Color);
		}
	}
	else if (Alpha > 0 && Card == ECard::Photo)
	{
		BlackBox(Layer + 3, Alpha);
		if (Photograph.IsValid()) Image(Layer + 4, PhotoBrush, true, Alpha);
	}
	return SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer + 5, Style, bParentEnabled);
}

bool SS47Ending::SelfTest(TArray<FString>& Lines)
{
	// Ending.ts timer scenarios without a scene. These do not claim visual or real-input coverage.
	bool bOk = true;
	auto Check = [&](bool bPass, const TCHAR* Name) { Lines.Add(FString(bPass ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("Ending: ") + Name); bOk &= bPass; };
	const TSharedRef<SS47Ending> E = SNew(SS47Ending);
	int32 Done = 0, Paper = 0;
	E->Title([&] { ++Done; });
	E->Advance(.059);
	Check(E->CardAlpha() == 0 && Done == 0, TEXT("title waits 60 ms before fading in"));
	E->Advance(.001);
	E->Advance(5.6);
	Check(E->Card == ECard::Title && Done == 0 && FMath::IsNearlyEqual(E->CardAlpha(), 1.0, 1e-8), TEXT("title remains for the original 5.6 seconds"));
	E->Advance(1.399);
	Check(Done == 0, TEXT("title waits through its 1.4 second fade"));
	E->Advance(.002);
	Check(Done == 1 && E->Card == ECard::None, TEXT("title completes after 7.06 seconds"));
	E->Credits([&] { ++Done; });
	E->Advance(4.059);
	Check(!E->SkipOffered() && !E->Skip(), TEXT("credits cannot be skipped before four seconds of rolling"));
	E->Advance(.002);
	Check(E->SkipOffered() && E->Skip(), TEXT("Skip is offered four seconds after the 60 ms lead"));
	E->Advance(1.199);
	Check(Done == 1, TEXT("Skip preserves the credits' 1.2 second fade"));
	E->Advance(.002);
	Check(Done == 2, TEXT("Skip finishes the credits once"));
	E->Credits([&] { ++Done; });
	E->Advance(CreditsSeconds + .059);
	Check(Done == 2 && E->SkipOffered(), TEXT("unskipped credits roll for CreditsSeconds"));
	E->Advance(.002);
	Check(Done == 2 && !E->SkipOffered(), TEXT("credits begin fading after the roll and the 60 ms lead"));
	E->Advance(1.2);
	Check(Done == 3, TEXT("unskipped credits complete after the fade"));
	E->Photo(nullptr, [&] { ++Paper; }, [&] { ++Done; });
	E->Advance(1.499);
	Check(Paper == 0 && !E->CardPresent(), TEXT("photograph waits 1.5 seconds before paper and card"));
	E->Advance(.002);
	Check(Paper == 1 && E->CardPresent() && E->CardAlpha() == 0, TEXT("paper plays once before the photograph's 60 ms lead"));
	E->Advance(.059);
	E->Advance(14);
	Check(Paper == 1 && Done == 3 && FMath::IsNearlyEqual(E->CardAlpha(), 1.0, 1e-8), TEXT("photograph holds for 6000 plus 8000 milliseconds"));
	E->Advance(2.201);
	Check(!E->CardPresent() && Done == 3, TEXT("photograph is removed 2.2 seconds into its six second fade"));
	E->Advance(1.801);
	Check(Done == 4, TEXT("photograph completes after its final 1.8 second wait"));
	E->Black(true, 600);
	E->Advance(.3);
	const double Halfway = E->CoverAlpha();
	E->Black(true, 600);
	Check(E->CoverAlpha() == Halfway, TEXT("repeated black target preserves the running transition"));
	E->Advance(.3);
	Check(E->CoverAlpha() == 1, TEXT("black reaches full opacity after 600 ms despite repeated calls"));
	E->Black(false, 600);
	E->Advance(.3);
	const double Interrupted = E->CoverAlpha();
	E->Black(true, 600);
	Check(FMath::IsNearlyEqual(E->CoverAlpha(), Interrupted, 1e-8), TEXT("interrupted cover transitions preserve their current opacity"));
	E->Title([&] { ++Done; });
	E->Dispose();
	E->Advance(100);
	Check(E->Disposed() && Done == 4 && !E->Skip(), TEXT("dispose cancels pending callbacks and the overlay"));
	const TArray<TPair<FString, FString>> Free = CreditsRows(false), Bought = CreditsRows(true);
	Check(Free.Num() == 18 && Bought.Num() == 19 && Bought[11].Value == TEXT("Ultimate SFX for Games, Function Loops"), TEXT("bought SFX credit is conditional and in the original order"));
	const TPair<FString, FString>* Models = Free.FindByPredicate([](const TPair<FString, FString>& R) { return R.Key.StartsWith(TEXT("3D models, CC BY")); });
	TArray<FString> ModelLines;
	if (Models) Models->Value.ParseIntoArray(ModelLines, TEXT("\n"));
	Check(ModelLines.Num() == 48 && Free.ContainsByPredicate([](const TPair<FString, FString>& R) { return R.Value.Contains(TEXT("Vintage Liminal")); }), TEXT("the 48 CC BY models and the bought Vintage Liminal pack are credited"));
	Check(Free[0].Value == TEXT("A night in New Mexico, 14 April 1986") && Free.Last().Value == TEXT("Everything in this story is invented. The comet was real."), TEXT("credits retain their original opening and closing text"));
	return bOk;
}
