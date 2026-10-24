#include "SS47Notebook.h"
#include "S47Keys.h"
#include "S47Gamepad.h"

#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/Regex.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "S47GameStyle.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "SS47AreaMap.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"
#include "TextureResource.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCanvas.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	using S47Game::P;
	using S47Game::P2;
	using S47Game::C;

	// the page's own colours (notebook.html)
	FLinearColor Ph2Red() { return C(0x8a3b2c); }       // .ph2: the line under a heading
	FLinearColor UseInk() { return C(0x5d4f3a); }       // .use: the actions at a page's foot
	FLinearColor TypedInk() { return C(0x25211a); }     // .desc
	const FButtonStyle* Flat() { return &FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"); }

	// a button that is only its content (not focusable: the notebook keeps the keys)
	TSharedRef<SButton> Click(const TSharedRef<SWidget>& Content, TFunction<void()> Fn)
	{
		return SNew(SButton).ButtonStyle(Flat()).ContentPadding(0).IsFocusable(false)
			.OnClicked_Lambda([Fn] { if (Fn) Fn(); return FReply::Handled(); })[Content];
	}

	// the spiral down the gutter (.rings: a bar of wire every 46px)
	class SRings : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SRings) {}
		SLATE_END_ARGS()
		void Construct(const FArguments&) {}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8, 8); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const FVector2f S = FVector2f(G.GetLocalSize());
			const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
			for (float Y = P(22); Y + P(10) <= S.Y; Y += P(46))
			{
				FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(S.X, P(8)), FSlateLayoutTransform(FVector2f(0, Y))), White, ESlateDrawEffect::None, C(0x9aa0a6));
				FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(S.X, P(2)), FSlateLayoutTransform(FVector2f(0, Y + P(8)))), White, ESlateDrawEffect::None, C(0x5f656b));
			}
			return Layer + 1;
		}
	};

	// the four arrow keys' arrows, drawn (VT323 has none)
	class SArrows : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SArrows) {}
		SLATE_END_ARGS()
		void Construct(const FArguments&) {}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(P(4 * 14 + 3 * 8), P(22)); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const FLinearColor Col = S47Game::Paper();
			const float Y = P(11), H = P(5), Wd = FMath::Max(1.f, P(1.4f));
			auto Arrow = [&](float Cx, FVector2f Dir)
			{
				const FVector2f Cn(Cx, Y), Tip = Cn + Dir * P(6), Tail = Cn - Dir * P(6), Side(-Dir.Y, Dir.X);
				FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), TArray<FVector2f>{ Tail, Tip }, ESlateDrawEffect::None, Col, true, Wd);
				FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), TArray<FVector2f>{ Tip - Dir * H + Side * H * 0.8f, Tip, Tip - Dir * H - Side * H * 0.8f }, ESlateDrawEffect::None, Col, true, Wd);
			};
			Arrow(P(7), FVector2f(-1, 0)); Arrow(P(29), FVector2f(1, 0)); Arrow(P(51), FVector2f(0, -1)); Arrow(P(73), FVector2f(0, 1));
			return Layer + 1;
		}
	};

	// a page's heading in the hand and the line under it (.ph, .ph2)
	TSharedRef<SWidget> Head(const FString& Title, const FString& Line)
	{
		using namespace S47Game;
		TSharedRef<SVerticalBox> B = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(P(56))[SNew(STextBlock).Text(FText::FromString(Title)).Font(Hand(52)).ColorAndOpacity(InkBlue())]];
		if (!Line.IsEmpty()) B->AddSlot().AutoHeight().Padding(P(2), 0, 0, 0)[SNew(STextBlock).Text(FText::FromString(Line)).Font(Term(20, 2)).ColorAndOpacity(Ph2Red())];
		return B;
	}

	// a text in the hand on the page's lines (34px apart)
	TSharedRef<STextBlock> Inked(const FString& S, float Px = 30.f, const FLinearColor& Col = S47Game::InkBlue())
	{
		using namespace S47Game;
		return SNew(STextBlock).Text(FText::FromString(S)).Font(Hand(Px)).ColorAndOpacity(Col).AutoWrapText(true).LineHeightPercentage(LinePercent(Hand(Px), 34));
	}
	TSharedRef<STextBlock> Typed(const FString& S, float Px = 19.f, const FLinearColor& Col = TypedInk(), float Pitch = 34.f)
	{
		using namespace S47Game;
		return SNew(STextBlock).Text(FText::FromString(S)).Font(Type(Px)).ColorAndOpacity(Col).AutoWrapText(true).LineHeightPercentage(LinePercent(Type(Px), Pitch));
	}
	TSharedRef<SWidget> Empty(const FString& S)
	{
		// (.empty: read as a description, UI.notebook's descriptionReadings)
		return SNew(SBox).Padding(FMargin(0, P(10), 0, 0))[Typed(S, 17.f, C(0x7d7563))];
	}

	// a picture by its data URL or its art name (core/art.ts)
	UTexture2D* Picture(const FString& Src)
	{
		if (Src.IsEmpty()) return nullptr;
		return Src.StartsWith(TEXT("data:")) ? S47Image::FromDataUrl(Src) : S47Image::Art(Src);
	}

	// CONTROLS: every control, keyboard and mouse, and a phone or tablet (the hints about them are
	// said only once on a device, core/hints.ts, so they are all kept here). The keyboard's page shows
	// the keys as bound (S47Keys, Settings > Controls), where the web version's are fixed; with the
	// keys the game came with it reads as the web's table.
	struct FCtlGroup { FString Head; TArray<TPair<FString, FString>> Rows; };
	struct FCtlPage { FString Title; TArray<FCtlGroup> Groups; };

	// The keyboard table describes the two rebound keyboard/mouse slots. KeysOf also contains
	// fixed Xbox gameplay aliases, which belong to the controller hints rather than this table.
	TArray<FKey, TInlineAllocator<2>> KeyboardKeys(FName Action)
	{
		TArray<FKey, TInlineAllocator<2>> Out;
		for (int32 Slot = 0; Slot < 2; ++Slot)
		{
			const FKey K = S47Keys::Key(Action, Slot);
			if (K.IsValid() && !K.IsGamepadKey()) Out.Add(K);
		}
		return Out;
	}

	// an action's keys for the key column: "Tab, N", "E, Enter" ("(no key)" when it has none)
	FString KeysSaid(FName Action, bool bFirstOnly = false)
	{
		TArray<FString> W;
		for (const FKey& K : KeyboardKeys(Action))
		{
			FString N = K == EKeys::Escape ? FString(TEXT("Esc")) : S47Keys::Name(K);
			if (N.StartsWith(TEXT("a "))) N.RightChopInline(2);
			if (N.StartsWith(TEXT("the "))) N.RightChopInline(4);
			if (!N.IsEmpty()) N[0] = FChar::ToUpper(N[0]);
			W.Add(N);
			if (bFirstOnly) break;
		}
		return W.IsEmpty() ? FString(TEXT("(no key)")) : FString::Join(W, TEXT(", "));
	}
	// an action's key in a sentence ("Shift: run."), "(no key)" when it has none
	FString KeySaid(FName Action)
	{
		const TArray<FKey, TInlineAllocator<2>> Keys = KeyboardKeys(Action);
		const FString S = Keys.Num() ? S47Keys::Name(Keys[0]) : FString();
		return S.IsEmpty() ? FString(TEXT("(no key)")) : S;
	}

	// the four walk keys: "W A S D, arrows" (each slot's keys in the order forward, left, back, right)
	FString WalkKeys()
	{
		static const FName Move[4] = { TEXT("forward"), TEXT("left"), TEXT("back"), TEXT("right") };
		const FKey Arrows[4] = { EKeys::Up, EKeys::Left, EKeys::Down, EKeys::Right };
		TArray<FString> Out;
		for (int32 Slot = 0; Slot < 2; ++Slot)
		{
			TArray<FString> W;
			bool bAny = false, bArrows = true;
			for (int32 i = 0; i < 4; ++i)
			{
				const FKey K = S47Keys::Key(Move[i], Slot);
				bAny |= K.IsValid();
				bArrows &= K == Arrows[i];
				W.Add(K.IsValid() ? S47Keys::Short(K) : FString(TEXT("-")));
			}
			if (bAny) Out.Add(bArrows ? FString(TEXT("arrows")) : FString::Join(W, TEXT(" ")));
		}
		return Out.IsEmpty() ? FString(TEXT("(no key)")) : FString::Join(Out, TEXT(", "));
	}

	FCtlPage Controls(bool bPhone)
	{
		if (!bPhone)
		{
			auto First = [](const TCHAR* A) { return KeysSaid(A, true); };
			// (Space is both the jump and the shutter until one of them is given another key)
			const FKey Jump = S47Keys::Key(TEXT("jump"), 0);
			const TArray<FKey, TInlineAllocator<2>> Shutter = KeyboardKeys(TEXT("shutter"));
			const bool bShared = Jump.IsValid() && Shutter.Num() == 1 && Shutter[0] == Jump;
			FCtlGroup Foot = { TEXT("On foot"), {
				{ WalkKeys(), FString::Printf(TEXT("Walk. %s: run."), *KeySaid(TEXT("run"))) },
				{ TEXT("Mouse"), TEXT("Look. Click the picture first to take the mouse.") },
				{ KeysSaid(TEXT("use")) + TEXT(", click"), TEXT("Use what the crosshair is on. Put a paper back.") },
				{ KeysSaid(TEXT("zoom")) + TEXT(", held"), TEXT("A closer look at what the crosshair is on.") },
				{ KeysSaid(TEXT("describe")), TEXT("Read the current or last description. Press again to stop.") },
				{ KeysSaid(TEXT("light")), TEXT("The flashlight, once you have it.") },
				{ KeysSaid(TEXT("camera")), TEXT("Raise or lower the field camera.") } } };
			if (bShared) Foot.Rows.Add({ KeysSaid(TEXT("jump")), TEXT("Jump. With the camera raised: the shutter.") });
			else
			{
				Foot.Rows.Add({ KeysSaid(TEXT("jump")), TEXT("Jump.") });
				Foot.Rows.Add({ KeysSaid(TEXT("shutter")), TEXT("The shutter, with the camera raised.") });
			}
			// (the second choice and the case board have rows of their own here; the web's table has neither)
			Foot.Rows.Add({ KeysSaid(TEXT("alt")), TEXT("Manage takeaway: set down, stow, take out or discard. Otherwise, the other choice at a prompt.") });
			Foot.Rows.Add({ KeysSaid(TEXT("notebook")), FString::Printf(TEXT("The notebook. %s: its map. %s: the kit. %s: these controls."), *KeySaid(TEXT("map")), *KeySaid(TEXT("kit")), *KeySaid(TEXT("controls"))) });
			Foot.Rows.Add({ KeysSaid(TEXT("board")), TEXT("The case board, once there is one.") });
			Foot.Rows.Add({ KeysSaid(TEXT("pause")), TEXT("Pause: settings, save and load.") });
			FCtlGroup Truck = { TEXT("In the truck"), {
				{ First(TEXT("forward")) + TEXT(", ") + First(TEXT("back")), FString::Printf(TEXT("Throttle and brake. %s at a stop: reverse."), *KeySaid(TEXT("back"))) },
				{ First(TEXT("left")) + TEXT(", ") + First(TEXT("right")), TEXT("Steer.") },
				{ TEXT("Mouse"), TEXT("Look round the cab.") },
				{ KeysSaid(TEXT("radio")) + TEXT(", or click the radio"), TEXT("The radio: on, the next station, off.") },
				{ First(TEXT("use")) + TEXT(", stopped"), TEXT("Get out. When the engine has died: turn the key.") } } };
			return { TEXT("Keyboard and mouse"), { Foot, Truck } };
		}
		static const FCtlPage Phone = { TEXT("Phone and tablet"), {
			{ TEXT("On foot"), {
				{ TEXT("Left thumb"), TEXT("Walk.") },
				{ TEXT("Right thumb"), TEXT("Look.") },
				{ TEXT("Tap, or Use"), TEXT("Use a thing. Tap a paper to put it back.") },
				{ TEXT("Zoom, held"), TEXT("A closer look.") },
				{ TEXT("Light"), TEXT("The flashlight, once you have it.") },
				{ TEXT("Camera"), TEXT("Raise or lower the field camera. Shutter: take the picture.") },
				{ TEXT("Notes"), TEXT("The notebook.") },
				{ TEXT("II"), TEXT("Pause: settings, save and load.") } } },
			{ TEXT("In the truck"), {
				{ TEXT("Left stick"), TEXT("Throttle, brake and steering.") },
				{ TEXT("Radio, or tap it"), TEXT("The radio: on, the next station, off.") },
				{ TEXT("Get out"), TEXT("Stopped: out of the truck. Turn the key: when the engine has died.") } } } } };
		return Phone;
	}

	TSharedRef<SWidget> ControlsBlock(const FCtlPage& Pg)
	{
		using namespace S47Game;
		TSharedRef<SVerticalBox> B = SNew(SVerticalBox);
		B->AddSlot().AutoHeight()[Head(Pg.Title, FString())];
		for (const FCtlGroup& G : Pg.Groups)
		{
			B->AddSlot().AutoHeight().Padding(0, P(14), 0, P(4))[SNew(STextBlock).Text(FText::FromString(G.Head.ToUpper())).Font(Term(20, 2)).ColorAndOpacity(Ph2Red())];
			for (const TPair<FString, FString>& R : G.Rows)
			{
				B->AddSlot().AutoHeight().Padding(0, 0, 0, P(4))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(P(170))[SNew(STextBlock).Text(FText::FromString(R.Key)).Font(Term(20)).ColorAndOpacity(C(0x2e2a22))]]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(P(12), P(1), 0, 0)[SNew(STextBlock).Text(FText::FromString(R.Value)).Font(Type(15)).ColorAndOpacity(C(0x4c463a)).AutoWrapText(true)]
				];
			}
		}
		return B;
	}

	/** Initials for an empty frame: first and last name, without a title or "the". */
	FString Initials(const FString& Name)
	{
		TArray<FString> W;
		Name.ParseIntoArrayWS(W);
		W.RemoveAll([](const FString& Xw) { const FString L = Xw.ToLower(); return L == TEXT("dr") || L == TEXT("dr.") || L == TEXT("the"); });
		FString S;
		if (W.Num()) S += W[0].Left(1);
		if (W.Num() > 1) S += W.Last().Left(1);
		return S.ToUpper();
	}

	// ui/kitArt.ts KIT_ART: the drawings beside the things in the Kit tab (Content/WebData/art)
	FString KitArt(const FString& Art)
	{
		return Art == TEXT("nickel") ? TEXT("kit_nickel_1947") : TEXT("kit_") + Art;
	}

	const float TILT[8] = { -1.4f, 0.9f, -0.5f, 1.3f, -1.f, 0.4f, 1.1f, -0.7f };

	// ui/album.ts developing(k): a print still coming up, pale, flat and cold at first, and soft, the
	// shapes before the detail: brightness(1 + .9 (1 - k)) contrast(.25 + .75 k) saturate(k), and
	// while k < .85 blur(var(--w) * .012 (1 - k / .85)^1.5). Worked into the picture's pixels (each
	// function clamped, as a filter is); the blur three box blurs as wide as the gaussian.
	UTexture2D* Developing(const FString& Id, const FString& Url, double K)
	{
		if (K >= 1) return Picture(Url);
		static TMap<FString, TPair<int32, TStrongObjectPtr<UTexture2D>>> Cache;
		const int32 Q = FMath::RoundToInt(K * 100);
		if (const TPair<int32, TStrongObjectPtr<UTexture2D>>* Have = Cache.Find(Id); Have && Have->Key == Q && Have->Value.IsValid()) return Have->Value.Get();
		TArray<FColor> Px;
		int32 W = 0, H = 0;
		if (!S47Image::DecodePixels(Url, Px, W, H)) return nullptr;
		const double Br = 1 + 0.9 * (1 - K), Co = 0.25 + 0.75 * K, Sa = K;
		auto Cl = [](double V) { return FMath::Clamp(V, 0.0, 1.0); };
		for (FColor& Pc : Px)
		{
			double R = Cl(Pc.R / 255.0 * Br), G = Cl(Pc.G / 255.0 * Br), B = Cl(Pc.B / 255.0 * Br);
			R = Cl((R - 0.5) * Co + 0.5); G = Cl((G - 0.5) * Co + 0.5); B = Cl((B - 0.5) * Co + 0.5);
			const double R2 = (0.213 + 0.787 * Sa) * R + (0.715 - 0.715 * Sa) * G + (0.072 - 0.072 * Sa) * B;
			const double G2 = (0.213 - 0.213 * Sa) * R + (0.715 + 0.285 * Sa) * G + (0.072 - 0.072 * Sa) * B;
			const double B2 = (0.213 - 0.213 * Sa) * R + (0.715 - 0.715 * Sa) * G + (0.072 + 0.928 * Sa) * B;
			Pc.R = uint8(FMath::RoundToInt(Cl(R2) * 255)); Pc.G = uint8(FMath::RoundToInt(Cl(G2) * 255)); Pc.B = uint8(FMath::RoundToInt(Cl(B2) * 255)); Pc.A = 255;
		}
		if (K < 0.85)
		{
			// (--w is the print's width, 236 px; the picture is .875 of it)
			const double Sigma = 236 * 0.012 * FMath::Pow(1 - K / 0.85, 1.5) * W / (0.875 * 236);
			const int32 Rad = FMath::RoundToInt(FMath::Sqrt(Sigma * Sigma + 0.25) - 0.5);
			if (Rad > 0)
			{
				TArray<FColor> Tmp = Px;
				auto Pass = [&](bool bRows)
				{
					const int32 Len = bRows ? W : H, Count = bRows ? H : W;
					for (int32 N = 0; N < Count; ++N)
					{
						auto At = [&](int32 I) -> FColor& { const int32 Ci = FMath::Clamp(I, 0, Len - 1); return bRows ? Px[N * W + Ci] : Px[Ci * W + N]; };
						int32 S[3] = { 0, 0, 0 };
						for (int32 I = -Rad; I <= Rad; ++I) { const FColor& Cc = At(I); S[0] += Cc.R; S[1] += Cc.G; S[2] += Cc.B; }
						for (int32 I = 0; I < Len; ++I)
						{
							FColor& O = bRows ? Tmp[N * W + I] : Tmp[I * W + N];
							O.R = uint8(S[0] / (2 * Rad + 1)); O.G = uint8(S[1] / (2 * Rad + 1)); O.B = uint8(S[2] / (2 * Rad + 1));
							const FColor& In = At(I + Rad + 1); const FColor& Out = At(I - Rad);
							S[0] += In.R - Out.R; S[1] += In.G - Out.G; S[2] += In.B - Out.B;
						}
					}
					Px = Tmp;
				};
				for (int32 Pi = 0; Pi < 3; ++Pi) { Pass(true); Pass(false); }
			}
		}
		UTexture2D* T = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
		if (!T) return nullptr;
		T->SRGB = true;
		void* Data = T->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Data, Px.GetData(), W * H * sizeof(FColor));
		T->GetPlatformData()->Mips[0].BulkData.Unlock();
		T->UpdateResource();
		Cache.Add(Id, TPair<int32, TStrongObjectPtr<UTexture2D>>(Q, TStrongObjectPtr<UTexture2D>(T)));
		return T;
	}

}

