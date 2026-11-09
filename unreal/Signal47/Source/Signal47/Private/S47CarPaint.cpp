#include "S47CarPaint.h"
#include "S47StreetLamps.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MaterialShared.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RHIShaderPlatform.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47CarPaint, Log, All);

namespace S47CarPaintLocal
{
 const TCHAR* const LitPath = TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque");
 const TCHAR* const GlassPath = TEXT("/Game/Web/Base/M_Web_Lit_Trans_2S.M_Web_Lit_Trans_2S");
 const FName Tag(TEXT("S47CarPaint")), Skip(TEXT("S47CarPaintSkip"));
 double Scan = 0;
 TSet<FString> Logged; // a mesh's original slots are logged once a run
 // Whether a mesh is a car, worked out once a mesh (the scan looks at every static mesh the world has, every two seconds).
 TMap<TWeakObjectPtr<const UStaticMesh>, bool> MeshIsCar;
 bool CarMesh(const UStaticMesh* Mesh)
 {
  if (!Mesh) return false;
  if (const bool* B = MeshIsCar.Find(Mesh)) return *B;
  const FString P = Mesh->GetPathName();
  // (and SARO's two parked cars, made from the same free sedan and pickup: Tools/sarofleet/build_fleet.py)
  const bool B = P.StartsWith(TEXT("/Game/Free/Vehicles/")) || P.StartsWith(TEXT("/Game/Licensed/TripoFleet/police_car/")) || P.StartsWith(TEXT("/Game/Free/ServiceFleet/SM_SaroPark_"));
  if (MeshIsCar.Num() > 4096) MeshIsCar.Reset();
  MeshIsCar.Add(Mesh, B);
  return B;
 }

