> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Niagara i SIGNAL / 47: undersøkelse og forslag

28. oktober 2026. Grunnlag: worktree `~/scratch-bm/wt/sc02` på `eaf9914e` (gren fra main), prosjektet `unreal/Signal47` og lokal UE 5.8.4 i `~/.local/opt/UnrealEngine-5.8.4`. Bare lesing av kilde, konfigurasjon, motorens plugins og dokumentene. Ingen Unreal, editor, bygg, pakking eller filendring i repoet. Tall for kostnad er anslag, ikke målinger.

## Kort svar

1. Niagara er allerede lastet og lenket inn i spillet, men brukes ikke. Det står ikke i `Signal47.uproject`, men `Niagara.uplugin` har `EnabledByDefault: true`, og HairStrands (som prosjektet slår på) krever Niagara. Det monolittiske spillbygget har `NIAGARA_API`, `NIAGARASHADER_API` og `NIAGARAVERTEXFACTORIES_API` i `Intermediate/Build/Linux/x64/UnrealGame/Development/Signal47/Definitions.Signal47.h`. `Signal47.Build.cs` har ikke Niagara som avhengighet. Ingen kode bruker `UNiagaraComponent`, Cascade eller `UParticleSystem`, og Content har ingen `NS_`- eller `BP_`-assets.
2. Alle visuelle effekter lages i dag som runtime C++-komponenter (`NewObject` og `RegisterComponent`). Prosjektet har allerede en egen «Niagara-lite»: møll, flaggermus, støv i lommelykt og frontlys, tumbleweeds og arkivstøv tegnes med instanserte quads og vertex-animerte materialer. Niagara bør brukes der den gir noe disse ikke gir: fysiske bursts med tyngde og sprett, buer og stråler, GPU-skyer med tusenvis av partikler, og per-effekt skalering.
3. Vanlig UE 5.8.4-Python kan ikke bygge et Niagara-system (bare lage tomme assets eller duplisere). Men 5.8.4 har den eksperimentelle `NiagaraToolsets`, som prosjektet allerede slår på gjennom `AllToolsets`. Den kan lage systemer fra mal, legge til emittere, moduler og renderere, sette stack-verdier og User-parametre, og lese kompileringsstatus. Den kan kalles fra Python med `unreal.ToolsetRegistry.execute_tool(...)`. Dette er den mest lovende reproduserbare løypa, men den er ikke prøvd i dette prosjektet.
4. Første implementering: en kort lysbue med gnister fra trafoens busstøtteisolatorer i det øyeblikket den bleke kroppen dukker opp i SC02. Ikke under T01, som skal være normal. CPU-simulert burst, C++-eid blitslys, ingen RT, ingen skygger, ingen nye lagringsfelt, testet gjennom eksisterende T01-, trafo- og SC02-kontrakter.

## 1. Dagens tilstand

### Plugin og avhengigheter

| Punkt | Funn |
|---|---|
| `Signal47.uproject` | Plugins: ModelContextProtocol, AllToolsets, PythonScriptPlugin, RigLogic, HairStrands, ControlRig, MetaHumanSDK. Niagara er ikke oppført. |
| Motorens `Plugins/FX/Niagara/Niagara.uplugin` | `EnabledByDefault: true`, moduler Niagara, NiagaraCore, NiagaraShader, NiagaraVertexFactories (runtime) og NiagaraEditor (editor). |
| `HairStrands.uplugin` og `HairStrandsCore.Build.cs` | Krever Niagara og NiagaraCore/NiagaraShader. Niagara følger derfor uansett med i pakken. |
| `Signal47.Build.cs` | Ingen Niagara-modul. Må legges til (`"Niagara"` i PrivateDependencyModuleNames) før C++ kan bruke `UNiagaraComponent`. |
| Kode og assets | Ingen treff på Niagara, Cascade eller ParticleSystem i `Source`, `Config`, `Tools` eller `Scripts`. Ingen NS_/BP_ i Content. Eneste klasse lastet fra asset er servitrisens AnimInstance (`S47Waitress.cpp:54`). |
| Cook | Bare `/Engine/Maps/Entry` som kart. Alt annet lastes ved sti og må ligge i `DirectoriesToAlwaysCook` i `Config/DefaultGame.ini`. |

