#include "S47PropCatalogue.h"

#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// A named namespace, not an anonymous one: unity builds put several files in one translation unit.
namespace S47PropCatalogueLocal
{
	FString FilePath() { return FPaths::ProjectConfigDir() / TEXT("Props") / TEXT("catalogue.json"); }
	const TCHAR* Prefix = TEXT("cat:");

	const TMap<FString, S47PropCatalogue::FEntry>& Cached()
	{
		static TMap<FString, S47PropCatalogue::FEntry> Map;
		static bool bRead = false;
		if (!bRead) { bRead = true; S47PropCatalogue::Read(Map); }
		return Map;
	}
}
namespace SPC = S47PropCatalogueLocal;

bool S47PropCatalogue::Read(TMap<FString, FEntry>& Out, FString* Error)
{
	auto Fail = [&](const FString& Why) { if (Error) *Error = Why; return false; };
	FString Text;
	TSharedPtr<FJsonObject> J;
	if (!FFileHelper::LoadFileToString(Text, *SPC::FilePath())) return Fail(TEXT("no Config/Props/catalogue.json"));
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J.IsValid()) return Fail(TEXT("catalogue.json does not read"));
	const TSharedPtr<FJsonObject>* Models = nullptr;
	if (!J->TryGetObjectField(TEXT("models"), Models)) return Fail(TEXT("catalogue.json has no models"));
	for (const auto& KV : (*Models)->Values)
	{
		const TSharedPtr<FJsonObject> O = KV.Value.IsValid() ? KV.Value->AsObject() : nullptr;
		if (!O.IsValid()) return Fail(FString::Printf(TEXT("%s is not an object"), *KV.Key));
		FEntry E;
		E.Id = KV.Key;
		O->TryGetStringField(TEXT("asset"), E.Asset);
		O->TryGetStringField(TEXT("what"), E.What);
		O->TryGetStringField(TEXT("front"), E.Front);
		const TArray<TSharedPtr<FJsonValue>>* S = nullptr;
		if (O->TryGetArrayField(TEXT("size_cm"), S) && S->Num() == 3) E.SizeCm = FVector((*S)[0]->AsNumber(), (*S)[1]->AsNumber(), (*S)[2]->AsNumber());
		E.bLicensed = E.Asset.StartsWith(TEXT("/Game/Licensed/"));
		Out.Add(E.Id, MoveTemp(E));
	}
	return true;
}

const S47PropCatalogue::FEntry* S47PropCatalogue::Find(const FString& Id)
{
	return SPC::Cached().Find(Id.StartsWith(SPC::Prefix) ? Id.Mid(4) : Id);
}

FString S47PropCatalogue::Resolve(const FString& Ref)
{
	if (Ref.IsEmpty() || Ref.StartsWith(TEXT("/"))) return Ref;
	const FEntry* E = Find(Ref);
	return E ? E->Asset : Ref;
}

bool S47PropCatalogue::Here(const FString& AssetPath)
{
	return AssetPath.StartsWith(TEXT("/Game/")) && FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(AssetPath));
}

UStaticMesh* S47PropCatalogue::LoadIfHere(const FString& AssetPath)
{
	return Here(AssetPath) ? LoadObject<UStaticMesh>(nullptr, *AssetPath) : nullptr;
}

bool S47PropCatalogue::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s prop catalogue: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bCond; };
	TMap<FString, FEntry> Map;
	FString Error;
	const bool bRead = Read(Map, &Error);
	Check(bRead && Map.Num() > 0, bRead ? FString::Printf(TEXT("Config/Props/catalogue.json: %d models"), Map.Num()) : Error);
	if (!bRead) return bOk;
	TArray<FString> Bad;
	int32 HereN = 0;
	const TArray<FString> Fronts = { TEXT("+X"), TEXT("-X"), TEXT("+Y"), TEXT("-Y") };
	for (const auto& KV : Map)
	{
		const FEntry& E = KV.Value;
		// a /Game object path (package.object), a size, a front; a bought pack only under /Game/Licensed (never in Git)
		const bool bPath = E.Asset.StartsWith(TEXT("/Game/")) && E.Asset.Contains(TEXT("."));
		const bool bBought = E.Asset.Contains(TEXT("Cosmos")) || E.Asset.Contains(TEXT("Tripo"));
		const bool bGood = !E.Id.IsEmpty() && !E.Id.Contains(TEXT(":")) && bPath && (!bBought || E.bLicensed)
			&& E.SizeCm.GetMin() > 0 && E.SizeCm.GetMax() < 1000 && Fronts.Contains(E.Front) && !E.What.IsEmpty();
		if (!bGood) Bad.Add(E.Id.IsEmpty() ? TEXT("(no id)") : E.Id);
		HereN += Here(E.Asset) ? 1 : 0;
	}
	Check(Bad.IsEmpty(), Bad.IsEmpty() ? TEXT("every model has a /Game path (bought ones under /Game/Licensed), a size, a front and a line saying what it is")
		: TEXT("incomplete: ") + FString::Join(Bad, TEXT(", ")));
	// the resolver: a catalogue id and its "cat:" form give the path; a path or an unknown name comes back as it was
	if (Map.Num())
	{
		const FEntry& First = Map.CreateConstIterator()->Value;
		Check(Resolve(First.Id) == First.Asset && Resolve(FString(SPC::Prefix) + First.Id) == First.Asset && Resolve(TEXT("/Game/X.X")) == TEXT("/Game/X.X")
			&& Resolve(TEXT("no_such_model")) == TEXT("no_such_model") && !Here(TEXT("/Game/Licensed/NoSuchPack/SM_None.SM_None")) && !Here(TEXT("office_phone")),
			TEXT("ids and cat:ids resolve to their path, other refs pass through, a missing package is not here"));
	}
	// load-if-present: either way is a pass; the line says which
	Lines.Add(FString::Printf(TEXT("PASS prop catalogue: %d of %d models here%s"), HereN, Map.Num(), HereN == 0 ? TEXT(" (none: every place keeps its stand-in)") : TEXT("")));
	return bOk;
}
