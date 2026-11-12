#include "S47DirectorMarks.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "S47ScriptPlayer.h"
#include "S47Shot.h"
#include "S47Three.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DirectorMarks,Log,All);

// DIRECTOR-MODE-01, marks (S47DirectorMarks.h): the mark's JSON, the one file that keeps them over sessions, and their drawing while flying.

namespace S47DirectorMarksLocal
{
 constexpr int32 MaxDrawn=200;
 constexpr double NearCm=150; // nothing drawn within 1.5 m of the camera: it stands in it
 double R2(double V){return FMath::RoundToDouble(V*100)/100;}
 double R4(double V){return FMath::RoundToDouble(V*10000)/10000;}

 S47J::FObj PointJson(const FS47ScriptPoint& P)
 {
  const S47J::FObj O=S47J::New();
  S47J::Set(O,TEXT("x"),R2(P.X));S47J::Set(O,TEXT("z"),R2(P.Z));S47J::Set(O,TEXT("up"),R2(P.Up));
  return O;
 }
 FS47ScriptPoint PointOf(const S47J::FObj& O)
 {
  FS47ScriptPoint P;
  if(!S47J::Has(O,TEXT("x"))||!S47J::Has(O,TEXT("z")))return P;
  P.X=S47J::Num(O,TEXT("x"));P.Z=S47J::Num(O,TEXT("z"));P.Up=S47J::Num(O,TEXT("up"));P.bSet=true;
  return P;
 }

 // The file being written to now and what it holds (one game at a time on this machine).
 struct FState
 {
  FString TestPath;        // SetPathForTest
  FString Side,SideOf;     // where this session writes while SideOf (the real file) does not read, and which file that is
  bool bKnown=false;FString KnownOf;TArray<FS47DirectorMark> Marks; // Known()
 };
 FState& St(){static FState S;return S;}

 bool Save(const FString& File,const TArray<FS47DirectorMark>& Marks)
 {
  IFileManager& Fm=IFileManager::Get();
  Fm.MakeDirectory(*FPaths::GetPath(File),true);
  const FString Tmp=File+TEXT(".tmp");
  if(!FFileHelper::SaveStringToFile(S47DirectorMarks::Write(Marks),*Tmp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))return false;
  return Fm.Move(*File,*Tmp,true,true);
 }