### Hvordan effekter lages i dag

Komponenttyper laget i C++ (antall `NewObject`): 33 StaticMesh, 10 SpotLight, 9 PointLight, 9 InstancedStaticMesh, 4 HISM, 4 MaterialBillboard, 5 RectLight, ett VolumetricCloud, ett Groom.

Partikkellignende effekter som allerede finnes, uten Niagara:

- `S47NightLife.cpp` (port av `world/nightlife.ts`): møll og flaggermus rundt natriumlamper og verandalys (`M_WebMoths`, én ISM-sky per område med 14 custom data floats og bevegelse i vertex-shaderen), støv i lommelyktstrålen (900) og i frontlysene (3000) (`M_WebMotes`), tumbleweeds med vindkast, fjerne lys. Low gir færre møll og ingen flaggermus (`S47NightLife.cpp:330` og `:358`). Komponentene har `SetVisibleInRayTracing(false)` og ingen skygger.
- `S47Annex.cpp:158` til `:260`: 192 støvkorn som drysser fra en skjøt, CPU-integrert, ISM-quads vendt mot kamera, prosedyral rund tekstur fra `S47Image::FromPixels`.
- `S47Glow`: glødebillboards. `S47Sky`: stjerner som ISM. `S47SkyVisitors`: MaterialBillboard. `S47Abduction`: punktlys og prosedyral frosttekstur.

Konklusjon: mønsteret er «C++ eier tilstand, tid og lys, materialet eller komponenten tegner». Niagara bør gå inn i det samme mønsteret, ikke erstatte det.

## 2. Rammer og reproduserbar laging

### 2.1 Rammer som styrer valgene

- Offentlig repo. Bare originalt eller CC0. Kjøpte assets under `/Game/Licensed` legges aldri inn. Teksturer til effekter bør lages prosedyralt (materialmatematikk eller generert PNG), slik Annex og frosten gjør.
- Pakket Linux, Vulkan SM6. Effekter lastes ved sti og må inn i cook-listen.
- RT AlwaysResident har ligget på 76,3 til 79,8 MiB mot varselgrensen 80 MiB (`SC01_RUNTIME.md`, `T02_RUNTIME.md`, `NIGHT_PRODUCTION_RUNTIME.md`). Niagara tar med sprites, ribbons og meshpartikler i ray tracing som standard (`r.RayTracing.Geometry.NiagaraSprites=1` i `NiagaraRendererSprites.cpp:34`). Alle Niagara-komponenter må få `SetVisibleInRayTracing(false)`.
- Low setter bare utvalgte `sg.*`-variabler (`S47Lighting.cpp:226`) og slår av Lumen GI og volumetrisk tåke. `sg.EffectsQuality` settes ikke. Den må heller ikke settes for Niagaras skyld: `EffectsQuality@0` i `BaseScalability.ini:798` setter også `r.MaterialQualityLevel=0`, endrer himmelatmosfæren og slår av FSSS. Det ville endre alle materialer og gi nye PSO-er. Bruk heller `fx.Niagara.QualityLevel` direkte, eller la C++ skalere antall via User-parametre etter `Flow->Quality`, slik NightLife gjør.
- PSO-cache: prosjektet har et omfattende opptak og forkompilering (`DefaultEngine.ini`, `Scripts/pso-cache.py`). Hvert nytt partikkelmateriale gir nye PSO-er for Niagara sprite-vertexfactory. Uten opptak risikerer vi hakk første gang gnistene vises, altså nøyaktig i skremmeøyeblikket.
- TSR (`r.AntiAliasingMethod=4`), ingen motion blur, ingen autoeksponering, bloom på. Små lyse additive sprites kan smøres ut av TSR. Strek må lages med hastighetsorienterte sprites, og lysstyrken må stilles mot den faste eksponeringen (trafolampens emissive er `Warm * 4`).
- Tilgjengelighet: `Flow->bCalmFlash` (redusert blinking). Lysbuer og blits må ha en rolig variant uten strobing.
- Testkulturen: effekter skal kunne avleses som faktiske verdier (jf. `FS47Substation::RenderedResult` som leser materialparameter og lysstyrke), ha egne av-flagg for A/B (jf. `-S47SubstationDetailOff`) og være inerte når en asset mangler.
- Pause-menyen bruker `SetGamePaused`, så Niagara står stille der. Dokumentpaneler pauser ikke verden; effektlogikken bør derfor ligge i Step-funksjoner som allerede respekterer `ModalOpen()`.

