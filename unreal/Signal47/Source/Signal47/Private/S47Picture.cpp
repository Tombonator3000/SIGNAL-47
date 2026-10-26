#include "S47Picture.h"

FString S47Picture::Normalize(const FString& Value)
{
	const FString Id = Value.TrimStartAndEnd().ToLower();
	if (Id == TEXT("off") || Id == TEXT("clean")) return TEXT("off");
	if (Id == TEXT("carpenter") || Id == TEXT("35mm")) return TEXT("carpenter");
	if (Id == TEXT("16mm")) return TEXT("16mm");
	if (Id == TEXT("super8") || Id == TEXT("super-8") || Id == TEXT("super 8")) return TEXT("super8");
	// Old 'heavy' saves used the worn tape. Unknown values keep the existing VHS fallback.
	return TEXT("vhs");
}

int32 S47Picture::Mode(const FString& Value)
{
	const FString Id = Normalize(Value);
	if (Id == TEXT("off")) return 0;
	if (Id == TEXT("carpenter")) return 2;
	if (Id == TEXT("16mm")) return 3;
	if (Id == TEXT("super8")) return 4;
	return 1;
}

FString S47Picture::DefaultForQuality(const FString& Quality)
{
	return Quality.TrimStartAndEnd().Equals(TEXT("low"), ESearchCase::IgnoreCase) ? TEXT("off") : TEXT("carpenter");
}

float S47Picture::Strength(float Value)
{
	return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 1.f;
}

const TArray<FString>& S47Picture::Ids()
{
	static const TArray<FString> Values = { TEXT("carpenter"), TEXT("16mm"), TEXT("super8"), TEXT("vhs"), TEXT("off") };
	return Values;
}

TArray<FString> S47Picture::Labels()
{
	return { TEXT("CARPENTER 35 MM"), TEXT("DESERT 16 MM"), TEXT("SUPER 8"), TEXT("VHS TAPE"), TEXT("CLEAN") };
}

FString S47Picture::Label(const FString& Value)
{
	switch (Mode(Value))
	{
	case 0: return TEXT("Clean");
	case 2: return TEXT("Carpenter 35 mm");
	case 3: return TEXT("Desert 16 mm");
	case 4: return TEXT("Super 8");
	default: return TEXT("VHS tape");
	}
}

FString S47Picture::Description(const FString& Value)
{
	switch (Mode(Value))
	{
	case 0: return TEXT("The game's base picture without film grain, tape noise or an extra colour grade. Picture strength has no effect on Clean.");
	case 2: return TEXT("The main look: a 1986 science-fiction thriller on 35 mm. Cool blue shadows, warm practical lights, deep blacks, soft highlight glow and fine film grain. The camera stays steady.");
	case 3: return TEXT("A desert field documentary on 16 mm: dusty, muted colours, greenish shadows, soft highlights, visible grain and a very slight frame weave. Slow fades or Still shots also steadies this look.");
	case 4: return TEXT("A Super 8 field reel: warm, saturated reversal-film colour, coarse grain, darkened corners, a slight frame weave and gentle exposure flicker. Slow fades or Still shots also steadies this look.");
	default: return TEXT("A 1986 video tape: softer detail, colour bleed, fine scan lines and tracking noise near the bottom of the frame. Slow fades or Still shots also stops the moving tape disturbances.");
	}
}

bool S47Picture::SelfTest(TArray<FString>& Out)
{
	bool bOk = true;
	auto Check = [&](const TCHAR* Name, bool bPass)
	{
		Out.Add(bPass ? FString::Printf(TEXT("ok  picture: %s"), Name) : FString::Printf(TEXT("FAIL picture: %s"), Name));
		bOk &= bPass;
	};
	Check(TEXT("legacy clean and tape settings"), Normalize(TEXT("off")) == TEXT("off") && Normalize(TEXT("vhs")) == TEXT("vhs") && Normalize(TEXT("heavy")) == TEXT("vhs"));
	Check(TEXT("unknown values retain safe VHS fallback"), Normalize(TEXT("")) == TEXT("vhs") && Normalize(TEXT("unknown")) == TEXT("vhs"));
	Check(TEXT("case and surrounding whitespace"), Normalize(TEXT("  CARPENTER  ")) == TEXT("carpenter") && Normalize(TEXT(" Super 8 ")) == TEXT("super8"));
	Check(TEXT("readable command line aliases"), Normalize(TEXT("clean")) == TEXT("off") && Normalize(TEXT("35mm")) == TEXT("carpenter") && Normalize(TEXT("super-8")) == TEXT("super8"));
	Check(TEXT("shader mode contract"), Mode(TEXT("off")) == 0 && Mode(TEXT("vhs")) == 1 && Mode(TEXT("carpenter")) == 2 && Mode(TEXT("16mm")) == 3 && Mode(TEXT("super8")) == 4);
	Check(TEXT("new quality defaults"), DefaultForQuality(TEXT("high")) == TEXT("carpenter") && DefaultForQuality(TEXT("ultra")) == TEXT("carpenter") && DefaultForQuality(TEXT("LOW")) == TEXT("off"));
	Check(TEXT("strength is bounded"), Strength(-1.f) == 0.f && Strength(2.f) == 1.f && Strength(0.35f) == 0.35f);
	Check(TEXT("menu starts with Carpenter and covers every mode"), Ids().Num() == 5 && Labels().Num() == Ids().Num() && Ids()[0] == TEXT("carpenter"));
	for (const FString& Id : Ids())
	{
		Check(*FString::Printf(TEXT("%s keeps its identity and description"), *Id), Normalize(Id) == Id && !Label(Id).IsEmpty() && !Description(Id).IsEmpty());
	}
	return bOk;
}