 // The file this session writes to and its marks: the real one, or the side file while the real one does not read. False with Why:
 // neither reads.
 bool Open(FString& File,TArray<FS47DirectorMark>& Marks,FString& Why)
 {
  FState& S=St();
  const FString Main=S47DirectorMarks::Path();
  const S47DirectorMarks::ERead R=S47DirectorMarks::ReadFile(Main,Marks);
  if(R==S47DirectorMarks::ERead::Bad)
  {
   if(S.Side.IsEmpty()||S.SideOf!=Main)
   {
    S.Side=FPaths::GetPath(Main)/FString::Printf(TEXT("marks-%s.json"),*FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")));
    S.SideOf=Main;
   }
   if(S47DirectorMarks::ReadFile(S.Side,Marks)==S47DirectorMarks::ERead::Bad){Why=FPaths::GetCleanFilename(S.Side)+TEXT(" does not read either");return false;}
  }
  File=S47DirectorMarks::TargetFile(Main,R,S.Side);
  return true;
 }

 void Keep(const TArray<FS47DirectorMark>& Marks){FState& S=St();S.bKnown=true;S.KnownOf=S47DirectorMarks::Path();S.Marks=Marks;}
}

namespace S47DirectorMarks
{
const TArray<FString>& Kinds(){static const TArray<FString> L={TEXT("place"),TEXT("view"),TEXT("path"),TEXT("avoid")};return L;}

FString NextKind(const FString& Kind){const int32 I=Kinds().IndexOfByKey(Kind);return Kinds()[(I+1)%Kinds().Num()];}

S47J::FObj ToJson(const FS47DirectorMark& K)
{
 using namespace S47DirectorMarksLocal;
 const S47J::FObj O=S47J::New();
 S47J::Set(O,TEXT("n"),K.N);
 S47J::Set(O,TEXT("kind"),K.Kind);
 if(!K.Note.IsEmpty())S47J::Set(O,TEXT("note"),K.Note);
 S47J::Set(O,TEXT("session"),K.Session);
 S47J::Set(O,TEXT("time"),K.Time);
 S47J::Set(O,TEXT("area"),K.Area);
 S47J::Set(O,TEXT("eye"),TArray<double>{R2(K.Eye.X),R2(K.Eye.Y),R2(K.Eye.Z)});
 S47J::Set(O,TEXT("yaw"),R4(K.Yaw));S47J::Set(O,TEXT("pitch"),R4(K.Pitch));S47J::Set(O,TEXT("fov"),R2(K.Fov));
 if(K.Cam.bSet)S47J::Set(O,TEXT("cam"),PointJson(K.Cam));
 if(K.Look.bSet){const S47J::FObj L=PointJson(K.Look);S47J::Set(L,TEXT("dist"),R2(K.LookDist));S47J::Set(O,TEXT("look"),L);}
 if(K.Ahead.bSet)S47J::Set(O,TEXT("ahead"),PointJson(K.Ahead));
 S47J::Set(O,TEXT("indoors"),K.Indoors);
 if(!K.Clock.IsEmpty())S47J::Set(O,TEXT("clock"),K.Clock);
 if(K.Day>0)S47J::Set(O,TEXT("day"),K.Day);
 if(K.Weather.Num())S47J::Set(O,TEXT("weather"),K.Weather);
 if(!K.Place.IsEmpty()){const S47J::FObj P=S47J::New();S47J::Set(P,TEXT("id"),K.Place);S47J::Set(P,TEXT("m"),R2(K.PlaceDist));S47J::Set(O,TEXT("place"),P);}
 if(!K.Draft.IsEmpty())S47J::Set(O,TEXT("draft"),K.Draft);
 if(!K.Picture.IsEmpty())S47J::Set(O,TEXT("picture"),K.Picture);
 return O;
}

FS47DirectorMark FromJson(const S47J::FObj& O)
{
 using namespace S47DirectorMarksLocal;
 FS47DirectorMark K;
 K.N=S47J::Int(O,TEXT("n"));
 K.Kind=S47J::Str(O,TEXT("kind"),TEXT("place"));
 K.Note=S47J::Str(O,TEXT("note"));
 K.Session=S47J::Str(O,TEXT("session"));K.Time=S47J::Str(O,TEXT("time"));K.Area=S47J::Str(O,TEXT("area"));
 const TArray<double> E=S47J::Nums(O,TEXT("eye"));
 if(E.Num()>=3)K.Eye=FVector(E[0],E[1],E[2]);
 K.Yaw=S47J::Num(O,TEXT("yaw"));K.Pitch=S47J::Num(O,TEXT("pitch"));K.Fov=S47J::Num(O,TEXT("fov"));
 K.Cam=PointOf(S47J::Obj(O,TEXT("cam")));
 const S47J::FObj L=S47J::Obj(O,TEXT("look"));
 K.Look=PointOf(L);
 K.LookDist=K.Look.bSet?S47J::Num(L,TEXT("dist"),-1):-1;
 K.Ahead=PointOf(S47J::Obj(O,TEXT("ahead")));
 K.Indoors=S47J::Str(O,TEXT("indoors"),TEXT("unknown"));
 K.Clock=S47J::Str(O,TEXT("clock"));K.Day=S47J::Int(O,TEXT("day"));
 K.Weather=S47J::Strs(O,TEXT("weather"));
 const S47J::FObj P=S47J::Obj(O,TEXT("place"));
 K.Place=S47J::Str(P,TEXT("id"));K.PlaceDist=K.Place.IsEmpty()?-1:S47J::Num(P,TEXT("m"),-1);
 K.Draft=S47J::Str(O,TEXT("draft"));K.Picture=S47J::Str(O,TEXT("picture"));
 return K;
}

ERead Parse(const FString& Text,TArray<FS47DirectorMark>& Out)
{
 Out.Reset();
 if(Text.TrimStartAndEnd().IsEmpty())return ERead::Empty;
 TSharedPtr<FJsonObject> O;
 if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),O)||!O.IsValid())return ERead::Bad;
 const TArray<TSharedPtr<FJsonValue>>* List=nullptr;
 if(S47J::Int(O,TEXT("v"))!=1||!O->TryGetArrayField(TEXT("marks"),List)||!List)return ERead::Bad;
 for(const TSharedPtr<FJsonValue>& V:*List)
 {
  const TSharedPtr<FJsonObject>* E=nullptr;
  if(!V.IsValid()||!V->TryGetObject(E)||!E||!E->IsValid()){Out.Reset();return ERead::Bad;}
  Out.Add(FromJson(*E));
 }
 return ERead::Good;
}

