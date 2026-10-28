> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../STATUS.md).

# Porteringen av hele spillet til C++

Tom valgte 22. oktober 2026 en ren C++-port: all spillogikk fra `web/src` skrives om i C++ under `unreal/Signal47/Source/Signal47`. Verdenen (geometri, materialer, lys, soner) hentes fortsatt fra nettversjonen med `Tools/webexport`; det er oppførselen som porteres.

Denne fila er arbeidsinstruksen for porteringen, for Claude og for hjelpere (egne underagenter og Codex). Les også `AGENTS.md` i denne mappa.

## Prinsippet: én TypeScript-fil blir én C++-fil

En portert modul skal kunne leses ved siden av TypeScript-originalen linje for linje. Samme navn på klasser, felt og funksjoner (i Unreal-stil: `this.s.stage` blir `S.Stage`, `begin()` blir `Begin()`), samme rekkefølge, samme tall, samme tekst. Ikke forbedre, ikke forenkle, ikke slå sammen. Avvik som må til (en Slate-detalj, en Unreal-begrensning) skrives som en kort kommentar der de er.

| TypeScript | C++ |
|---|---|
| `story/Chapter1.ts` | `S47Chapter1.h/.cpp`, klassen `US47Chapter1 : US47Chapter` |
| `story/boardRules.ts` | `S47BoardRules.h/.cpp` |
| `ui/Panels.ts` | `S47Panels.h/.cpp` (funksjoner i navnerommet `S47Panels`) |
| `ui/Board.ts` | `SS47Board.h/.cpp` (Slate-widget) |
| `core/FieldCamera.ts` | `S47FieldCamera.h/.cpp` |
| `world/ServiceYard.ts` (oppførselen) | `S47Yard.h/.cpp` |
| `main.ts` | `US47Main` (`S47Main.h/.cpp`) og verten `AS47Story` |

Spilltekst er engelsk og kopieres nøyaktig. Kommentarer i koden er engelske, i samme stil som resten av modulen: korte, og de sier hvilken TypeScript-fil og funksjon koden kommer fra.

## Koordinater, farger og tid

- All spillogikk regner i nettversjonens rom: meter, y opp, kameraet ser langs -Z, Euler-vinkler i radianer i XYZ-rekkefølge. `FVector(x, y, z)` betyr three sin (x, y, z). Bare tegningen regnes om (`S47Three::ToUE` og `ToThree`).
- Farger er lineære som i three. Et heksadesimalt tall fra koden (`color.set(0x0d1410)`) blir `S47Three::Hex(0x0d1410)`.
- Klokka (`host.clock`) er sekunder etter midnatt. Spilltid (`host.gt`) går bare når ingenting er åpent. Tidtakere går på spilltid: `Host->After(2.2, [this] { ... }, TEXT("lab"))`.

## Grunnlaget (API-et modulene bruker)

Hver portert modul har `US47Main* M` (det `main.ts` gir som `deps`) og kapitlene i tillegg `AS47Story* Host` (det `Prologue.ts` er for kapitlene).

**Verdenen, `M->Web` (`AS47Web`, `S47Obj.h`)**
- `M->Web->Obj(TEXT("room.objs.mug"))` gir et `FS47Obj`. Stiene er de samme som i TypeScript. Alle stier eksporten fant står under `handles` i `Content/WebData/manifest.json`; `PathsUnder(prefix)` lister dem. Et håndtak som ikke finnes, gjør ingenting.
- `FS47Obj`:
  - `Position`/`SetPosition`/`SetX`/`SetY`/`SetZ`, `Rotation`/`SetRotation`/`SetRotX`/`SetRotY`/`SetRotZ`, `Quaternion`, `Scale`, `Visible`/`SetVisible`, `Shown()`.
  - `WorldPosition`, `WorldQuaternion`, `WorldDirection`, `WorldBox`, `LookAt`, `Find(name)`, `Children`, `Traverse`.
  - `SetColor`, `SetEmissive`, `SetOpacity`, `SetMap(texture)`: materialet på det objektet og alt under det; hvert får sin egen kopi.
