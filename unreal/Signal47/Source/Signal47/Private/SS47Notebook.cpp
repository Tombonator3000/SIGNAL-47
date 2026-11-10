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
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47Script.h"
#include "S47StationFiles.h"
#include "S47Marginalia.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "SS47AreaMap.h"
#include "SS47FieldMap.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"
#include "TextureResource.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
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
		if (!Line.IsEmpty()) B->AddSlot().AutoHeight().Padding(P(2), 0, 0, 0)[SNew(STextBlock).Text(FText::FromString(Line)).Font(Term(16, 1)).AutoWrapText(true).ColorAndOpacity(Ph2Red())];
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

	FCtlPage Controls(bool bPhone, bool bPad = false)
	{
		if (bPad) return { TEXT("Controller"), {
			{ TEXT("On foot"), {
				{ TEXT("LS / L3"), TEXT("Walk / run.") },
				{ TEXT("RS"), TEXT("Look around.") },
				{ TEXT("A / B"), TEXT("Use / the other choice, or the takeout bag.") },
				{ TEXT("X / RT"), TEXT("Raise or lower the camera / shutter while raised.") },
				{ TEXT("LT / Y"), TEXT("Hold to look closer / jump.") },
				{ TEXT("D-pad Up / Down"), TEXT("Flashlight / the walkie: call Dale, or answer him. In the truck, Down is the radio.") },
				{ TEXT("RS click"), TEXT("Keep low, or stand up again.") },
				{ TEXT("D-pad Left / Right"), TEXT("Map / spoken description.") },
				{ TEXT("LB / View"), TEXT("The notebook. View has two overlapping rectangles.") },
				{ TEXT("RB / Menu"), TEXT("The case board / pause, settings, save and load.") } } },
			{ TEXT("In the truck"), {
				{ TEXT("LS / RS"), TEXT("Steer / look around the cab.") },
				{ TEXT("RT / LT"), TEXT("Throttle / brake or reverse.") },
				{ TEXT("A, stopped"), TEXT("Get out, or turn the key when the engine has died.") },
				{ TEXT("D-pad Down"), TEXT("The radio: on, the next station, off.") } } },
			{ TEXT("Menus and the case board"), {
				{ TEXT("D-pad / A / B"), TEXT("Select / confirm / go back.") },
				{ TEXT("LB / RB"), TEXT("Turn tabs. On the case board: zoom out / in.") },
				{ TEXT("RS / A held"), TEXT("Move a pointer / click or drag a card, tracing or thread.") } } }
		} };
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
				{ KeysSaid(TEXT("light")), TEXT("The flashlight, once you have it.") },
				{ KeysSaid(TEXT("camera")), TEXT("Raise or lower the field camera.") } } };
			if (bShared) Foot.Rows.Add({ KeysSaid(TEXT("jump")), TEXT("Jump. With the camera raised: the shutter.") });
			else
			{
				Foot.Rows.Add({ KeysSaid(TEXT("jump")), TEXT("Jump.") });
				Foot.Rows.Add({ KeysSaid(TEXT("shutter")), TEXT("The shutter, with the camera raised.") });
			}
			// (the second choice and the case board have rows of their own here; the web's table has neither)
			Foot.Rows.Add({ KeysSaid(TEXT("alt")), TEXT("The takeout bag: set it down, stow it, take it out or throw it away. At a prompt, the other choice.") });
			// M7-HUD-20261109: loose things in the hand, and the stones (PHYSICS-20261108, STONE-POCKET-20261109)
			Foot.Rows.Add({ KeysSaid(TEXT("throw")), TEXT("Hold to throw what you carry; tap to set it down, or to pocket a stone (three fit). Empty-handed: a stone from the pocket, or outdoors, one off bare ground.") });
			Foot.Rows.Add({ KeysSaid(TEXT("notebook")), FString::Printf(TEXT("The notebook. %s: its map. %s: the kit. %s: these controls."), *KeySaid(TEXT("map")), *KeySaid(TEXT("kit")), *KeySaid(TEXT("controls"))) });
			Foot.Rows.Add({ KeysSaid(TEXT("board")), TEXT("The case board, once there is one.") });
			Foot.Rows.Add({ KeysSaid(TEXT("pause")), TEXT("Pause: settings, save and load.") });
			FCtlGroup Truck = { TEXT("In the truck"), {
				{ First(TEXT("forward")) + TEXT(", ") + First(TEXT("back")), FString::Printf(TEXT("Throttle and brake. %s at a stop: reverse."), *KeySaid(TEXT("back"))) },
				{ First(TEXT("left")) + TEXT(", ") + First(TEXT("right")), TEXT("Steer.") },
				{ TEXT("Mouse"), TEXT("Look around the cab.") },
				{ KeysSaid(TEXT("radio")) + TEXT(", or click the radio"), TEXT("The radio: on, the next station, off.") },
				{ First(TEXT("use")) + TEXT(", stopped"), TEXT("Get out. When the engine has died: turn the key.") } } };
			// SNEAK-20261108 and WALKIE-USE-20261108
			FCtlGroup Low = { TEXT("Walkie and keeping low"), {
				{ KeysSaid(TEXT("crouch")), TEXT("Keep low, or stand up again: slower and quieter, a little harder to see and hear.") },
				{ KeysSaid(TEXT("walkie")), TEXT("The walkie, once you carry it: call Dale, or answer him when he calls.") },
				{ KeysSaid(TEXT("channel")), TEXT("The walkie's channel: 1 is Dale's, 2 the service band, 3 nobody's.") },
				// M7-HUD-20261109: the field recorder and the counter, carried as the walkie is (TAPE-20261108, GEIGER-20261108)
				{ KeysSaid(TEXT("tape")), TEXT("The field recorder, once you carry it: start or stop recording.") },
				{ KeysSaid(TEXT("counter")), TEXT("The Geiger counter, once you have built it: hold it up, or put it away.") } } };
			return { TEXT("Keyboard and mouse"), { Foot, Truck, Low } };
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
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(P(170))[SNew(STextBlock).Text(FText::FromString(R.Key)).Font(Term(20)).ColorAndOpacity(C(0x2e2a22)).AutoWrapText(true)]]
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


	// ---------- PT-NOTEBOOK-20261109: Freeform's papers as the notebook shows them (the saved ones are not changed) ----------
	// what a paper is, as its card says it ("Field note", "Telex"), and the same in capitals on the open page
	FString KindWord(const FS47Doc& D, bool bFreeform = true)
	{
		// (Story's book as it was: its own four, the rest a print)
		if (!bFreeform && D.Kind != TEXT("hand") && D.Kind != TEXT("printout") && D.Kind != TEXT("typed") && D.Kind != TEXT("map")) return TEXT("Print");
		static const TMap<FString, FString> Words = { { TEXT("hand"), TEXT("Handwritten") }, { TEXT("printout"), TEXT("Printout") }, { TEXT("typed"), TEXT("Typed") }, { TEXT("map"), TEXT("Map") },
			{ TEXT("fieldnote"), TEXT("Field note") }, { TEXT("telex"), TEXT("Telex") }, { TEXT("comparison"), TEXT("Comparison") }, { TEXT("photo"), TEXT("Photograph") } };
		const FString* W = Words.Find(D.Kind);
		return W ? *W : FString(TEXT("Print"));
	}
	// a field record's title without the "Field record: " every one of them starts with
	FString ShownTitle(const FS47Doc& D, bool bFreeform)
	{
		static const FString Lead = TEXT("Field record: ");
		return bFreeform && D.Title.StartsWith(Lead, ESearchCase::CaseSensitive) ? D.Title.Mid(Lead.Len()) : D.Title;
	}
	// a field record's own words: no stamp and OBSERVATION before them, and no standing sentences after (FS47FreeformSession::RecordEvidence)
	const TCHAR* const FieldTail = TEXT("Interpretation remains separate from the retained observation. Compare independent sources before assigning an origin.");
	FString ShownPage(const FS47Doc& D, bool bFreeform)
	{
		if (!bFreeform) return D.Page;
		FString T = D.Page;
		const FString HeadLine = D.Stamp + TEXT("\n\nOBSERVATION\n");
		if (D.Kind == TEXT("fieldnote") && !D.Stamp.IsEmpty() && T.StartsWith(HeadLine, ESearchCase::CaseSensitive)) T.RightChopInline(HeadLine.Len());
		T.ReplaceInline(FieldTail, TEXT(""), ESearchCase::CaseSensitive);
		return T.TrimStartAndEnd();
	}
	// the night a paper belongs to by what is stamped on it: "DAY 2", a job's "FF-D0002", a log's "nightlog-2"; 0: it does not say
	int32 NightSaid(const FS47Doc& D)
	{
		const FString Stamp = D.Stamp.ToUpper(), Both = D.Id + TEXT(" ") + D.Stamp;
		static const FRegexPattern DayN(TEXT("DAY (\\d+)")), Job(TEXT("FF-D(\\d{4})")), Log(TEXT("^(?:nightlog|shiftlog)-(\\d+)$"));
		{ FRegexMatcher Mt(DayN, Stamp); if (Mt.FindNext()) return FCString::Atoi(*Mt.GetCaptureGroup(1)); }
		{ FRegexMatcher Mt(Job, Both); if (Mt.FindNext()) return FCString::Atoi(*Mt.GetCaptureGroup(1)); }
		{ FRegexMatcher Mt(Log, D.Id); if (Mt.FindNext()) return FCString::Atoi(*Mt.GetCaptureGroup(1)); }
		return 0;
	}
	// the next step's red arrow on NOW (the fonts have none)
	class SNextArrow : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SNextArrow) {}
		SLATE_END_ARGS()
		void Construct(const FArguments&) {}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(P(24), P(24)); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const FLinearColor Col = S47Game::StampRed();
			const float Wd = FMath::Max(1.f, P(2.2f));
			FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(P(2), P(13)), FVector2f(P(20), P(13)) }, ESlateDrawEffect::None, Col, true, Wd);
			FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(P(13), P(6)), FVector2f(P(21), P(13)), FVector2f(P(13), P(20)) }, ESlateDrawEffect::None, Col, true, Wd);
			return Layer + 1;
		}
	};

	// MARGINALIA-20261109: WHO & WHAT's squared paper, faint blue lines under the pencil
	class SGraphPaper : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGraphPaper) {}
		SLATE_END_ARGS()
		void Construct(const FArguments&) {}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8, 8); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const FVector2f Sz = FVector2f(G.GetLocalSize());
			const float Step = FMath::Max(6.f, P(24.f));
			const FLinearColor Col = C(0x233a7a, 0.09f);
			for (float X = Step; X < Sz.X; X += Step) FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(X, 0), FVector2f(X, Sz.Y) }, ESlateDrawEffect::None, Col, false, 1.f);
			for (float Y = Step; Y < Sz.Y; Y += Step) FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(0, Y), FVector2f(Sz.X, Y) }, ESlateDrawEffect::None, Col, false, 1.f);
			return Layer + 1;
		}
	};
	// MARGINALIA-20261109: a line through struck text. Slate draws a text block's strike only when its brush names a resource (the
	// plain-text marshaller tests GetResourceName), and the colour brush "WhiteBrush" names none: an image brush, the engine's white.
	FSlateBrush StrikeBrush(const FLinearColor& Ink)
	{
		FSlateBrush Strike = *FCoreStyle::Get().GetBrush("GenericWhiteBox");
		Strike.TintColor = Ink;
		return Strike;
	}
	// A scribble's pencil: soft grey lead, and a line through it when the guess was proved wrong.
	FTextBlockStyle PencilStyle(float Px, bool bStruck)
	{
		const FLinearColor Lead = bStruck ? C(0x3a3a40, 0.55f) : C(0x3a3a40);
		FTextBlockStyle St = FTextBlockStyle().SetFont(S47Game::Hand(Px)).SetColorAndOpacity(Lead);
		if (bStruck) St.SetStrikeBrush(StrikeBrush(Lead));
		return St;
	}
	// One scribble: the pencil, struck through when proved wrong, and in red pencil where it points on to ([MAP], [BOARD]).
	TSharedRef<SWidget> ScribbleLine(const FS47NbScribble& S, float Px, TFunction<void()> OnFollow)
	{
		using namespace S47Game;
		const FTextBlockStyle St = PencilStyle(Px, S.bStruck);
		TSharedRef<SVerticalBox> L = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(S.Text)).TextStyle(&St).AutoWrapText(true).LineHeightPercentage(LinePercent(Hand(Px), 30))];
		if (!S.FollowText.IsEmpty())
		{
			const FString Tag = S.Follow == TEXT("map") ? TEXT("  [MAP]") : S.Follow == TEXT("board") ? TEXT("  [BOARD]") : TEXT("");
			const TSharedRef<SWidget> F = SNew(STextBlock).Text(FText::FromString(TEXT("-> ") + S.FollowText + Tag)).Font(Hand(Px - 2.f)).ColorAndOpacity(StampRed()).AutoWrapText(true);
			L->AddSlot().AutoHeight().Padding(P(18), 0, 0, 0)[OnFollow ? StaticCastSharedRef<SWidget>(Click(F, OnFollow)) : F];
		}
		return L;
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
	auto IsFinding = [](const FS47Doc& D) { FRegexMatcher M(Pnn, D.Id); return D.Kind == TEXT("fieldnote") || D.Kind == TEXT("comparison") || M.FindNext() || D.Id == TEXT("finding") || (D.Id == TEXT("anomaly:n01") || D.Id == TEXT("anomaly:n02") || D.Id == TEXT("anomaly:a02") || D.Id == TEXT("anomaly:p01")); };
	for (const FS47Doc& D : InArgs._Docs) if (D.Kind == TEXT("photo") && !D.Image.IsEmpty()) Photos.Add(D);
	if (!X.bFreeform)
	{
		for (const FS47Doc& D : InArgs._Docs) if (IsFinding(D)) Findings.Add(D);
		for (const FS47Doc& D : InArgs._Docs) if (!(D.Kind == TEXT("photo") && !D.Image.IsEmpty())) Papers.Add(D);
	}
	else
	{
		// PT-NOTEBOOK-20261109 (Tom, 9 November: knowledge and documents hard to find things in). Only how it is shown: the field records
		// under Knowledge and only there, DOCUMENTS the papers as the pinboard's tray has them (FS47PinBoard::Papers), the photographs in
		// the ALBUM only; each thing once (a Reyes tag's or a monitor printout's field record is its paper's twin); an older save's case
		// records hidden as the case file now keeps them (FS47FreeformCases::IsCaseRecord); newest first, by the night stamped on each.
		int32 Carry = 1;
		for (const FS47Doc& D : InArgs._Docs) { const int32 N = NightSaid(D); Carry = N > 0 ? N : Carry; DocNight.Add(D.Id, Carry); }
		TSet<FString> Ids, Answers;
		for (const FS47Doc& D : InArgs._Docs)
		{
			Ids.Add(D.Id);
			if (D.Id.StartsWith(TEXT("answer-printout-"))) Answers.Add(FString::FromInt(FCString::Atoi(*D.Id.Mid(16)) % 100000));
		}
		const auto OldCase = [](const FS47Doc& D) { return D.Id.StartsWith(TEXT("freeform-")) && D.Kind == TEXT("fieldnote") && FS47FreeformCases::IsCaseRecord(D.Id.Mid(9)); };
		const auto Twin = [&Ids, &Answers](const FS47Doc& D)
		{
			if (!D.Id.StartsWith(TEXT("freeform-"))) return false;
			const FString Base = D.Id.Mid(9);
			FString Left, Num;
			return Ids.Contains(Base) || (Base.StartsWith(TEXT("answer_d")) && Base.Split(TEXT("_"), &Left, &Num, ESearchCase::CaseSensitive, ESearchDir::FromEnd) && Answers.Contains(Num));
		};
		for (const FS47Doc& D : InArgs._Docs) if (IsFinding(D) && !OldCase(D) && !Twin(D)) Findings.Add(D);
		TSet<FString> Shown;
		for (const FS47Doc& D : X.Papers ? *X.Papers : InArgs._Docs)
		{
			if (D.Kind == TEXT("photo") || IsFinding(D) || OldCase(D) || Shown.Contains(D.Id)) continue;
			Shown.Add(D.Id);
			if (!DocNight.Contains(D.Id)) DocNight.Add(D.Id, NightSaid(D) > 0 ? NightSaid(D) : Carry);
			Papers.Add(D);
		}
		const auto NewestFirst = [this](TArray<FS47Doc>& A)
		{
			for (int32 I = 0, J = A.Num() - 1; I < J; ++I, --J) A.Swap(I, J);
			A.StableSort([this](const FS47Doc& L, const FS47Doc& R) { return DocNight.FindRef(L.Id) > DocNight.FindRef(R.Id); });
		};
		NewestFirst(Findings); NewestFirst(Papers);
	}

	Order = { TEXT("tasks"), TEXT("findings"), TEXT("papers"), TEXT("photos"), TEXT("kit"), TEXT("map"), TEXT("guide") };
	// PT-NOTEBOOK-20261109: Freeform's NOW has the list's lines (X.Now, S47TonightNote::Gather); otherwise every task not among the
	// work items is a line of its own (no longer skipped once there is structured work: the post, Reyes and the truck were lost)
	if (!X.bNow) for (int32 I = 0; I < Tasks.Num(); ++I)
	{
		if (X.WorkItems.ContainsByPredicate([&](const FS47NbWorkItem& W) { return W.Title == Tasks[I].Text; })) continue;
		FS47NbWorkItem W; W.Title = Tasks[I].Text; W.bDone = Tasks[I].bDone;
		W.Status = W.bDone ? TEXT("Complete") : TEXT("Open"); W.NextAction = W.bDone ? TEXT("Already completed.") : Tasks[I].Text;
		X.WorkItems.Add(W);
	}

	AllPapers = Papers;
	AllWorkItems = X.WorkItems;
	FilterWork();
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
	Keys->AddSlot().AutoWidth().Padding(0, 0, P(28), 0)[Bar(Keycap(TEXT("Q E")), TEXT("TAB / , SECTIONS"), nullptr)];
	Keys->AddSlot().AutoWidth().Padding(0, 0, P(28), 0)[Bar(Arrows, TEXT("CHOOSE"), nullptr)];
	// CASES-OPS-20261109: Freeform's B is the PINBOARD (CASE BOARD is the OPS screen there)
	if (OnBoard) Keys->AddSlot().AutoWidth().Padding(0, 0, P(28), 0)[Bar(Keycap(S47Keys::Label(TEXT("board")).ToUpper()), X.bFreeform ? TEXT("PINBOARD") : TEXT("CASE BOARD"), [this] { TFunction<void()> Fn = OnBoard; Close(); if (Fn) Fn(); })];
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
						+ SOverlay::Slot()[SNew(SOverlay) + SOverlay::Slot()[SAssignNew(LeftSlot, SBox)] + SOverlay::Slot().VAlign(VAlign_Top).Padding(P(84), P(10), P(20), 0)[SAssignNew(SectionSlot, SBox)]]
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
						SNew(SBox).HAlign(HAlign_Right)[Osd(TAttribute<FText>::CreateLambda([] { const US47Main* M = US47Main::Get(nullptr); if (M && M->IsFreeform() && M->Freeform) return FText::FromString(S47Game::OsdLine(M->Freeform->Date(), M->Freeform->Clock())); return FText::FromString(M && M->Story ? OsdLine(M->Story->Clock()) : FString()); }), 26.f)]
					])
				])
			])
		]
		// .al-view: one free print big, over the notebook
		+ SOverlay::Slot()[SAssignNew(BigSlot, SBox).Visibility_Lambda([this] { return Big != INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed; })]
	];
	// show(): the tab used last, if this notebook has it; D45-20261110: opened with its own key (not at a page asked for), NOW when
	// something new is on it
	const FString Last = UI.IsValid() && (Order.Contains(UI->NbTab) || UI->NbTab == TEXT("notes") || UI->NbTab == TEXT("people") || UI->NbTab == TEXT("photos") || UI->NbTab == TEXT("controls")) ? UI->NbTab : FString(TEXT("tasks"));
	const bool bPlain = S47TonightNote::Seen().bPlain; S47TonightNote::Seen().bPlain = false;
	Show(bPlain ? OpenOn(X, Last) : Last);
}