FString SS47Notebook::Excerpt(const FS47Doc& D)
{
	// one line of a finding: its page (P01 and the rest) or its transcript, without the
	// "P01  SUPPORTED / " that starts it, at most 150 characters
	static const FRegexPattern Pnn(TEXT("^p\\d\\d$"));
	FRegexMatcher IsP(Pnn, D.Id);
	FString One = IsP.FindNext() ? D.Page : D.Transcript;
	{
		static const FRegexPattern Lead(TEXT("^\\S*\\s*SUPPORTED / "));
		FRegexMatcher M(Lead, One);
		if (M.FindNext()) One = One.Mid(M.GetMatchEnding());
	}
	// .replace(/\s+/g, ' ')
	FString Flat;
	bool bSpace = false;
	for (const TCHAR Ch : One)
	{
		if (FChar::IsWhitespace(Ch)) { if (!bSpace) Flat.AppendChar(TEXT(' ')); bSpace = true; }
		else { Flat.AppendChar(Ch); bSpace = false; }
	}
	if (Flat.Len() <= 150) return Flat;
	FString Cut = Flat.Left(147);
	static const FRegexPattern Tail(TEXT("[\\s.,;:]+\\S*$"));
	FRegexMatcher M(Tail, Cut);
	if (M.FindNext()) Cut = Cut.Left(M.GetMatchBeginning());
	return Cut + TEXT("...");
}

