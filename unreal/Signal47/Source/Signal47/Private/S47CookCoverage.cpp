#include "S47CookCoverage.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// A named namespace, not an anonymous one: unity builds put several files in one translation unit.
namespace S47CookCoverageLocal
{
	TSharedPtr<FJsonObject> Read(const FString& Path)
	{
		FString Text;
		TSharedPtr<FJsonObject> J;
		if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J)) return nullptr;
		return J;
	}

	FString PackageOf(const FString& AssetPath)
	{
		return AssetPath.Contains(TEXT(".")) ? FString(FPackageName::ObjectPathToPackageName(AssetPath)) : AssetPath;
	}

	bool Here(const FString& AssetPath)
	{
		return AssetPath.StartsWith(TEXT("/Game/")) && FPackageName::DoesPackageExist(PackageOf(AssetPath));
	}

	// every string value that is a /Game path (one word: a _doc sentence that mentions a path is not one)
	void PathsOf(const TSharedPtr<FJsonValue>& V, TSet<FString>& Out)
	{
		if (!V.IsValid()) return;
		switch (V->Type)
		{
		case EJson::String:
		{
			const FString S = V->AsString();
			if (S.StartsWith(TEXT("/Game/")) && !S.Contains(TEXT(" "))) Out.Add(S);
			break;
		}
		case EJson::Array:
			for (const TSharedPtr<FJsonValue>& E : V->AsArray()) PathsOf(E, Out);
			break;
		case EJson::Object:
			for (const auto& KV : V->AsObject()->Values) PathsOf(KV.Value, Out);
			break;
		default:
			break;
		}
	}

	void PathsIn(const TSharedPtr<FJsonObject>& O, TSet<FString>& Out)
	{
		if (O.IsValid()) for (const auto& KV : O->Values) PathsOf(KV.Value, Out);
	}

	// the "asset" of every row under "meshes" that is not hidden: the patch tables' cut meshes, all in Git, never optional
	void Required(const TSharedPtr<FJsonObject>& O, TSet<FString>& Out)
	{
		const TSharedPtr<FJsonObject>* Meshes = nullptr;
		if (!O.IsValid() || !O->TryGetObjectField(TEXT("meshes"), Meshes)) return;
		for (const auto& KV : (*Meshes)->Values)
		{
			const TSharedPtr<FJsonObject> R = KV.Value.IsValid() && KV.Value->Type == EJson::Object ? KV.Value->AsObject() : nullptr;
			FString Asset;
			bool bHidden = false;
			if (!R.IsValid()) continue;
			R->TryGetBoolField(TEXT("hidden"), bHidden);
			if (!bHidden && R->TryGetStringField(TEXT("asset"), Asset) && !Asset.IsEmpty()) Out.Add(Asset);
		}
	}
}
namespace SCC = S47CookCoverageLocal;

TArray<FString> S47CookCoverage::CookRules()
{
	TArray<FString> Rules, Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *(FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini")))) return Rules;
	const FString Key = TEXT("+DirectoriesToAlwaysCook=(Path=\"");
	for (const FString& L : Lines)
	{
		const FString T = L.TrimStartAndEnd();
		if (!T.StartsWith(Key)) continue;
		const int32 End = T.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Key.Len());
		if (End > Key.Len()) Rules.AddUnique(T.Mid(Key.Len(), End - Key.Len()));
	}
	return Rules;
}

bool S47CookCoverage::Covered(const FString& AssetPath, const TArray<FString>& Rules)
{
	const FString Package = SCC::PackageOf(AssetPath);
	for (const FString& R : Rules) if (Package == R || Package.StartsWith(R + TEXT("/"))) return true;
	return false;
}