FString SS47Notebook::OpenOn(const FS47NbExtras& Ex, const FString& Last)
{
	return Ex.bFreeform && Ex.bNow && Ex.Now.ContainsByPredicate([](const FS47NbNowLine& N) { return N.bNew && !N.bDone; }) ? FString(TEXT("tasks")) : Last;
}

FString SS47Notebook::NowHead(const FString& Group)
{
	// D45-20261110: by importance (S47TonightNote::RankOf); the older groups' words for a list made the old way
	static const TMap<FString, FString> Heads = { { TEXT("first"), TEXT("DO FIRST") }, { TEXT("tonight"), TEXT("TONIGHT") }, { TEXT("later"), TEXT("WHEN YOU CAN") }, { TEXT("done"), TEXT("DONE TONIGHT") },
		{ TEXT("clock"), TEXT("BY THE CLOCK") }, { TEXT("todo"), TEXT("TO DO") }, { TEXT("also"), TEXT("ALSO OPEN") } };
	const FString* Hd = Heads.Find(Group);
	return Hd ? *Hd : FString();
}

namespace { uint32 NotebooksShut = 0; }
uint32 SS47Notebook::Shut() { return NotebooksShut; }

SS47Notebook::~SS47Notebook()
{
	if (Reading.IsValid() && Reading->bActive) Reading->Stop();
	if (UI.IsValid()) ++NotebooksShut;
}

TSharedRef<SWidget> SS47Notebook::TabStack()
{
	using namespace S47Game;
	// .tb: the coloured paper tabs, their word up the edge; the open one stands out further and darker
	static const TMap<FString, TPair<const TCHAR*, uint32>> Look = {
		{ TEXT("tasks"), { TEXT("NOW"), 0xd7c7a0 } }, { TEXT("notes"), { TEXT("NOTES"), 0xc9b48a } }, { TEXT("findings"), { TEXT("KNOWLEDGE"), 0xbfc7a6 } },
		{ TEXT("papers"), { TEXT("DOCUMENTS"), 0xd3b3a0 } }, { TEXT("photos"), { TEXT("ALBUM"), 0xc3c9cf } }, { TEXT("map"), { TEXT("MAP"), 0xe0c27a } },
		{ TEXT("kit"), { TEXT("KIT"), 0xcfbfa9 } }, { TEXT("people"), { TEXT("PEOPLE"), 0xb9c2b0 } }, { TEXT("guide"), { TEXT("GUIDE"), 0xd9d0bf } } };
	const float Gap = 6.f;
	TSharedRef<SVerticalBox> Col = SNew(SVerticalBox);
	// Render rotation does not swap Slate's layout axes. Give the unrotated text its
	// measured rectangle inside a canvas, then expose the swapped desired size to the tab.
	const FSlateFontInfo TabFont = Term(UI.IsValid() && UI->bLargeText ? 22.f : 19.f, 1);
	auto Measure = [TabFont](const FString& S) -> FVector2D
	{
		const FVector2D Size = FSlateApplication::IsInitialized()
			? FVector2D(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, TabFont))
			: FVector2D(P(float(S.Len()) * 14.f), P(28.f));
		return Size + P2(10, 8); // the word's own padding and its ink/shadow bearing must fit inside the allocation
	};
	auto Long = [Measure](const FString& S) { return FMath::Max(58.f, float(Measure(S).X) / U + 10.f); };
	auto Word = [TabFont, Measure](const FString& S, const FLinearColor& Ink, bool bBold) -> TSharedRef<SWidget>
	{
		const FVector2D Size = Measure(S);
		const TSharedRef<SWidget> T = SNew(STextBlock).Text(FText::FromString(S)).Font(TabFont).ColorAndOpacity(Ink)
			.ShadowOffset(FVector2D(bBold ? P(0.6f) : 0.f, 0)).ShadowColorAndOpacity(Ink);
		return SNew(SBox).WidthOverride(Size.Y).HeightOverride(Size.X)
			[SNew(SCanvas) + SCanvas::Slot().Position(FVector2D((Size.Y - Size.X) / 2, (Size.X - Size.Y) / 2)).Size(Size)
				[Turned(SNew(SBox).Padding(P(2))[T], -90.f)]];
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
				Click(SNew(SBox).HeightOverride(P(H)).WidthOverride_Lambda([this, T] { return FOptionalSize(P(MainTab() == T ? 76.f : 64.f)); })
					[
						Shadowed(SNew(SBorder).BorderImage(Box4(C(Rgb), FVector4(8, 0, 0, 8))).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0)
							[
								SNew(SBox).Padding(FMargin(P(0), 0, P(0), 0))
								[
									SNew(SOverlay)
									+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).Visibility_Lambda([this, T] { return MainTab() == T ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })[Word(Label, C(0x2c2418), false)]]
									+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).Visibility_Lambda([this, T] { return MainTab() == T ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })[Word(Label, C(0x120c05), true)]]
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
	if (Tab == TEXT("tasks")) return X.bNow ? NowRows.Num() : X.WorkItems.Num();
	if (Tab == TEXT("guide")) return X.GuideEntries.Num();
	if (Tab == TEXT("findings")) return Findings.Num();
	if (Tab == TEXT("papers")) return Papers.Num();
	if (Tab == TEXT("photos")) return Photos.Num() + (Album ? Album->Shots.Num() : 0);
	if (Tab == TEXT("kit")) return X.Kit ? X.Kit->Num() : 0;
	if (Tab == TEXT("people")) return X.People ? X.People->Num() : 0;
	return 0;
}