 bool Has(const FString& S, std::initializer_list<const TCHAR*> Words)
 {
  for (const TCHAR* W : Words) if (S.Contains(W)) return true;
  return false;
 }
 // The colour map a slot's original draws its base colour from: the glTF instances' BaseColorTexture, else the texture among the
 // material's own whose name says it is the base colour (the Tripo car's atlas, a repainted livery).
 UTexture* ColourMap(UMaterialInterface* M)
 {
  if (!M) return nullptr;
  UTexture* T = nullptr;
  if (M->GetTextureParameterValue(FHashedMaterialParameterInfo(TEXT("BaseColorTexture")), T) && T) return T;
  TArray<UTexture*> Used; M->GetUsedTextures(Used);
  for (UTexture* U : Used) if (U)
  {
   const FString N = U->GetName().ToLower();
   if (Has(N, { TEXT("basecolor"), TEXT("base_color"), TEXT("_s47") }) && !Has(N, { TEXT("normal"), TEXT("rough"), TEXT("metal"), TEXT("specular") })) return U;
  }
  return nullptr;
 }
 // The glTF instances' plain colour where they have no map (the El Alamo's near black paint).
 bool Factor(UMaterialInterface* M, FLinearColor& Out)
 {
  return M && M->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("BaseColorFactor")), Out);
 }
 // Whether the engine can draw a material here at all: a resource with a shader map for this platform, and not the default.
 // In the editor (-game included) shaders are compiled on demand, and a map that is there and drawn can still say it is not
 // finalized (S47FinaleSky ShaderReady): every slot read "no shader map" in the sweep of 9 November, the town's own master
 // (M_Web_Lit_Opaque, every building in Caliche) as much as the glTF instances. There it says which it is: "editor map" (there, not
 // finalized; drawn), "compiling", or "no shader map" (none at all). A cooked build's maps are finalized: there "ok" or not.
 FString Drawable(UMaterialInterface* M)
 {
  if (!M) return TEXT("none");
  if (M->GetName() == TEXT("WorldGridMaterial") || M->GetName() == TEXT("DefaultMaterial")) return TEXT("engine default");
  const FMaterialResource* R = M->GetMaterialResource(GMaxRHIShaderPlatform);
  if (!R) return TEXT("no resource");
  if (R->IsDefaultMaterial()) return TEXT("is default");
  if (R->HasValidGameThreadShaderMap()) return TEXT("ok");
#if WITH_EDITOR
  if (R->GetGameThreadShaderMap()) return TEXT("editor map");
  if (const UMaterial* Base = M->GetMaterial(); Base && Base->IsCompiling()) return TEXT("compiling");
#endif
  return TEXT("no shader map");
 }
 bool Draws(UMaterialInterface* M)
 {
  const FString D = Drawable(M);
  return D == TEXT("ok") || D == TEXT("editor map");
 }
 struct FLook { FLinearColor Colour = FLinearColor(.5f, .5f, .5f); float Rough = .5f, Metal = 0, Opacity = 1; bool bGlass = false, bKeepMap = true; };
 FLook LookOf(const FString& Role, bool bSheriff)
 {
  FLook L;
  if (Role == TEXT("paint")) { L.Colour = FLinearColor(1, 1, 1); L.Rough = .32f; }
  else if (Role == TEXT("atlas")) { L.Colour = FLinearColor(1, 1, 1); L.Rough = .45f; }
  else if (Role == TEXT("glass")) { L.Colour = FLinearColor(.035f, .045f, .05f); L.Rough = .05f; L.Opacity = .42f; L.bGlass = true; }
  else if (Role == TEXT("lamps")) { L.Colour = FLinearColor(1, 1, 1); L.Rough = .15f; }
  else if (Role == TEXT("tyre")) { L.Colour = FLinearColor(.55f, .55f, .55f); L.Rough = .88f; }
  else if (Role == TEXT("chrome")) { L.Colour = FLinearColor(.78f, .78f, .76f); L.Rough = .18f; L.Metal = 1; }
  else if (Role == TEXT("trim")) { L.Colour = FLinearColor(.75f, .75f, .75f); L.Rough = .7f; }
  else if (Role == TEXT("plate")) { L.Colour = FLinearColor(1, 1, 1); L.Rough = .4f; }
  else if (Role == TEXT("lens")) { L.Colour = FLinearColor(.75f, .14f, .1f); L.Rough = .12f; }
  else if (Role == TEXT("interior")) { L.Colour = FLinearColor(.85f, .85f, .85f); L.Rough = .8f; }
  else { L.Colour = FLinearColor(.8f, .8f, .8f); L.Rough = .6f; }
  (void)bSheriff;
  return L;
 }
 // Without a map: the colour a slot of this kind has when nothing else says (a tyre is black rubber, chrome is bright).
 FLinearColor Plain(const FString& Role)
 {
  if (Role == TEXT("tyre")) return FLinearColor(.025f, .025f, .025f);
  if (Role == TEXT("chrome")) return FLinearColor(.78f, .78f, .76f);
  if (Role == TEXT("trim")) return FLinearColor(.04f, .04f, .04f);
  if (Role == TEXT("interior")) return FLinearColor(.12f, .09f, .07f);
  if (Role == TEXT("lens")) return FLinearColor(.6f, .05f, .03f);
  if (Role == TEXT("lamps")) return FLinearColor(.8f, .78f, .7f);
  return FLinearColor(.3f, .3f, .3f);
 }
}

bool S47CarPaint::TripoPatrol() { return FParse::Param(FCommandLine::Get(), TEXT("S47TripoPatrol")); }

bool S47CarPaint::IsCar(const UStaticMeshComponent* C)
{
 return C && S47CarPaintLocal::CarMesh(C->GetStaticMesh().Get());
}

