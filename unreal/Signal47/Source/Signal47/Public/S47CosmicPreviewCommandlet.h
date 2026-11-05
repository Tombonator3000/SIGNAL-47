#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "S47CosmicPreviewCommandlet.generated.h"

/** Explicit offline listening tool. Never loads a player save or a gameplay map. */
UCLASS()
class US47CosmicPreviewCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    US47CosmicPreviewCommandlet();
    virtual int32 Main(const FString& Params) override;
};