int32 SS47Notebook::Columns() const
{
	if (Tab == TEXT("kit") || (Tab == TEXT("people") && !X.bFreeform)) return 3;
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
	FieldMap.Reset();
	bRightIsEntries = false;
	if (!LeftSlot.IsValid()) return;
	LeftSlot->SetContent(SNullWidget::NullWidget);
	SpreadSlot->SetContent(SNullWidget::NullWidget);
	RightSlot->SetContent(SNullWidget::NullWidget);
	UseSlot->SetContent(SNullWidget::NullWidget);
	if (SectionSlot)
	{
		TSharedRef<SHorizontalBox> Links = SNew(SHorizontalBox);
		auto InlineLabel = [&](const TCHAR* Label) { auto Text = Typed(Label, 13); Text->SetAutoWrapText(false); return Text; };
		auto Link = [&](const TCHAR* Id, const TCHAR* Label) { Links->AddSlot().AutoWidth().Padding(0, 0, P(12), 0)[Click(InlineLabel(Label), [this, Id] { Show(Id); })]; };
		if (MainTab() == TEXT("tasks")) Links->AddSlot().AutoWidth()[Click(InlineLabel(bShowHistory ? TEXT("Hide history [, / LT]") : TEXT("Show history [, / LT]")), [this] { Section(1); })];
		// PT-NOTEBOOK-20261109: Freeform's Knowledge is the evidence and WHO & WHAT (Notes only while there are any); its photographs are
		// the ALBUM's alone
		if (MainTab() == TEXT("findings") && X.bFreeform) { Link(TEXT("findings"), TEXT("Evidence")); Link(TEXT("people"), TEXT("Who & what")); if (Notes.Num()) Link(TEXT("notes"), TEXT("Notes")); }
		else if (MainTab() == TEXT("findings")) { Link(TEXT("findings"), TEXT("Evidence")); Link(TEXT("notes"), TEXT("Notes")); Link(TEXT("people"), TEXT("People")); }
		if (MainTab() == TEXT("papers"))
		{
			Link(TEXT("papers"), TEXT("Originals")); if (!X.bFreeform) Link(TEXT("photos"), TEXT("Photos"));
			if (Tab == TEXT("papers")) Links->AddSlot().FillWidth(1.f)[SNew(SEditableTextBox).Text(FText::FromString(PaperQuery)).HintText(FText::FromString(TEXT("Job / day / type; Enter"))).Font(Type(12))
				.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type Commit) { if (Commit == ETextCommit::OnEnter) { FilterDocuments(Text.ToString()); Build(); FSlateApplication::Get().SetKeyboardFocus(SharedThis(this)); } })];
		}
		if (MainTab() == TEXT("guide")) { Link(TEXT("guide"), TEXT("Procedures")); Link(TEXT("controls"), TEXT("Controls")); }
		SectionSlot->SetContent(Links);
	}
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
			if (bDone) St.SetStrikeBrush(StrikeBrush(Ink)); // (MARGINALIA-20261109: a brush Slate draws the strike with)
			TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
			if (bTasks) Row->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(P(28))[SNew(STextBlock).Text(FText::FromString(bDone ? TEXT("x") : TEXT("-"))).Font(Hand(Li)).ColorAndOpacity(Ink)]];
			Row->AddSlot().FillWidth(1.f)[SNew(STextBlock).Text(FText::FromString(Items[I])).TextStyle(&St).AutoWrapText(true).LineHeightPercentage(LinePercent(Hand(Li), 34))];
			L->AddSlot().AutoHeight()[Row];
		}
		return L;
	};

	// PT-NOTEBOOK-20261109 (Freeform): a heading over each night's papers, newest night first
	auto NightHead = [&](int32 Night, bool bFirst) -> TSharedRef<SWidget>
	{
		return SNew(SBox).Padding(FMargin(0, P(bFirst ? 0 : 10), 0, P(4)))[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("DAY %d"), Night))).Font(Term(20, 2)).ColorAndOpacity(Ph2Red())];
	};
	if (IsNow())
	{
		// PT-NOTEBOOK-20261109: NOW as the pause menu's note at full length, in its words: the next step with the red arrow, then
		// (D45-20261110) DO FIRST, TONIGHT, WHEN YOU CAN (the job cards on one line that opens), DONE TONIGHT struck through; NEW in red
		// before a line not shown before
		TSharedRef<SVerticalBox> L = SNew(SVerticalBox);
		FString Group;
		const float Px = bLarge ? 30.f : 27.f;
		for (int32 I = 0; I < NowRows.Num(); ++I)
		{
			const FNowRow& R = NowRows[I];
			if (R.Line == INDEX_NONE)
			{
				if (!AllWorkItems.IsValidIndex(R.Item)) continue;
				const FS47NbWorkItem& W = AllWorkItems[R.Item];
				L->AddSlot().AutoHeight().Padding(P(40), 0, 0, P(6))[Entry(I, SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Inked(W.Title, Px - 3.f, W.bDone ? C(0x6d7895) : InkBlue())]
					+ SVerticalBox::Slot().AutoHeight()[Typed(W.Status, bLarge ? 16.f : 13.f)])];
				continue;
			}
			const FS47NbNowLine& N = X.Now[R.Line];
			if (N.Group != Group)
			{
				Group = N.Group;
				if (const FString Hd = NowHead(Group); !Hd.IsEmpty()) L->AddSlot().AutoHeight().Padding(0, P(I ? 12 : 2), 0, P(2))[SNew(STextBlock).Text(FText::FromString(Hd)).Font(Term(19, 2)).ColorAndOpacity(Ph2Red())];
			}
			const bool bDone = N.bDone, bNext = N.Group == TEXT("next");
			const FLinearColor Ink = bDone ? C(0x6d7895) : InkBlue();
			FTextBlockStyle St = FTextBlockStyle().SetFont(Hand(bNext ? Px + 3.f : Px)).SetColorAndOpacity(Ink);
			if (bDone) St.SetStrikeBrush(StrikeBrush(Ink)); // (MARGINALIA-20261109: a brush Slate draws the strike with)
			TSharedRef<SWidget> Mark = SNullWidget::NullWidget;
			if (bNext) Mark = SNew(SNextArrow);
			else if (N.Group == TEXT("also") || N.bSummary) Mark = SNew(STextBlock).Text(FText::FromString(N.Fold.IsEmpty() ? TEXT("-") : OpenFolds.Contains(N.Fold) ? TEXT("v") : TEXT(">"))).Font(Term(22)).ColorAndOpacity(Ink);
			else Mark = SNew(SBox).WidthOverride(P(16)).HeightOverride(P(16))[SNew(SBorder).BorderImage(Box(FLinearColor::Transparent, Ink, 1.5f)).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0)
				[SNew(STextBlock).Text(FText::FromString(bDone ? TEXT("x") : N.Group == TEXT("first") ? TEXT("!") : TEXT(" "))).Font(Term(16)).ColorAndOpacity(bDone ? Ink : N.Group == TEXT("first") ? Ph2Red() : Ink)]];
			const bool bFresh = N.bNew && !bDone;
			L->AddSlot().AutoHeight().Padding(0, 0, 0, P(6))[Entry(I, SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0, P(8), P(10), 0)[SNew(SBox).WidthOverride(P(26))[Mark]]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0, P(9), bFresh ? P(8) : 0.f, 0)[SNew(STextBlock).Visibility(bFresh ? EVisibility::Visible : EVisibility::Collapsed).Text(FText::FromString(TEXT("NEW"))).Font(Term(16, 2)).ColorAndOpacity(Ph2Red())]
				+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(FText::FromString(N.Text)).TextStyle(&St).AutoWrapText(true).LineHeightPercentage(LinePercent(Hand(Px), 32))])];
			Read.Add(bFresh ? TEXT("New. ") + N.Text : N.Text);
		}
		// D45-20261110: what has been shown is no longer new the next time the notebook opens
		if (X.bFreeform) for (const FS47NbNowLine& N : X.Now) if (N.bNew && !N.Key.IsEmpty()) S47TonightNote::Seen().Keys.Add(N.Key);
		LeftSlot->SetContent(Lay(Head(TEXT("Now"), Heading), NowRows.Num() ? StaticCastSharedRef<SWidget>(L) : Empty(TEXT("Nothing on tonight's list. The clock and OPS will bring the next thing.")), LeftScroll));
	}
	else if (Tab == TEXT("tasks") || Tab == TEXT("guide"))
	{
		const bool Work = Tab == TEXT("tasks");
		TSharedRef<SVerticalBox> L = SNew(SVerticalBox);
		for (int32 I = 0; I < Count(); ++I)
		{
			const FString Title = Work ? X.WorkItems[I].Title : X.GuideEntries[I].Title;
			const FString State = Work ? X.WorkItems[I].Status : (X.EntryRead && !X.EntryRead(X.GuideEntries[I].Id) ? TEXT("NEW / Reference") : TEXT("Reference / no case conclusions"));
			L->AddSlot().AutoHeight().Padding(0, 0, 0, P(12))[Entry(I, SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Inked(Title, bLarge ? 31.f : 27.f)]
				+ SVerticalBox::Slot().AutoHeight()[Typed(State, bLarge ? 17.f : 14.f)])];
		}
		LeftSlot->SetContent(Lay(Head(Work ? TEXT("Now") : TEXT("Field guide"), Work ? Heading : TEXT("PROCEDURES · CONTROLS IN SECTION ABOVE")), Count() ? StaticCastSharedRef<SWidget>(L) : Empty(TEXT("No open work recorded.")), LeftScroll));
	}
	else if (Tab == TEXT("notes"))
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
			RightSlot->SetContent(Lay(Head(TEXT("Observations"), NoteLine), Notes.Num() ? Lines(Notes, 0, Notes.Num(), false) : Empty(TEXT("Nothing worth writing down so far.")), RightScroll, 146.f, true)); // D45-20261110: no one-word sentence
			if (Tasks.Num()) Read = TaskText; else Read.Add(TEXT("Nothing yet. Dale left the shift log on the desk."));
		}
		else
		{
			// the observations across the spread: the first half on the left, the rest on the right
			const int32 Half = (Notes.Num() + 1) / 2;
			LeftSlot->SetContent(Lay(Head(TEXT("Observations"), NoteLine), Notes.Num() ? Lines(Notes, 0, Half, false) : Empty(TEXT("Nothing worth writing down so far.")), LeftScroll));
			if (Notes.Num() > Half) RightSlot->SetContent(Lay(SNullWidget::NullWidget, Lines(Notes, Half, Notes.Num(), false), RightScroll, 146.f, true));
			if (Notes.Num()) Read = Notes; else Read.Add(TEXT("Nothing worth writing down so far."));
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
			FString Label = !D.Stamp.IsEmpty() && D.Stamp.Len() <= 6 ? D.Stamp : Parts.Num() ? Parts[0] : D.Title;
			FString Under = (D.Kind == TEXT("fieldnote") ? FString(TEXT("Observation / retained source\n")) : FString(TEXT("Interpretation / retained comparison\n"))) + Excerpt(D);
			if (X.bFreeform)
			{
				// PT-NOTEBOOK-20261109: the card says where (the stamp's place), what in the record's own first words, and the rest of it under
				// them; no "Field record:", no stamp and no standing sentences
				const int32 Night = DocNight.FindRef(D.Id);
				if (I == 0 || DocNight.FindRef(Findings[I - 1].Id) != Night) L->AddSlot().AutoHeight()[NightHead(Night, I == 0)];
				Rest = ShownTitle(D, true);
				Label = D.Stamp.StartsWith(TEXT("IN THE FIELD")) ? FString(TEXT("FIELD")) : D.Stamp.Contains(TEXT("TELEX")) ? FString(TEXT("TELEX")) : FString(TEXT("SARO"));
				FString Body = ShownPage(D, true);
				FString Head = Rest; Head.RemoveFromEnd(TEXT("..."));
				if (!Head.IsEmpty() && Body.StartsWith(Head)) { Body.RightChopInline(Head.Len()); Body.TrimStartInline(); Body.RemoveFromStart(TEXT(".")); Body.TrimStartInline(); }
				FS47Doc Short; Short.Id = D.Id; Short.Transcript = Body;
				Under = KindWord(D) + (Body.IsEmpty() ? FString() : TEXT("\n") + Excerpt(Short));
			}
			if (X.EntryRead && !X.EntryRead(D.Id)) Rest = TEXT("NEW / ") + Rest;
			L->AddSlot().AutoHeight().Padding(0, 0, 0, P(10))
			[
				Entry(I, Shadowed(SNew(SBorder).BorderImage(Box(C(0xf6f1e3))).Padding(FMargin(P(12), P(8)))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)[SNew(SBox).WidthOverride(P(X.bFreeform ? 72 : 100))[SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(X.bFreeform ? 22 : 26)).ColorAndOpacity(C(0x2f6a3c)).AutoWrapText(true)]]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(P(10), 0, 0, 0)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Rest)).Font(Type(16)).ColorAndOpacity(Ink()).AutoWrapText(true)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, P(3), 0, 0)[SNew(STextBlock).Text(FText::FromString(Under)).Font(Type(13)).ColorAndOpacity(C(0x6d6656)).AutoWrapText(true)]
						]
					], 6.f, FVector2D(0, 2), 0.18f), true)
			];
		}
		const FString Line = Findings.Num() ? FString::Printf(TEXT("FINDINGS · %d"), Findings.Num()) : FString(TEXT("FINDINGS"));
		LeftSlot->SetContent(Lay(Head(TEXT("Knowledge"), Line), Findings.Num() ? StaticCastSharedRef<SWidget>(L) : Empty(TEXT("Nothing recorded yet. A finding goes here once the records support it.")), LeftScroll));
		if (!Findings.Num())
		{
			Read.Add(TEXT("Nothing recorded yet. A finding goes here once the records support it."));
			// Departs from the web version: with nothing recorded, the right page says how a finding
			// is made (Tom: the clues should be better explained to a new player).
			const FString Key = S47Keys::Label(TEXT("board")).ToUpper();
			const TArray<FString> How = X.bFreeform ? TArray<FString>{ TEXT("Observation: no field evidence has been retained yet."), TEXT("Interpretation: none recorded. An unexplained signal does not by itself identify a cause."), TEXT("Open question: what survives a repeat observation or an independent control?"), TEXT("Follow an assigned work order in Now. Collect the original, complete its requested comparison, then register the result at OPS.") } : TArray<FString>{
				FString::Printf(TEXT("Open the case board (%s). Every paper I pick up is pinned there."), *Key),
				TEXT("Pull a red thread from a pin to the record it agrees with. A wrong one falls slack."),
				TEXT("A question card fills its lines when the right threads hold."),
				TEXT("Every line filled: RECORD. The finding comes here."),
				TEXT("Stuck: the Hint on the card."),
			};
			RightSlot->SetContent(Lay(Head(X.bFreeform ? TEXT("What is known") : TEXT("How a finding is made"), X.bFreeform ? TEXT("OBSERVE · COMPARE · RETAIN") : TEXT("ON THE CASE BOARD")), Lines(How, 0, How.Num(), false), RightScroll, 146.f, true));
			Read.Append(How);
		}
	}
	else if (Tab == TEXT("papers"))
	{
		// the papers as index cards, two to a row; in Freeform a heading over each night's, newest first (PT-NOTEBOOK-20261109)
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(P(5), P(6)));
		TSharedRef<SVerticalBox> Nights = SNew(SVerticalBox);
		TSharedPtr<SUniformGridPanel> NightGrid;
		int32 InNight = 0;
		auto Card = [&](int32 I) -> TSharedRef<SWidget>
		{
			const FS47Doc& D = Papers[I];
			const bool bHand = D.Kind == TEXT("hand");
			return Entry(I, Shadowed(SNew(SBorder).BorderImage(Box(D.Kind == TEXT("printout") ? C(0xeef2e6) : C(0xf6f1e3))).Padding(FMargin(P(12), P(9)))
					[
						SNew(SBox).MinDesiredHeight(P(58)).VAlign(VAlign_Top)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString((X.EntryRead && !X.EntryRead(D.Id) ? TEXT("NEW / ") : TEXT("")) + ShownTitle(D, X.bFreeform))).Font(bHand ? Hand(25) : Type(15)).ColorAndOpacity(bHand ? InkBlue() : Ink()).AutoWrapText(true)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, P(4), 0, 0)
							[
								SNew(STextBlock).Text(FText::FromString(KindWord(D, X.bFreeform) + (D.Stamp.IsEmpty() ? FString() : TEXT(" / ") + D.Stamp))).Font(Term(17)).ColorAndOpacity(C(0x8a8270)).AutoWrapText(true)
							]
						]
					], 6.f, FVector2D(0, 2), 0.18f), true);
		};
		for (int32 I = 0; I < Papers.Num(); ++I)
		{
			if (!X.bFreeform) { Grid->AddSlot(I % 2, I / 2)[Card(I)]; continue; }
			const int32 Night = DocNight.FindRef(Papers[I].Id);
			if (I == 0 || DocNight.FindRef(Papers[I - 1].Id) != Night)
			{
				if (NightGrid.IsValid() && InNight == 1) NightGrid->AddSlot(1, 0)[SNew(SSpacer)];
				Nights->AddSlot().AutoHeight()[NightHead(Night, I == 0)];
				Nights->AddSlot().AutoHeight()[SAssignNew(NightGrid, SUniformGridPanel).SlotPadding(FMargin(P(5), P(6)))];
				InNight = 0;
			}
			NightGrid->AddSlot(InNight % 2, InNight / 2)[Card(I)];
			++InNight;
		}
		if (NightGrid.IsValid() && InNight == 1) NightGrid->AddSlot(1, 0)[SNew(SSpacer)];
		if (Papers.Num() == 1 && !X.bFreeform) Grid->AddSlot(1, 0)[SNew(SSpacer)];
		const FString Line = Papers.Num() ? FString::Printf(TEXT("PAPERS · %d"), Papers.Num()) : FString(TEXT("PAPERS"));
		LeftSlot->SetContent(Lay(Head(TEXT("Documents"), Line), Papers.Num() ? (X.bFreeform ? StaticCastSharedRef<SWidget>(Nights) : StaticCastSharedRef<SWidget>(Grid)) : Empty(PaperQuery.IsEmpty() ? TEXT("None collected.") : TEXT("No owned document matches. Clear the search and press Enter.")), LeftScroll));
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
			: NShots ? SNullWidget::NullWidget : Empty(FString::Printf(TEXT("No exposures yet. Once you have the field camera, %s raises it anywhere."), *S47Keys::Said(TEXT("camera")))), LeftScroll));
		if (!Photos.Num() && !NShots) Read.Add(FString::Printf(TEXT("No exposures yet. Once you have the field camera, %s raises it anywhere."), *S47Keys::Said(TEXT("camera"))));
		TSharedRef<SWrapBox> Prints = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(P(22), P(24)));
		for (int32 I = 0; I < NShots; ++I) Prints->AddSlot()[Entry(Photos.Num() + I, SNew(SBox).Padding(P(8))[Print(I, 214.f, false)])];
		bRightIsEntries = NShots > 0;
		if (NShots) RightSlot->SetContent(Lay((Album && Album->OnDiscard ? Head(TEXT("Your photographs"), FString::Printf(TEXT("%d KEPT"), NShots)) : Head(TEXT("Tonight"), FString::Printf(TEXT("FREE FRAMES · %d"), NShots))), Prints, RightScroll, 146.f, true));
	}
	else if (Tab == TEXT("map") && X.Map && X.Map->bField)
	{
		// Freeform (MAP-20261108): the survey sheet, to scale, with zoom and the player exactly where they are. Laid flat, so a
		// mouse on it means what it points at; taped down, how to work it pencilled under it.
		TSharedRef<SS47FieldMap> Sheet = SNew(SS47FieldMap).View(*X.Map).Width(P(960)).OnNote(X.OnMapNote);
		FieldMap = Sheet;
		TSharedRef<SCanvas> Spread = SNew(SCanvas)
			+ SCanvas::Slot().Position(P2(148, 16)).Size(P2(960, 640))[Shadowed(Sheet, 16.f, FVector2D(0, 8), 0.4f)]
			+ SCanvas::Slot().Position(P2(118, 6)).Size(P2(74, 26))[Tape(70, 20, -32)]
			+ SCanvas::Slot().Position(P2(1066, 6)).Size(P2(74, 26))[Tape(70, 20, 34)]
			+ SCanvas::Slot().Position(P2(118, 632)).Size(P2(74, 26))[Tape(70, 20, 30)]
			+ SCanvas::Slot().Position(P2(1066, 632)).Size(P2(74, 26))[Tape(70, 20, -28)];
		Spread->AddSlot().Position(P2(170, 668)).Size(P2(920, 44))[SNew(STextBlock).Font(Hand(26)).ColorAndOpacity(InkBlue()).Text_Lambda([]
		{
			return FText::FromString(S47Gamepad::UsingGamepad()
				? TEXT("LT / RT zoom   left stick or D-pad move   A back to me   R3 mark it")
				: TEXT("wheel or + / - zoom   drag or arrows move   Home back to me   X or right click: mark it"));
		})];
		// The index card tucked under the sheet's corner: what the pencil and the ink on it mean (Tom, 8 November: "my marks").
		{
			TSharedRef<SVerticalBox> Card = SNew(SVerticalBox);
			const FLinearColor Lead = C(0x3a3a40);
			// A ruled index card: a red rule under its heading, pale blue ones under each line.
			const auto CardLine = [&Card](const TCHAR* Words, const FLinearColor& Col, float Size, const FLinearColor& Rule)
			{
				Card->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Words)).Font(Hand(Size)).ColorAndOpacity(Col)];
				Card->AddSlot().AutoHeight().Padding(FMargin(0, 0, 0, P(3)))[SNew(SBox).HeightOverride(P(1.5f))[SNew(SBorder).BorderImage(Box(Rule))]];
			};
			const FLinearColor BlueRule = C(0x9fb6d3), RedRule = C(0xc0504d);
			CardLine(TEXT("my marks"), InkBlue(), 28, RedRule);
			CardLine(TEXT("x 1    marked it myself"), Lead, 21, BlueRule);
			CardLine(TEXT("(?)    heard of, not found"), Lead, 21, BlueRule);
			CardLine(TEXT("struck out: looked into"), Lead, 21, BlueRule);
			CardLine(TEXT("- - -  bearing from SARO"), Lead, 21, BlueRule);
			CardLine(TEXT("blue ink: work to do"), InkBlue(), 21, BlueRule);
			Spread->AddSlot().Position(P2(-74, 428)).Size(P2(256, 228))[Turned(Shadowed(SNew(SBorder).BorderImage(Box(C(0xf7f3e6))).Padding(FMargin(P(12), P(8)))[Card], 8.f, FVector2D(0, 3), 0.3f), -3.f)];
		}
		SpreadSlot->SetContent(Spread);
		Read = X.MapDescription;
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
		LeftSlot->SetContent(Lay(Head(TEXT("Equipment and supplies"), FString::Printf(TEXT("KIT · %d %s"), X.Kit->Num(), X.Kit->Num() == 1 ? TEXT("THING") : TEXT("THINGS"))), Grid, LeftScroll));
	}
	else if (Tab == TEXT("people") && X.People && X.bFreeform)
	{
		// PT-NOTEBOOK-20261109: WHO & WHAT, the minimal one: a pencilled box for each of Caliche's regulars the player has met or missed,
		// who they are and one plain line from what they have told (FS47Townsfolk::Cards). MARGINALIA-20261109: on squared paper, with
		// the player's own scribbles under each (X.Scribbles' PEOPLE), and a box for each one met only there (Dale, Earl, Baca, Ward).
		const auto Follow = [this](const FS47NbScribble& S) -> TFunction<void()>
		{
			if (S.Follow == TEXT("map")) return [this] { Show(TEXT("map")); };
			if (S.Follow == TEXT("board") && OnBoard) return [this] { TFunction<void()> Fn = OnBoard; Close(); if (Fn) Fn(); };
			return nullptr;
		};
		TSharedRef<SVerticalBox> L = SNew(SVerticalBox);
		TSet<FString> Boxed;
		const auto Pencil = [&](const TSharedRef<SVerticalBox>& Bx, const FString& Name)
		{
			Boxed.Add(Name);
			for (const FS47NbScribble& S : X.Scribbles)
				if (S.Side == TEXT("people") && S.Box == Name) { Bx->AddSlot().AutoHeight().Padding(0, P(4), 0, 0)[ScribbleLine(S, 24.f, Follow(S))]; Read.Add(S.Text); }
		};
		const auto Framed = [&](const TSharedRef<SVerticalBox>& Bx) { return SNew(SBorder).BorderImage(Box(C(0xf7f3e6, 0.55f), C(0x3a3a40, 0.55f), 1.5f, 2.f)).Padding(FMargin(P(14), P(8)))[Bx]; };
		const auto Named = [&](const FString& Name, const FString& Role) -> TSharedRef<SVerticalBox>
		{
			return SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Name)).Font(Hand(32)).ColorAndOpacity(InkBlue())]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Role.ToUpper())).Font(Term(17, 1)).ColorAndOpacity(Ph2Red()).AutoWrapText(true)];
		};
		for (int32 I = 0; I < X.People->Num(); ++I)
		{
			const FS47PersonCard& Pc = (*X.People)[I];
			TSharedRef<SVerticalBox> Bx = Named(Pc.Name, Pc.Role);
			if (!Pc.Note.IsEmpty()) Bx->AddSlot().AutoHeight().Padding(0, P(4), 0, 0)[Inked(Pc.Note, 26.f, C(0x3a3a40))];
			Read.Add(Pc.Name + TEXT(". ") + Pc.Role + TEXT(". ") + Pc.Note);
			Pencil(Bx, Pc.Name);
			L->AddSlot().AutoHeight().Padding(0, 0, 0, P(12))[Entry(I, Framed(Bx))];
		}
		int32 Extra = 0;
		for (const FS47NbScribble& S : X.Scribbles)
		{
			if (S.Side != TEXT("people") || Boxed.Contains(S.Box)) continue;
			TSharedRef<SVerticalBox> Bx = Named(S.Box, S.Role);
			Read.Add(S.Box + TEXT(". ") + S.Role + TEXT("."));
			Pencil(Bx, S.Box);
			L->AddSlot().AutoHeight().Padding(0, 0, 0, P(12))[Framed(Bx)];
			++Extra;
		}
		const FString None = TEXT("Nobody in town has said much to me yet. The gas station and the Mesa are open late.");
		const int32 N = X.People->Num() + Extra;
		LeftSlot->SetContent(SNew(SOverlay) + SOverlay::Slot()[SNew(SGraphPaper)]
			+ SOverlay::Slot()[Lay(Head(TEXT("Who & what"), FString::Printf(TEXT("PEOPLE · %d"), N)), N ? StaticCastSharedRef<SWidget>(L) : Empty(None), LeftScroll)]);
		if (!N) Read.Add(None);
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
		// Unreal desktop: the active controls first, with the other supported input on the facing page.
		auto Page = [](const FCtlPage& Pg, TSharedPtr<SScrollBox>& Scroll)
		{
			return SNew(SBox).Padding(FMargin(P(84), P(40), P(30), P(30)))[SAssignNew(Scroll, SScrollBox).Style(S47Game::QuietScroll()).ScrollBarStyle(S47Game::QuietBar(C(0x233a7a, 0.22f))).ScrollBarThickness(FVector2D(P(3), P(3))) + SScrollBox::Slot()[ControlsBlock(Pg)]];
		};
		const bool bPad = S47Gamepad::UsingGamepad();
		LeftSlot->SetContent(Page(Controls(X.bTouch, bPad), LeftScroll));
		RightSlot->SetContent(Page(Controls(false, !bPad), RightScroll));
		KeysShown = S47Keys::Changes();
		PadShown = S47Gamepad::Changes();
	}
	if (!Read.IsEmpty() && UI.IsValid()) Reading = UI->ReadingAction(Read, TEXT("notebook-description"));
	BuildRight();
}