bool S47CookCoverage::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s cook coverage: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bCond; };

	// the pure part first: the rule test on made-up paths
	const TArray<FString> Fake = { TEXT("/Game/Free"), TEXT("/Game/Signal47/Look") };
	Check(Covered(TEXT("/Game/Free/Town/gas_station/Trim/StaticMeshes/g999.g999"), Fake) && Covered(TEXT("/Game/Signal47/Look/M_StationLook.M_StationLook"), Fake)
		&& !Covered(TEXT("/Game/FreeOther/SM_X.SM_X"), Fake) && !Covered(TEXT("/Game/Signal47/Lookdev/M_X.M_X"), Fake) && !Covered(TEXT("/Game/Signal47/M_X.M_X"), Fake),
		TEXT("a path is covered by its own directory's rule and no other (/Game/Free is not /Game/FreeOther)"));

	TSet<FString> Named, Must;
	TArray<FString> Trims;
	const FString Props = FPaths::ProjectConfigDir() / TEXT("Props");
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Props / TEXT("*.json")), true, false);
	Files.Sort();
	for (const FString& F : Files)
	{
		const TSharedPtr<FJsonObject> J = SCC::Read(Props / F);
		SCC::PathsIn(J, Named);
		// a prop that cuts the exported town ("trim": a model of free_models.json's trims) needs its cut meshes
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		if (J.IsValid() && J->TryGetArrayField(TEXT("props"), A)) for (const TSharedPtr<FJsonValue>& V : *A)
		{
			FString Trim;
			if (V.IsValid() && V->Type == EJson::Object && V->AsObject()->TryGetStringField(TEXT("trim"), Trim) && !Trim.IsEmpty()) Trims.AddUnique(Trim);
		}
	}
	Check(Files.Num() > 0, FString::Printf(TEXT("Config/Props: %d tables read for paths"), Files.Num()));

	const FString Content = FPaths::ProjectContentDir();
	if (const TSharedPtr<FJsonObject> Free = SCC::Read(Content / TEXT("Free/free_models.json")))
	{
		const TSharedPtr<FJsonObject>* AllTrims = nullptr;
		Free->TryGetObjectField(TEXT("trims"), AllTrims);
		for (const FString& T : Trims)
		{
			const TSharedPtr<FJsonObject>* M = nullptr;
			const bool bKnown = AllTrims && (*AllTrims)->TryGetObjectField(T, M);
			Check(bKnown, FString::Printf(TEXT("the trim %s a prop asks for is in free_models.json"), *T));
			if (bKnown) { SCC::Required(*M, Must); SCC::PathsIn(*M, Named); }
		}
	}
	else Lines.Add(FString::Printf(TEXT("PASS cook coverage: no Content/Free/free_models.json (free models not imported here)%s"),
		Trims.Num() ? TEXT("; the props' trims are not asked for") : TEXT("")));

	// the town's patch tables: their cut meshes are in Git and the whole layer is rejected without them
	for (const TCHAR* Table : { TEXT("TownBuildings/town-patches.json"), TEXT("TownBuildings/room-patches.json"),
		TEXT("CalicheStreet/street-patches.json"), TEXT("CalicheHomes/assembly.json") })
	{
		const TSharedPtr<FJsonObject> J = SCC::Read(Content / Table);
		Check(J.IsValid(), FString::Printf(TEXT("Content/%s reads"), Table));
		SCC::Required(J, Must);
		SCC::PathsIn(J, Named);
	}
	Named.Append(Must);

	const TArray<FString> Rules = CookRules();
	if (Rules.IsEmpty()) Lines.Add(TEXT("PASS cook coverage: no Config/DefaultGame.ini cook rules here (only presence checked)"));
	else
	{
		TArray<FString> Uncovered;
		for (const FString& P : Named) if (!Covered(P, Rules)) Uncovered.Add(P);
		Uncovered.Sort();
		Check(Uncovered.IsEmpty(), Uncovered.IsEmpty()
			? FString::Printf(TEXT("all %d /Game paths the tables name lie under one of %d +DirectoriesToAlwaysCook rules"), Named.Num(), Rules.Num())
			: FString::Printf(TEXT("%d paths named by a table have no cook rule and are not in a package: %s"), Uncovered.Num(), *FString::Join(Uncovered, TEXT(", "))));
	}
	TArray<FString> Missing;
	for (const FString& P : Must) if (!SCC::Here(P)) Missing.Add(P);
	Missing.Sort();
	Check(Missing.IsEmpty(), Missing.IsEmpty()
		? FString::Printf(TEXT("all %d required trim/patch meshes are here"), Must.Num())
		: FString::Printf(TEXT("%d required meshes are not here (deleted by a re-import, or not cooked): %s"), Missing.Num(), *FString::Join(Missing, TEXT(", "))));
	return bOk;
}