### 2.2 Hva UE 5.8.4 faktisk tillater

Vanlig Python (PythonScriptPlugin):

- `UNiagaraSystemFactoryNew` og `UNiagaraEmitterFactoryNew` lager tomme assets. Feltene `SystemToCopy` og `EmittersToAddToNewSystem` er ikke UPROPERTY (`NiagaraSystemFactoryNew.h:22`), og dermed utilgjengelige fra Python.
- `UNiagaraSystem` og `UNiagaraEmitter` har ingen BlueprintCallable-funksjoner. I NiagaraEditor er bare `NiagaraBakerFunctionLibrary` og `UpgradeNiagaraScriptResults` skripteksponert.
- Lightweight (stateless) emittere er rene dataobjekter (`UNiagaraStatelessEmitter` med `SpawnInfos`, `Modules`, `RendererProperties`, `FixedBounds`, `RandomSeed`, `bDeterministic`), og kan kjøre på CPU eller GPU uten eget skript per system. Modulene og `FNiagaraStatelessSpawnInfo` er ikke BlueprintType, så Python får ikke ordinære attributter for dem (`PyGenUtil.cpp:1537` og `:1609`). Automatisk laging av slike er derfor UNVERIFIED.
- `EditorAssetLibrary.duplicate_asset` kan kopiere maler, men da blir assetet en avledning av Epics malsystem.
- Materialer kan lages fullt ut med `MaterialEditingLibrary`, slik alle `Tools/*/import_assets.py` allerede gjør. Bruksflaggene `MATUSAGE_NIAGARA_SPRITES` og `MATUSAGE_NIAGARA_RIBBONS` settes som i `Tools/material_usage.py`.

Nytt i 5.8.4, allerede aktivert:

- `Engine/Plugins/Experimental/Toolsets/NiagaraToolsets` (Experimental, EditorOnly) er med i `AllToolsets`, som `Signal47.uproject` slår på. `UNiagaraToolset_System` (`NiagaraToolset_System.h`) har blant annet `CreateNiagaraSystem(navn, sti, malsystem)`, `AddEmitter`, `AddModule`, `RemoveModule`, `SetModuleEnabled`, `SetStackInputData`, `AddRenderer`, `SetRendererData`, `SetEmitterData`, `SetSystemData`, `AddUserVariables`, `GetSystemSummary`, `GetUserVariables`, `GetSystemCompileState` og `GetStackIssues`. `UNiagaraToolset_Component` kan sette User-variabler på komponenter.
- Kalles fra Python via `unreal.ToolsetRegistry.execute_tool(toolset, verktøy, json)` (`UToolsetRegistry.h:45`), som gir et asynkront resultat med `is_complete`, `value` og `error`. Skjemaene kan dumpes med `unreal.ToolsetRegistry.get_all_toolset_json_schemas()`.
- De samme operasjonene finnes synkront i C++ som `FNiagaraExternalSystemEditorUtilities` (`NiagaraEditor/Public/NiagaraExternalSystemEditorUtilities.h`, eksportert med NIAGARAEDITOR_API). Det er reserveløypa hvis Python-kallene ikke fullføres i commandlet.

Maler i motoren (`/Niagara/DefaultAssets/Templates`): systemene DirectionalBurst, DirectionalBurstLightweight, FountainLightweight, MinimalLightweight, RadialBurst, SimpleExplosion; emitterne Minimal, SimpleSpriteBurst, OmnidirectionalBurst, DirectionalBurst, Fountain, HangingParticulates, BlowingParticles, DynamicBeam, StaticBeam, RecycleParticlesInView m.fl.

### 2.3 Anbefalt løype

Rangert etter hvor godt den passer prosjektets «alt fra kode og skript»:

| Løype | Hva | Fordel | Ulempe |
|---|---|---|---|
| A, anbefalt å prøve først | `Tools/fx/import_fx.py` kjøres som de andre importene (`UnrealEditor-Cmd -run=pythonscript`, med `unreal_guard require-idle`). Lager originale materialer med MaterialEditingLibrary og bygger systemer med NiagaraToolsets fra en JSON-oppskrift i repoet. `verify_fx.py` leser tilbake sammendrag, User-parametre og kompileringsstatus og skriver PASS/FAIL-rapport. | Hele effekten er tekst i repoet. Følger eksisterende import/verify-mønster. | Eksperimentelt API. Asynkrone kall og kompilering i commandlet er ikke prøvd. |
| B, reserve | Liten editor-modul `Signal47Editor` (Type Editor) med en commandlet som bruker `FNiagaraExternalSystemEditorUtilities` synkront og `UNiagaraSystem::WaitForCompilationComplete()`. | Synkron, full kontroll, ingen async-pumping. | Ny modul i uproject, eget editorbygg, mer kode å vedlikeholde. |
| C | Ett system lages for hånd i Niagara-editoren, og `verify_fx.py` låser kontrakten (User-parametre, renderer, materiale, simulering, bounds, maks antall). | Rask første effekt. | Selve laging er ikke reproduserbar. |
| D, uten Niagara | Gnistene som ISM-quads med C++-integrert ballistikk, som Annex-støvet, pluss C++-lys. | Ingen ny teknologi, kjent PSO- og pakkeoppførsel. | Ingen buer, ingen GPU-skyer, mer egen kode per effekt. |

Felles regler uansett løype:

- Kontrakt mellom C++ og asset: navngitte User-parametre (for eksempel `User.SparkScale`, `User.SparkColor`, `User.GroundZ`, `User.Direction`). C++ eier tid, tilstand, lys, antall og tilfeldig frø (`SetRandomSeedOffset`). Systemene har faste bounds og `bAutoActivate=false`.
- Små bursts (under omtrent 200 partikler) simuleres på CPU: ingen compute-PSO, deterministiske, lettere å teste. Store skyer (støv i stråler) på GPU.
- Komponentene: `SetVisibleInRayTracing(false)`, `SetCastShadow(false)`, `bAffectDistanceFieldLighting=false`, ingen kollisjon. Kollisjon for gnister mot bakken som analytisk plan med `User.GroundZ`, ikke scene-traces.
- Lys fra effekter lages som vanlige C++-lys (uten skygger på Low), ikke Niagara Light Renderer. Da kan testene lese faktisk lysstyrke, og `bCalmFlash` styres ett sted.
- Lisens: bygg fra `Minimal`-emitteren og legg til Niagaras egne moduler ved referanse, med egne verdier og egne materialer. Unngå å kopiere ferdige Epic-malsystemer inn i det offentlige repoet. Dette er ikke juridisk vurdert, men det holder repoet på samme side som dagens bruk av `/Engine/BasicShapes`.
- Ny cook-linje: `+DirectoriesToAlwaysCook=(Path="/Game/Signal47/FX")`. Niagaras egne moduler under `/Niagara` følger som avhengigheter.
- PSO: legg en substasjonsvisning med aktiv effekt inn i `pso-cache.py record`, og forvarm systemet usynlig ved installasjon bak lasteskjermen.

## 3. Prioriterte bruksområder

Kostnad på Low er anslag ut fra partikkelantall og skjermdekning. Alle skal måles med samme varme GPU-vindu som i `ATMOSPHERE_2026-10-27.md`.

### 1. Trafo: lysbue og gnister når lyset krysser linjen (SC02)

