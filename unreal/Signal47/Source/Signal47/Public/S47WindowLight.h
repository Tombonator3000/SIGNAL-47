#pragma once

#include "CoreMinimal.h"

class AS47Web;
class FJsonObject;
class UStaticMeshComponent;
class URectLightComponent;

// Native light transmitted through the exported windows. The original ground-projection mesh
// supplies each facade's span and normal and stays hidden; a soft rect at each pane lights the
// actual ground and casts shadows. Story flood slots and SetLevel still drive the same windows.
class SIGNAL47_API FS47WindowLights
{
public:
	~FS47WindowLights();
	FString Name;
	bool Build(AS47Web* W, const TSharedPtr<FJsonObject>& J);
	void SetLevel(const FString& Id, double K);
	void Update();
	int32 Windows() const { return Wins.Num(); }
	TMap<FString, double> Levels() const;
	/** Native lighting no longer attenuates one light because another lamp is nearby. */
	double Darkness() const;

private:
	struct FWin
	{
		FString Id;
		int32 Slot = -1;
		double K = 0, W0 = 1, Level = 1;
		TWeakObjectPtr<URectLightComponent> Light;
	};
	TArray<FWin> Wins;
	int32 Set = -1, Parent = -1;
	double BaseScale = 1;
	TWeakObjectPtr<AS47Web> Web;
	TWeakObjectPtr<UStaticMeshComponent> Mesh;
	FDelegateHandle LightTick;
};
