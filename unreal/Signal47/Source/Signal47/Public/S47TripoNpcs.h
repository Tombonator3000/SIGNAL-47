#pragma once

#include "CoreMinimal.h"

class US47Main;
class UStaticMeshComponent;

// Temporary static bodies for Tom's Tripo roster. Existing scenes own presence,
// movement and interactions; the original rigs and story state are untouched.
class FS47TripoNpcs
{
public:
 explicit FS47TripoNpcs(US47Main* InM);
 ~FS47TripoNpcs();
 void Step();
 UStaticMeshComponent* Component(const FString& Role) const;
 int32 BoundCount() const;
private:
 struct FState;
 TUniquePtr<FState> State;
 US47Main* M = nullptr;
 bool bInitialized = false, bOff = false, bChecked = false;
 void Build();
 void Update();
};