- Hvor: `FS47Substation` (`S47Substation.cpp`, `Install()` linje 81 og `Step()` linje 239), utløst av `M->Horror->Threat` (`S47HorrorThreat.cpp:373`, der kroppen dukker opp ved posten). Ankre fra `FS47Substation::Site()` på busstøtteisolatorene i Blender-koordinater `(x, 3.25, 7.69)` med x lik -1.04, 0 og 1.04 (`Tools/substation/geometry.py:85`).
- Verdi: Historien sier at «et lys går på skrå gjennom stolpene» når kroppen står der (`HISTORIE_SC02_SC13_REVISJON.md`, SC02). Det lyset er ikke laget i koden. En overslag med knitring i gården foran spilleren trekker blikket mot gjerdet idet kroppen dukker opp bak dem. Det er en ærlig, diegetisk oppmerksomhetsmagnet og et pålitelig tegn i Exit 8-ånd: lysbue betyr at noe er kommet.
- Low: 20 gnister CPU, ett C++-punktlys uten skygge i under 1,5 s. Neglisjerbart.
- Risiko: må aldri skje under T01, ellers motsier den «alt er normalt». Må ikke gi en verdslig forklaring som undergraver T02. PSO-hakk i skremmeøyeblikket hvis ikke forvarmet.

### 2. Den bleke kroppens fotfall i grusen (SC02, senere SC04, SC08, SC11)

- Hvor: `FS47HorrorThreat::Step` og gangfasen `Phase += Moved / Stride * 2 * PI` (`S47HorrorThreat.cpp:461`). Utløs et lite støvdrag når fasen passerer et fotfall og kroppen står på caliche eller grus.
- Verdi: Gjør spillerens lesning av posisjon og retning bedre i lommelyktstrålen, og støtter regelen om at trussellyd kommer fra faktisk posisjon. Et kraftig bilde: støv som løfter seg der ingen fot var synlig.
- Low: 6 til 10 sprites per fotfall, maks 3 samtidige drag. Liten.
- Risiko: Må synkroniseres med fotlyden. Overdreven støv kan se tegneserieaktig ut.

### 3. Det fremmede lyset: støv og grus i lyssøylen (SC03 Ortiz, SC11 SARO)

- Hvor: SC03 er ikke bygget ennå. Når det bygges: der Ortiz løftes ved bilen, og ved SC11-avsøkningen i senere `S47Horror`-kode. Bruk GPU-sprites i en kjegle, og noen få meshpartikler (grus) som løftes.
- Verdi: Høy. På Low er volumetrisk tåke av (`S47Lighting.cpp:244`), så en lyskjegle er usynlig i lufta. Opplyste støvkorn gjør strålen lesbar på alle nivåer, og løftende grus og termosen som ruller gir en fysisk, konkret skrekk.
- Low: 300 til 600 GPU-sprites mens sekvensen varer. Overdraw må holdes nede med avstandsfading mot kamera.
- Risiko: GPU-simulering gir compute-PSO-er. Må forvarmes. Sekvensen er ikke designet i kode ennå.

### 4. Kulden før bortføringen: pust og iskrystaller

- Hvor: `S47Abduction.cpp`, varslingen (ved :31 kommer trykket og lommelykten begynner å hakke). Frostringen finnes allerede (`S47Abduct::Frost`).
- Verdi: «Kulden kommer før lyset» binder varselet til frosten spilleren ser etterpå. Pusten blir synlig i lommelyktstrålen noen sekunder før :40.
- Low: under 50 sprites festet foran øyet med lav dekning.
- Risiko: Pust foran kamera kan se billig ut og plage under TSR. Billigere alternativ: gi de eksisterende 900 lommelyktsstøvkornene (`M_WebMotes`) et iskrystall-glimt via materialparameter, uten Niagara.

### 5. Vindkast med caliche-støv langs bakken

- Hvor: `FS47NightLife::TumbleNow` og `WeedStep` (vindkastene som allerede sender tumbleweeds), med retning og styrke fra `S47Wind::Wind()`.
- Verdi: Middels. Gir vindlyden et synlig motstykke på åpne strekninger og parkeringsplassen, og forsterker stillheten når vinden dør i varselet.
- Low: 100 til 200 GPU-sprites i et kort drag nær bakken, bare mens et kast pågår.
- Risiko: Overlapper delvis med eksisterende støv i frontlys. Må holdes lavt og sjeldent.

### 6. SARO 07: dekkstøv på grusveien og eksos i kald luft

- Hvor: `S47Truck`/`S47Drive` (hastighet og om bilen står på servicesporet). Eksos ved motorstart i T01-returen og flukten i SC02.
- Verdi: Middels. Gjør bilen fysisk, og eksos i baklyset når motoren går og pusten er tung er et godt bilde i SC02-avslutningen.
- Low: Dekkstøv bare over en viss fart, 100 til 300 GPU-sprites, av når bilen står.
- Risiko: Mange områder og kjøretester berøres. Gjør dette senere.