FString S47CarPaint::RoleOf(const FString& SlotName, const FString& MaterialName)
{
 using namespace S47CarPaintLocal;
 const FString S = (SlotName + TEXT(" ") + MaterialName).ToLower();
 if (Has(S, { TEXT("transperent"), TEXT("transparent"), TEXT("_glass"), TEXT("glass_door"), TEXT("window") }) && !S.Contains(TEXT("lights_and_glass ")) ) return TEXT("glass");
 if (Has(S, { TEXT("lights_and_glass") })) return TEXT("lamps");
 if (Has(S, { TEXT("tire"), TEXT("tyre"), TEXT("rubber") })) return TEXT("tyre");
 if (Has(S, { TEXT("hubcap"), TEXT("chrome"), TEXT("carbadges"), TEXT("badge") })) return TEXT("chrome");
 if (Has(S, { TEXT("numberplate"), TEXT("plate") })) return TEXT("plate");
 if (Has(S, { TEXT("lens") })) return TEXT("lens");
 if (Has(S, { TEXT("bodymat"), TEXT("paint") })) return TEXT("paint");
 if (Has(S, { TEXT("interior") })) return TEXT("interior");
 if (Has(S, { TEXT("bottom"), TEXT("trim"), TEXT("steel") })) return TEXT("trim");
 if (Has(S, { TEXT("_00") })) return TEXT("atlas");
 return TEXT("other");
}