void SS47Notebook::Construct(const FArguments& InArgs)
{
	using namespace S47Game;
	UI = InArgs._UI;
	OnBoard = InArgs._OnBoard;
	OnDoc = InArgs._OnDoc;
	Heading = InArgs._Heading.IsEmpty() ? FString(TEXT("Tonight")) : InArgs._Heading;
	Tasks = InArgs._Tasks;
	Notes = InArgs._Notes;
	X = InArgs._Extras;
	Album = X.Album;
	static const FRegexPattern Pnn(TEXT("^p\\d\\d$"));
	auto IsFinding = [](const FS47Doc& D) { FRegexMatcher M(Pnn, D.Id); return M.FindNext() || D.Id == TEXT("finding") || (D.Id == TEXT("anomaly:n01") || D.Id == TEXT("anomaly:n02") || D.Id == TEXT("anomaly:a02") || D.Id == TEXT("anomaly:p01")); };
	for (const FS47Doc& D : InArgs._Docs) if (D.Kind == TEXT("photo") && !D.Image.IsEmpty()) Photos.Add(D);
	for (const FS47Doc& D : InArgs._Docs) if (IsFinding(D)) Findings.Add(D);
	for (const FS47Doc& D : InArgs._Docs) if (!(D.Kind == TEXT("photo") && !D.Image.IsEmpty()) && !IsFinding(D)) Papers.Add(D);

	Order = { TEXT("tasks"), TEXT("notes"), TEXT("findings"), TEXT("papers"), TEXT("photos") };
	if (X.Map) Order.Add(TEXT("map"));
	if (X.Kit) Order.Add(TEXT("kit"));
	if (X.People) Order.Add(TEXT("people"));
	Order.Add(TEXT("controls"));

	// the foot of the screen: the keys, the board, and the camcorder's date and time
	auto Bar = [this](const TSharedRef<SWidget>& Cap, const TCHAR* Word, TFunction<void()> Fn) -> TSharedRef<SWidget>
	{
		const TSharedRef<SWidget> W = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(6), 0)[Cap]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Word)).Font(Term(22, 1)).ColorAndOpacity(Hint())];
		return Fn ? StaticCastSharedRef<SWidget>(Click(W, Fn)) : W;
	};
	const TSharedRef<SWidget> Arrows = SNew(SBorder).BorderImage(Box(FLinearColor::Transparent, C(0x5d584e), 1.f, 2.f)).Padding(FMargin(P(6), P(1)))
		[SNew(SArrows)];
	TSharedRef<SHorizontalBox> Keys = SNew(SHorizontalBox);
	Keys->AddSlot().AutoWidth().Padding(0, 0, P(28), 0)[Bar(Keycap(TEXT("Q E")), TEXT("TAB"), nullptr)];
	Keys->AddSlot().AutoWidth().Padding(0, 0, P(28), 0)[Bar(Arrows, TEXT("CHOOSE"), nullptr)];
	if (OnBoard) Keys->AddSlot().AutoWidth().Padding(0, 0, P(28), 0)[Bar(Keycap(S47Keys::Label(TEXT("board")).ToUpper()), TEXT("CASE BOARD"), [this] { TFunction<void()> Fn = OnBoard; Close(); if (Fn) Fn(); })];
	Keys->AddSlot().AutoWidth()[Bar(Keycap(S47Keys::Label(TEXT("notebook")).ToUpper()), TEXT("CLOSE"), [this] { Close(); })];

	const FVector2D Book(150, 70);
	const FVector2D LeftAt = Book + FVector2D(22, 18), RightAt = Book + FVector2D(658, 18);
	auto Pass = [](const TSharedRef<SWidget>& W) { W->SetVisibility(EVisibility::SelfHitTestInvisible); return W; };
	ChildSlot
	[
		SNew(SOverlay)
		// the game behind, blurred and darkened (.bg { filter: blur(5px) brightness(.38) }); a click on it closes
		+ SOverlay::Slot()[SNew(SBackgroundBlur).BlurStrength(6.f)]
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(Box(FLinearColor(0, 0, 0, 0.6f)))
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&) { Close(); return FReply::Handled(); })
		]
		+ SOverlay::Slot()
		[
			Pass(SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[
				Pass(SNew(SBox).WidthOverride(P(1600)).HeightOverride(P(900))
				[
					Pass(SNew(SCanvas)
					// .book: the leather, its shadow under it
					+ SCanvas::Slot().Position(P2(Book.X, Book.Y)).Size(P2(1300, 770))
					[
						Shadowed(SNew(SBorder).BorderImage(Box(Leather(), C(0x2a190f), 3.f, 14.f))
							.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); }), 80.f, FVector2D(0, 30), 0.75f)
					]
					+ SCanvas::Slot().Position(P2(LeftAt.X, LeftAt.Y)).Size(P2(620, 734))
					[
						SNew(SOverlay)
						+ SOverlay::Slot()[SNew(SS47GamePage).bLeft(true)]
						+ SOverlay::Slot()[SAssignNew(LeftSlot, SBox)]
					]
					+ SCanvas::Slot().Position(P2(RightAt.X, RightAt.Y)).Size(P2(620, 734))
					[
						SNew(SOverlay)
						+ SOverlay::Slot()[SNew(SS47GamePage).bLeft(false).Margin(0)]
						+ SOverlay::Slot()[SAssignNew(RightSlot, SBox)]
					]
					+ SCanvas::Slot().Position(P2(Book.X + 638, Book.Y + 40)).Size(P2(24, 690))[SNew(SRings)]
					// a sheet across the spread (the map)
					+ SCanvas::Slot().Position(P2(LeftAt.X, LeftAt.Y)).Size(P2(1256, 734))[Pass(SAssignNew(SpreadSlot, SBox))]
					// .use: the actions at the foot of the right page
					+ SCanvas::Slot().Position(P2(RightAt.X + 60, RightAt.Y + 734 - 40 - 34)).Size(P2(540, 34))[Pass(SAssignNew(UseSlot, SBox).VAlign(VAlign_Center))]
					// .tabs: down the right page's edge
					+ SCanvas::Slot().Position(P2(RightAt.X + 578, RightAt.Y + 40)).Size(P2(88, 716))[TabStack()]
					// .close
					+ SCanvas::Slot().Position(P2(1300, 26)).Size(P2(264, 34))
					[
						SNew(SBox).HAlign(HAlign_Right).VAlign(VAlign_Center)
						[
							Click(SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(6), 0)[SNew(STextBlock).Text(FText::FromString(TEXT("CLOSE"))).Font(Term(22, 2)).ColorAndOpacity(PaperDim())]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Keycap(S47Keys::Label(TEXT("notebook")).ToUpper())], [this] { Close(); })
						]
					]
					+ SCanvas::Slot().Position(P2(150, 846)).Size(P2(900, 32))
					[
						SNew(SOverlay)
						+ SOverlay::Slot()[SNew(SBox).Visibility_Lambda([] { return S47Gamepad::UsingGamepad() ? EVisibility::Collapsed : EVisibility::SelfHitTestInvisible; })[Keys]]
						+ SOverlay::Slot()[SNew(STextBlock).Font(Term(20)).ColorAndOpacity(Hint())
							.Visibility_Lambda([] { return S47Gamepad::UsingGamepad() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
							.Text_Lambda([this] { return FText::FromString(OnBoard ? TEXT("LB/RB TABS   D-PAD CHOOSE   A OPEN   X READ   Y BOARD   B CLOSE") : TEXT("LB/RB TABS   D-PAD CHOOSE   A OPEN   X READ   B CLOSE")); })]
					]
					+ SCanvas::Slot().Position(P2(1100, 842)).Size(P2(444, 36))
					[
						SNew(SBox).HAlign(HAlign_Right)[Osd(TAttribute<FText>::CreateLambda([] { const US47Main* M = US47Main::Get(nullptr); return FText::FromString(M && M->Story ? OsdLine(M->Story->Clock()) : FString()); }), 26.f)]
					])
				])
			])
		]
		// .al-view: one free print big, over the notebook
		+ SOverlay::Slot()[SAssignNew(BigSlot, SBox).Visibility_Lambda([this] { return Big != INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed; })]
	];
	// show(): the tab used last, if this notebook has it
	Show(UI.IsValid() && Order.Contains(UI->NbTab) ? UI->NbTab : FString(TEXT("tasks")));
}

SS47Notebook::~SS47Notebook()
{
	if (Reading.IsValid() && Reading->bActive) Reading->Stop();
}

TSharedRef<SWidget> SS47Notebook::TabStack()
{
	using namespace S47Game;
	// .tb: the coloured paper tabs, their word up the edge; the open one stands out further and darker
	static const TMap<FString, TPair<const TCHAR*, uint32>> Look = {
		{ TEXT("tasks"), { TEXT("TASKS"), 0xd7c7a0 } }, { TEXT("notes"), { TEXT("NOTES"), 0xc9b48a } }, { TEXT("findings"), { TEXT("FINDINGS"), 0xbfc7a6 } },
		{ TEXT("papers"), { TEXT("PAPERS"), 0xd3b3a0 } }, { TEXT("photos"), { TEXT("ALBUM"), 0xc3c9cf } }, { TEXT("map"), { TEXT("MAP"), 0xe0c27a } },
		{ TEXT("kit"), { TEXT("KIT"), 0xcfbfa9 } }, { TEXT("people"), { TEXT("PEOPLE"), 0xb9c2b0 } }, { TEXT("controls"), { TEXT("CONTROLS"), 0xd9d0bf } } };
	const float Gap = 6.f;
	TSharedRef<SVerticalBox> Col = SNew(SVerticalBox);
	// (a tab is as long as its word needs, FINDINGS and CONTROLS longer than the rest)
	auto Long = [](const FString& S)
	{
		if (!FSlateApplication::IsInitialized()) return 58.f;
		return FMath::Max(58.f, float(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Term(19, 1)).X) / U + 10.f);
	};
	auto Word = [](const FString& S, const FLinearColor& Ink, bool bBold) -> TSharedRef<SWidget>
	{
		const FSlateFontInfo F = Term(19, 1);
		const TSharedRef<SWidget> T = SNew(STextBlock).Text(FText::FromString(S)).Font(F).ColorAndOpacity(Ink)
			.ShadowOffset(FVector2D(bBold ? P(0.6f) : 0.f, 0)).ShadowColorAndOpacity(Ink);
		return Turned(T, -90.f);
	};
	for (const FString& T : Order)
	{
		const TPair<const TCHAR*, uint32>* L = Look.Find(T);
		if (!L) continue;
		const FString Label = L->Key;
		const uint32 Rgb = L->Value;
		const float H = Long(Label);
		Col->AddSlot().AutoHeight().Padding(0, 0, 0, P(Gap))
		[
			SNew(SBox).HeightOverride(P(H)).WidthOverride(P(88)).HAlign(HAlign_Right)
			[
				Click(SNew(SBox).HeightOverride(P(H)).WidthOverride_Lambda([this, T] { return FOptionalSize(P(Tab == T ? 76.f : 64.f)); })
					[
						Shadowed(SNew(SBorder).BorderImage(Box4(C(Rgb), FVector4(8, 0, 0, 8))).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0)
							[
								SNew(SBox).Padding(FMargin(P(0), 0, P(0), 0))
								[
									SNew(SOverlay)
									+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).Visibility_Lambda([this, T] { return Tab == T ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })[Word(Label, C(0x2c2418), false)]]
									+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).Visibility_Lambda([this, T] { return Tab == T ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })[Word(Label, C(0x120c05), true)]]
								]
							], 4.f, FVector2D(-2, 2), 0.35f)
					], [this, T] { Show(T); })
			]
		];
	}
	if (OnBoard)
	{
		// the case board: its own screen (B); a darker tab with the red pin (.nb-boardtab)
		const float H = 72.f;
		Col->AddSlot().AutoHeight().Padding(0, P(10), 0, 0)
		[
			SNew(SBox).HeightOverride(P(H)).WidthOverride(P(88)).HAlign(HAlign_Right)
			[
				Click(SNew(SBox).HeightOverride(P(H)).WidthOverride(P(64))
					[
						Shadowed(SNew(SBorder).BorderImage(Box4(C(0x8a5f3a), FVector4(8, 0, 0, 8))).Padding(0)
							[
								SNew(SOverlay)
								+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0, 0, 0, P(10))[Word(TEXT("BOARD"), C(0xf3e6cf), false)]
								+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, P(6))
								[
									SNew(SBox).WidthOverride(P(10)).HeightOverride(P(10))[SNew(SBorder).BorderImage(Box(C(0xc4241c), FLinearColor::Transparent, 0.f, 5.f))]
								]
							], 4.f, FVector2D(-2, 2), 0.35f)
					], [this] { TFunction<void()> Fn = OnBoard; Close(); if (Fn) Fn(); })
			]
		];
	}
	return Col;
}