void SS47Notebook::BuildRight()
{
	using namespace S47Game;
	const bool bLarge = UI.IsValid() && UI->bLargeText;
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
	if (IsNow() && NowRows.IsValidIndex(I))
	{
		// PT-NOTEBOOK-20261109: the chosen line: WHAT, WHERE (and the map), WHY in the list's own words, the work item's details and hints,
		// and what the clock allows in red pencil
		const FNowRow& R = NowRows[I];
		const FS47NbNowLine* N = X.Now.IsValidIndex(R.Line) ? &X.Now[R.Line] : nullptr;
		const FS47NbWorkItem* W = ChosenWork();
		const FString Title = N ? N->What : W ? W->Title : FString();
		TSharedRef<SVerticalBox> Pg = SNew(SVerticalBox);
		TArray<FString> Said = { Title };
		auto Row = [&](const TCHAR* Word, const FString& Text, TFunction<void()> Link = nullptr, const TCHAR* LinkWord = nullptr)
		{
			if (Text.TrimStartAndEnd().IsEmpty()) return;
			TSharedRef<SHorizontalBox> H = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, P(2), P(10), 0)[SNew(SBox).WidthOverride(P(74))[SNew(STextBlock).Text(FText::FromString(Word)).Font(Term(20, 2)).ColorAndOpacity(StampRed())]]
				+ SHorizontalBox::Slot().FillWidth(1.f)[Typed(Text, bLarge ? 21.f : 18.f)];
			if (Link) H->AddSlot().AutoWidth().Padding(P(8), P(2), 0, 0)[Click(SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("[%s]"), LinkWord))).Font(Term(20, 2)).ColorAndOpacity(InkBlue()), Link)];
			Pg->AddSlot().AutoHeight().Padding(0, P(10), 0, 0)[H];
			Said.Add(FString(Word) + TEXT(". ") + Text);
		};
		Pg->AddSlot().AutoHeight()[Inked(Title, bLarge ? 34.f : 30.f)];
		if (N && !N->When.IsEmpty()) Row(TEXT("WHEN"), N->When);
		FString Where = N ? N->Where : FString();
		if (W && !W->Location.IsEmpty()) Where = Where.IsEmpty() ? W->Location : Where + TEXT(" / ") + W->Location;
		Row(TEXT("WHERE"), Where, X.Map ? TFunction<void()>([this] { Show(TEXT("map")); }) : TFunction<void()>(), TEXT("MAP"));
		FString Why = N ? N->Why : FString();
		if (W) Why = (Why.IsEmpty() || Why == Title ? FString() : Why + TEXT("\n\n")) + W->Status + TEXT("\nNEXT / ") + W->NextAction;
		if (N && !N->Fold.IsEmpty())
		{
			// CRIT9: the key as the player has it (the pad's A with a gamepad; the notebook's Enter on the keyboard), never a fixed one.
			const FString Opens = S47Gamepad::UsingGamepad() && !S47Gamepad::Label(TEXT("use")).IsEmpty() ? S47Gamepad::Label(TEXT("use")) : FString(TEXT("Enter"));
			Why = N->Why + TEXT("\n\n") + (OpenFolds.Contains(N->Fold) ? FString::Printf(TEXT("Open below. %s folds them away again."), *Opens) : (Opens + (N->Fold == TEXT("cards") ? TEXT(" opens the job cards.") : TEXT(" opens the cases."))));
		}
		Row(TEXT("WHY"), Why);
		if (W)
		{
			const int32 Level = X.HintLevel ? X.HintLevel(W->Id) : 0;
			for (int32 Hn = 0; Hn < FMath::Min(Level, W->Hints.Num()); ++Hn) Row(TEXT("HINT"), W->Hints[Hn]);
			if (!W->bDone && !W->Hints.IsEmpty() && X.RevealHint && Level < W->Hints.Num()) Acts.Add(MakeTuple(FString(TEXT("F1 / RT")), FString(TEXT("REVEAL HINT")), TFunction<void()>([this] { RevealHint(); })));
			if (!W->bDone && W->bCanFollow && X.OnFollow && !W->Id.IsEmpty()) Acts.Add(MakeTuple(FString(TEXT("ENTER")), FString(TEXT("FOLLOW")), TFunction<void()>([this] { Activate(); })));
			if (OnBoard && (W->Id.StartsWith(TEXT("case/")) || W->Id.StartsWith(TEXT("field-plan/")))) Acts.Add(MakeTuple(S47Keys::Label(TEXT("board")), FString(TEXT("BOARD")), TFunction<void()>([this] { TFunction<void()> Fn = OnBoard; Close(); if (Fn) Fn(); })));
		}
		if (N && !N->Fold.IsEmpty()) Acts.Add(MakeTuple(FString(TEXT("ENTER")), FString(OpenFolds.Contains(N->Fold) ? TEXT("FOLD AWAY") : TEXT("OPEN")), TFunction<void()>([this] { Activate(); })));
		if (!X.NowClock.IsEmpty())
		{
			Pg->AddSlot().AutoHeight().Padding(0, P(26), 0, 0)[Turned(Inked(X.NowClock, bLarge ? 30.f : 27.f, StampRed()), -1.2f)];
			Said.Add(X.NowClock);
		}
		RightSlot->SetContent(SNew(SBox).Padding(FMargin(P(60), P(48), P(30), P(95)))[SAssignNew(RightScroll, SScrollBox) + SScrollBox::Slot()[Pg]]);
		Readings(Said);
	}
	else if ((Tab == TEXT("tasks") && X.WorkItems.IsValidIndex(I)) || (Tab == TEXT("guide") && X.GuideEntries.IsValidIndex(I)))
	{
		const bool Work = Tab == TEXT("tasks");
		const FString Id = Work ? X.WorkItems[I].Id : X.GuideEntries[I].Id;
		const FString Title = Work ? X.WorkItems[I].Title : X.GuideEntries[I].Title;
		FString Body = Work ? X.WorkItems[I].Status + TEXT("\n\nNEXT / ") + X.WorkItems[I].NextAction : X.GuideEntries[I].Text;
		if (Work)
		{
			const FS47NbWorkItem& W = X.WorkItems[I];
			if (!W.Location.IsEmpty()) Body += TEXT("\n\nWHERE / ") + W.Location;
			const int32 Level = X.HintLevel ? X.HintLevel(Id) : 0;
			for (int32 H = 0; H < FMath::Min(Level, W.Hints.Num()); ++H) Body += TEXT("\n\nHINT / ") + W.Hints[H];
			if (!W.bDone && !W.Hints.IsEmpty() && X.RevealHint && Level < W.Hints.Num()) Acts.Add(MakeTuple(FString(TEXT("F1 / RT")), FString(TEXT("REVEAL HINT")), TFunction<void()>([this] { RevealHint(); })));
			if (!W.bDone && W.bCanFollow && X.OnFollow && !Id.IsEmpty()) Acts.Add(MakeTuple(FString(TEXT("ENTER")), FString(TEXT("FOLLOW")), TFunction<void()>([this] { Activate(); })));
		}
		else if (X.MarkRead) Acts.Add(MakeTuple(FString(TEXT("ENTER")), FString(X.EntryRead && X.EntryRead(Id) ? TEXT("READ") : TEXT("MARK READ")), TFunction<void()>([this] { Activate(); })));
		RightSlot->SetContent(SNew(SBox).Padding(FMargin(P(60), P(48), P(30), P(95)))
			[SAssignNew(RightScroll, SScrollBox) + SScrollBox::Slot()[SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Inked(Title, bLarge ? 34.f : 30.f)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, P(22), 0, 0)[Typed(Body, bLarge ? 23.f : 19.f)]]]);
		Readings({ Title, Body });
	}
	else if (Tab == TEXT("kit") && X.Kit && X.Kit->IsValidIndex(I))
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
			// the keys' leather tag is blank in the drawing; the page writes SERVICE on it
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
	else if (Tab == TEXT("people") && X.bFreeform)
	{
		// MARGINALIA-20261109: THEM, on the facing page of squared paper: a box for each thing out there the player has a name for (only
		// their own names, never what it is), the scribbles under it as they grew, the wrong guesses struck through.
		const auto Follow = [this](const FS47NbScribble& S) -> TFunction<void()>
		{
			if (S.Follow == TEXT("map")) return [this] { Show(TEXT("map")); };
			if (S.Follow == TEXT("board") && OnBoard) return [this] { TFunction<void()> Fn = OnBoard; Close(); if (Fn) Fn(); };
			return nullptr;
		};
		TArray<FString> Boxes, Said;
		for (const FS47NbScribble& S : X.Scribbles) if (S.Side == TEXT("them")) Boxes.AddUnique(S.Box);
		TSharedRef<SVerticalBox> L = SNew(SVerticalBox);
		for (const FString& Name : Boxes)
		{
			TSharedRef<SVerticalBox> Bx = SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Name)).Font(Hand(30)).ColorAndOpacity(InkBlue()).AutoWrapText(true)];
			Said.Add(Name);
			for (const FS47NbScribble& S : X.Scribbles)
				if (S.Side == TEXT("them") && S.Box == Name) { Bx->AddSlot().AutoHeight().Padding(0, P(4), 0, 0)[ScribbleLine(S, 24.f, Follow(S))]; Said.Add(S.Text); }
			L->AddSlot().AutoHeight().Padding(0, 0, 0, P(12))[SNew(SBorder).BorderImage(Box(FLinearColor::Transparent, C(0x3a3a40, 0.45f), 1.2f, 2.f)).Padding(FMargin(P(12), P(6)))[Bx]];
		}
		const FString None = TEXT("Nothing out there I could put a name to so far."); // D45-20261110: no one-word sentence
		RightSlot->SetContent(SNew(SOverlay) + SOverlay::Slot()[SNew(SGraphPaper)]
			+ SOverlay::Slot()
			[
				SNew(SCanvas)
				+ SCanvas::Slot().Position(P2(60, 40)).Size(P2(506, 100))[Head(TEXT("Them"), FString::Printf(TEXT("THEM · %d"), Boxes.Num()))]
				+ SCanvas::Slot().Position(P2(46, 146)).Size(P2(536, 734 - 146 - 92))
				[
					SAssignNew(RightScroll, SScrollBox).Style(S47Game::QuietScroll()).ScrollBarStyle(S47Game::QuietBar(C(0x233a7a, 0.22f))).ScrollBarThickness(FVector2D(P(3), P(3)))
					+ SScrollBox::Slot().Padding(FMargin(P(14), 0, P(16), P(10)))[Boxes.Num() ? StaticCastSharedRef<SWidget>(L) : Empty(None)]
				]
			]);
		Readings(Boxes.Num() ? Said : TArray<FString>{ None });
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
		const FString Kind = KindWord(D, X.bFreeform).ToUpper();
		const FString Line = (bFinding ? (D.Kind == TEXT("fieldnote") ? FString(TEXT("OBSERVATION · ")) : FString(TEXT("INTERPRETATION · "))) : FString()) + Kind + (D.Stamp.IsEmpty() ? FString() : TEXT(" · ") + D.Stamp);
		UTexture2D* Image = Picture(D.Image);
		TSharedRef<SVerticalBox> Lines = SNew(SVerticalBox);
		TArray<FString> PageLines;
		ShownPage(D, X.bFreeform).ParseIntoArray(PageLines, TEXT("\n"), false);
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
		if (bFinding && !X.bFreeform) { Lines->AddSlot().AutoHeight().Padding(0, P(12), 0, 0)[Typed((D.Id.StartsWith(TEXT("freeform-")) ? FString() : TEXT("SOURCE / ") + D.Id + TEXT("\n")) + (D.Kind == TEXT("fieldnote") ? TEXT("OPEN QUESTION / What does an independent record support or contradict?") : TEXT("INTERPRETATION / This is the retained comparison, not a new notebook conclusion.")), 13)]; }
		if (Image && D.Kind == TEXT("map")) { Lines->ClearChildren(); Lines->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(P(300))[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Picture(Image))]]]; }
		const TSharedRef<SWidget> Sheet = SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SS47GameSheet).Color(bPrint ? C(0xeef0e4) : C(0xf3ecdb)).bTorn(bHand)]
			+ SOverlay::Slot().Padding(FMargin(P(26), P(24), P(22), P(14)))[SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[Lines]];
		RightSlot->SetContent(SNew(SCanvas)
			+ SCanvas::Slot().Position(P2(60, 50)).Size(P2(520, 140))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Line)).Font(Term(20, 2)).ColorAndOpacity(StampRed())]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, P(6), 0, 0)[SNew(STextBlock).Text(FText::FromString(ShownTitle(D, X.bFreeform))).Font(bHand ? Hand(40) : Type(26)).ColorAndOpacity(bHand ? InkBlue() : Ink()).AutoWrapText(true)]
			]
			+ SCanvas::Slot().Position(P2(70, 196)).Size(P2(470, 440))[Turned(Shadowed(Sheet, 18.f, FVector2D(0, 10), 0.35f), bFinding ? 0.8f : -1.f)]);
		Acts.Add({ TEXT("ENTER"), X.EntryRead && X.EntryRead(D.Id) ? TEXT("OPEN ORIGINAL") : TEXT("READ ORIGINAL"), [this] { Activate(); } });
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
	if (IsNow())
	{
		// NOW: a fold opens or closes; a line with a work item follows it, as the work item does
		if (!NowRows.IsValidIndex(I)) return;
		const FNowRow R = NowRows[I];
		if (X.Now.IsValidIndex(R.Line) && !X.Now[R.Line].Fold.IsEmpty())
		{
			const FString Fold = X.Now[R.Line].Fold;
			if (OpenFolds.Contains(Fold)) OpenFolds.Remove(Fold); else OpenFolds.Add(Fold);
			FilterNow(); Sel.FindOrAdd(TEXT("tasks")) = I; Build(); return;
		}
		const FS47NbWorkItem* W = ChosenWork();
		if (W && X.OnFollow && W->bCanFollow && !W->bDone) { auto Fn = X.OnFollow; const FString Id = W->Id; Close(); Fn(Id); }
		return;
	}
	if (Tab == TEXT("tasks") && X.WorkItems.IsValidIndex(I)) { if (X.OnFollow && X.WorkItems[I].bCanFollow && !X.WorkItems[I].bDone) { auto Fn = X.OnFollow; const FString Id = X.WorkItems[I].Id; Close(); Fn(Id); } return; }
	if (Tab == TEXT("guide") && X.GuideEntries.IsValidIndex(I))
	{
		if (X.MarkRead) X.MarkRead(X.GuideEntries[I].Id);
		// D45-20261110: the field guide's first entry is tonight's list; Enter turns to it on NOW
		if (X.GuideEntries[I].Id == TEXT("guide:tonight") && X.bNow) { Show(TEXT("tasks")); return; }
		Build(); return;
	}

	if ((Tab == TEXT("findings") && Findings.IsValidIndex(I)) || (Tab == TEXT("papers") && Papers.IsValidIndex(I)))
	{
		// (as a click on it did: the paper opens over the game, game.openDoc)
		const FS47Doc D = Tab == TEXT("findings") ? Findings[I] : Papers[I];
		if (X.MarkRead) X.MarkRead(D.Id);
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

FString SS47Notebook::MainTab() const
{
	if (Tab == TEXT("notes") || Tab == TEXT("people")) return TEXT("findings");
	if (Tab == TEXT("controls")) return TEXT("guide");
	return Tab;
}

void SS47Notebook::FilterDocuments(const FString& Query)
{
	PaperQuery = Query.TrimStartAndEnd(); Papers.Reset();
	TArray<FString> Terms; PaperQuery.ParseIntoArrayWS(Terms);
	for (const FS47Doc& D : AllPapers)
	{
		const FString Hay = D.Id + TEXT(" ") + D.Title + TEXT(" ") + D.Kind + TEXT(" ") + D.Stamp + TEXT(" ") + D.Page;
		bool Match = true; for (const FString& Term : Terms) if (!Hay.Contains(Term, ESearchCase::IgnoreCase)) { Match = false; break; }
		if (Match) Papers.Add(D);
	}
	Sel.FindOrAdd(TEXT("papers")) = 0;
}

void SS47Notebook::FilterWork()
{
	X.WorkItems.Reset();
	for (const FS47NbWorkItem& W : AllWorkItems) if (bShowHistory || W.bFollowed || (!W.bDone && !W.bEarlier)) X.WorkItems.Add(W);
	Sel.FindOrAdd(TEXT("tasks")) = 0;
	if (X.bNow) FilterNow();
}

void SS47Notebook::FilterNow()
{
	// PT-NOTEBOOK-20261109: every line of the note, and under an open fold its work items (with history: the filed cards too)
	NowRows.Reset();
	for (int32 L = 0; L < X.Now.Num(); ++L)
	{
		NowRows.Add({ L, INDEX_NONE });
		const FString& Fold = X.Now[L].Fold;
		if (Fold.IsEmpty() || !OpenFolds.Contains(Fold)) continue;
		for (int32 W = 0; W < AllWorkItems.Num(); ++W)
		{
			const FS47NbWorkItem& It = AllWorkItems[W];
			const bool bIn = Fold == TEXT("cards") ? It.bCard : It.Id.StartsWith(TEXT("case/")) && !X.Now.ContainsByPredicate([&It](const FS47NbNowLine& N) { return N.Item == It.Id; });
			if (bIn && (bShowHistory || It.bFollowed || (!It.bDone && !It.bEarlier))) NowRows.Add({ INDEX_NONE, W });
		}
	}
}

const FS47NbWorkItem* SS47Notebook::ChosenWork() const
{
	const int32 I = Chosen();
	if (!NowRows.IsValidIndex(I)) return nullptr;
	const FNowRow& R = NowRows[I];
	if (AllWorkItems.IsValidIndex(R.Item)) return &AllWorkItems[R.Item];
	if (!X.Now.IsValidIndex(R.Line) || X.Now[R.Line].Item.IsEmpty()) return nullptr;
	const FString& Id = X.Now[R.Line].Item;
	return AllWorkItems.FindByPredicate([&Id](const FS47NbWorkItem& W) { return W.Id == Id; });
}

void SS47Notebook::Section(int32 Dir)
{
	if (MainTab() == TEXT("tasks")) { bShowHistory = !bShowHistory; FilterWork(); Build(); return; }
	TArray<FString> Tabs;
	if (MainTab() == TEXT("findings")) Tabs = !X.bFreeform ? TArray<FString>{ TEXT("findings"), TEXT("notes"), TEXT("people") } : Notes.Num() ? TArray<FString>{ TEXT("findings"), TEXT("people"), TEXT("notes") } : TArray<FString>{ TEXT("findings"), TEXT("people") };
	if (MainTab() == TEXT("papers") && !X.bFreeform) Tabs = { TEXT("papers"), TEXT("photos") };
	if (MainTab() == TEXT("guide")) Tabs = { TEXT("guide"), TEXT("controls") };
	if (!Tabs.IsEmpty()) Show(Tabs[(FMath::Max(0, Tabs.IndexOfByKey(Tab)) + Dir + Tabs.Num()) % Tabs.Num()]);
}

void SS47Notebook::RevealHint()
{
	if (Tab != TEXT("tasks") || !X.RevealHint) return;
	const FS47NbWorkItem* Work = IsNow() ? ChosenWork() : X.WorkItems.IsValidIndex(Chosen()) ? &X.WorkItems[Chosen()] : nullptr;
	if (!Work) return;
	const FS47NbWorkItem& W = *Work;
	if (!W.bDone && !W.Hints.IsEmpty()) { X.RevealHint(W.Id); BuildRight(); }
}

void SS47Notebook::Turn(int32 Dir)
{
	const int32 At = FMath::Max(0, Order.IndexOfByKey(MainTab()));
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
	if (Tab == TEXT("controls") && (KeysShown != S47Keys::Changes() || PadShown != S47Gamepad::Changes())) Build();
	if (Tab == TEXT("map") && FieldMap.IsValid()) FieldMap->Step(Dt);
}

FReply SS47Notebook::OnAnalogValueChanged(const FGeometry& G, const FAnalogInputEvent& E)
{
	// The survey sheet moves under the left stick.
	if (Tab == TEXT("map") && FieldMap.IsValid() && Big == INDEX_NONE && (E.GetKey() == EKeys::Gamepad_LeftX || E.GetKey() == EKeys::Gamepad_LeftY))
	{
		FieldMap->Stick(E.GetKey(), E.GetAnalogValue());
		return FReply::Handled();
	}
	return SCompoundWidget::OnAnalogValueChanged(G, E);
}

FReply SS47Notebook::OnMouseWheel(const FGeometry& G, const FPointerEvent& E)
{
	// The wheel off the sheet, and LT and RT (the pad turns them into the wheel), zoom it about its middle.
	if (Tab == TEXT("map") && FieldMap.IsValid() && Big == INDEX_NONE) { FieldMap->Wheel(E.GetWheelDelta()); return FReply::Handled(); }
	return SCompoundWidget::OnMouseWheel(G, E);
}

FReply SS47Notebook::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	// main.ts input.onKey with the notebook open: Tab, N, M, I, H, Escape and P close it; B lays
	// the case out on the board (the board's own key processor takes B first)
	const FKey K = E.GetKey();
	// one print open big: these only page through the album and put the print back (ui/album.ts)
	if (Big != INDEX_NONE)
	{
		if (K == EKeys::X || K == EKeys::Delete || K == EKeys::Gamepad_FaceButton_Left) { DiscardBig(); return FReply::Handled(); }
		if (K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_LeftShoulder) { ShowBig(Big - 1); return FReply::Handled(); }
		if (K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right || K == EKeys::Gamepad_RightShoulder) { ShowBig(Big + 1); return FReply::Handled(); }
		if (K == EKeys::Escape || K == EKeys::E || K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom || K == EKeys::Gamepad_FaceButton_Right) { CloseBig(); return FReply::Handled(); }
	}
	// Gameplay pad bindings share these keys (LB notebook, RB board, D-pad map/light).
	// This modal owns its raw pad layout; rebound keyboard shortcuts still work as before.
	const bool bKeyboard = !K.IsGamepadKey();
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Gamepad_Special_Left || K == EKeys::Gamepad_Special_Right || (bKeyboard && (S47Keys::Is(TEXT("notebook"), K) || S47Keys::Is(TEXT("map"), K) || S47Keys::Is(TEXT("kit"), K) || S47Keys::Is(TEXT("controls"), K) || S47Keys::Is(TEXT("pause"), K)))) { Close(); return FReply::Handled(); }
	if (((bKeyboard && S47Keys::Is(TEXT("board"), K)) || K == EKeys::Gamepad_FaceButton_Top) && OnBoard) { TFunction<void()> Fn = OnBoard; Close(); Fn(); return FReply::Handled(); }
	if (K == EKeys::F1 || K == EKeys::Gamepad_RightTrigger) { RevealHint(); return FReply::Handled(); }
	if (K == EKeys::Comma || K == EKeys::Gamepad_LeftTrigger) { Section(1); return FReply::Handled(); }
	// Q and E turn the tabs; the arrows (or W A S D) choose on a page of entries, else scroll it
	if (K == EKeys::Q || K == EKeys::Gamepad_LeftShoulder) { Turn(-1); return FReply::Handled(); }
	if (K == EKeys::E || K == EKeys::Gamepad_RightShoulder) { Turn(1); return FReply::Handled(); }
	// The survey sheet's own: move, zoom, back to me, a mark (SS47FieldMap::Key).
	if (Tab == TEXT("map") && FieldMap.IsValid() && FieldMap->Key(K)) return FReply::Handled();
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

