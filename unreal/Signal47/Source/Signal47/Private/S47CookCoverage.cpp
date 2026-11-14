#include "S47CookCoverage.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProperties.h"
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
	// A file the check needs that cannot be read: in the source tree it is there, so that FAILS; in a package it may not be staged
	// (Config/DefaultGame.ini and the tables are not proven to be readable there), so that part is NOT RUN, said as INFO, never a PASS.
	const bool bPackaged = FPlatformProperties::RequiresCookedData();
	TArray<FString> NotRun;
	auto Unread = [&](const FString& What)
	{
		if (bPackaged) { NotRun.Add(What); Lines.Add(FString::Printf(TEXT("INFO cook coverage: not checked here (%s cannot be read in this package)"), *What)); }
		else Check(false, FString::Printf(TEXT("%s reads"), *What));
	};

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
	int32 Tables = 0;
	for (const FString& F : Files)
	{
		const TSharedPtr<FJsonObject> J = SCC::Read(Props / F);
		if (J.IsValid()) ++Tables;
		SCC::PathsIn(J, Named);
		// a prop that cuts the exported town ("trim": a model of free_models.json's trims) needs its cut meshes, or what stands for them
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		if (J.IsValid() && J->TryGetArrayField(TEXT("props"), A)) for (const TSharedPtr<FJsonValue>& V : *A)
		{
			FString Trim;
			if (V.IsValid() && V->Type == EJson::Object && V->AsObject()->TryGetStringField(TEXT("trim"), Trim) && !Trim.IsEmpty()) Trims.AddUnique(Trim);
		}
	}
	if (Tables == 0) Unread(TEXT("Config/Props/*.json"));
	else Check(Tables == Files.Num(), FString::Printf(TEXT("Config/Props: %d of %d tables read for paths"), Tables, Files.Num()));

	// the town's patch tables: their cut meshes are in Git and the whole layer is rejected without them
	const FString Content = FPaths::ProjectContentDir();
	TSharedPtr<FJsonObject> TownPatches;
	for (const TCHAR* Table : { TEXT("TownBuildings/town-patches.json"), TEXT("TownBuildings/room-patches.json"),
		TEXT("CalicheStreet/street-patches.json"), TEXT("CalicheHomes/assembly.json") })
	{
		const TSharedPtr<FJsonObject> J = SCC::Read(Content / Table);
		if (!J.IsValid()) { Unread(FString(TEXT("Content/")) + Table); continue; }
		if (FString(Table) == TEXT("TownBuildings/town-patches.json")) TownPatches = J;
		SCC::Required(J, Must);
		SCC::PathsIn(J, Named);
	}
	// PACKAGE-GAS-TRIM-20261114 (review): the gas station's old parts go by the composed town patches now. Each patch mesh that cuts the
	// station (a row of town-patches.json with gasRemoved, not hidden) must be here, or the town layer is rejected and the old station
	// comes back beside the owner's model.
	TMap<FString, int32> GasCut; // the row's key (g999): how many of the station's triangles it takes out
	{
		TArray<FString> GasMeshes, GasGone;
		const TSharedPtr<FJsonObject>* Rows = nullptr;
		if (TownPatches.IsValid() && TownPatches->TryGetObjectField(TEXT("meshes"), Rows)) for (const auto& KV : (*Rows)->Values)
		{
			const TSharedPtr<FJsonObject> R = KV.Value.IsValid() && KV.Value->Type == EJson::Object ? KV.Value->AsObject() : nullptr;
			double Gas = 0; bool bHidden = false; FString Asset;
			if (!R.IsValid() || !R->TryGetNumberField(TEXT("gasRemoved"), Gas) || Gas <= 0) continue;
			GasCut.Add(FString(KV.Key.ToView()), int32(Gas));
			R->TryGetBoolField(TEXT("hidden"), bHidden);
			if (bHidden || !R->TryGetStringField(TEXT("asset"), Asset) || Asset.IsEmpty()) continue;
			GasMeshes.Add(Asset);
			if (!SCC::Here(Asset)) GasGone.Add(Asset);
		}
		if (TownPatches.IsValid())
		{
			GasGone.Sort();
			Check(GasMeshes.Num() > 0 && GasGone.IsEmpty(), GasGone.IsEmpty()
				? FString::Printf(TEXT("the %d composed patch meshes that cut the old gas station (%d rows) are here"), GasMeshes.Num(), GasCut.Num())
				: FString::Printf(TEXT("%d of the composed patch meshes that cut the old gas station are not here: %s"), GasGone.Num(), *FString::Join(GasGone, TEXT(", "))));
		}
	}

	// the props' trims: a hidden row needs no mesh (ApplyTrim hides the exported part and takes its collision off); a cut row needs its
	// mesh, or, for the gas station, a composed patch row that takes out at least as much of the station (else the old part returns)
	if (Trims.Num())
	{
		const TSharedPtr<FJsonObject> Free = SCC::Read(Content / TEXT("Free/free_models.json"));
		const TSharedPtr<FJsonObject>* AllTrims = nullptr;
		if (!Free.IsValid()) { NotRun.Add(TEXT("the props' trims")); Lines.Add(TEXT("INFO cook coverage: not checked here (no Content/Free/free_models.json: free models not imported here; the props' trims are not checked)")); }
		else for (const FString& T : Trims)
		{
			const TSharedPtr<FJsonObject>* M = nullptr;
			const TSharedPtr<FJsonObject>* Meshes = nullptr;
			const bool bKnown = Free->TryGetObjectField(TEXT("trims"), AllTrims) && (*AllTrims)->TryGetObjectField(T, M) && (*M)->TryGetObjectField(TEXT("meshes"), Meshes);
			Check(bKnown, FString::Printf(TEXT("the trim %s a prop asks for is in free_models.json"), *T));
			if (!bKnown) continue;
			int32 Hidden = 0, Cut = 0, Stood = 0;
			TArray<FString> Bare;
			for (const auto& KV : (*Meshes)->Values)
			{
				const TSharedPtr<FJsonObject> R = KV.Value.IsValid() && KV.Value->Type == EJson::Object ? KV.Value->AsObject() : nullptr;
				if (!R.IsValid()) continue;
				bool bHidden = false; FString Asset; double Removed = 0;
				R->TryGetBoolField(TEXT("hidden"), bHidden);
				if (bHidden) { ++Hidden; continue; }
				R->TryGetStringField(TEXT("asset"), Asset); R->TryGetNumberField(TEXT("removed"), Removed);
				if (SCC::Here(Asset)) { ++Cut; Named.Add(Asset); continue; }
				const int32* ByPatch = T == TEXT("gas_station") ? GasCut.Find(FString(KV.Key.ToView())) : nullptr;
				if (ByPatch && *ByPatch >= int32(Removed)) ++Stood;
				else Bare.Add(FString(KV.Key.ToView()));
			}
			Bare.Sort();
			Check(Bare.IsEmpty(), Bare.IsEmpty()
				? FString::Printf(TEXT("the trim %s: %d hidden rows (no mesh needed), %d cut meshes here, %d not here and taken out by the composed town patches instead"), *T, Hidden, Cut, Stood)
				: FString::Printf(TEXT("the trim %s: %d cut meshes are not here and nothing takes their part out (%s): import_free.sh with the trim again"), *T, Bare.Num(), *FString::Join(Bare, TEXT(", "))));
		}
	}
	Named.Append(Must);

	const TArray<FString> Rules = CookRules();
	if (Rules.IsEmpty()) Unread(TEXT("Config/DefaultGame.ini's +DirectoriesToAlwaysCook rules"));
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
	if (Must.Num() || !bPackaged) Check(Must.Num() > 0 && Missing.IsEmpty(), Missing.IsEmpty()
		? FString::Printf(TEXT("all %d required patch meshes are here"), Must.Num())
		: FString::Printf(TEXT("%d required meshes are not here (deleted by a re-import, or not cooked): %s"), Missing.Num(), *FString::Join(Missing, TEXT(", "))));
	if (NotRun.Num()) Lines.Add(FString::Printf(TEXT("INFO cook coverage: not run here in part (%s); counted as not run, not as a pass"), *FString::Join(NotRun, TEXT("; "))));
	return bOk;
}
