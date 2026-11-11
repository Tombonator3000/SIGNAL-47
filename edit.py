import sys
def sub(path, pairs):
    s=open(path).read()
    for a,b in pairs:
        assert s.count(a)==1,(a,s.count(a))
        s=s.replace(a,b)
    open(path,'w').write(s)

sub('Public/S47DoorLoop.h',[
("static const TCHAR* ThoughtFor(int32 N,bool bFromFront);",
 "// N1110 (critic): Variant (0 or 1, by the night) picks one of two texts of each; 4 and 5 are the ceiling and the floor.\n static const TCHAR* ThoughtFor(int32 N,bool bFromFront,int32 Variant=0);\n static int32 VariantFor(int32 Seed,int32 Night);\n // N1110 (critic): where along the corridor the ceiling spot is, by night (inside the corridor, clear of its ends).\n static double CeilingX(int32 Seed,int32 Night);\n static int32 ColorShiftFor(int32 Seed,int32 Night); // which of the four colours the light starts with"),
("static FLinearColor FloodColor(double Seconds,bool bCalm);","static FLinearColor FloodColor(double Seconds,bool bCalm,int32 Shift=0);"),
("static const TCHAR* ChaseThought(int32 N);","static const TCHAR* ChaseThought(int32 N);\n static const TCHAR* NotThatOne(){return TEXT(\"Not that one.\");} // N1110 (critic): said with the handle of step 3 when there are two doors"),
(" int32 StepsLeft=0,StepsTotal=0,KnobGate=-1;"," int32 StepsLeft=0,StepsTotal=0,KnobGate=-1,LastKnob=-1,ColorShift=0; // N1110 (critic): LastKnob, the door heard rattling at step 3: using it later in the chase is being caught\n double StepsLift=0;\n bool bFaded=false;"),
(" void Footsteps(int32 Gate,int32 Count);"," void Footsteps(int32 Gate,int32 Count,bool bOverhead=false);"),
])