TSharedRef<SWidget> SS47Notebook::Entry(int32 I, const TSharedRef<SWidget>& Content, bool bCard)
{
	using namespace S47Game;
	// an entry to choose: a click chooses it, a second click opens it; the chosen one is outlined in red
	// on a faint amber (.it.sel)
	const TSharedRef<SWidget> W = Click(
		SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(Box(C(0xffb052, bCard ? 0.22f : 0.16f))).Visibility_Lambda([this, I] { return Chosen() == I ? EVisibility::HitTestInvisible : EVisibility::Hidden; })]
		+ SOverlay::Slot()[Content]
		+ SOverlay::Slot()[SNew(SS47GameDash).Color(C(0x8a3b2c, 0.55f)).Inset(bCard ? 2.f : 4.f).Visibility_Lambda([this, I] { return Chosen() == I ? EVisibility::HitTestInvisible : EVisibility::Hidden; })],
		[this, I] { if (Chosen() == I) Activate(); else Choose(I); });
	Entries.Add(W);
	return W;
}

TSharedRef<SWidget> SS47Notebook::UseRow(const TArray<TTuple<FString, FString, TFunction<void()>>>& Acts)
{
	using namespace S47Game;
	// .use: a keycap in the page's brown and the word; V reads aloud when the reader has it, its state beside
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (const TTuple<FString, FString, TFunction<void()>>& A : Acts)
	{
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(16), 0)
		[
			Click(KeyHint(A.Get<0>(), A.Get<1>(), 22.f, UseInk(), EKey::Ink), A.Get<2>())
		];
	}
	if (Reading.IsValid())
	{
		const TSharedPtr<FS47ReadingAction> R = Reading;
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(12), 0)
		[
			SNew(SBorder).BorderImage(Box(FLinearColor::Transparent)).Padding(0)
			.ColorAndOpacity_Lambda([R] { return FLinearColor(1, 1, 1, R->bAvailable || R->bActive ? 1.f : 0.45f); })
			[
				Click(SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, P(6), 0)[Keycap(S47Keys::Label(TEXT("describe")), EKey::Ink)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(Term(22, 1)).ColorAndOpacity(UseInk()).Text_Lambda([R] { return FText::FromString(R->bActive ? TEXT("STOP READING") : TEXT("READ ALOUD")); })
					], [R] { R->Toggle(); })
			]
		];
		Row->AddSlot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(Type(13)).ColorAndOpacity(C(0x7d7563)).AutoWrapText(true).Text_Lambda([R] { return FText::FromString(R->Status()); })
		];
	}
	return Row;
}

int32 SS47Notebook::Count() const
{
	if (Tab == TEXT("findings")) return Findings.Num();
	if (Tab == TEXT("papers")) return Papers.Num();
	if (Tab == TEXT("photos")) return Photos.Num() + (Album ? Album->Shots.Num() : 0);
	if (Tab == TEXT("kit")) return X.Kit ? X.Kit->Num() : 0;
	if (Tab == TEXT("people")) return X.People ? X.People->Num() : 0;
	return 0;
}

int32 SS47Notebook::Columns() const
{
	if (Tab == TEXT("kit") || Tab == TEXT("people")) return 3;
	if (Tab == TEXT("papers") || Tab == TEXT("photos")) return 2;
	return 1;
}

