#pragma once
#include "CoreMinimal.h"

class UPoseableMeshComponent;
// Only the explicitly enabled CreaturePreview framebuffer recorder uses this adapter.
class FS47CreatureCapture
{
public:
 struct FSample
 {
  double PreviewTime=0;
  int32 Stage=-1;
  bool bStarted=false,bRendered=false;
  TWeakObjectPtr<UPoseableMeshComponent> Skin;
 };
 static TSharedPtr<FS47CreatureCapture> Create(TFunction<FSample()> Sample);
 struct FFrameScope
 {
  FS47CreatureCapture* Capture=nullptr;
  ~FFrameScope() { if(Capture)Capture->OnFrame(); }
 };
private:
 struct FState;
 TSharedPtr<FState> State;
 void OnFrame();
 bool Poll();
 void Finish(bool Complete,const TCHAR* Reason);
};