- `M->Web->Light(path)` gir et `FS47Light` (`SetIntensity`, `SetColor`, `SetVisible`, `SetPosition`).
- `M->Web->Material(path)` gir et `FS47Mat`, et delt materiale som endres overalt.
- `M->Web->Vec(path)` gir en `Vector3` koden holder. `M->Web->Rect(path)` gir en sone `{minX, maxX, minZ, maxZ}` som `FS47Area`.
- Flomlysene: `FloodSet(key)`, `SetFlood(set, index, width)`, `SetFloodColor`, `SetFloodScale` (som `kit.ts` `setFlood`). En lampe som flytter seg (lyktene på bilen): `SetFloodPool(set, index, pos, width, colour, false)` for hver plass og `FlushFlood(set)` til slutt.
- En instans i et `InstancedMesh` (hjulene, viserne): `FS47Obj::SetInstance(k, pos, quat, scale)`, som `setMatrixAt`.
- Bilen og kjøringen: `M->World` (`US47World`, som World.ts) med `DriveOut`, `DriveBack`, `DriveToMotel`, `DriveToDiner`, `Roam`, `DriveOldRoad`, `CabTruck()`, `CabDrive()`, `Trouble` og `EngineSound()`. Det eksporten vet om kjøringen (hindringene, lyktenes flomlysplasser) står i `M->Web->DriveData()`.
- Dørene, antennene, kontrollrommet: setterne i `S47Web.h` (`ToggleDoor`, `PointDishes`, `SetLever` ...).

**Ting man kan bruke, `M->Inter` (`US47Inter`, som `core/Interaction.ts`)**
- `M->Inter->Add({ Id, Object, Label, Use, Range, Examine, ExamineRange, bLook })`: `Label` returnerer tom streng når tingen er treg akkurat nå.
- `Get(id)`, `Looks` (`story/looks.ts`). Siktet plukker med samme regler som `pick()` og `examine()`.

**Skjermen, `M->UI` (`US47UI`, som `ui/UI.ts`)**
- `Toast(text, secs)`, `ClearToasts`, `Objective(text)`, `Examine(text)`, `Fade(on, text)`, `ClockSeconds(text)`.
- `Document(FS47Doc, onClose)`. `FS47Doc` er `DocSpec`: `Kind` (`hand`, `typed`, `printout`, `photo`, `map`), `Stamp`, `Page`, `Transcript`, `Image`, `Paper`, `PaperRatio`, `Sheets`.
- `Open(widget, onClose)`, `Close()`, `Modal()`: ett panel om gangen, og spilltiden står mens det er åpent.
- `Viewfinder(on, FS47Viewfinder)`, `ShutterFlash()`, `CameraButton()`, `Prompt(label, alt)`.

**Panelene (`S47Kit.h`, som `.labpanel` og `.lp-*` i `style.css`)**
- `Shell(UI, title, body)`: panelet med overskrift og Close.
- Tekst: `Text(s, tone)`, `Sub`, `Small`, `Question`, `Paper`.
- Elementer: `Steps(items, active)`, `Button(label, fn, primary)`, `Card(title, line, fn)`, `Slider`, `Image(texture, width)`.
- Oppsett: `Row`, `Col`, `Two`, `Cards`.

Et panel fra `ui/Panels.ts` bygges av disse bitene i samme rekkefølge som HTML-en.

**Bilder (`S47Image.h`)**
- Fotografier og tegninger lagres som data-URL-er (JPEG/PNG), som i nettversjonen.
- `FromDataUrl` gir en tekstur, `ToDataUrl` lager en data-URL av piksler, `ReadPixels` leser et render target, og `Art(name)` henter bildene fra `core/art.ts`.

**Lagring (`S47Json.h`)**
- `SaveState()` returnerer samme JSON som `serialize()` i TypeScript, med de samme feltnavnene, og `Begin(saved)` leser den.
- Hjelperne i `S47J` brukes til dette: `Str`, `Num`, `Bool`, `Uv`, `Strs`, `Set`, `SetUv`, `SetOrNull`.

**Kapitlene (`S47Chapter.h`)**
- `US47Chapter` har `Begin(saved)`, `Reset()`, `SaveState()`, `Update(dt, t)`, `Objective()`, `Tasks()`, `SaveBlock()`, `DoorChanged(id, open)` og `Key()`.
- Verten `AS47Story` har `Clock()`/`SetClock`, `Gt()`, `After(s, fn, tag)`, `Cancel(tag)`, `Note(s)`, `AddDoc(doc)` (erstatter etter id), `Toast`, `IsCinematic`/`SetCinematic` og `Chapters`.