void SS47Notebook::Build()
{
	using namespace S47Game;
	if (Reading.IsValid() && Reading->bActive) Reading->Stop();
	Reading.Reset();
	Entries.Reset();
	LeftScroll.Reset(); RightScroll.Reset();
	bRightIsEntries = false;
	if (!LeftSlot.IsValid()) return;
	SpreadSlot->SetContent(SNullWidget::NullWidget);
	RightSlot->SetContent(SNullWidget::NullWidget);
	UseSlot->SetContent(SNullWidget::NullWidget);
	Sel.FindOrAdd(Tab) = FMath::Clamp(Chosen(), 0, FMath::Max(0, Count() - 1));
	const bool bLarge = UI.IsValid() && UI->bLargeText;
	const float Li = bLarge ? 34.f : 30.f;
	TArray<FString> Read;   // what V reads on this spread (UI.notebook's descriptionReadings)

	// a page: the heading at 84, 40, then what it holds from 146 down, scrolling when it is long
	// (the right page keeps its foot for the actions, .use)
	auto Lay = [](const TSharedRef<SWidget>& Top, const TSharedRef<SWidget>& Body, TSharedPtr<SScrollBox>& Scroll, float From = 146.f, bool bRight = false) -> TSharedRef<SWidget>
	{
		return SNew(SCanvas)
			+ SCanvas::Slot().Position(P2(bRight ? 60 : 84, 40)).Size(P2(506, 100))[Top]
			+ SCanvas::Slot().Position(P2(bRight ? 46 : 70, From)).Size(P2(536, 734 - From - (bRight ? 92 : 30)))
			[
				SAssignNew(Scroll, SScrollBox).Style(S47Game::QuietScroll()).ScrollBarStyle(S47Game::QuietBar(C(0x233a7a, 0.22f))).ScrollBarThickness(FVector2D(P(3), P(3)))
				+ SScrollBox::Slot().Padding(FMargin(P(14), 0, P(16), P(10)))[Body]
			];
	};
	auto Lines = [&](const TArray<FString>& Items, int32 From, int32 To, bool bTasks) -> TSharedRef<SWidget>
	{
		TSharedRef<SVerticalBox> L = SNew(SVerticalBox);
		for (int32 I = From; I < To; ++I)
		{
			const bool bDone = bTasks && Tasks.IsValidIndex(I) && Tasks[I].bDone;
			// li.done: struck through, the ink paler
			const FLinearColor Ink = bDone ? C(0x6d7895) : InkBlue();
			FTextBlockStyle St = FTextBlockStyle().SetFont(Hand(Li)).SetColorAndOpacity(Ink);
			if (bDone)
			{
				FSlateBrush Strike = *FCoreStyle::Get().GetBrush("WhiteBrush");
				Strike.TintColor = Ink;
				St.SetStrikeBrush(Strike);
			}
			TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
			if (bTasks) Row->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(P(28))[SNew(STextBlock).Text(FText::FromString(bDone ? TEXT("x") : TEXT("-"))).Font(Hand(Li)).ColorAndOpacity(Ink)]];
			Row->AddSlot().FillWidth(1.f)[SNew(STextBlock).Text(FText::FromString(Items[I])).TextStyle(&St).AutoWrapText(true).LineHeightPercentage(LinePercent(Hand(Li), 34))];
			L->AddSlot().AutoHeight()[Row];
		}
		return L;
	};

	if (Tab == TEXT("tasks") || Tab == TEXT("notes"))
	{
		TArray<FString> TaskText;
		int32 Open = 0;
		for (const FS47Task& T : Tasks) { TaskText.Add(T.Text); Open += !T.bDone; }
		const FString NoteLine = Notes.Num() ? FString::Printf(TEXT("NOTES · %d"), Notes.Num()) : FString(TEXT("NOTES"));
		if (Tab == TEXT("tasks"))
		{
			// tonight's tasks on the left, the observations so far on the right
			const FString Line = !Tasks.Num() ? FString(TEXT("TASKS")) : Open ? FString::Printf(TEXT("TASKS · %d OPEN"), Open) : FString(TEXT("TASKS · ALL DONE"));
			LeftSlot->SetContent(Lay(Head(Heading, Line), Tasks.Num() ? Lines(TaskText, 0, TaskText.Num(), true) : Empty(TEXT("Nothing yet. Dale left the shift log on the desk.")), LeftScroll));
			RightSlot->SetContent(Lay(Head(TEXT("Observations"), NoteLine), Notes.Num() ? Lines(Notes, 0, Notes.Num(), false) : Empty(TEXT("Nothing worth writing down. Yet.")), RightScroll, 146.f, true));
			if (Tasks.Num()) Read = TaskText; else Read.Add(TEXT("Nothing yet. Dale left the shift log on the desk."));
		}
		else
		{
			// the observations across the spread: the first half on the left, the rest on the right
			const int32 Half = (Notes.Num() + 1) / 2;
			LeftSlot->SetContent(Lay(Head(TEXT("Observations"), NoteLine), Notes.Num() ? Lines(Notes, 0, Half, false) : Empty(TEXT("Nothing worth writing down. Yet.")), LeftScroll));
			if (Notes.Num() > Half) RightSlot->SetContent(Lay(SNullWidget::NullWidget, Lines(Notes, Half, Notes.Num(), false), RightScroll, 146.f, true));
			if (Notes.Num()) Read = Notes; else Read.Add(TEXT("Nothing worth writing down. Yet."));
		}
	}
	else if (Tab == TEXT("findings"))
	{
		// index cards on the left (P01 in green, the title and its line), the chosen one on the right
		TSharedRef<SVerticalBox> L = SNew(SVerticalBox);
		for (int32 I = 0; I < Findings.Num(); ++I)
		{
			const FS47Doc& D = Findings[I];
			TArray<FString> Parts = S47Text::Split(D.Title, TEXT(" / "));
			FString Rest = Parts.Num() > 1 ? FString::Join(TArray<FString>(Parts.GetData() + 1, Parts.Num() - 1), TEXT(" / ")) : FString();
			if (Rest.IsEmpty()) Rest = D.Title;
			const FString Label = !D.Stamp.IsEmpty() && D.Stamp.Len() <= 6 ? D.Stamp : Parts.Num() ? Parts[0] : D.Title;
			L->AddSlot().AutoHeight().Padding(0, 0, 0, P(10))
			[
				Entry(I, Shadowed(SNew(SBorder).BorderImage(Box(C(0xf6f1e3))).Padding(FMargin(P(12), P(8)))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)[SNew(SBox).WidthOverride(P(62))[SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(26)).ColorAndOpacity(C(0x2f6a3c))]]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(P(10), 0, 0, 0)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Rest)).Font(Type(16)).ColorAndOpacity(Ink()).AutoWrapText(true)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, P(3), 0, 0)[SNew(STextBlock).Text(FText::FromString(Excerpt(D))).Font(Type(13)).ColorAndOpacity(C(0x6d6656)).AutoWrapText(true)]
						]
					], 6.f, FVector2D(0, 2), 0.18f), true)
			];
		}
		const FString Line = Findings.Num() ? FString::Printf(TEXT("FINDINGS · %d"), Findings.Num()) : FString(TEXT("FINDINGS"));
		LeftSlot->SetContent(Lay(Head(TEXT("Findings"), Line), Findings.Num() ? StaticCastSharedRef<SWidget>(L) : Empty(TEXT("Nothing recorded yet. A finding goes here once the records support it.")), LeftScroll));
		if (!Findings.Num())
		{
			Read.Add(TEXT("Nothing recorded yet. A finding goes here once the records support it."));
			// Departs from the web version: with nothing recorded, the right page says how a finding
			// is made (Tom: the clues should be better explained to a new player).
			const FString Key = S47Keys::Label(TEXT("board")).ToUpper();
			const TArray<FString> How = {
				FString::Printf(TEXT("Open the case board (%s). Every paper I pick up is pinned there."), *Key),
				TEXT("Pull a red thread from a pin to the record it agrees with. A wrong one falls slack."),
				TEXT("A question card fills its lines when the right threads hold."),
				TEXT("Every line filled: RECORD. The finding comes here."),
				TEXT("Stuck: the Hint on the card."),
			};
			RightSlot->SetContent(Lay(Head(TEXT("How a finding is made"), TEXT("ON THE CASE BOARD")), Lines(How, 0, How.Num(), false), RightScroll, 146.f, true));
			Read.Append(How);
		}
	}
	else if (Tab == TEXT("papers"))
	{
		// the papers as index cards, two to a row
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(P(5), P(6)));
		auto KindName = [](const FS47Doc& D) { return D.Kind == TEXT("hand") ? TEXT("Handwritten") : D.Kind == TEXT("printout") ? TEXT("Printout") : D.Kind == TEXT("typed") ? TEXT("Typed") : D.Kind == TEXT("map") ? TEXT("Map") : TEXT("Print"); };
		for (int32 I = 0; I < Papers.Num(); ++I)
		{
			const FS47Doc& D = Papers[I];
			const bool bHand = D.Kind == TEXT("hand");
			Grid->AddSlot(I % 2, I / 2)
			[
				Entry(I, Shadowed(SNew(SBorder).BorderImage(Box(D.Kind == TEXT("printout") ? C(0xeef2e6) : C(0xf6f1e3))).Padding(FMargin(P(12), P(9)))
					[
						SNew(SBox).MinDesiredHeight(P(58)).VAlign(VAlign_Top)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(D.Title)).Font(bHand ? Hand(25) : Type(15)).ColorAndOpacity(bHand ? InkBlue() : Ink()).AutoWrapText(true)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, P(4), 0, 0)
							[
								SNew(STextBlock).Text(FText::FromString(FString(KindName(D)) + (D.Stamp.IsEmpty() ? FString() : TEXT(" / ") + D.Stamp))).Font(Term(17)).ColorAndOpacity(C(0x8a8270))
							]
						]
					], 6.f, FVector2D(0, 2), 0.18f), true)
			];
		}
		if (Papers.Num() == 1) Grid->AddSlot(1, 0)[SNew(SSpacer)];
		const FString Line = Papers.Num() ? FString::Printf(TEXT("PAPERS · %d"), Papers.Num()) : FString(TEXT("PAPERS"));
		LeftSlot->SetContent(Lay(Head(TEXT("Papers"), Line), Papers.Num() ? StaticCastSharedRef<SWidget>(Grid) : Empty(TEXT("None collected.")), LeftScroll));
		if (!Papers.Num()) Read.Add(TEXT("None collected."));
	}
	else if (Tab == TEXT("photos"))
	{
		// the album (ui/album.ts albumPage) on the pages: the case prints on the left, the free frames
		// (core/album.ts) on the right as instant prints, in the order they were taken
		const int32 NShots = Album ? Album->Shots.Num() : 0;
		const FString Roll = Album ? FString::Printf(TEXT("%d of %d free frames left"), Album->Left, Album->Roll).ToUpper() : FString(TEXT("THE CASE"));
		TSharedRef<SWrapBox> Thumbs = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(P(26), P(24)));
		for (int32 I = 0; I < Photos.Num(); ++I)
		{
			const FS47Doc& D = Photos[I];
			UTexture2D* Tex = Picture(D.Image);
			const float W = 210.f, Hh = Tex ? W * Tex->GetSizeY() / FMath::Max(1, Tex->GetSizeX()) : 140.f;
			Thumbs->AddSlot()
			[
				Entry(I, SNew(SBox).WidthOverride(P(W + 20)).Padding(P(10))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							Turned(Shadowed(SNew(SBorder).BorderImage(Box(C(0xf4f1e8))).Padding(P(7))
								[SNew(SBox).WidthOverride(P(W - 14)).HeightOverride(P(Hh - 14))[SNew(SImage).Image(Picture(Tex))]], 10.f, FVector2D(0, 5), 0.4f), TILT[I % 8])
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, P(10), 0, 0)[SNew(STextBlock).Text(FText::FromString(D.Title)).Font(Hand(23)).ColorAndOpacity(InkBlue()).AutoWrapText(true)]
					])
			];
		}
		LeftSlot->SetContent(Lay(Head(TEXT("Album"), Roll), Photos.Num() ? StaticCastSharedRef<SWidget>(Thumbs)
			: NShots ? SNullWidget::NullWidget : Empty(TEXT("No exposures yet. Once you have the field camera, C raises it anywhere.")), LeftScroll));
		if (!Photos.Num() && !NShots) Read.Add(TEXT("No exposures yet. Once you have the field camera, C raises it anywhere."));
		TSharedRef<SWrapBox> Prints = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(P(22), P(24)));
		for (int32 I = 0; I < NShots; ++I) Prints->AddSlot()[Entry(Photos.Num() + I, SNew(SBox).Padding(P(8))[Print(I, 214.f, false)])];
		bRightIsEntries = NShots > 0;
		if (NShots) RightSlot->SetContent(Lay(Head(TEXT("Tonight"), FString::Printf(TEXT("FREE FRAMES · %d"), NShots)), Prints, RightScroll, 146.f, true));
	}
	else if (Tab == TEXT("map") && X.Map)
	{
		// the pencil map across the spread, taped down at its corners, the pencilled line under it
		const TSharedRef<SWidget> Sheet = Shadowed(SNew(SS47AreaMap).View(*X.Map).Width(P(960)), 16.f, FVector2D(0, 8), 0.4f);
		TSharedRef<SCanvas> Spread = SNew(SCanvas)
			+ SCanvas::Slot().Position(P2(148, 16)).Size(P2(960, 640))[Turned(Sheet, -0.4f)]
			+ SCanvas::Slot().Position(P2(118, 6)).Size(P2(74, 26))[Tape(70, 20, -32)]
			+ SCanvas::Slot().Position(P2(1066, 6)).Size(P2(74, 26))[Tape(70, 20, 34)]
			+ SCanvas::Slot().Position(P2(118, 632)).Size(P2(74, 26))[Tape(70, 20, 30)]
			+ SCanvas::Slot().Position(P2(1066, 632)).Size(P2(74, 26))[Tape(70, 20, -28)];
		if (!X.MapNote.IsEmpty()) Spread->AddSlot().Position(P2(170, 668)).Size(P2(920, 44))[Turned(Inked(X.MapNote, 30.f), -0.6f)];
		SpreadSlot->SetContent(Spread);
		Read = X.MapDescription;
	}
	else if (Tab == TEXT("kit") && X.Kit)
	{
		// the drawings taped to the page, three to a row, the name in the hand under each (.grid .it)
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(P(8), P(9))).MinDesiredSlotHeight(P(150));
		for (int32 I = 0; I < X.Kit->Num(); ++I)
		{
			const FS47KitItem& K = (*X.Kit)[I];
			UTexture2D* Art = K.Art.IsEmpty() ? nullptr : S47Image::Art(KitArt(K.Art));
			Grid->AddSlot(I % 3, I / 3)
			[
				Entry(I, SNew(SBox).HeightOverride(P(150))
					[
						SNew(SOverlay)
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0, P(2), 0, 0)[Tape()]
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, P(6))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
							[
								Art ? Drawing(Art, 120, 100, 5.f) : StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(P(100)).HeightOverride(P(96)).HAlign(HAlign_Center).VAlign(VAlign_Center)
									[SNew(STextBlock).Text(FText::FromString(Initials(K.Name))).Font(Type(34)).ColorAndOpacity(C(0x6d6656, 0.7f))])
							]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, P(4), 0, 0)
							[
								SNew(STextBlock).Text(FText::FromString(K.Name)).Font(Hand(27)).ColorAndOpacity(InkBlue()).Justification(ETextJustify::Center)
							]
						]
					])
			];
		}
		for (int32 I = X.Kit->Num(); I < 3; ++I) Grid->AddSlot(I, 0)[SNew(SSpacer)];
		LeftSlot->SetContent(Lay(Head(TEXT("What I have on me"), FString::Printf(TEXT("KIT · %d %s"), X.Kit->Num(), X.Kit->Num() == 1 ? TEXT("THING") : TEXT("THINGS"))), Grid, LeftScroll));
	}
	else if (Tab == TEXT("people") && X.People)
	{
		// the faces as photographs clipped to the page, the name under each
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(P(8), P(10)));
		for (int32 I = 0; I < X.People->Num(); ++I)
		{
			const FS47PersonCard& Pc = (*X.People)[I];
			UTexture2D* Face = Pc.Portrait.IsEmpty() ? nullptr : S47Image::Art(Pc.Portrait);
			const float Fw = 124.f;
			const TSharedRef<SWidget> Frame = Face ? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(P(Fw)).HeightOverride(P(Fw * 5 / 4))[SNew(SImage).Image(Picture(Face))])
				: StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(P(Fw)).HeightOverride(P(Fw * 5 / 4))
					[SNew(SBorder).BorderImage(Box(C(0xe9e2cf), C(0x3c3228, 0.25f), 1.f)).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Initials(Pc.Name))).Font(Type(34)).ColorAndOpacity(C(0x6d6656))]]);
			Grid->AddSlot(I % 3, I / 3)
			[
				Entry(I, SNew(SBox).Padding(FMargin(P(8), P(8), P(8), P(6)))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()[Turned(Shadowed(SNew(SBorder).BorderImage(Box(C(0xf4f1e8))).Padding(P(6))[Frame], 8.f, FVector2D(0, 4), 0.35f), TILT[(I + 2) % 8])]
							+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0, P(-8), 0, 0)[Tape(46, 14, TILT[I % 8] * 3)]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, P(8), 0, 0)
						[
							SNew(STextBlock).Text(FText::FromString(Pc.Name)).Font(Hand(25)).ColorAndOpacity(InkBlue()).Justification(ETextJustify::Center).AutoWrapText(true)
						]
					])
			];
		}
		for (int32 I = X.People->Num(); I < 3; ++I) Grid->AddSlot(I, 0)[SNew(SSpacer)];
		LeftSlot->SetContent(Lay(Head(TEXT("People in the case"), FString::Printf(TEXT("PEOPLE · %d"), X.People->Num())), Grid, LeftScroll));
	}
	else if (Tab == TEXT("controls"))
	{
		// every control: the keyboard and mouse on the left, a phone's on the right (a phone's first on a phone)
		auto Page = [](const FCtlPage& Pg, TSharedPtr<SScrollBox>& Scroll)
		{
			return SNew(SBox).Padding(FMargin(P(84), P(40), P(30), P(30)))[SAssignNew(Scroll, SScrollBox).Style(S47Game::QuietScroll()).ScrollBarStyle(S47Game::QuietBar(C(0x233a7a, 0.22f))).ScrollBarThickness(FVector2D(P(3), P(3))) + SScrollBox::Slot()[ControlsBlock(Pg)]];
		};
		LeftSlot->SetContent(Page(Controls(X.bTouch), LeftScroll));
		RightSlot->SetContent(Page(Controls(!X.bTouch), RightScroll));
		KeysShown = S47Keys::Changes();
	}
	if (!Read.IsEmpty() && UI.IsValid()) Reading = UI->ReadingAction(Read, TEXT("notebook-description"));
	BuildRight();
}

