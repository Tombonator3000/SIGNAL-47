#pragma once
#include "CoreMinimal.h"
class US47Main;
class USceneComponent;
class AS47Web;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTexture2D;

// Original EastWing furnishing. The wing owns visibility through its existing Parts list.
class FS47EastWingProps
{
public:
 bool Build(US47Main* Main, TArray<TWeakObjectPtr<USceneComponent>>& Parts);
 void SetPowered(bool bPowered){SetLevel(bPowered?1.f:0.f);}
 void SetLevel(float Level); // 0..1 tube output, follows the fixtures' strike/die-off flicker
 // The wall calendars (wing and control room) show the game's date: one shared 1024x816 sheet, redrawn only when the date changes.
 void UpdateCalendar(US47Main* Main);
 int32 CalendarCount() const { return CalendarSheets.Num(); }
 // T16-08: both calendars hang on a wall of S47SaroClip's table and go into none, and the control room's is clear of its neighbours.
 static bool SelfTest(TArray<FString>& Lines);
private:
 struct FEmission { TWeakObjectPtr<UMaterialInstanceDynamic> Material; FLinearColor Powered; };
 TArray<FEmission> Emission;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> CalendarSheets; // one overlay plane per SM_EW_Calendar1986 instance
 TWeakObjectPtr<UTexture2D> CalendarTexture;
 int32 CalendarKey=-1; // yyyymmdd of the sheet now on the walls
 void BuildCalendars(AS47Web* Web,const TArray<UStaticMeshComponent*>& Blades,TArray<TWeakObjectPtr<USceneComponent>>& Parts);
 bool bBuilt=false;
};
