#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Story.h"
#include "UObject/Object.h"
#include "S47Chapter.generated.h"

class US47Main;

// A chapter of the story (story/Chapter1.ts ... Chapter6.ts) or a system the host steps the same
// way. Each keeps the TypeScript class's shape: begin(saved), reset(), serialize() (the saved
// state, the same JSON the web version writes; SaveState here, as UObject has a Serialize), update(dt, t), objective(), tasks(),
// saveBlock() and doorChanged(). It reaches the rest through M (US47Main: M->Web, M->UI,
// M->Audio, M->Inter, M->Player) and its host through Host (AS47Story, as Prologue.ts).
UCLASS(Abstract)
class SIGNAL47_API US47Chapter : public UObject
{
	GENERATED_BODY()

public:
	void Bind(US47Main* InM, AS47Story* InHost) { M = InM; Host = InHost; }

	virtual void Begin(const S47J::FObj& Saved) {}
	virtual void Reset() {}
	virtual S47J::FObj SaveState() const { return nullptr; } // serialize()
	virtual void Update(double Dt, double T) {}
	virtual FString Objective() const { return FString(); }
	virtual TArray<FS47Task> Tasks() const { return {}; }
	virtual FString SaveBlock() const { return FString(); } // a reason the game cannot be saved now
	virtual void DoorChanged(const FString& Id, bool bOpen) {}
	virtual FString Key() const { return FString(); }       // 'ch1' ...

	bool bActive = false;

protected:
	UPROPERTY() TObjectPtr<US47Main> M;
	UPROPERTY() TObjectPtr<AS47Story> Host;
};