**Lyd, `M->Audio` (`US47Audio`, som `core/Audio.ts`)**
- Lyden går gjennom en liten Web Audio i C++ (`S47WebAudio.h`): `FS47AudioContext` med noder og `AudioParam`-automasjon som i nettleseren, spilt av én `USynthComponent` (`S47Synth`). Grafene i en modul porteres linje for linje: `ctx.createGain()` er `Ctx->CreateGain()`, `g.gain.setTargetAtTime(0, t, 0.3)` er `G->Gain.SetTargetAtTime(0, T, 0.3)`, `o.type = 'square'` er `O->SetType(TEXT("square"))`. Hold `FS47AudioContext::FBatch B(*Ctx);` mens en graf bygges (som én JavaScript-oppgave). `setTimeout` er `Ctx->SetTimeout`, `onended` er `Src->OnEnded`; begge kjøres på spilltråden.
- Funksjonene i `Audio.ts` har sine egne navn med posisjoner i three.js-mål: `Sfx(name, FS47Sound)` er `play()` og gir `{Src, Gain}`, `LoopSfx` er `loop()`, `Beep`, `Hum`, `Servo`, `Shutter`, `PhoneCall`, `Dial`, `Ringback`, `CabRadio`, `RadioIn()`, `Spatial(at, ref)`, `NoiseBuffer()`, `Pressure()`, `Night` (`core/ambience.ts`) og resten. Bussene er `Master`, `SfxBus` (`sfx`), `Music`, `Amb` og `RadioBus`.
- Opptakene har nettversjonens navn (`phoneRing`, `thudSoft`, `stepConcrete0`, `lic:door_heavy`, `phone:<id>`) og dekodes fra WAV-filene `Scripts/import-audio.sh` skriver til `Content/Audio/Raw` og `Content/Licensed/Raw`. Stemmebankene (Kinski, Serling, Art Bell, ensemblet) importeres som lydassets under `/Game/Audio/Voice/<Bank>/v_<fil>`, med katalogene i `Content/Audio/Raw/voices.json`.
- Prologens gamle navn (`Play(name, gain, atUE, rate)`, `Loop`, `Stop`, `StartCarrier` ...) virker fortsatt og går gjennom de samme grafene.

**Lagring og flyt, `M->Flow` (`US47Flow`, main.ts sin flyt)**
- Et kapittel ber om autolagring med `M->RequestAutosave()` (Prologue-deps `saveCase`) og starter en ny generasjon med `Host->OnCheckpoint(TEXT("chapterN"))` (`milestone`); `BeginChapter` gjør det siste selv for kapittel 1 til 5.
- Kapitlets `SaveState()` havner i saken under `s` (kapittel 1) eller `ch2` til `ch5` når kapitlet er begynt (`Host->Begun`); `Host->LoadCase()` gir saken som åpnes, så hoppet til kapitlet kan gjøre som `jumpChapterN`.
- `SaveBlock()` i kapittel 3 til 6 sier hvorfor det ikke kan lagres nå (tom streng: det kan lagres).
- Notatboken (`SS47Notebook`) åpnes med Tab; Board-fanen bruker `M->Board.Open` når saksbrettet finnes.

## Rekkefølgen

1. Grunnlaget (dette): håndtakene, registeret, grensesnittet, panelbitene, JSON, bilder. Ferdig 22. oktober.
2. Bølge 1, parallelt:
   - kapittel 1 med feltkameraet, mørkerommet og tjenestegården;
   - kapittel 2 med arkivfløyen og telefonsamtalene;
   - saksbrettet (`Board.ts`, `boardRules.ts`, `caseBoard.ts`);
   - lyden (de syntetiske lydene og stemmene);
   - lagring, tittel, pause og notatbok.
3. Bølge 2: kjøringen (`drive/`), kapittel 3 (STATION 01) og kapittel 4 (motellet, rom 6).
4. Bølge 3: kapittel 5 og 6, den gamle veien, slutten.
5. Bølge 4: nattens systemer (kurven, hendelsene, radioen, albumet, glimtene, bortføringen og resten fra `main.ts` sin `step()`).

Hver bølge går som egne grener og PR-er, og Tom fletter.

## For hjelpere

- Du jobber på din egen gren eller arbeidskopi. Rør bare filene oppgaven din nevner. Delte filer (`S47Story.*`, `S47HUD.*`, `S47Main.*`, `Signal47.Build.cs`) endres bare der oppgaven sier det, og da så lite som mulig.
- Les TypeScript-originalen helt før du skriver. Bruk testene i `web/tools/chapterN.py` som fasit for hvordan kapittelet spilles.
- Kompiler med `Scripts/build-editor.sh` fra `unreal/Signal47`. Ingen advarsler: modulen bygges med advarsler som feil.
- Lever med en kort rapport: hvilke filer, hva som er portert, hva som mangler eller avviker, og hvordan det er prøvd.