### 7. Mørkerommet: støv i forstørrerens lyskjegle

- Hvor: `S47LabLighting.cpp:52` og `:128` (forstørrerens lysdriver) og `S47Chapter1.cpp:826` (printing).
- Verdi: Lav til middels. Et svært fotografisk bilde, og det understreker at rommet er stille.
- Low: under 100 CPU-sprites i et lite volum.
- Risiko: Lav. Kan like gjerne løses med Annex-mønsteret (ISM), så Niagara er ikke nødvendig her.

### Bevisst utelatt

- Møll og insekter rundt natriumlamper: finnes allerede (`M_WebMoths`). Ikke dupliser.
- CRT og statisk støy: er Slate (`SS47Crt`) og skjermmaterialer. Niagara passer ikke.
- Kjemisk damp i mørkerommet: fotokjemi ved romtemperatur damper ikke synlig. Det ville være uærlig.
- Gnist ved blackout 02:29 (`S47SaroDark`): en synlig sikringsgnist gir en verdslig forklaring på at bare SARO går mørkt, og svekker scenen.
- Trafolampens T02-sekvens (lampen lyser etter isolasjon): gnister ville antyde en ordinær feilstrøm og blande seg inn i fotobeviset.

## 4. Minimal første implementering: trafognister

### 4.1 Oppførsel

- Aldri i T01 og aldri i gamle saker eller legacy. Ingen ny elektrisk spillmekanikk og ingen nye lagringsfelt.
- Utløses én gang når kroppen dukker opp ved posten og møtet ennå ikke er sett (`Threat->Appeared()` går fra false til true og `!Threat->Seen()`). Etter fangst og ny start, eller Continue midt i møtet med sett forvarsel, kommer den ikke igjen. Alt avledes av eksisterende SC02-tilstand.
- Forløp på omtrent 1,2 s: kort hvit-blå blits ved én av de tre busstøtteisolatorene, 40 gnister (20 på Low) som faller omtrent 7,7 m, spretter én gang på grusen innenfor gjerdet og slukner. Trafolampen dipper samtidig kort (bare via eksisterende `Apply()`-verdier, ikke ny tilstand). Deretter stillhet.
- `bCalmFlash`: én myk puls uten flimring, færre gnister, samme timing.
- Valgfritt senere: sjeldne små knitringer mens kroppen søker eller jager nær gjerdet.

### 4.2 Asset

- `/Game/Signal47/FX/NS_S47_ArcSparks`: CPU-simulering, én emitter for gnister (burst, kjegle nedover og utover, tyngde, luftmotstand, plankollisjon på `User.GroundZ`, hastighetsorienterte sprites, farge og størrelse over levetid) og eventuelt en kort blitssprite (0,12 s). Faste bounds rundt portalbjelken. `bDeterministic` og frø fra C++.
- User-parametre: `User.SparkScale` (0 til 1), `User.SparkColor`, `User.GroundZ`, `User.Direction`.
- `/Game/Signal47/FX/M_S47_FxSpark`: unlit additiv, rund maske i materialet uten tekstur, `MATUSAGE_NIAGARA_SPRITES`.
- Lages av `Tools/fx/import_fx.py` fra `Tools/fx/fx_contract.json`, kontrolleres av `Tools/fx/verify_fx.py`, startes via `Tools/fx/import.sh` (samme oppbygning som `Tools/substation/import.sh`).

### 4.3 Filer