void SS47Notebook::BuildRight()
{
	using namespace S47Game;
	const int32 I = Chosen();
	TArray<TTuple<FString, FString, TFunction<void()>>> Acts;
	auto Readings = [this](const TArray<FString>& Texts)
	{
		if (Reading.IsValid() && Reading->bActive) Reading->Stop();
		Reading.Reset();
		TArray<FString> T = Texts;
		T.RemoveAll([](const FString& S) { return S.TrimStartAndEnd().IsEmpty(); });
		if (T.Num() && UI.IsValid()) Reading = UI->ReadingAction(T, TEXT("notebook-description"));
	};
	if (Tab == TEXT("kit") && X.Kit && X.Kit->IsValidIndex(I))
	{
		// .big, .rt, .desc: the drawing large, its name, its state, the first of its note in the hand and the
		// rest typed on the lines; what can be done with it at the foot
		const FS47KitItem& K = (*X.Kit)[I];
		UTexture2D* Art = K.Art.IsEmpty() ? nullptr : S47Image::Art(KitArt(K.Art));
		FString Lead, Rest;
		SplitNote(K.Note, Lead, Rest);
		TSharedRef<SOverlay> Large = SNew(SOverlay) + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[Drawing(Art, 240, 230, 10.f, 0.38f)];
		if (!K.Tag.IsEmpty() && Art)
		{
			// the keys' leather tag is blank in the drawing; the page writes SARO 07 on it
			Large->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(P(0.8f * 240 - 36), P(0.62f * 240 - 14), 0, 0)
			[
				Turned(SNew(STextBlock).Text(FText::FromString(K.Tag)).Font(Type(240 * 0.095f)).ColorAndOpacity(FLinearColor::FromSRGBColor(FColor(52, 26, 10)) * FLinearColor(1, 1, 1, 0.78f)), 58.f)
			];
		}
		const float Tx = Art ? 330.f : 60.f;
		TSharedRef<SVerticalBox> Rt = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(K.Name)).Font(Type(30)).ColorAndOpacity(Ink()).AutoWrapText(true).ShadowOffset(FVector2D(P(0.8f), 0)).ShadowColorAndOpacity(Ink())];
		if (!K.State.IsEmpty()) Rt->AddSlot().AutoHeight().Padding(0, P(6), 0, 0)[SNew(STextBlock).Text(FText::FromString(K.State.ToUpper())).Font(Term(22, 2)).ColorAndOpacity(StampRed()).AutoWrapText(true)];
		Rt->AddSlot().AutoHeight().Padding(0, P(10), 0, 0)[SNew(STextBlock).Text(FText::FromString(Lead)).Font(Hand(30)).ColorAndOpacity(InkBlue()).AutoWrapText(true).LineHeightPercentage(LinePercent(Hand(30), 31))];
		TSharedRef<SCanvas> Pg = SNew(SCanvas)
			+ SCanvas::Slot().Position(P2(Tx, 58)).Size(P2(580 - Tx, 270))[Rt]
			+ SCanvas::Slot().Position(P2(60, 318)).Size(P2(516, 300))[Typed(Rest)];
		if (Art) Pg->AddSlot().Position(P2(50, 40)).Size(P2(260, 250))[Large];
		RightSlot->SetContent(Pg);
		Readings({ K.Name, K.State, K.Note });
		// (the kit's own keys, as they work in play: the notebook closes and the key is pressed)
		if (K.Art == TEXT("camera")) Acts.Add({ S47Keys::Label(TEXT("camera")), TEXT("RAISE"), [this] { Press(S47Keys::Key(TEXT("camera"), 0)); } });
		if (K.Art == TEXT("flashlight")) Acts.Add({ S47Keys::Label(TEXT("light")), K.State == TEXT("On") ? TEXT("SWITCH OFF") : TEXT("SWITCH ON"), [this] { Press(S47Keys::Key(TEXT("light"), 0)); } });
		if (K.Art == TEXT("papers")) Acts.Add({ TEXT("ENTER"), TEXT("PAPERS"), [this] { Show(TEXT("papers")); } });
	}
	else if (Tab == TEXT("people") && X.People && X.People->IsValidIndex(I))
	{
		// the face large, clipped to the page; the name, the role, and the note in the hand on the lines
		const FS47PersonCard& Pc = (*X.People)[I];
		UTexture2D* Face = Pc.Portrait.IsEmpty() ? nullptr : S47Image::Art(Pc.Portrait);
		const float Fw = 200.f;
		const TSharedRef<SWidget> Frame = Face ? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(P(Fw)).HeightOverride(P(Fw * 5 / 4))[SNew(SImage).Image(Picture(Face))])
			: StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(P(Fw)).HeightOverride(P(Fw * 5 / 4))
				[SNew(SBorder).BorderImage(Box(C(0xe9e2cf), C(0x3c3228, 0.25f), 1.f)).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Initials(Pc.Name))).Font(Type(54)).ColorAndOpacity(C(0x6d6656))]]);
		RightSlot->SetContent(SNew(SCanvas)
			+ SCanvas::Slot().Position(P2(60, 44)).Size(P2(Fw + 20, Fw * 5 / 4 + 20))
			[
				SNew(SOverlay)
				+ SOverlay::Slot()[Turned(Shadowed(SNew(SBorder).BorderImage(Box(C(0xf4f1e8))).Padding(P(10))[Frame], 14.f, FVector2D(0, 8), 0.4f), 1.6f)]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0, P(-10), 0, 0)[Tape(70, 20, -4)]
			]
			+ SCanvas::Slot().Position(P2(310, 58)).Size(P2(270, 260))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Pc.Name)).Font(Type(30)).ColorAndOpacity(Ink()).AutoWrapText(true).ShadowOffset(FVector2D(P(0.8f), 0)).ShadowColorAndOpacity(Ink())]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, P(6), 0, 0)[SNew(STextBlock).Text(FText::FromString(Pc.Role.ToUpper())).Font(Term(22, 2)).ColorAndOpacity(StampRed()).AutoWrapText(true)]
			]
			+ SCanvas::Slot().Position(P2(60, 364)).Size(P2(516, 300))[Inked(Pc.Note, 32.f)]);
		Readings({ Pc.Name, Pc.Role, Pc.Note });
	}
	else if ((Tab == TEXT("findings") && Findings.IsValidIndex(I)) || (Tab == TEXT("papers") && Papers.IsValidIndex(I)))
	{
		// the chosen paper: what it is, its title, and the paper itself, as far as the page shows it
		const bool bFinding = Tab == TEXT("findings");
		const FS47Doc& D = bFinding ? Findings[I] : Papers[I];
		const bool bHand = D.Kind == TEXT("hand"), bPrint = D.Kind == TEXT("printout");
		const FString Kind = D.Kind == TEXT("hand") ? TEXT("HANDWRITTEN") : bPrint ? TEXT("PRINTOUT") : D.Kind == TEXT("typed") ? TEXT("TYPED") : D.Kind == TEXT("map") ? TEXT("MAP") : TEXT("PRINT");
		const FString Line = (bFinding ? FString(TEXT("FINDING · ")) : FString()) + Kind + (D.Stamp.IsEmpty() ? FString() : TEXT(" · ") + D.Stamp);
		UTexture2D* Image = Picture(D.Image);
		TSharedRef<SVerticalBox> Lines = SNew(SVerticalBox);
		TArray<FString> PageLines;
		D.Page.ParseIntoArray(PageLines, TEXT("\n"), false);
		const FSlateFontInfo Font = bHand ? Hand(26) : bPrint ? Term(18) : Type(14);
		for (const FString& L : PageLines)
		{
			const bool bPencil = L.StartsWith(TEXT("~ "));
			Lines->AddSlot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(bPencil ? L.Mid(2) : (L.IsEmpty() ? TEXT(" ") : L))).Font(bPencil ? Hand(22) : Font)
				.ColorAndOpacity(bPencil ? C(0x4a4a52) : bHand ? InkBlue() : Ink()).AutoWrapText(true)
			];
		}
		if (Image && D.Kind == TEXT("map")) { Lines->ClearChildren(); Lines->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(P(300))[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Picture(Image))]]]; }
		const TSharedRef<SWidget> Sheet = SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SS47GameSheet).Color(bPrint ? C(0xeef0e4) : C(0xf3ecdb)).bTorn(bHand)]
			+ SOverlay::Slot().Padding(FMargin(P(26), P(24), P(22), P(14)))[SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[Lines]];
		RightSlot->SetContent(SNew(SCanvas)
			+ SCanvas::Slot().Position(P2(60, 50)).Size(P2(520, 140))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Line)).Font(Term(20, 2)).ColorAndOpacity(StampRed())]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, P(6), 0, 0)[SNew(STextBlock).Text(FText::FromString(D.Title)).Font(bHand ? Hand(40) : Type(26)).ColorAndOpacity(bHand ? InkBlue() : Ink()).AutoWrapText(true)]
			]
			+ SCanvas::Slot().Position(P2(70, 196)).Size(P2(470, 440))[Turned(Shadowed(Sheet, 18.f, FVector2D(0, 10), 0.35f), bFinding ? 0.8f : -1.f)]);
		Acts.Add({ TEXT("ENTER"), TEXT("READ IT"), [this] { Activate(); } });
		if (bFinding && OnBoard) Acts.Add({ S47Keys::Label(TEXT("board")), TEXT("LAY IT ALL OUT ON THE BOARD"), [this] { TFunction<void()> Fn = OnBoard; Close(); if (Fn) Fn(); } });
	}
	else if (Tab == TEXT("photos") && Count()) Acts.Add({ TEXT("ENTER"), TEXT("LOOK CLOSER"), [this] { Activate(); } });
	else if (Tab == TEXT("findings") && OnBoard) Acts.Add({ S47Keys::Label(TEXT("board")), TEXT("LAY IT ALL OUT ON THE BOARD"), [this] { TFunction<void()> Fn = OnBoard; Close(); if (Fn) Fn(); } });
	UseSlot->SetContent(UseRow(Acts));
}

