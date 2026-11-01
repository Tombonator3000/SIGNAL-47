#pragma once
#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
namespace S47FreeformTerminal
{
 using FW=TSharedRef<SWidget>;
 FW Text(const FString& Value,int32 Tone=0);
 FW Text(TAttribute<FString> Value,TAttribute<int32> Tone);
 FW Paper(const FString& Value);
 FW Small(const FString& Value);
 FW Button(const FString& Label,TFunction<void()> Action,bool Primary=false,TAttribute<bool> Enabled=true);
 FW ButtonLive(TAttribute<FString> Label,TFunction<void()> Action,bool Primary=false,TAttribute<bool> Enabled=true);
 FW Pair(FW Left,FW Right);
 FW Frame(const FString& Title,FW Body);
}
