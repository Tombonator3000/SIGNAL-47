#pragma once
#include "S47Json.h"

// Per-case presentation state only. Never stores inventory, evidence or completion.
struct FS47Journal
{
 TMap<FString,int32> Hints;
 TSet<FString> Read;
 void Load(const S47J::FObj& O)
 {
  Hints.Reset();Read.Reset();if(!O||S47J::Int(O,TEXT("v"))!=1)return;
  if(auto H=S47J::Obj(O,TEXT("hints")))for(const auto& P:H->Values){double V;if(P.Key.Len()<=160&&P.Value&&P.Value->TryGetNumber(V)&&FMath::IsFinite(V)&&V>=0&&V<=3)Hints.Add(FString(*P.Key),int32(V));}
  for(const auto& V:S47J::Arr(O,TEXT("read"))){FString Id;if(V&&V->TryGetString(Id)&&Id.Len()<=160&&Read.Num()<4096)Read.Add(Id);}
 }
 S47J::FObj Save()const
 {
  auto O=S47J::New(),H=S47J::New();S47J::Set(O,TEXT("v"),1);for(const auto& P:Hints)S47J::Set(H,*P.Key,P.Value);S47J::Set(O,TEXT("hints"),H);
  TArray<FString> IDs=Read.Array();IDs.Sort();TArray<TSharedPtr<FJsonValue>> A;for(const auto& Id:IDs)A.Add(MakeShared<FJsonValueString>(Id));O->SetArrayField(TEXT("read"),A);return O;
 }
 static bool Guide(const FString& Id){return Id.StartsWith(TEXT("guide:"))&&Id.Len()<=80;}
};