void SS47Notebook::Choose(int32 I)
{
	const int32 N = Count();
	if (!N) return;
	I = FMath::Clamp(I, 0, N - 1);
	if (I == Chosen()) return;
	// (another entry: what was being read of the last one stops)
	if (UI.IsValid()) UI->StopSpeech();
	Sel.FindOrAdd(Tab) = I;
	if (Entries.IsValidIndex(I) && Entries[I].IsValid())
	{
		const bool bRight = bRightIsEntries && I >= Photos.Num();
		const TSharedPtr<SScrollBox> S = bRight ? RightScroll : LeftScroll;
		if (S.IsValid()) S->ScrollDescendantIntoView(Entries[I], true, EDescendantScrollDestination::IntoView);
	}
	BuildRight();
}

void SS47Notebook::Activate()
{
	const int32 I = Chosen();
	if ((Tab == TEXT("findings") && Findings.IsValidIndex(I)) || (Tab == TEXT("papers") && Papers.IsValidIndex(I)))
	{
		// (as a click on it did: the paper opens over the game, game.openDoc)
		const FS47Doc D = Tab == TEXT("findings") ? Findings[I] : Papers[I];
		if (OnDoc) OnDoc(D);
		return;
	}
	if (Tab == TEXT("photos"))
	{
		if (Photos.IsValidIndex(I)) { const FS47Doc D = Photos[I]; if (OnDoc) OnDoc(D); }
		else if (Album && Album->Shots.IsValidIndex(I - Photos.Num())) ShowBig(I - Photos.Num());
		return;
	}
	if (Tab == TEXT("kit") && X.Kit && X.Kit->IsValidIndex(I))
	{
		const FString Art = (*X.Kit)[I].Art;
		if (Art == TEXT("papers")) Show(TEXT("papers"));
		else if (Art == TEXT("camera")) Press(S47Keys::Key(TEXT("camera"), 0));
		else if (Art == TEXT("flashlight")) Press(S47Keys::Key(TEXT("light"), 0));
	}
}

void SS47Notebook::Turn(int32 Dir)
{
	const int32 At = FMath::Max(0, Order.IndexOfByKey(Tab));
	Show(Order[(At + Dir + Order.Num()) % Order.Num()]);
}

void SS47Notebook::Press(const FKey& Key)
{
	// the notebook closes, then the key does what it does in play (main.ts input.onKey's handlers)
	Close();
	if (US47Main* M = US47Main::Get(nullptr))
		for (const US47Main::FKeyHandler& K : TArray<US47Main::FKeyHandler>(M->Keys))
			if ((K.Action.IsNone() ? K.Key == Key : S47Keys::Is(K.Action, Key)) && K.Fn && K.Fn()) break;
}

void SS47Notebook::Scroll(float By)
{
	for (const TSharedPtr<SScrollBox>& S : { LeftScroll, RightScroll })
		if (S.IsValid()) S->SetScrollOffset(FMath::Clamp(S->GetScrollOffset() + By, 0.f, S->GetScrollOffsetOfEnd()));
}

void SS47Notebook::Show(const FString& T)
{
	// show(t): a page turned stops what was being read
	if (UI.IsValid()) UI->StopSpeech();
	Tab = T;
	if (UI.IsValid())
	{
		UI->NbTab = T;
		if (UI->OnNotebookTab) UI->OnNotebookTab(T);
	}
	Build();
}

void SS47Notebook::Close()
{
	if (UI.IsValid()) UI->Close();
}

void SS47Notebook::Tick(const FGeometry& G, const double Now, const float Dt)
{
	SCompoundWidget::Tick(G, Now, Dt);
	if (Tab == TEXT("controls") && KeysShown != S47Keys::Changes()) Build();
}

FReply SS47Notebook::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	// main.ts input.onKey with the notebook open: Tab, N, M, I, H, Escape and P close it; B lays
	// the case out on the board (the board's own key processor takes B first)
	const FKey K = E.GetKey();
	// one print open big: these only page through the album and put the print back (ui/album.ts)
	if (Big != INDEX_NONE)
	{
		if (K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_LeftShoulder) { ShowBig(Big - 1); return FReply::Handled(); }
		if (K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right || K == EKeys::Gamepad_RightShoulder) { ShowBig(Big + 1); return FReply::Handled(); }
		if (K == EKeys::Escape || K == EKeys::E || K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom || K == EKeys::Gamepad_FaceButton_Right) { CloseBig(); return FReply::Handled(); }
	}
	// Gameplay pad bindings share these keys (LB notebook, RB board, D-pad map/light).
	// This modal owns its raw pad layout; rebound keyboard shortcuts still work as before.
	const bool bKeyboard = !K.IsGamepadKey();
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Gamepad_Special_Left || K == EKeys::Gamepad_Special_Right || (bKeyboard && (S47Keys::Is(TEXT("notebook"), K) || S47Keys::Is(TEXT("map"), K) || S47Keys::Is(TEXT("kit"), K) || S47Keys::Is(TEXT("controls"), K) || S47Keys::Is(TEXT("pause"), K)))) { Close(); return FReply::Handled(); }
	if (((bKeyboard && S47Keys::Is(TEXT("board"), K)) || K == EKeys::Gamepad_FaceButton_Top) && OnBoard) { TFunction<void()> Fn = OnBoard; Close(); Fn(); return FReply::Handled(); }
	// Q and E turn the tabs; the arrows (or W A S D) choose on a page of entries, else scroll it
	if (K == EKeys::Q || K == EKeys::Gamepad_LeftShoulder) { Turn(-1); return FReply::Handled(); }
	if (K == EKeys::E || K == EKeys::Gamepad_RightShoulder) { Turn(1); return FReply::Handled(); }
	const bool bUp = K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up, bDown = K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down, bLeft = K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left, bRight = K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right;
	if (bUp || bDown || bLeft || bRight)
	{
		if (Count()) Choose(Chosen() + (bUp ? -Columns() : bDown ? Columns() : bLeft ? -1 : 1));
		else if (bUp || bDown) Scroll(S47Game::P(bUp ? -102.f : 102.f));
		return FReply::Handled();
	}
	if (K == EKeys::PageUp || K == EKeys::PageDown) { Scroll(S47Game::P(K == EKeys::PageUp ? -400.f : 400.f)); return FReply::Handled(); }
	if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) { Activate(); return FReply::Handled(); }
	if ((bKeyboard && S47Keys::Is(TEXT("describe"), K)) || K == EKeys::Gamepad_FaceButton_Left) { if (Reading.IsValid()) Reading->Toggle(); return FReply::Handled(); }
	// the kit's own keys on its page (C raises the camera, F the light)
	if (bKeyboard && (S47Keys::Is(TEXT("camera"), K) || S47Keys::Is(TEXT("light"), K)) && Tab == TEXT("kit") && X.Kit)
	{
		const FString Want = S47Keys::Is(TEXT("camera"), K) ? TEXT("camera") : TEXT("flashlight");
		if (X.Kit->ContainsByPredicate([&](const FS47KitItem& It) { return It.Art == Want; })) { Press(K); return FReply::Handled(); }
	}
	return FReply::Unhandled();
}

// ---------- the free prints (ui/album.ts) ----------

TSharedRef<SWidget> SS47Notebook::Print(int32 I, float W, bool bBig)
{
	using namespace S47Game;
	// .al-print: the instant print, its picture 16:10 on the paper's frame, the caption in the player's
	// hand on its wide bottom edge (b: two lines, three on a big one; small: where and when). W is
	// --w in CSS pixels; the picture is still coming up for 90 s of the night (developing)
	const FS47AlbumView::FShot& S = Album->Shots[I];
	UTexture2D* Tex = Developing(S.Id, S.Url, S.Dev);
	const float Iw = 0.875f * W, Ih = Iw * 10.f / 16.f, Edge = 0.0625f * W, Bottom = 0.234f * W;
	const float Bf = (bBig ? 0.056f : 0.074f) * W, Sf = (bBig ? 0.04f : 0.058f) * W;
	// (.al-cap b and small: the big print's caption is read as its description, bindAlbum's show)
	auto Cap = [bBig](TSharedRef<SWidget> T) { return bBig ? S47Kit::Readable(T) : T; };
	TSharedRef<SWidget> Img = Tex ? StaticCastSharedRef<SWidget>(SNew(SImage).Image(Picture(Tex))) : StaticCastSharedRef<SWidget>(SNew(SBorder).BorderImage(Box(C(0x0c0c0d))));
	TSharedRef<SWidget> Paper = SNew(SBorder).BorderImage(Box(C(0xebe5d4))).Padding(FMargin(P(Edge), P(Edge), P(Edge), 0))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).WidthOverride(P(Iw)).HeightOverride(P(Ih))[Img]]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).WidthOverride(P(Iw)).HeightOverride(P(Bottom)).Padding(FMargin(0, P(0.024f * W), 0, 0)).Clipping(EWidgetClipping::ClipToBounds)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).MaxDesiredHeight(P(Bf * 0.98f * (bBig ? 3 : 2) * 1.25f)).Clipping(EWidgetClipping::ClipToBounds)
					[Cap(SNew(STextBlock).Text(FText::FromString(S.Text)).Font(Hand(Bf)).ColorAndOpacity(C(0x22305a)).AutoWrapText(true).LineHeightPercentage(0.98f))]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, P(2), 0, 0)[Cap(SNew(STextBlock).Text(FText::FromString(S.Meta)).Font(Hand(Sf)).ColorAndOpacity(C(0x4b4a58)))]
			]
		]
	];
	// a dark shadow under it (box-shadow: 0 7px 16px rgba(0,0,0,.6))
	const TSharedRef<SWidget> Shadow = Turned(Shadowed(Paper, 16.f, FVector2D(0, 7), 0.6f), bBig ? -0.5f : TILT[(I + 3) % 8]);
	// (a click or a tap opens it big: the entry round it, SS47Notebook::Entry)
	return Shadow;
}

