#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// Freeform only: what comes over the air in the truck. The CB under the dash, and callers on 1420. Mostly nothing; a few of them
// point at a place on the land, or at what the player has been doing. They never explain anything. Text as the rest of the
// radio is, no recorded voices yet. More people tell more once the county and Caliche know the station (SheriffTrust).
class FS47RadioTips
{
public:
 explicit FS47RadioTips(US47Main* Main):M(Main){}
 struct FLine{const TCHAR* Id;const TCHAR* Text;const TCHAR* Place;const TCHAR* Needs;int32 Trust;};
 // Two speakers at most: a caller and Earl. Empty answer: he lets it go.
 struct FCall{const TCHAR* Id;const TCHAR* Who;const TCHAR* Text;const TCHAR* Answer;const TCHAR* Place;const TCHAR* Needs;int32 Trust;};
 static const TArray<FLine>& CbLines();
 static const TArray<FCall>& AmCalls();
 void Begin(const S47J::FObj& Saved);
 void Step(double Dt);
 S47J::FObj Save() const;
 static bool SelfTest(TArray<FString>& Lines);
 const TSet<FString>& Said() const{return Used;}
 // Developer menu: the next one now, whatever the waiting. False: there is none left that fits.
 bool DevCb();
 bool DevAm();
private:
 US47Main* M=nullptr;
 TSet<FString> Used;
 double CbIn=40,AmIn=60;
 bool Ready(const TCHAR* Needs,int32 Trust) const;
 void StepCb();
 void StepAm();
};