FString Write(const TArray<FS47DirectorMark>& Marks)
{
 const S47J::FObj O=S47J::New();
 S47J::Set(O,TEXT("v"),1);
 TArray<TSharedPtr<FJsonValue>> List;
 for(const FS47DirectorMark& K:Marks)List.Add(MakeShared<FJsonValueObject>(ToJson(K)));
 O->SetArrayField(TEXT("marks"),List);
 return S47J::Write(O,true);
}

FString TargetFile(const FString& Main,ERead Read,const FString& Side){return Read==ERead::Bad?Side:Main;}

int32 Append(TArray<FS47DirectorMark>& Marks,FS47DirectorMark K)
{
 int32 Top=0;
 for(const FS47DirectorMark& X:Marks)Top=FMath::Max(Top,X.N);
 K.N=Top+1;
 Marks.Add(MoveTemp(K));
 return Top+1;
}

bool RemoveLast(TArray<FS47DirectorMark>& Marks,const FString& OfSession,int32& OutN)
{
 OutN=0;
 if(OfSession.IsEmpty())return false;
 for(int32 I=Marks.Num()-1;I>=0;--I)if(Marks[I].Session==OfSession)
 {
  OutN=Marks[I].N;
  Marks.RemoveAt(I);
  return true;
 }
 return false;
}

FS47ScriptAction MarkToShot(const FS47DirectorMark& K)
{
 FS47ScriptAction A;
 A.Do=ES47ScriptAct::Shot;
 A.At=0;A.Secs=5;
 A.Fov=K.Fov>0?FMath::Clamp(FMath::RoundToDouble(K.Fov),S47Shot::FovMin,S47Shot::FovMax):55.;
 if(K.Cam.bSet)A.Path.Add(K.Cam);
 A.Point=K.Look.bSet?K.Look:K.Ahead;
 return A;
}

FS47ScriptTrigger MarkToTrigger(const FS47DirectorMark& K)
{
 FS47ScriptTrigger T;
 T.Shape=ES47ScriptShape::Circle;T.Radius=8;
 T.Point=K.Look.bSet?K.Look:K.Cam;
 T.Point.Up=0;
 return T;
}

FString Line(const FS47DirectorMark& K)
{
 const FString At=K.Cam.bSet?FString::Printf(TEXT("at (%.0f, %.0f) +%.1f m"),K.Cam.X,K.Cam.Z,K.Cam.Up):FString(TEXT("off the land of the area"));
 const FString Looks=K.Look.bSet?FString::Printf(TEXT("looking at (%.0f, %.0f) %.1f m"),K.Look.X,K.Look.Z,K.LookDist):FString(TEXT("looking at nothing within 400 m"));
 const FString Near=K.Place.IsEmpty()?FString(TEXT("no place near")):FString::Printf(TEXT("%s %.0f m"),*K.Place,K.PlaceDist);
 return FString::Printf(TEXT("%s %s, %s, %s"),*K.Kind,*At,*Looks,*Near);
}