sub('Private/S47DoorLoop.cpp',[
# fade in Use
(" Pending=CutAfter;PendingStep=S;PendingFrom=Door;\n return true;\n}",
 " Pending=CutAfter;PendingStep=S;PendingFrom=Door;\n if(bChase&&LastKnob>=0&&Door==LastKnob&&S!=EStep::Out){Caught();return true;} // N1110 (critic): the door it was trying\n FadeDown();\n return true;\n}"),
# thought fn
("const TCHAR* FS47DoorLoop::ThoughtFor(int32 N,bool bFromFront)\n{\n switch(N)\n {\n case 1:return bFromFront?TEXT(\"That was the front door.\"):TEXT(\"That was the service door.\");\n case 2:return TEXT(\"No.\");\n case 3:return TEXT(\"I went out. I know I went out.\");\n default:return TEXT(\"\");\n }\n}",
 """const TCHAR* FS47DoorLoop::ThoughtFor(int32 N,bool bFromFront,int32 Variant)
{
 const bool B=(Variant&1)!=0;
 switch(N)
 {
 case 1:return B?(bFromFront?TEXT("I was just out of the front door."):TEXT("I was just out of the service door.")):(bFromFront?TEXT("That was the front door."):TEXT("That was the service door."));
 case 2:return B?TEXT("That is wrong."):TEXT("No.");
 case 3:return B?TEXT("I know I went out. I felt the air."):TEXT("I went out. I know I went out.");
 case 4:return B?TEXT("I am up by the ceiling."):TEXT("The ceiling?");
 case 5:return B?TEXT("Up through the floor."):TEXT("Underneath. That is not possible.");
 default:return TEXT("");
 }
}

int32 FS47DoorLoop::VariantFor(int32 Seed,int32 Night){return FMath::Clamp(FMath::FloorToInt(FS47NightRhythm::Roll(Seed,Night,S47DoorLoopLocal::Salt+5)*2.f),0,1);}

// The corridor runs x -5.75 to 1.75; the spot stays within .9 m of the usual one, well clear of both ends and of the doors at them.
double FS47DoorLoop::CeilingX(int32 Seed,int32 Night){return S47DoorLoopLocal::CeilX+(2.*FS47NightRhythm::Roll(Seed,Night,S47DoorLoopLocal::Salt+4)-1.)*.9;}

int32 FS47DoorLoop::ColorShiftFor(int32 Seed,int32 Night){return FMath::Clamp(FMath::FloorToInt(FS47NightRhythm::Roll(Seed,Night,S47DoorLoopLocal::Salt+3)*4.f),0,3);}"""),
# burst low
("<.5?1.7:.08;","<.5?1.7:.3;"),
# color
("FLinearColor FS47DoorLoop::FloodColor(double Seconds,bool bCalm)\n{","FLinearColor FS47DoorLoop::FloodColor(double Seconds,bool bCalm,int32 Shift)\n{"),
("FloodColors[FMath::Clamp(FMath::FloorToInt(Into*StrobeRate),0,1000)%4];","FloodColors[(FMath::Clamp(FMath::FloorToInt(Into*StrobeRate),0,1000)+Shift)%4];"),
(" const int32 A=FMath::Clamp(FMath::FloorToInt(U),0,3);\n const double K=FMath::Clamp((U-A-(1.-Blend))/Blend,0.,1.);\n return FLinearColor::LerpUsingHSV(FloodColors[A],FloodColors[(A+1)%4],",
 " const int32 A=FMath::Clamp(FMath::FloorToInt(U),0,3),C=(A+Shift)%4;\n const double K=FMath::Clamp((U-A-(1.-Blend))/Blend,0.,1.);\n return FLinearColor::LerpUsingHSV(FloodColors[C],FloodColors[(C+1)%4],"),
("Frame.Color=FloodColor(FloodT,bCalm);","Frame.Color=FloodColor(FloodT,bCalm,ColorShift);"),
# chase thought
(" case 3:return TEXT(\"Somebody is trying the handle.\");\n default:return TEXT(\"\");",
 " case 3:return TEXT(\"Somebody is trying the handle.\");\n case 4:return TEXT(\"The ceiling?\");\n case 5:return TEXT(\"Underneath. That is not possible.\");\n default:return TEXT(\"\");"),
# Start: color shift
("  ChaseDay=F->Day();\n  YieldHunt();","  ChaseDay=F->Day();\n  ColorShift=ColorShiftFor(F->Seed(),FS47FreeformSession::NightOf(F->Clock()));\n  YieldHunt();"),
# Restore
("bWarned=false;bChase=false;\n if(bDimmed","bWarned=false;bChase=false;LastKnob=-1;StepsLift=0;ColorShift=0;\n if(bFaded){bFaded=false;if(M&&M->UI)M->UI->Fade(false);}\n if(bDimmed"),
# Cut ceiling x
(" if(S==EStep::Ceiling&&Where.bUp&&M->Player->Walkable(Where.Up.X,Where.Up.Z)){X=Where.Up.X;Z=Where.Up.Z;if(Where.bUpYaw)Yaw=Where.UpYaw;bSpot=true;}",
 " if(S==EStep::Ceiling&&Where.bUp)\n {\n  // N1110 (critic): in the SARO corridor the spot moves along it by night; where that cannot be walked, the usual one.\n  const double Ux=Where.Kind==EPlace::Saro?CeilingX(F->Seed(),FS47FreeformSession::NightOf(F->Clock())):Where.Up.X;\n  if(M->Player->Walkable(Ux,Where.Up.Z)){X=Ux;Z=Where.Up.Z;if(Where.bUpYaw)Yaw=Where.UpYaw;bSpot=true;}\n  else if(M->Player->Walkable(Where.Up.X,Where.Up.Z)){X=Where.Up.X;Z=Where.Up.Z;if(Where.bUpYaw)Yaw=Where.UpYaw;bSpot=true;}\n }"),
("  if(M->Audio)M->Audio->Beep(46.f,float(RiseSecs)+.6f,.06f,0.f,TEXT(\"sine\"));",
 "  if(M->Audio)\n  {\n   M->Audio->Beep(46.f,float(RiseSecs)+.6f,.06f,0.f,TEXT(\"sine\"));\n   M->Audio->Beep(92.f,float(RiseSecs)+.6f,.03f,0.f,TEXT(\"sine\")); // N1110 (critic): the octave above, so a small speaker has it\n  }"),
("  if(N==3)Knob(Other,1.6,1.1);",
 "  if(N==4)Footsteps(Other,3,true); // N1110 (critic): over the ceiling\n  if(N==3)\n  {\n   Knob(Other,1.6,1.1);\n   LastKnob=Where.Gates.Num()>1?Other:-1;\n  }"),
# said
(" const FString Said=bChase?FString(ChaseThought(N)):FString(ThoughtFor(N,From==0));",
 " FString Said=bChase?FString(ChaseThought(N)):FString(ThoughtFor(N,From==0,VariantFor(F->Seed(),FS47FreeformSession::NightOf(F->Clock()))));\n if(bChase&&N==3&&LastKnob>=0)Said+=TEXT(\" \")+FString(NotThatOne());"),
(" M->Player->Place(X,Z,Yaw);\n"," M->Player->Place(X,Z,Yaw);\n if(bFaded){bFaded=false;if(M->UI)M->UI->Fade(false);} // N1110 (critic): the view comes back up out of the dark\n"),
# Footsteps
("void FS47DoorLoop::Footsteps(int32 Gate,int32 Count)\n{\n if(!Where.Gates.IsValidIndex(Gate)||Count<=0)return;",
 "void FS47DoorLoop::Footsteps(int32 Gate,int32 Count,bool bOverhead)\n{\n if(!Where.Gates.IsValidIndex(Gate)||Count<=0)return;\n StepsLift=0;"),
(" StepsA=Out+Along*3.5;StepsB=Out-Along*3.5;StepsLeft=StepsTotal=Count;StepsIn=.5;",
 " StepsA=Out+Along*3.5;StepsB=Out-Along*3.5;StepsLeft=StepsTotal=Count;StepsIn=.5;\n // Overhead: over the ceiling of the place, along it, heavier and slower, 2.6 m above the floor.\n if(bOverhead&&Where.bUp){StepsA=Where.Up+FVector(-3,0,0);StepsB=Where.Up+FVector(3,0,0);StepsLift=2.6;StepsIn=.8;}"),
("FS47Sound{.Gain=.75f,.Rate=FMath::FRandRange(.62f,.8f),.At=FVector(At.X,Floor+.1,At.Z)}","FS47Sound{.Gain=.75f,.Rate=FMath::FRandRange(.62f,.8f),.At=FVector(At.X,Floor+.1+StepsLift,At.Z)}"),
# crossing: defer with fade
("  const EStep S=Run.Press();\n  GateShut(Where,Door);\n  Cut(S,Door);\n  return;\n }\n // Anything bigger",
 "  const EStep S=Run.Press();\n  GateShut(Where,Door);\n  if(bChase&&LastKnob>=0&&Door==LastKnob&&S!=EStep::Out){Caught();return;} // N1110 (critic)\n  Pending=CutAfter;PendingStep=S;PendingFrom=Door;FadeDown(); // N1110 (critic): a blink of dark, then the cut\n  return;\n }\n // Anything bigger"),
("  const EStep S=Run.Press();\n  if(S==EStep::Out){Break(Door);return;}\n  GateShut(Where,Door);\n  Cut(S,Door);\n  return;",
 "  const EStep S=Run.Press();\n  if(S==EStep::Out){Break(Door);return;}\n  GateShut(Where,Door);\n  if(bChase&&LastKnob>=0&&Door==LastKnob){Caught();return;} // N1110 (critic): the door it was trying\n  Pending=CutAfter;PendingStep=S;PendingFrom=Door;FadeDown(); // N1110 (critic): a blink of dark, then the cut\n  return;"),
])
