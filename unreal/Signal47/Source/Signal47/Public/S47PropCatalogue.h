#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

// PROPS-REUSE-20261113 (Tom, 13 November: «3D-objekter må kunne brukes flere steder»): one shared list of the real models (bought
// Cosmos pieces, free models, the project's own) that more than one place may stand up, so a fuse box, a chair, a coffee machine or a
// phone is named once and placed by its catalogue id wherever it fits. Config/Props/catalogue.json: id -> asset path, its size in cm,
// which way its front faces (in the mesh's own axes) and what it is. S47Props leaves the file alone (PropFiles).
//
// The placing files say "model": "<id>" (gas_cosmos.json's pieces, a site's dress file) or "model": "cat:<id>" (an S47Props placement
// file, where a bare name is a free model). A catalogue entry is load-if-present: a missing package (a bought pack not on this machine,
// Content/Free not imported) is not asked for, so the place keeps its stand-in.
namespace S47PropCatalogue
{
	struct FEntry
	{
		FString Id, Asset, What;
		FVector SizeCm = FVector::ZeroVector; // the mesh's bounds, x y z in its own axes, as imported
		FString Front;                        // "+Y", "-X": the mesh axis the front looks along (yaw 0 on the station: +Y is town +z)
		bool bLicensed = false;               // under /Game/Licensed: bought, never in Git
	};
	/** Config/Props/catalogue.json read into Out (Find and Resolve keep one read for the run). False with the reason when it does not read. */
	bool Read(TMap<FString, FEntry>& Out, FString* Error = nullptr);
	const FEntry* Find(const FString& Id);
	/** "cat:fuseBox" or "fuseBox" (a catalogue id) to its asset path; anything else comes back as it was. */
	FString Resolve(const FString& Ref);
	/** The package is on disk (asked without a load, so a missing bought pack logs nothing). */
	bool Here(const FString& AssetPath);
	/** The mesh if its package is here; nullptr otherwise, without a load warning. */
	UStaticMesh* LoadIfHere(const FString& AssetPath);
	/** The file reads; every entry has an id, a /Game path, a size and a front; bought ones under /Game/Licensed; how many are here. */
	bool SelfTest(TArray<FString>& Lines);
}