FString Path()
{
 const S47DirectorMarksLocal::FState& S=S47DirectorMarksLocal::St();
 if(!S.TestPath.IsEmpty())return S.TestPath;
 return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Director")/TEXT("marks.json"));
}

void SetPathForTest(const FString& File)
{
 S47DirectorMarksLocal::FState& S=S47DirectorMarksLocal::St();
 S.TestPath=File;
 S.bKnown=false;S.Marks.Reset();
}

FString Session()
{
 static const FString Name=FString::Printf(TEXT("%s-%u"),*FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")),FPlatformProcess::GetCurrentProcessId());
 return Name;
}

ERead ReadFile(const FString& File,TArray<FS47DirectorMark>& Out)
{
 Out.Reset();
 if(!FPaths::FileExists(File))return ERead::Empty;
 FString Text;
 if(!FFileHelper::LoadFileToString(Text,*File))return ERead::Bad;
 return Parse(Text,Out);
}

bool Add(FS47DirectorMark K,bool bPicture,FString& Said)
{
 FString File,Why;
 TArray<FS47DirectorMark> Marks;
 if(!S47DirectorMarksLocal::Open(File,Marks,Why))
 {
  Said=TEXT("no mark: ")+Why;
  UE_LOG(LogS47DirectorMarks,Warning,TEXT("S47: director mark not written: %s"),*Why);
  return false;
 }
 const bool bSide=File!=Path();
 K.Session=Session();
 K.Time=FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
 K.Picture.Reset();
 const int32 N=Append(Marks,MoveTemp(K));
 FS47DirectorMark& New=Marks.Last();
 if(bPicture)New.Picture=bSide?FPaths::GetBaseFilename(File)+FString::Printf(TEXT("-%03d.png"),N):FString::Printf(TEXT("mark-%03d.png"),N);
 if(!S47DirectorMarksLocal::Save(File,Marks))
 {
  Said=TEXT("no mark: could not write ")+File;
  UE_LOG(LogS47DirectorMarks,Warning,TEXT("S47: director mark not written: could not write %s"),*File);
  return false;
 }
 S47DirectorMarksLocal::Keep(Marks);
 if(bPicture)FScreenshotRequest::RequestScreenshot(FPaths::GetPath(File)/New.Picture,false,false);
 UE_LOG(LogS47DirectorMarks,Display,TEXT("S47: director mark %d: %s"),N,*Line(New));
 if(bSide)
 {
  UE_LOG(LogS47DirectorMarks,Warning,TEXT("S47: director marks: %s does not read, so it is left as it is; this mark went to %s"),*Path(),*File);
 }
 Said=FString::Printf(TEXT("mark %d (%s)"),N,*New.Kind);
 if(!New.Picture.IsEmpty())Said+=TEXT(" with ")+New.Picture;
 if(bSide)Said+=TEXT(": marks.json does not read, written to ")+FPaths::GetCleanFilename(File);
 return true;
}

bool RemoveLastOfSession(FString& Said)
{
 FString File,Why;
 TArray<FS47DirectorMark> Marks;
 if(!S47DirectorMarksLocal::Open(File,Marks,Why)){Said=TEXT("nothing taken back: ")+Why;return false;}
 int32 N=0;
 if(!RemoveLast(Marks,Session(),N)){Said=TEXT("nothing taken back: no mark of this session");return false;}
 if(!S47DirectorMarksLocal::Save(File,Marks)){Said=TEXT("nothing taken back: could not write ")+File;return false;}
 S47DirectorMarksLocal::Keep(Marks);
 UE_LOG(LogS47DirectorMarks,Display,TEXT("S47: director mark %d taken back (%s; its picture, if any, stays)"),N,*File);
 Said=FString::Printf(TEXT("mark %d taken back"),N);
 return true;
}

const TArray<FS47DirectorMark>& Known()
{
 S47DirectorMarksLocal::FState& S=S47DirectorMarksLocal::St();
 const FString Main=Path();
 if(!S.bKnown||S.KnownOf!=Main)
 {
  S.bKnown=true;S.KnownOf=Main;
  if(ReadFile(Main,S.Marks)==ERead::Bad)
  {
   if(S.SideOf==Main&&!S.Side.IsEmpty()){if(ReadFile(S.Side,S.Marks)==ERead::Bad)S.Marks.Reset();}
   else S.Marks.Reset();
  }
 }
 return S.Marks;
}

void Draw(US47Main* M,UWorld* World,const FVector& CamUE)
{
 using namespace S47DirectorMarksLocal;
 const TArray<FS47DirectorMark>& All=Known();
 if(!M||!World||All.IsEmpty())return;
 struct FSpot{FVector At=FVector::ZeroVector,Dir=FVector::ZeroVector;int32 N=0;double D2=0;};
 TArray<FSpot> Spots;
 Spots.Reserve(All.Num());
 for(const FS47DirectorMark& K:All)
 {
  if(!K.Cam.bSet)continue; // (no camera point: not on the land, nowhere to draw it from the area the player is in)
  FSpot Sp;
  Sp.At=S47Three::ToUE(FS47ScriptPlayer::WorldOfRoad(M,K.Cam));
  Sp.D2=FVector::DistSquared(Sp.At,CamUE);
  if(Sp.D2<NearCm*NearCm)continue;
  Sp.N=K.N;
  const FS47ScriptPoint& To=K.Look.bSet?K.Look:K.Ahead;
  if(To.bSet)Sp.Dir=(S47Three::ToUE(FS47ScriptPlayer::WorldOfRoad(M,To))-Sp.At).GetSafeNormal();
  Spots.Add(Sp);
 }
 if(Spots.Num()>MaxDrawn)
 {
  Spots.Sort([](const FSpot& A,const FSpot& B){return A.D2<B.D2;});
  Spots.SetNum(MaxDrawn);
 }
 const FColor Pink=FColor::Magenta;
 constexpr float Arm=30.f;
 for(const FSpot& Sp:Spots)
 {
  DrawDebugLine(World,Sp.At-FVector(Arm,0,0),Sp.At+FVector(Arm,0,0),Pink,false,-1.f,0,3.f);
  DrawDebugLine(World,Sp.At-FVector(0,Arm,0),Sp.At+FVector(0,Arm,0),Pink,false,-1.f,0,3.f);
  DrawDebugLine(World,Sp.At-FVector(0,0,Arm),Sp.At+FVector(0,0,Arm),Pink,false,-1.f,0,3.f);
  if(!Sp.Dir.IsNearlyZero())DrawDebugLine(World,Sp.At,Sp.At+Sp.Dir*120.f,Pink,false,-1.f,0,3.f);
  DrawDebugString(World,Sp.At+FVector(0,0,45),FString::Printf(TEXT("m%d"),Sp.N),nullptr,Pink,0.f,true,1.1f);
 }
}

bool SelfTest(TArray<FString>& Lines)
{
 bool bAll=true;
 const auto Check=[&](bool bOk,const FString& What){bAll&=bOk;Lines.Add(FString::Printf(TEXT("%s MARKS: %s"),bOk?TEXT("PASS"):TEXT("FAIL"),*What));};
 const auto Pt=[](double X,double Z,double Up){FS47ScriptPoint P;P.X=X;P.Z=Z;P.Up=Up;P.bSet=true;return P;};
 const auto Same=[](const FS47ScriptPoint& A,const FS47ScriptPoint& B){return A.bSet==B.bSet&&(!A.bSet||(FMath::IsNearlyEqual(A.X,B.X,.006)&&FMath::IsNearlyEqual(A.Z,B.Z,.006)&&FMath::IsNearlyEqual(A.Up,B.Up,.006)));};
 FS47DirectorMark Full;
 Full.N=7;Full.Kind=TEXT("view");Full.Note=TEXT("the porch light");Full.Session=TEXT("s-now");Full.Time=TEXT("2026-11-11 23:16:40");Full.Area=TEXT("saro");
 Full.Eye=FVector(12.5,4.25,-30.1);Full.Yaw=.7123;Full.Pitch=-.12;Full.Fov=70;
 Full.Cam=Pt(-310.4,1220.7,3.1);Full.Look=Pt(-325,1204.2,0);Full.LookDist=24.6;Full.Ahead=Pt(-323.6,1205.6,.7);
 Full.Indoors=TEXT("no");Full.Clock=TEXT("23:16");Full.Day=2;Full.Weather={TEXT("clear"),TEXT("calm")};
 Full.Place=TEXT("farm");Full.PlaceDist=140.2;Full.Draft=TEXT("script_1111");Full.Picture=TEXT("mark-007.png");
 FS47DirectorMark Bare;
 Bare.N=8;Bare.Session=TEXT("s-now");Bare.Eye=FVector(1,2,3);
 {
  TArray<FS47DirectorMark> Back;
  const ERead R=Parse(Write({Full,Bare}),Back);
  const bool bTwo=R==ERead::Good&&Back.Num()==2;
  const FS47DirectorMark& A=bTwo?Back[0]:Full;
  Check(bTwo&&A.N==7&&A.Kind==Full.Kind&&A.Note==Full.Note&&A.Session==Full.Session&&A.Time==Full.Time&&A.Area==Full.Area&&FVector::Dist(A.Eye,Full.Eye)<.01
   &&FMath::IsNearlyEqual(A.Yaw,Full.Yaw,1e-4)&&FMath::IsNearlyEqual(A.Pitch,Full.Pitch,1e-4)&&A.Fov==70&&Same(A.Cam,Full.Cam)&&Same(A.Look,Full.Look)
   &&FMath::IsNearlyEqual(A.LookDist,24.6,.006)&&Same(A.Ahead,Full.Ahead)&&A.Indoors==TEXT("no")&&A.Clock==Full.Clock&&A.Day==2&&A.Weather==Full.Weather
   &&A.Place==TEXT("farm")&&FMath::IsNearlyEqual(A.PlaceDist,140.2,.006)&&A.Draft==Full.Draft&&A.Picture==Full.Picture,TEXT("a mark reads back as it was written, field for field"));
  const FS47DirectorMark& B=bTwo?Back[1]:Full;
  Check(bTwo&&!B.Cam.bSet&&!B.Look.bSet&&B.LookDist<0&&!B.Ahead.bSet&&B.Place.IsEmpty()&&B.Picture.IsEmpty()&&B.Note.IsEmpty()&&B.Kind==TEXT("place")&&B.Indoors==TEXT("unknown"),
   TEXT("a mark without points, place, note or picture reads back without them (place and unknown by default)"));
 }
 {
  TArray<FS47DirectorMark> Was={Full,Bare};
  Was[0].N=1;Was[1].N=4;
  TArray<FS47DirectorMark> Marks;
  Parse(Write(Was),Marks);
  FS47DirectorMark Next;Next.Kind=TEXT("avoid");Next.N=99;
  const int32 N=Append(Marks,Next);
  TArray<FS47DirectorMark> Back;
  Check(N==5&&Parse(Write(Marks),Back)==ERead::Good&&Back.Num()==3&&Back[0].N==1&&Back[0].Kind==TEXT("view")&&Back[1].N==4&&Back[2].N==5&&Back[2].Kind==TEXT("avoid"),
   TEXT("appending keeps the earlier marks and numbers the new one after the highest"));
  TArray<FS47DirectorMark> None;
  Check(Append(None,Next)==1,TEXT("the first mark of a file is number 1"));
 }
 {
  TArray<FS47DirectorMark> Out;
  const bool bBad=Parse(TEXT("{\"v\":1,\"marks\":["),Out)==ERead::Bad&&Parse(TEXT("not json"),Out)==ERead::Bad&&Parse(TEXT("{\"v\":2,\"marks\":[]}"),Out)==ERead::Bad
   &&Parse(TEXT("{\"v\":1,\"marks\":[1]}"),Out)==ERead::Bad&&Parse(TEXT("{\"v\":1}"),Out)==ERead::Bad&&Parse(TEXT("[]"),Out)==ERead::Bad;
  Check(bBad&&Out.IsEmpty(),TEXT("a cut-off, foreign, newer or misshapen file reads as bad"));
  Check(Parse(TEXT(""),Out)==ERead::Empty&&Parse(TEXT(" \n"),Out)==ERead::Empty&&Parse(TEXT("{\"v\":1,\"marks\":[]}"),Out)==ERead::Good,TEXT("an empty file is as none; no marks is good"));
  Check(TargetFile(TEXT("marks.json"),ERead::Bad,TEXT("marks-1.json"))==TEXT("marks-1.json")&&TargetFile(TEXT("marks.json"),ERead::Good,TEXT("marks-1.json"))==TEXT("marks.json")
   &&TargetFile(TEXT("marks.json"),ERead::Empty,TEXT("marks-1.json"))==TEXT("marks.json"),TEXT("a file that does not read is never the one written: the side file is"));
 }
 {
  FS47DirectorMark Old;Old.Session=TEXT("s-old");Old.N=1;
  FS47DirectorMark Now2=Bare,Now3=Bare;Now2.N=2;Now3.N=3;
  TArray<FS47DirectorMark> Marks={Old,Now2,Now3};
  int32 N1=0,N2=0,N3=0;
  const bool b1=RemoveLast(Marks,TEXT("s-now"),N1),b2=RemoveLast(Marks,TEXT("s-now"),N2),b3=RemoveLast(Marks,TEXT("s-now"),N3);
  Check(b1&&N1==3&&b2&&N2==2&&!b3&&N3==0&&Marks.Num()==1&&Marks[0].Session==TEXT("s-old"),TEXT("taking back removes this session's marks, last first, never an earlier session's"));
  Check(!RemoveLast(Marks,FString(),N1)&&Marks.Num()==1,TEXT("no session name takes nothing back"));
 }
 {
  const FS47ScriptAction A=MarkToShot(Full);
  Check(A.Do==ES47ScriptAct::Shot&&A.Path.Num()==1&&Same(A.Path[0],Full.Cam)&&Same(A.Point,Full.Look)&&A.Fov==70&&A.Secs>0,TEXT("a shot from a mark: its eye at the camera, looking at the crosshair"));
  FS47DirectorMark NoHit=Full;NoHit.Look=FS47ScriptPoint();NoHit.Fov=0;
  const FS47ScriptAction B=MarkToShot(NoHit);
  Check(B.Path.Num()==1&&Same(B.Point,Full.Ahead)&&B.Fov==55,TEXT("without a crosshair hit the shot looks 20 m ahead (lens 55 without one)"));
  FS47DirectorMark Wide=Full;Wide.Fov=150;
  Check(MarkToShot(Wide).Fov==S47Shot::FovMax&&MarkToShot(Bare).Path.IsEmpty(),TEXT("the lens is held within the shot's; no camera point gives no eye"));
  const FS47ScriptTrigger T=MarkToTrigger(Full),U=MarkToTrigger(NoHit);
  Check(T.Point.bSet&&T.Point.X==Full.Look.X&&T.Point.Z==Full.Look.Z&&T.Point.Up==0&&T.Radius==8&&T.Shape==ES47ScriptShape::Circle&&T.Place.IsEmpty(),TEXT("a trigger from a mark: a circle of 8 m on the ground at the crosshair"));
  Check(U.Point.bSet&&U.Point.X==Full.Cam.X&&U.Point.Z==Full.Cam.Z&&U.Point.Up==0&&!MarkToTrigger(Bare).Point.bSet,TEXT("without a hit the trigger is under the camera; without either it has no point"));
 }
 Check(NextKind(TEXT("place"))==TEXT("view")&&NextKind(TEXT("avoid"))==TEXT("place")&&NextKind(TEXT("?"))==TEXT("place"),TEXT("0 cycles place, view, path, avoid"));
 return bAll;
}
}