| Fil | Endring |
|---|---|
| `unreal/Signal47/Signal47.uproject` | Valgfritt: `{ "Name": "Niagara", "Enabled": true }` uttrykkelig. |
| `Source/Signal47/Signal47.Build.cs` | `"Niagara"` i PrivateDependencyModuleNames. |
| `Source/Signal47/Public/S47Fx.h`, `Private/S47Fx.cpp` (ny) | Liten hjelper: laster system ved sti, lager `UNiagaraComponent` med RT av, ingen skygge, ingen kollisjon, `bAutoActivate=false`, festet til gitt forelder. `-S47FxOff` gir inert fallback. Logger «ready» eller «absent». Antallsskala fra `Flow->Quality`. |
| `Private/S47Substation.cpp`, `Public/S47Substation.h` (eller ny `S47SubstationArc.cpp`) | Ankre fra `Site()`, komponent og blitslys i `Install()`, utløsning i `Step()`, ren konvoluttfunksjon og selvtest i `SelfTest()`. |
| `Config/DefaultGame.ini` | `+DirectoriesToAlwaysCook=(Path="/Game/Signal47/FX")`. |
| `Tools/fx/` (ny) | `README.md`, `fx_contract.json`, `import_fx.py`, `verify_fx.py`, `import.sh`. |
| `Content/Signal47/FX/` (ny, LFS) | Genererte assets. |
| `Private/S47EveningServiceTest.cpp` | T01-regresjon: gnistene er inaktive og blitslyset er 0 gjennom hele T01. |
| `Private/S47HorrorTest.cpp` | SC02-kontroll rundt trinn 107 til 110. Denne fila har ucommittede endringer i sc02-worktreet nå, så SC02-eieren må koordineres. |
| `unreal/FX_NIAGARA.md`, `log.md`, `todo.md` | Dokumentasjon og logg. |

Kjernen i hjelperen (skisse):

```cpp
UNiagaraComponent* S47Fx::Make(AS47Web* W, USceneComponent* Parent, const TCHAR* Path, const TCHAR* Name)
{
 if (!W || !Parent || FParse::Param(FCommandLine::Get(), TEXT("S47FxOff"))) return nullptr;
 UNiagaraSystem* S = LoadObject<UNiagaraSystem>(nullptr, Path);
 if (!S) { UE_LOG(LogS47Fx, Display, TEXT("S47: fx %s absent; inert"), Name); return nullptr; }
 UNiagaraComponent* C = NewObject<UNiagaraComponent>(W, Name);
 C->SetAsset(S); C->SetAutoActivate(false); C->SetupAttachment(Parent);
 C->SetCastShadow(false); C->SetVisibleInRayTracing(false); C->bAffectDistanceFieldLighting = false;
 C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->RegisterComponent();
 return C;
}
```

### 4.4 Rekkefølge og prøving

Hver tung jobb kjøres alene gjennom `Scripts/unreal_guard.py`.

1. Prøve (én editor-cmd-kjøring, ingen bygg): et lite Python-skript som sjekker `unreal.ToolsetRegistry.is_available()`, dumper alle toolset-skjemaer til scratch, lager et system fra `Minimal` i `/Game/Developers/FxProbe` (ikke committet), legger til en spriterenderer, venter på kompilering og lagrer. Svarer på de største ukjente før noe annet skrives. Feiler det, gå til løype B eller C.
2. Import og verifikasjon av `NS_S47_ArcSparks` og materialet (`Tools/fx/import.sh`), rapport med PASS per kontroll.
3. C++ og editorbygg, deretter Linux-pakke.
4. Selvtest (`-S47SelfTest`): konvolutten er deterministisk, rolig modus har ingen ny topp etter første (ingen strobing), Low-antall er under taket, og utløsningsregelen er sann bare for (møte, dukket opp, ikke sett).
5. `-S47T01Contract` med isolert `-UserDir`: alle eksisterende kontroller uendret, pluss at gnistkomponenten aldri er aktiv og blitslyset er 0 gjennom journal, service, lampe og arkivering.
6. `-S47SubstationTest` og `-S47SubstationTestLoad`: fysiske blikkplukk på journal, bryter og lampe treffer fortsatt (komponentene har ingen kollisjon), og diskreload starter ingen gnister.
7. `-S47HorrorContract=SC02` på High og `-S47Quality=low`: ved trinn 108 skal komponenten bli aktiv én gang, blitslyset ha en topp over 0 og være 0 etter 2 s. Etter `StoreRestore("sc02-mid-meeting")` skal den ikke utløses på nytt. Nytt skjermbilde `sc02-arc`, og samme med `bCalmFlash`.
8. Ytelse og minne: A/B med `-S47FxOff` i samme pakke og samme GPU-vindu. RT AlwaysResident skal være uendret og under 80 MiB.
9. PSO: ny opptakstur ved trafoen om natten med aktiv effekt i `pso-cache.py record`, deretter kald start med vanlig lasting og kontroll av at første gnist ikke gir hakk.
10. Hele `Scripts/screenshots.sh` før fletting, som AGENTS.md krever.