void SS47Notebook::ShowBig(int32 I)
{
	using namespace S47Game;
	if (!Album || !Album->Shots.IsValidIndex(I) || !BigSlot.IsValid()) return;
	// (reading.change(): another print, or none, stops what was being read)
	if (UI.IsValid()) UI->StopSpeech();
	Big = I;
	Sel.FindOrAdd(TEXT("photos")) = Photos.Num() + I;
	// --w: min(100vw - 150px, (100vh - 48px) / .844, 920px), in the mockup's pixels
	const FVector2D View = FVector2D(GetCachedGeometry().GetLocalSize()) / U;
	const float W = float(FMath::Max(200.0, FMath::Min3(View.X - 150, (View.Y - 48) / 0.844, 920.0)));
	const int32 N = Album->Shots.Num();
	auto Nav = [this](const TCHAR* Label, int32 To, bool bOn) -> TSharedRef<SWidget>
	{
		return SNew(SBox).WidthOverride(P(50)).HeightOverride(P(96))
		[
			SNew(SButton).ButtonStyle(Flat()).ContentPadding(0).IsEnabled(bOn).IsFocusable(false).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			.OnClicked_Lambda([this, To] { ShowBig(To); return FReply::Handled(); })
			[
				SNew(SBorder).BorderImage(Box(C(0xffffff, 0.06f), FLinearColor::Transparent, 0.f, 4.f)).HAlign(HAlign_Center).VAlign(VAlign_Center).ColorAndOpacity(FLinearColor(1, 1, 1, bOn ? 1.f : 0.18f))
				[SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(58)).ColorAndOpacity(C(0xe8e2d2))]
			]
		];
	};
	const TSharedRef<SWidget> BigPrint = Print(I, W, true);
	const TSharedRef<SWidget> Readings = UI.IsValid() ? UI->DescriptionReadings(BigPrint, TEXT("photo-description")) : SNullWidget::NullWidget;
	BigSlot->SetContent(
		SNew(SBorder).BorderImage(Box(C(0x030509, 0.93f))).Padding(0)
		// a tap outside the print puts it back
		.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&) { CloseBig(); return FReply::Handled(); })
		[
			SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(0)
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
				[BigPrint]
			]
			// .al-view > .read-aloud: under the print, in the middle, on dark (photo-description)
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, P(65))
			[
				SNew(SBorder).BorderImage(Box(C(0x111417))).Padding(FMargin(P(10), P(4)))
				.Visibility_Lambda([Readings] { return Readings->GetDesiredSize().X > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
				[Readings]
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(P(6), 0, 0, 0)[Nav(TEXT("‹"), I - 1, I > 0)]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0, 0, P(6), 0)[Nav(TEXT("›"), I + 1, I < N - 1)]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, P(8), P(12), 0)
			[
				SNew(SButton).ButtonStyle(Flat()).ContentPadding(FMargin(0, P(6))).IsFocusable(false).OnClicked_Lambda([this] { CloseBig(); return FReply::Handled(); })
				[
					SNew(STextBlock).Font(Term(22)).ColorAndOpacity(C(0xcfc8b5)).Text_Lambda([] { return FText::FromString(S47Gamepad::UsingGamepad() ? TEXT("B  Back to the album     LB/RB  Previous / next") : TEXT("E  Back to the album")); })
				]
			]
		]);
	// the album is told which print is being looked at (core/album.ts, the face)
	if (Album->Look) Album->Look(Album->Shots[I].Id);
}

void SS47Notebook::CloseBig()
{
	// (reading.change(); the box goes with the view)
	if (Big != INDEX_NONE && UI.IsValid()) UI->StopSpeech();
	Big = INDEX_NONE;
	if (BigSlot.IsValid()) BigSlot->SetContent(SNullWidget::NullWidget);
}

// ---------- self test ----------

bool SS47Notebook::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](const TCHAR* Name, bool bPass, const FString& Got = FString())
	{
		Lines.Add(bPass ? FString::Printf(TEXT("ok  notebook: %s"), Name) : FString::Printf(TEXT("FAIL notebook: %s %s"), Name, *Got));
		bOk &= bPass;
	};
	if (!FSlateApplication::IsInitialized()) { Check(TEXT("slate"), false); return bOk; }
	FS47Doc Log; Log.Id = TEXT("log"); Log.Title = TEXT("Dale's shift log"); Log.Kind = TEXT("hand"); Log.Page = TEXT("Kid,");
	FS47Doc Typed; Typed.Id = TEXT("s03log"); Typed.Title = TEXT("S-03 / No command recorded"); Typed.Kind = TEXT("typed"); Typed.Page = TEXT("SARO ARRAY");
	FS47Doc Find; Find.Id = TEXT("p01"); Find.Title = TEXT("P01 / One"); Find.Kind = TEXT("typed"); Find.Page = TEXT("P01  SUPPORTED / one");
	FS47NbExtras Ex;
	Ex.Map = FS47MapView();
	Ex.Kit = TArray<FS47KitItem>{ { TEXT("Notebook"), TEXT("Tab opens it."), FString(), TEXT("notebook"), FString() }, { TEXT("SARO jacket"), TEXT("From the hook."), TEXT("On"), TEXT("jacket"), FString() },
		{ TEXT("Truck keys"), TEXT("SARO 07."), TEXT("On the pad"), TEXT("keys"), TEXT("SARO 07") }, { TEXT("Papers"), TEXT("Folded."), TEXT("2"), TEXT("papers"), FString() } };
	Ex.People = TArray<FS47PersonCard>{ { TEXT("Dale"), TEXT("SARO"), TEXT("Left."), FString() } };
	TArray<FS47Task> T; T.Add({ TEXT("Read the log"), true }); T.Add({ TEXT("Answer the line"), false });
	FString Opened;
	int32 Boards = 0;
	const TSharedRef<SS47Notebook> Nb = SNew(SS47Notebook).Tasks(T).Notes({ TEXT("One."), TEXT("Two.") }).Docs({ Log, Typed, Find })
		.OnDoc([&Opened](const FS47Doc& D) { Opened = D.Id; }).OnBoard([&Boards] { ++Boards; }).Extras(Ex);
	auto Key = [&](const FKey& K) { Nb->OnKeyDown(FGeometry(), FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0)); };
	Check(TEXT("tabs in order"), FString::Join(Nb->Order, TEXT(" ")) == TEXT("tasks notes findings papers photos map kit people controls"), FString::Join(Nb->Order, TEXT(" ")));
	Check(TEXT("opens on tasks"), Nb->Tab == TEXT("tasks"), Nb->Tab);
	Key(EKeys::E);
	Check(TEXT("E turns on"), Nb->Tab == TEXT("notes"), Nb->Tab);
	Key(EKeys::Q); Key(EKeys::Q);
	Check(TEXT("Q turns back, round to the last"), Nb->Tab == TEXT("controls"), Nb->Tab);
	Nb->Show(TEXT("kit"));
	Key(EKeys::Right);
	Check(TEXT("right chooses the next"), Nb->Chosen() == 1, FString::FromInt(Nb->Chosen()));
	Key(EKeys::Down);
	Check(TEXT("down goes a row, to the last"), Nb->Chosen() == 3, FString::FromInt(Nb->Chosen()));
	Key(EKeys::Enter);
	Check(TEXT("Enter on the papers turns to them"), Nb->Tab == TEXT("papers"), Nb->Tab);
	Check(TEXT("papers and findings apart"), Nb->Papers.Num() == 2 && Nb->Findings.Num() == 1);
	Key(EKeys::Right); Key(EKeys::Enter);
	Check(TEXT("Enter opens the chosen paper"), Opened == TEXT("s03log"), Opened);
	Nb->Show(TEXT("kit"));
	Check(TEXT("each tab keeps its choice"), Nb->Chosen() == 3, FString::FromInt(Nb->Chosen()));
	// Xbox uses the same selected entries, tab state and callbacks as the keyboard.
	Nb->Show(TEXT("tasks")); Key(EKeys::Gamepad_RightShoulder);
	Check(TEXT("RB turns to the next tab without opening the board"), Nb->Tab == TEXT("notes") && Boards == 0);
	Key(EKeys::Gamepad_LeftShoulder);
	Check(TEXT("LB returns to the previous tab"), Nb->Tab == TEXT("tasks"));
	Nb->Show(TEXT("papers")); Nb->Choose(0); Key(EKeys::Gamepad_DPad_Right);
	Opened.Empty(); Key(EKeys::Gamepad_FaceButton_Bottom);
	Check(TEXT("D-pad and A open the selected paper"), Nb->Chosen() == 1 && Opened == TEXT("s03log"));
	Key(EKeys::Gamepad_DPad_Left);
	Check(TEXT("D-pad returns to the first paper"), Nb->Chosen() == 0);
	Nb->Album = FS47AlbumView(); Nb->Album->Shots = { { TEXT("one"), FString(), TEXT("One"), FString() }, { TEXT("two"), FString(), TEXT("Two"), FString() } };
	Nb->Show(TEXT("photos")); Nb->ShowBig(0); Key(EKeys::Gamepad_RightShoulder);
	Check(TEXT("RB pages a large print without turning notebook tabs"), Nb->Big == 1 && Nb->Tab == TEXT("photos"));
	Key(EKeys::Gamepad_FaceButton_Right);
	Check(TEXT("B puts a large print back in the album"), Nb->Big == INDEX_NONE && Nb->Tab == TEXT("photos"));
	Key(EKeys::Gamepad_FaceButton_Top);
	Check(TEXT("Y opens the board once"), Boards == 1);
	// CONTROLS: the keys as bound, and the page built again when one changes
	{
		const TSharedPtr<FJsonObject> Keys = S47Keys::Save();
		S47Keys::ResetAll();
		auto Row = [](const FCtlPage& Pg, int32 G, int32 R) { return Pg.Groups.IsValidIndex(G) && Pg.Groups[G].Rows.IsValidIndex(R) ? Pg.Groups[G].Rows[R].Key + TEXT(" | ") + Pg.Groups[G].Rows[R].Value : FString(); };
		auto Has = [](const FCtlPage& Pg, const FString& Want) { for (const FCtlGroup& Gr : Pg.Groups) for (const TPair<FString, FString>& R : Gr.Rows) if (R.Key + TEXT(" | ") + R.Value == Want) return true; return false; };
		const FCtlPage Desk = Controls(false);
		Check(TEXT("controls: the walk keys as the web has them"), Row(Desk, 0, 0) == TEXT("W A S D, arrows | Walk. Shift: run."), Row(Desk, 0, 0));
		Check(TEXT("controls: the notebook's keys as the web has them"), Has(Desk, TEXT("Tab, N | The notebook. M: its map. I: the kit. H: these controls.")));
		Check(TEXT("controls: Space both jump and shutter"), Has(Desk, TEXT("Space | Jump. With the camera raised: the shutter.")));
		Nb->Show(TEXT("controls"));
		S47Keys::Bind(TEXT("forward"), 0, EKeys::I);
		S47Keys::Bind(TEXT("map"), 0, EKeys::K);
		S47Keys::Bind(TEXT("shutter"), 0, EKeys::G);
		const FCtlPage Now = Controls(false);
		Check(TEXT("controls: a walk key rebound"), Row(Now, 0, 0) == TEXT("I A S D, arrows | Walk. Shift: run."), Row(Now, 0, 0));
		Check(TEXT("controls: the kit's old key goes to the walk"), Has(Now, TEXT("Tab, N | The notebook. K: its map. W: the kit. H: these controls.")), Row(Now, 0, 9));
		Check(TEXT("controls: the shutter apart from the jump"), Has(Now, TEXT("G | The shutter, with the camera raised.")) && Has(Now, TEXT("Space | Jump.")));
		Check(TEXT("controls: keyboard rebinds preserve fixed Xbox aliases"), S47Keys::Is(TEXT("shutter"), EKeys::Gamepad_RightTrigger) && S47Keys::Is(TEXT("notebook"), EKeys::Gamepad_LeftShoulder) && S47Keys::KeysOf(TEXT("notebook")).Contains(EKeys::Gamepad_Special_Left));
		Check(TEXT("controls: the truck's throttle"), Row(Now, 1, 0) == TEXT("I, S | Throttle and brake. S at a stop: reverse."), Row(Now, 1, 0));
		Check(TEXT("controls: the page knows the keys moved"), Nb->KeysShown != S47Keys::Changes());
		S47Keys::Load(Keys);
	}
	return bOk;
}