int32 S47CarPaint::Dress(AS47Web* W)
{
 using namespace S47CarPaintLocal;
 if (!W || FParse::Param(FCommandLine::Get(), TEXT("S47CarPaintOff"))) return 0;
 UMaterialInterface* Lit = LoadObject<UMaterialInterface>(nullptr, LitPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
 UMaterialInterface* Glass = LoadObject<UMaterialInterface>(nullptr, GlassPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
 if (!Lit) return 0;
 TArray<UStaticMeshComponent*> All; W->GetComponents<UStaticMeshComponent>(All);
 int32 Done = 0;
 for (UStaticMeshComponent* C : All)
 {
  if (!IsCar(C) || C->ComponentTags.Contains(Tag) || C->ComponentTags.Contains(Skip)) continue;
  // A car someone has already painted all over for a scene of its own (the visitors' black sedan) is theirs. Critics, round 8: one
  // dynamic slot used to skip the whole car (wagon 23's white body left its glass, tyres and chrome grey); now only the slots that
  // already have a dynamic material are left as they are, slot by slot, and the rest are dressed.
  int32 Owned = 0;
  for (int32 I = 0; I < C->GetNumMaterials(); ++I) Owned += C->GetMaterial(I) && C->GetMaterial(I)->IsA<UMaterialInstanceDynamic>();
  if (Owned > 0 && Owned == C->GetNumMaterials()) { C->ComponentTags.Add(Skip); continue; }
  C->ComponentTags.Add(Tag);
  UStaticMesh* Mesh = C->GetStaticMesh();
  // Every LOD's slot draws what LOD0's slot of the same name draws (the street lamps' WorldGrid lesson, 3f946c47).
  const int32 Matched = S47StreetLamps::MatchLodSlots(C);
  const bool bSheriff = Mesh->GetPathName().Contains(TEXT("_cop")) || Mesh->GetPathName().Contains(TEXT("police_car"));
  const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
  TArray<FString> Was;
  // Slots of the same name share one dynamic material (a LOD's copy of a slot is the same paint).
  TMap<FString, UMaterialInstanceDynamic*> ByName;
  for (int32 I = 0; I < C->GetNumMaterials(); ++I)
  {
   UMaterialInterface* Old = C->GetMaterial(I);
   const FString SlotName = Slots.IsValidIndex(I) ? Slots[I].MaterialSlotName.ToString() : FString::Printf(TEXT("slot%d"), I);
   const FString Role = RoleOf(SlotName, Old ? Old->GetName() : FString());
   Was.Add(FString::Printf(TEXT("%s=%s(%s)->%s"), *SlotName, Old ? *Old->GetName() : TEXT("none"), *Drawable(Old), *Role));
   const FString Key = SlotName.Replace(TEXT("_LOD1"), TEXT("")).Replace(TEXT("_LOD2"), TEXT(""));
   // (somebody's own paint on this slot: kept, and a LOD's copy of the slot gets the same)
   if (auto* Theirs = Cast<UMaterialInstanceDynamic>(Old)) { if (!ByName.Contains(Key)) ByName.Add(Key, Theirs); Was.Last() += TEXT(" (kept)"); continue; }
   if (UMaterialInstanceDynamic** Same = ByName.Find(Key)) { C->SetMaterial(I, *Same); continue; }
   const FLook L = LookOf(Role, bSheriff);
   UMaterialInterface* Base = L.bGlass && Glass ? Glass : Lit;
   UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base, C);
   UTexture* Map = L.bKeepMap ? ColourMap(Old) : nullptr;
   FLinearColor Colour = L.Colour, F;
   if (Map)
   {
    // (the master has an sRGB and a linear sampler: the map goes to the one that matches it, or the engine would not sample it)
    const bool bLinear = !Map->SRGB;
    D->SetTextureParameterValue(bLinear ? TEXT("MapLinear") : TEXT("Map"), Map);
    D->SetScalarParameterValue(TEXT("MapIsLinear"), bLinear ? 1.f : 0.f); D->SetScalarParameterValue(TEXT("UseMap"), 1.f);
    // A glTF factor tints the map as it did in the original. Glass with a map of its own (the free cars' windows and lamp covers,
    // its alpha where it is clear): lightly tinted, its own alpha times the pane's.
    if (Factor(Old, F) && !L.bGlass) Colour = FLinearColor(Colour.R * F.R, Colour.G * F.G, Colour.B * F.B);
    if (L.bGlass) Colour = FLinearColor(.55f, .6f, .65f);
   }
   else Colour = Factor(Old, F) && !L.bGlass && Role != TEXT("tyre") ? FLinearColor(F.R, F.G, F.B) : L.bGlass ? L.Colour : Plain(Role);
   // The El Alamo's paint is a near black clear coat: a glossy dark paint, not a matt hole.
   const float Rough = Role == TEXT("paint") && Colour.GetLuminance() < .06f ? .22f : L.Rough;
   D->SetVectorParameterValue(TEXT("Color"), FLinearColor(Colour.R, Colour.G, Colour.B, 1));
   D->SetScalarParameterValue(TEXT("Roughness"), Rough);
   D->SetScalarParameterValue(TEXT("Metallic"), L.Metal);
   D->SetScalarParameterValue(TEXT("NativeLighting"), 1.f);
   D->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
   if (L.bGlass) D->SetScalarParameterValue(TEXT("Opacity"), Map ? .62f : L.Opacity);
   C->SetMaterial(I, D); ByName.Add(Key, D);
  }
  // The light bar's two halves (S47Props' beacon lenses on the car): red and blue glass, glossy, a little brighter than before.
  TArray<USceneComponent*> Kids; C->GetChildrenComponents(false, Kids);
  for (USceneComponent* K : Kids) if (auto* Lens = Cast<UStaticMeshComponent>(K))
  {
   const FString N = Lens->GetName();
   if (!N.StartsWith(TEXT("PropBeaconLens"))) continue;
   if (auto* Mid = Cast<UMaterialInstanceDynamic>(Lens->GetMaterial(0)))
   {
    Mid->SetVectorParameterValue(TEXT("Color"), N.EndsWith(TEXT("0")) ? FLinearColor(.62f, .025f, .02f) : FLinearColor(.03f, .07f, .62f));
    Mid->SetScalarParameterValue(TEXT("Roughness"), .1f);
   }
  }
  const FString Key = Mesh->GetPathName();
  if (!Logged.Contains(Key))
  {
   Logged.Add(Key);
   UE_LOG(LogS47CarPaint, Display, TEXT("S47 CARPAINT: %s: %d LOD slots matched; was %s"), *Mesh->GetName(), Matched, *FString::Join(Was, TEXT(", ")));
  }
  ++Done;
 }
 return Done;
}

void S47CarPaint::Step(AS47Web* W, double Dt)
{
 using namespace S47CarPaintLocal;
 Scan -= Dt;
 if (Scan > 0 || !W || !W->IsLoaded()) return;
 Scan = 2.0;
 const int32 N = Dress(W);
 if (N) UE_LOG(LogS47CarPaint, Display, TEXT("S47: car paint: %d car parts dressed in the town's materials"), N);
}

int32 S47CarPaint::Measure(AS47Web* W, TArray<FString>& Lines)
{
 using namespace S47CarPaintLocal;
 int32 Cars = 0, Bad = 0;
 TArray<FString> Each;
 if (W)
 {
  TArray<UStaticMeshComponent*> All; W->GetComponents<UStaticMeshComponent>(All);
  for (UStaticMeshComponent* C : All)
  {
   if (!IsCar(C) || !C->ComponentTags.Contains(Tag)) continue;
   ++Cars; int32 Left = 0;
   TArray<FString> Why;
   for (int32 I = 0; I < C->GetNumMaterials(); ++I)
   {
    UMaterialInterface* M = C->GetMaterial(I);
    // A slot is dressed when it draws a dynamic material that can be drawn here (S47CarPaint's own, on the town's master or glass,
    // or the one somebody painted the slot with for a scene of their own, kept).
    const bool bDynamic = Cast<UMaterialInstanceDynamic>(M) != nullptr;
    if (bDynamic && Draws(M)) continue;
    ++Left;
    if (Why.Num() < 3) Why.Add(FString::Printf(TEXT("%d %s(%s)"), I, M ? *M->GetName() : TEXT("none"), bDynamic ? *Drawable(M) : TEXT("its own")));
   }
   Bad += Left > 0;
   Each.Add(FString::Printf(TEXT("%s %s: %d slots, %d not on the town's materials%s"), Left ? TEXT("FAIL") : TEXT("PASS"), *C->GetStaticMesh()->GetName(), C->GetNumMaterials(), Left,
    Why.Num() ? *(TEXT(": ") + FString::Join(Why, TEXT(", "))) : TEXT("")));
  }
 }
 Lines.Add(FString::Printf(TEXT("%s car paint: %d car parts, %d still on their own materials"), Cars && !Bad ? TEXT("PASS") : TEXT("FAIL"), Cars, Bad));
 Lines.Append(Each);
 return Bad + (Cars ? 0 : 1);
}

bool S47CarPaint::SelfTest(TArray<FString>& Lines)
{
 bool bOk = true;
 const auto Check = [&](bool B, const FString& Why) { bOk &= B; Lines.Add(FString::Printf(TEXT("%s car paint: %s"), B ? TEXT("PASS") : TEXT("FAIL"), *Why)); };
 // The imported slot names of the cars in town (fairheaven_lt80_cop, el_alamo_78, the Tripo car).
 Check(RoleOf(TEXT("Fairheaven_LT80_Bodymat"), TEXT("Fairheaven_LT80_Bodymat")) == TEXT("paint") && RoleOf(TEXT("ElAlamo78_Bodymat"), TEXT("ElAlamo78_Bodymat")) == TEXT("paint"),
  TEXT("the body is paint"));
 Check(RoleOf(TEXT("UCB_Lights_and_Glass_Transperent"), TEXT("UCB_Lights_and_Glass_Transperent")) == TEXT("glass") && RoleOf(TEXT("M_TripoFleet_police_car_Glass_Door"), TEXT("")) == TEXT("glass"),
  TEXT("the windows are glass"));
 Check(RoleOf(TEXT("UCB_Lights_and_Glass"), TEXT("UCB_Lights_and_Glass")) == TEXT("lamps"), TEXT("the lamp atlas keeps its map, not glass"));
 Check(RoleOf(TEXT("RB1c_Tire_1k"), TEXT("RB1c_Tire_1k")) == TEXT("tyre") && RoleOf(TEXT("M_TripoFleet_police_car_Hubcap"), TEXT("")) == TEXT("chrome"),
  TEXT("tyres and hubcaps"));
 Check(RoleOf(TEXT("M_TripoFleet_police_car_00"), TEXT("")) == TEXT("atlas") && RoleOf(TEXT("M_TripoFleet_police_car_Lens"), TEXT("")) == TEXT("lens")
  && RoleOf(TEXT("UCB_BOTTOM_GT"), TEXT("UCB_BOTTOM_GT")) == TEXT("trim") && RoleOf(TEXT("Numberplates_Misk_U"), TEXT("")) == TEXT("plate"),
  TEXT("the Tripo atlas, lenses, underbody and plates"));
 return bOk;
}