### 4.5 Ærlige ukjente

- Om `ToolsetRegistry.execute_tool` er tilgjengelig og fullfører i `-run=pythonscript` uten at motoren tikker, særlig `GetSystemCompileState`, og om lagring venter på Niagara-kompilering. Avklares i trinn 1.
- Nøyaktige toolset-navn og JSON-skjemaer. NiagaraToolsets er Experimental og kan endres i en 5.8-hotfix.
- Om Niagara-kompilering og lagring går med `-nullrhi`. CPU-skript bør gå; GPU-emittere er usikre.
- Lisensstatus for systemer avledet av Epics maler i et offentlig repo er ikke juridisk vurdert. Forslaget unngår derfor kopier av ferdige malsystemer.
- Partikkelantall kan ikke leses gjennom et stabilt offentlig API. Testene bør bruke aktiv tilstand, alder (`GetSystemInstanceController`), User-parametre og C++-lyset, ikke antall.
- Hvordan små additive sprites ser ut med TSR, fast eksponering, VHS- og filmbildene og tåke. Krever faktiske bilder på High og Low.
- PSO-oppførsel ved første aktivering på en kald Vulkan-cache.
- Om Tom vil at overslaget skal være det synlige uttrykket for «lyset gjennom stolpene», eller om lyset også skal være en egen synlig kilde på himmelen.
- SC02 er under aktivt arbeid i dette worktreet. Kroken må passe med den endelige møtelogikken.

## 5. Kilder

- Prosjekt: `Signal47.uproject`, `Signal47.Build.cs`, `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`, `S47Substation.cpp/.h`, `S47SubstationDetail.cpp`, `S47SubstationFollowup.cpp`, `S47Horror.cpp/.h`, `S47HorrorThreat.cpp/.h`, `S47HorrorTest.cpp`, `S47EveningServiceTest.cpp`, `S47NightLife.cpp/.h`, `S47Annex.cpp`, `S47Abduction.h`, `S47SaroDark.h`, `S47SaroPower.h`, `S47Lighting.cpp`, `S47Flow.cpp`, `S47Wind.cpp`, `Tools/substation/*`, `Tools/substationdetail/import_assets.py`, `Tools/material_usage.py`, `Scripts/pso-cache.py`, `Scripts/screenshots.sh`.
- Dokumenter: `SUBSTATION.md`, `SUBSTATION_SITE.md`, `SUBSTATION_DETAIL.md`, `T01_RUNTIME.md`, `T02_RUNTIME.md`, `SC01_RUNTIME.md`, `ATMOSPHERE_2026-10-27.md`, `RENDERING_RESEARCH_2026-10-26.md`, `HORROR_STORY_PLAN.md`, `MISSION_FLOW_1800_ROSWELL.md`, `HISTORIE_SC02_SC13_REVISJON.md`, `PHOTOLAB_DETAIL.md`, `AGENTS.md`.
- Motor: `Plugins/FX/Niagara` (uplugin, maler, `NiagaraSystemFactoryNew.h`, `NiagaraComponent.h`, `NiagaraRendererSprites.cpp`, `Internal/Stateless`), `Plugins/Experimental/Toolsets/NiagaraToolsets`, `Plugins/Experimental/ToolsetRegistry` (C++ og `Content/Python/toolset_registry`), `Plugins/Experimental/PythonScriptPlugin/.../PyGenUtil.cpp`, `Config/BaseScalability.ini`.
- Prosjektbiblioteket ble søkt («Niagara particles Unreal C++» ga null treff; bredere søk ga generelle Unreal MCP-profiler, blant annet `EpicGames/unreal-engine-skills-for-claude-code-plugin` som peker på samme toolset-løype). Brukt bare som pekepinn; ingen kode derfra er tatt inn eller verifisert lokalt.