FString SS47Notebook::DiscardLabel() const
{
	if (!Album || !Album->Shots.IsValidIndex(Big)) return FString();
	if (Album->CanDiscard && !Album->CanDiscard(Album->Shots[Big].Id)) return TEXT("KEPT: USED IN YOUR WORK");
	return bDiscardArmed ? TEXT("X  AGAIN TO THROW IT AWAY") : TEXT("X  THROW AWAY PHOTO");
}

void SS47Notebook::DiscardBig()
{
	if (!Album || !Album->OnDiscard || !Album->Shots.IsValidIndex(Big)) return;
	const FString Id = Album->Shots[Big].Id;
	if (Album->CanDiscard && !Album->CanDiscard(Id)) return;
	if (!bDiscardArmed) { bDiscardArmed = true; return; }
	bDiscardArmed = false;
	TFunction<void(const FString&)> Fn = Album->OnDiscard;
	Fn(Id); // Rebuilds the notebook: this widget is gone afterwards.
}

void SS47Notebook::ShowBig(int32 I)
{
	using namespace S47Game;
	if (!Album || !Album->Shots.IsValidIndex(I) || !BigSlot.IsValid()) return;
	// (reading.change(): another print, or none, stops what was being read)
	if (UI.IsValid()) UI->StopSpeech();
	Big = I; bDiscardArmed = false;
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
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0, 0, P(12), P(14))
			[
				SNew(SButton).ButtonStyle(Flat()).ContentPadding(FMargin(P(10), P(6))).IsFocusable(false)
				.Visibility_Lambda([this] { return Album && Album->OnDiscard ? EVisibility::Visible : EVisibility::Collapsed; })
				.OnClicked_Lambda([this] { DiscardBig(); return FReply::Handled(); })
				[
					SNew(STextBlock).Font(Term(22)).ColorAndOpacity_Lambda([this] { return bDiscardArmed ? C(0xff8a5c) : C(0xcfc8b5); })
					.Text_Lambda([this] { return FText::FromString(DiscardLabel()); })
				]
			]
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
		{ TEXT("Truck keys"), TEXT("Service truck."), TEXT("On the pad"), TEXT("keys"), TEXT("SERVICE") }, { TEXT("Papers"), TEXT("Folded."), TEXT("2"), TEXT("papers"), FString() } };
	Ex.People = TArray<FS47PersonCard>{ { TEXT("Dale"), TEXT("SARO"), TEXT("Left."), FString() } };
	TArray<FS47Task> T; T.Add({ TEXT("Read the log"), true }); T.Add({ TEXT("Answer the line"), false });
	FString Opened;
	int32 Boards = 0;
	const TSharedRef<SS47Notebook> Nb = SNew(SS47Notebook).Tasks(T).Notes({ TEXT("One."), TEXT("Two.") }).Docs({ Log, Typed, Find })
		.OnDoc([&Opened](const FS47Doc& D) { Opened = D.Id; }).OnBoard([&Boards] { ++Boards; }).Extras(Ex);
	auto Key = [&](const FKey& K) { Nb->OnKeyDown(FGeometry(), FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0)); };
	Check(TEXT("tabs in order"), FString::Join(Nb->Order, TEXT(" ")) == TEXT("tasks findings papers photos kit map guide"), FString::Join(Nb->Order, TEXT(" ")));
	Check(TEXT("opens on tasks"), Nb->Tab == TEXT("tasks"), Nb->Tab);
	Key(EKeys::E);
	Check(TEXT("E turns on"), Nb->Tab == TEXT("findings"), Nb->Tab);
	Key(EKeys::Q); Key(EKeys::Q);
	Check(TEXT("Q turns back, round to the last"), Nb->Tab == TEXT("guide"), Nb->Tab);
	Nb->Show(TEXT("kit"));
	Key(EKeys::Right);
	Check(TEXT("right chooses the next"), Nb->Chosen() == 1, FString::FromInt(Nb->Chosen()));
	Key(EKeys::Down);
	Check(TEXT("down goes a row, to the last"), Nb->Chosen() == 3, FString::FromInt(Nb->Chosen()));
	Key(EKeys::Enter);
	Check(TEXT("Enter on the papers turns to them"), Nb->Tab == TEXT("papers"), Nb->Tab);
	Check(TEXT("original documents include knowledge sources"), Nb->Papers.Num() == 3 && Nb->Findings.Num() == 1);
	Key(EKeys::Right); Key(EKeys::Enter);
	Check(TEXT("Enter opens the chosen paper"), Opened == TEXT("s03log"), Opened);
	Nb->Show(TEXT("kit"));
	Check(TEXT("each tab keeps its choice"), Nb->Chosen() == 3, FString::FromInt(Nb->Chosen()));
	{
		int32 Reveals = 0, Reads = 0;
		FS47NbExtras Journal;
		FS47NbWorkItem Work; Work.Id = TEXT("test/0"); Work.Title = TEXT("Routine result"); Work.NextAction = TEXT("Register at OPS"); Work.Hints = { TEXT("Look at the current stage"), TEXT("OPS"), TEXT("Register") };
		Journal.WorkItems.Add(Work);
		FS47NbWorkItem Done = Work; Done.Id = TEXT("older/0"); Done.bDone = true; Done.bEarlier = true; Journal.WorkItems.Add(Done);
		Journal.GuideEntries.Add({ TEXT("guide:records"), TEXT("Records"), TEXT("Keep originals.") });
		Journal.HintLevel = [&Reveals](const FString&) { return Reveals; };
		Journal.RevealHint = [&Reveals](const FString&) { return ++Reveals; };
		Journal.MarkRead = [&Reads](const FString&) { ++Reads; };
		FS47Doc Evidence = Find; Evidence.Id = TEXT("freeform-control"); Evidence.Kind = TEXT("fieldnote");
		const auto Test = SNew(SS47Notebook).Extras(Journal).Docs({ Evidence });
		Test->Show(TEXT("tasks")); Test->BuildRight();
		Check(TEXT("opening and rendering journal is pure"), Reveals == 0 && Reads == 0);
		Check(TEXT("completed older work starts collapsed"), Test->Count() == 1);
		Test->Section(1); Check(TEXT("work history expands on request"), Test->Count() == 2); Test->Section(1);
		Test->RevealHint(); Check(TEXT("hint is explicitly requested"), Reveals == 1 && Reads == 0);
		Test->Show(TEXT("findings")); Check(TEXT("retained Freeform field note is knowledge and an original"), Test->Count() == 1 && Test->Papers.Num() == 1);
		Test->Activate(); Check(TEXT("opening original marks only explicit read"), Reads == 1);
		Test->FilterDocuments(TEXT("fieldnote freeform")); Check(TEXT("owned document search combines type and id"), Test->Papers.Num() == 1);
		Test->FilterDocuments(TEXT("unowned-secret")); Check(TEXT("search never invents unowned documents"), Test->Papers.IsEmpty()); Test->FilterDocuments(TEXT(""));
		Test->Section(1); Check(TEXT("knowledge keeps notes as section"), Test->Tab == TEXT("notes") && Test->MainTab() == TEXT("findings"));
		Test->Show(TEXT("photos")); Test->Turn(1); Check(TEXT("album belongs to Documents"), Test->Tab == TEXT("kit"));
		Test->Show(TEXT("guide")); Test->Section(1); Check(TEXT("controls remain in Field Guide"), Test->Tab == TEXT("controls") && Test->MainTab() == TEXT("guide"));
		Check(TEXT("browsing never reveals another hint"), Reveals == 1);
	}
	// PT-NOTEBOOK-20261109: Freeform's book: the field records only under Knowledge, each thing once, newest night first, the kinds
	// named, no "Field record:" and no standing sentences shown; NOW the note's lines, every one of them, the job cards on one line.
	{
		auto Doc = [](const TCHAR* Id, const TCHAR* Kind, const TCHAR* Title, const TCHAR* Stamp, const FString& Page = FString())
		{
			FS47Doc D; D.Id = Id; D.Kind = Kind; D.Title = Title; D.Stamp = Stamp; D.Page = Page.IsEmpty() ? FString(Stamp) + TEXT("\n\nOBSERVATION\nA corrugated stock tank, a third full. Tracks round it.\n\n") + FieldTail : Page; D.Transcript = D.Page; return D;
		};
		const FS47Doc Letter = Doc(TEXT("reyes_letter"), TEXT("typed"), TEXT("Letter from Dr. Ward"), TEXT("SARO / DUTY OFFICER"), TEXT("Dear"));
		const FS47Doc Tank = Doc(TEXT("freeform-place_tank"), TEXT("fieldnote"), TEXT("Field record: A corrugated stock tank, a third full"), TEXT("IN THE FIELD / DAY 1"));
		const FS47Doc OldCase = Doc(TEXT("freeform-case1-test1"), TEXT("fieldnote"), TEXT("Field record: CASE / THE REPEATED INTERVAL"), TEXT("SARO / DAY 1"));
		const FS47Doc TagNote = Doc(TEXT("freeform-reyes_tag_tank"), TEXT("fieldnote"), TEXT("Field record: Reyes at the tank, March 27"), TEXT("IN THE FIELD / DAY 2"));
		const FS47Doc Tag = Doc(TEXT("reyes_tag_tank"), TEXT("hand"), TEXT("Survey tag at the water tank"), TEXT("SARO RFI / 2"), TEXT("R."));
		const FS47Doc Telex = Doc(TEXT("freeform-case1-records"), TEXT("telex"), TEXT("Records reply: the repeated interval"), TEXT("SARO TELEX / DAY 2"), TEXT("REPLY"));
		const FS47Doc Print = Doc(TEXT("answer-printout-187654"), TEXT("printout"), TEXT("Printout, monitor channel, unscheduled"), TEXT("MONITOR"), TEXT("PULSES"));
		const FS47Doc Answer = Doc(TEXT("freeform-answer_d2_87654"), TEXT("fieldnote"), TEXT("Field record: An unscheduled printout from the monitor channel"), TEXT("SARO / DAY 2"));
		FS47Doc Photo = Doc(TEXT("frame1"), TEXT("photo"), TEXT("The orb"), TEXT("DAY 2"), TEXT("-")); Photo.Image = TEXT("art");
		const TArray<FS47Doc> All = { Letter, Tank, OldCase, TagNote, Tag, Telex, Print, Answer, Photo };
		FS47NbExtras Fx; Fx.bFreeform = true;
		Fx.Papers = TArray<FS47Doc>{ Letter, OldCase, Tag, Telex, Print, Photo }; // as FS47PinBoard::Papers leaves them: no field records but an old case's
		Fx.People = TArray<FS47PersonCard>{ { TEXT("THE DEPUTY"), TEXT("Deputy at the substation"), TEXT("Window dark 3 nights. Nobody asks."), FString() } };
		Fx.bNow = true; Fx.NowClock = TEXT("Too early: the window opens at 02:20. Do a job or rest until then.");
		auto Line = [](const TCHAR* Group, const TCHAR* Text, bool bDone = false, const TCHAR* Fold = TEXT(""), const TCHAR* Item = TEXT("")) { FS47NbNowLine N; N.Group = Group; N.Text = N.What = N.Why = Text; N.bDone = bDone; N.Fold = Fold; N.Item = Item; return N; };
		Fx.Now = { Line(TEXT("next"), TEXT("Remote feeder: lights reported. Go look.")), Line(TEXT("clock"), TEXT("02:20-02:40  Array window opens")), Line(TEXT("todo"), TEXT("PHOTO LAB: Film waiting.")),
			Line(TEXT("also"), TEXT("Job cards: 2 open at OPS."), false, TEXT("cards")), Line(TEXT("done"), TEXT("WORTH A LOOK: The remote feeder on the county road."), true) };
		for (int32 I = 0; I < 3; ++I) { FS47NbWorkItem W; W.Id = FString::Printf(TEXT("FF-D0002-J%d/0"), I); W.Title = W.Id; W.bCard = true; W.bCanFollow = true; W.bDone = I == 2; Fx.WorkItems.Add(W); }
		TArray<FS47Task> Ft; Ft.Add({ TEXT("MAILBOX / One paper you took out of the box is not read yet."), false });
		const auto Free = SNew(SS47Notebook).Tasks(Ft).Docs(All).Extras(Fx);
		bool bNoNotes = true, bOnce = true, bNoOld = true, bNoPhoto = true;
		TSet<FString> Seen;
		for (const FS47Doc& D : Free->Papers) { bNoNotes &= D.Kind != TEXT("fieldnote"); bNoOld &= D.Id != OldCase.Id; bNoPhoto &= D.Kind != TEXT("photo"); }
		for (const TArray<FS47Doc>* List : { &Free->Findings, &Free->Papers }) for (const FS47Doc& D : *List) { bOnce &= !Seen.Contains(D.Id); Seen.Add(D.Id); }
		for (const FS47Doc& D : Free->Findings) bNoOld &= D.Id != OldCase.Id;
		Check(TEXT("Freeform: no field record among the documents, and no photograph"), bNoNotes && bNoPhoto && Free->Papers.Num() == 4, FString::FromInt(Free->Papers.Num()));
		Check(TEXT("Freeform: a paper's field-record twin and an old save's case record are not shown again"), bOnce && bNoOld && Free->Findings.Num() == 1 && Free->Findings[0].Id == Tank.Id, FString::FromInt(Free->Findings.Num()));
		Check(TEXT("Freeform: newest night first, by the DAY stamped on it or the paper before it"), Free->Papers.Num() == 4 && Free->DocNight.FindRef(Free->Papers[0].Id) == 2 && Free->DocNight.FindRef(Free->Papers.Last().Id) == 1 && Free->Papers.Last().Id == Letter.Id && Free->DocNight.FindRef(Tag.Id) == 2);
		Check(TEXT("Freeform: the kinds named, the field record's own words without its standing sentences"), KindWord(Tank) == TEXT("Field note") && KindWord(Telex) == TEXT("Telex") && KindWord(Telex, false) == TEXT("Print")
			&& ShownTitle(Tank, true) == TEXT("A corrugated stock tank, a third full") && ShownTitle(Tank, false) == Tank.Title && ShownPage(Tank, true) == TEXT("A corrugated stock tank, a third full. Tracks round it.") && !ShownPage(Tank, true).Contains(TEXT("Interpretation remains")));
		Free->Show(TEXT("papers")); Free->Section(1);
		Check(TEXT("Freeform: the photographs are the album's alone, not a section of the documents"), Free->Tab == TEXT("papers"), Free->Tab);
		Free->Show(TEXT("findings")); Free->Section(1);
		Check(TEXT("Freeform: WHO & WHAT in Knowledge, one box a person"), Free->Tab == TEXT("people") && Free->Count() == 1 && Free->Columns() == 1, Free->Tab);
		// MARGINALIA-20261109: the scribbles (S47Marginalia's own checks: nothing before it happened, the tiers, the limits, no word for
		// what they are), a struck guess with a line Slate really draws, and the scribbles changing nothing of what can be chosen
		{
			TArray<FString> Marg;
			const bool bMarg = S47Marginalia::SelfTest(Marg);
			Lines.Append(Marg);
			Check(TEXT("Freeform WHO & WHAT: the scribbles keep to what happened, the tiers and the spoiler limits (S47Marginalia)"), bMarg);
			Check(TEXT("Freeform station files: SARO's own history keeps to the spoiler limits and names nobody real (S47StationFiles)"), S47StationFiles::Lint().IsEmpty(), S47StationFiles::Lint());
			Check(TEXT("Freeform WHO & WHAT: a struck guess has a line through it that Slate draws, a standing one none"),
				!PencilStyle(24.f, true).StrikeBrush.GetResourceName().IsNone() && PencilStyle(24.f, false).StrikeBrush.GetResourceName().IsNone() && !StrikeBrush(S47Game::InkBlue()).GetResourceName().IsNone());
			FS47NbExtras Mx = Fx;
			const auto Scrib = [](const TCHAR* Side, const TCHAR* Box, const TCHAR* Text, bool bStruck) { FS47NbScribble S; S.Side = Side; S.Box = Box; S.Text = Text; S.bStruck = bStruck; return S; };
			Mx.Scribbles = { Scrib(TEXT("people"), TEXT("THE DEPUTY"), TEXT("Writes everything down."), false), Scrib(TEXT("people"), TEXT("DALE"), TEXT("Never seen him."), false),
				Scrib(TEXT("them"), TEXT("THE LOW ONE"), TEXT("Coyote. A big one."), true) };
			const auto Pencilled = SNew(SS47Notebook).Docs(All).Extras(Mx);
			Pencilled->Show(TEXT("findings")); Pencilled->Section(1);
			Check(TEXT("Freeform WHO & WHAT: PEOPLE and THEM on the two pages, and the scribbles change nothing of what can be chosen"),
				Pencilled->Tab == TEXT("people") && Pencilled->Count() == 1 && Pencilled->RightSlot->GetChildren()->GetChildAt(0) != SNullWidget::NullWidget, Pencilled->Tab);
		}
		Free->Show(TEXT("tasks"));
		Check(TEXT("Freeform NOW: every line of the note is on it (so the note's \"+N more\" are all here), the job cards folded on one line"), Free->Count() == Fx.Now.Num(), FString::FromInt(Free->Count()));
		Free->Choose(3); Free->Activate();
		Check(TEXT("Freeform NOW: the job cards' line opens them under it (the filed one waits for the history)"), Free->Count() == Fx.Now.Num() + 2 && Free->NowRows[4].Item == 0, FString::FromInt(Free->Count()));
		Free->Section(1);
		Check(TEXT("Freeform NOW: with the history, the filed card too"), Free->Count() == Fx.Now.Num() + 3, FString::FromInt(Free->Count()));
		Free->Choose(0);
		Check(TEXT("Freeform NOW: the next step first, the night's done lines last, struck"), Free->X.Now[Free->NowRows[0].Line].Group == TEXT("next") && Free->X.Now[Free->NowRows.Last().Line].bDone);
		// D45-20261110 (Tom: a plain to-do list by importance): the heads in order, NOW first when something new is on it (also when the
		// field guide was the page used last), and the field guide's first entry, tonight's list, turns to NOW.
		Check(TEXT("Freeform NOW: the heads are DO FIRST, TONIGHT, WHEN YOU CAN and DONE TONIGHT"), NowHead(TEXT("first")) == TEXT("DO FIRST") && NowHead(TEXT("tonight")) == TEXT("TONIGHT")
			&& NowHead(TEXT("later")) == TEXT("WHEN YOU CAN") && NowHead(TEXT("done")) == TEXT("DONE TONIGHT") && NowHead(TEXT("next")).IsEmpty());
		{
			FS47NbExtras Nx = Fx;
			Nx.Now = { Line(TEXT("first"), TEXT("FEEDER: Power is out.")), Line(TEXT("tonight"), TEXT("BEARING SLIP: New circle on your map.")), Line(TEXT("later"), TEXT("STATION GOAL: Array 19 of 27.")),
				Line(TEXT("done"), TEXT("OPS: One job card registered and filed."), true) };
			Nx.Now[1].bNew = true; Nx.Now[1].Key = TEXT("d45-selftest:slip");
			Nx.GuideEntries = { { TEXT("guide:tonight"), TEXT("Tonight's list"), TEXT("TONIGHT\n- BEARING SLIP: New circle on your map.") }, { TEXT("guide:records"), TEXT("Records"), TEXT("Keep originals.") } };
			FS47NbExtras Looked = Nx; Looked.Now[1].bNew = false;
			Check(TEXT("Freeform: the notebook opens on NOW when something new is on it, even after the field guide; otherwise where it was left"),
				OpenOn(Nx, TEXT("guide")) == TEXT("tasks") && OpenOn(Looked, TEXT("guide")) == TEXT("guide") && OpenOn(FS47NbExtras(), TEXT("map")) == TEXT("map"));
			const auto Ranked = SNew(SS47Notebook).Docs(All).Extras(Nx);
			Ranked->Show(TEXT("guide")); Ranked->Choose(0); Ranked->Activate();
			Check(TEXT("Freeform field guide: its first entry is tonight's list, and it turns to NOW"), Ranked->Tab == TEXT("tasks") && Ranked->Count() == Nx.Now.Num(), Ranked->Tab);
		}
		bool bLint = true;
		for (const FS47NbNowLine& N : Fx.Now) bLint &= S47Script::LintText(N.Text).IsEmpty();
		for (const FS47PersonCard& Pc : *Fx.People) bLint &= S47Script::LintText(Pc.Name + TEXT(" ") + Pc.Role + TEXT(" ") + Pc.Note).IsEmpty();
		for (const FS47Doc& D : Free->Findings) bLint &= S47Script::LintText(ShownTitle(D, true) + TEXT(" ") + ShownPage(D, true)).IsEmpty();
		Check(TEXT("Freeform: the lines NOW, WHO & WHAT and the field records show keep to the spoiler limits"), bLint);
		const auto Story = SNew(SS47Notebook).Docs(All).Extras(FS47NbExtras());
		Check(TEXT("Story's book is as it was: every paper, field records among them"), Story->Papers.Num() == All.Num() - 1 && Story->Findings.Num() == 4, FString::FromInt(Story->Papers.Num()));
	}
	Nb->Show(TEXT("kit")); Nb->Show(TEXT("map"));
	Check(TEXT("map clears the previous left page"), Nb->LeftSlot->GetChildren()->GetChildAt(0) == SNullWidget::NullWidget);
	// Xbox uses the same selected entries, tab state and callbacks as the keyboard.
	Nb->Show(TEXT("tasks")); Key(EKeys::Gamepad_RightShoulder);
	Check(TEXT("RB turns to the next tab without opening the board"), Nb->Tab == TEXT("findings") && Boards == 0);
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
		const FCtlPage Pad = Controls(false, true);
		Check(TEXT("controls: Xbox has camera, trigger and driving help"), Pad.Title == TEXT("Controller") && Has(Pad, TEXT("X / RT | Raise or lower the camera / shutter while raised.")) && Has(Pad, TEXT("RT / LT | Throttle / brake or reverse.")));
		Check(TEXT("controls: the walk keys as the web has them"), Row(Desk, 0, 0) == TEXT("W A S D, arrows | Walk. Shift: run."), Row(Desk, 0, 0));
		Check(TEXT("controls: the notebook's keys as the web has them"), Has(Desk, TEXT("Tab, N | The notebook. M: its map. I: the kit. H: these controls.")));
		Check(TEXT("controls: Space both jump and shutter"), Has(Desk, TEXT("Space | Jump. With the camera raised: the shutter.")));
		// M7-HUD-20261109: X throws (and the stones), U the field recorder, Z the counter, as S47Keys binds them
		auto KeyOf = [](const FCtlPage& Pg, const TCHAR* Starts) { for (const FCtlGroup& Gr : Pg.Groups) for (const TPair<FString, FString>& R : Gr.Rows) if (R.Value.StartsWith(Starts)) return R.Key; return FString(); };
		Check(TEXT("controls: throw, the stones and the pocket"), KeyOf(Desk, TEXT("Hold to throw")).StartsWith(TEXT("X")) && KeyOf(Desk, TEXT("Hold to throw")).Contains(TEXT("Mouse")), KeyOf(Desk, TEXT("Hold to throw")));
		Check(TEXT("controls: the field recorder"), KeyOf(Desk, TEXT("The field recorder")) == TEXT("U") && S47Keys::Is(TEXT("tape"), EKeys::U), KeyOf(Desk, TEXT("The field recorder")));
		Check(TEXT("controls: the counter"), KeyOf(Desk, TEXT("The Geiger counter")) == TEXT("Z") && S47Keys::Is(TEXT("counter"), EKeys::Z), KeyOf(Desk, TEXT("The Geiger counter")));
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
