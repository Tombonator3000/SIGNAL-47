> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Port av Caliche, veien og motellets natt

Arbeidsgren: `port-town`, utgangspunkt `6b55c8f`. Repoet ble kontrollert som PUBLIC. Ingen push eller PR er utført. Nettversjonen og Unity-arkivet er uendret.

## Filer og innhold

Alle filpar ligger under `unreal/Signal47/Source/Signal47/{Public,Private}`. Geometrien kommer fra nettversjonens eksport.

| TypeScript | C++ | Portert |
| --- | --- | --- |
| `story/townEvents.ts` | `S47TownEvents.*` | Begge payphone-samtalene, 47 sekunder, mynten, reléklikkene, lampene og hundenes rekkefølge, bilen, pappfiguren og kartmerkene |
| `world/Town.ts` | `S47Town.*` | Personenes klokkeslett, bil og dør, aviser, røyk, vindusbilder, lamper, lysdeling, gulvhøyde og overflater |
| `core/roadside.ts` | `S47Roadside.*` | Ledningslyd, fjernbil, 47 sekunders stillhet, skikkelsen på motelltaket og silhuetthåndtaket |
| `story/pasture.ts` | `S47Pasture.*` | Ni på 026, åtte og frostringen, passeringer, notater og delte lagringsnøkler |
| `drive/herd.ts` | `S47Herd.*` | Kropps- og hodeinstanser, beiteposering, øyeglimt, lyktekjegle og `Set(still, missing, ring)` |
| `story/ninthCow.ts` | `S47NinthCow.*` | Plassering ute av syne, tikking, funn, nærblikk, kneling, øremerke, roen og brettets regler |
| `world/ninthCow.ts` | `S47NinthModel.*` | Den eksporterte modellen, frost, posisjoner, synslinjer og interaksjonsproksyer. Eget navn skiller modellen fra historiemodulen |
| `story/motelTv.ts` | `S47MotelTv.*` | Testkort, bærebølge, 47 sekunder, virkelig bilde av plassen og bilen, vitnekrav, dokument og `Refile` |
| `story/morningHolds.ts` | `S47MorningHolds.*` | Kjølekompressorens lydgraf, 14 sekunder på stedet før morgenhendelsen, 29,5 sekunders forløp, panelventing og tilbakekomst av lyd |
| `core/cutouts.ts` | `S47Cutouts.*` | Plassering utenfor synsfeltet, observasjon, forsvinning, skulderplassering og `cut:post` med 0,09 sekunders blink |
| `story/caliche.ts` | `S47Caliche.*` | Alle fem personene, emneregler, gjentakelsen etter to timer, vitnekopier, People-fanen og hooks for Earls spole |
| `world/Diner.ts` | `S47Diner.*` | Eksporterte personer/proksyer, `SetDawn`, `SetSignLit`, lysrør, neon og delte lyktebassenger |
| `core/recordedDialogue.ts`, `npcDialogue.ts`, `earlDialogue.ts` | `S47RecordedDialogue.*`, `S47NpcDialogue.*`, `S47EarlDialogue.*` | Caliches innspilte replikkøer, hele-replikk-validering, roller, 0,22 sekunders mellomrom, automatisk avspilling, replay og stopp |
| De relevante delene av `main.ts` | `S47TownLayer.*` | Deps, kontekstbyggere, trinn, hooks, observasjonslogg, kart, lagring, gjenoppretting og testflagg |

De delte endringene er avgrenset til innkoblingen i `S47Main.*`, `S47HUD.cpp`, `S47Night.cpp`, `S47Flow.cpp`, `S47Phones.*`, `S47Walker.*`, `S47World.*`, `S47Web.*`, `S47Obj.cpp` og `S47AlbumSubjects.cpp`. Den niende kua, Caliches personer og pappfigurene kan navngis på albumets bilder. Kartposisjon og fottrinn ved veiens stoppesteder følger `World.roadStopAt`. `SameThread` steppes etter morgenen og før kontortelefonen. Tilbakestillingen av tråden ligger etter den niende kua og morgenen.

`FS47NightHooks` og `Score.Held` får de faktiske tilstandene for stillheten, byen, TV-en, pappfiguren over stolpen, kua og morgenen. `GameState.town` deler det eksisterende nøkkelsettet. `GameState.motelTv` bruker `{id:"motel-tv-frame",url:...}` fra dokumentets faktiske bilde. Vitnekopier og TV-dokumentet gjeninnføres etter at saken og telefonenes dokumenter er tilbake.

## Eksport og motorforskjeller

`Tools/webexport/export.js` har små tillegg som må tas med ved neste eksport:

- Byens opprinnelige atlas-quads og glødepunkter eksporteres også som adresserbare `world.town.runtime.*`-deler. Originalattributter og plasseringer bevares; de sammenslåtte originalene skjules i eksporten.
- Nettversjonens egen `roadside.carNow` oppretter fjernbilens lys før innsamlingen. De eksporteres skjult. Punktets faktiske materialfarge og `pointScreen` bevares.
- `drive.townSlots` og `drive.neonSlots` tar vare på plassene for delte lys.

`import_web.py` har herdets nøyaktige `beamLit`-uttrykk. `notebook_art.py` kopierer også det eksisterende `tv_testcard_blank.jpg`, som lastes utenom `core/art.ts`. `Scripts/screenshots.sh` har fått scenariene for denne porten. Hver kjøring krever også vellykket prosessavslutning og fravær av kritiske feil, i tillegg til skjermbildet og de forventede logglinjene. `unreal/memory.md`, `todo.md` og `log.md` oppdateres. De fire midlertidige Content-symlinkene ble fjernet før commit. Ingen genererte assets committes.

Unreal-endringene som modulene trenger er en lesefunksjon for flomlysbassenger, bytte av pappfigurens lyssett, oppdaterbart halvkulelys og tilgang til billboard-materialer. Skjulte glødepunkter bygges også, slik at de senere kan vises. `Opacity` føres videre som billboard-lysstyrke. Byens bilkollider oppdaterer også kjøringens verdikopi. Gåeren bruker områdets eksakte gulvfunksjon ved SARO og i Caliche, som `Player.floor` i nettversjonen; dette retter plassering under bakken ved et direkte områdehopp.

Native forskjeller er kommentert i koden: Slate erstatter DOM/CSS, TV-lerretet rasteriseres på CPU, og et ekte SceneCapture lager lot-bildet. Stemmeklipp lastes synkront gjennom den eksisterende native RadioVoice-banken før en replikk begynner. Det eksporterte silhuettlegemet med kant brukes; malte alternative poser finnes ikke i den brukte eksporten. TV-opptakets ekstralys bruker fysisk candela med `700 * 22^(2-1.6)`, fordi Unreals legacy-eksponent ikke betyr three.js sitt avstandsfall. Originalens 700, 70 m, 1,6 og 22 m bevares. Illuminansen er lik på bakken 22 m rett under lyset; Unreals kvadratiske fall gir omtrent +3,9 % ved 20 m og -11,7 % ved 30 m. Eksponeringen er eksplisitt den samme som verdenens manuelle eksponering. Fjernbilens `size:5` blir i dagens felles billboard-tegning 5 meter i verdensrommet; three.js bruker 5 CSS-piksler. `pointScreen` bevares ved eksport, men eksakt størrelse per visning trenger egen native støtte, også for SceneCapture.

## Verifikasjon

| Kontroll | Resultat | Grunnlag |
| --- | --- | --- |
| Editorbygg | PASS | Siste `Scripts/build-editor.sh`: 10,09 s, ingen kompilatorfeil eller advarsler (`/tmp/s47-town-build9.log`) |
| Selvtest | PASS | 731 samlede sjekker i faktisk Vulkan-kjøring etter siste koderettelse |
| Runtime | PASS | 19 ulike scenarier med forventet logg, skjermbilde og feilfri prosessavslutning |
| TV-bilde/JSON/Refile | PASS | Nytt 320 x 240 JPEG på 16705 byte, byteidentisk med JSON-bildet; ev:tv og mark:tv@12047 beholdt, dokumentet gjeninnført |
| Syntaks og diff | PASS | `node --check export.js`, `py_compile` for import/kunst, `bash -n screenshots.sh` og `git diff --check` |
| Eksportør | PASS | Chromium hentet 5732 håndtak, inkludert nye runtime-paneler og glødepunkter |

Editorbygget ble kjørt uten en aktiv editor for denne arbeidskopien. Selvtesten og den lange TV-prøven ble gjentatt etter siste rettelse. Bildet er visuelt kontrollert: motellplassen, kontortaket og bilen er synlige ovenfra. TV-loggen måler luminansvarians 3477,51; den tidligere hvite fangsten hadde omtrent 0,77.

Sluttløypen dekker 19 ulike scenarier: selvtest; byens første telefon og retursamtalen, stoppestedet, lampene, patruljebilen og figuren; stillheten, fjernbilen og taket; flokken med ni og åtte; den niende ved STATION 01; TV-bærebølgen og bildet; morgenen; pappfiguren på bakken og stolpen; Caliches samtale og People-oppføring. Dette er portens del av `screenshots.sh`, kjørt gjennom en midlertidig editorstarter. Hele det eksisterende pakkede testskriptet er ikke kjørt her.

Faktiske feil funnet under kjøring og korrigert:

- Et direkte områdehopp plasserte spilleren under Caliches terreng. Player.floor er nå koblet før plassering. Bybildet viser riktig bakkehøyde og payphone-prompten.
- Takprøven krasjet ved avslutning etter at den hadde skrevet `smoke test OK`. Roadside-destruktoren gikk gjennom en allerede frigitt Web-nodetabell. Destruktoren stopper nå bare lydnodene; takprøven avslutter rent. TV-destruktoren har samme prinsipp. Testskriptet krever nå også exitstatus 0 og fravær av kritiske feil.
- Første TV-JPEG var nesten helt hvit selv om JSON/`Refile` bestod. Ekstralyset var omtrent 121 ganger for sterkt ved bakken. Den kildeavledede candela-konverteringen erstatter feil legacy-eksponent. Den faste TV-prøven kontrollerer nå også bildestørrelse og luminansvariasjon, etterfulgt av faktisk bildeinspeksjon.

Testprofil og bevis ligger separat under `/tmp/s47-town-profile` og `/tmp/s47-town-evidence-final`. Bygglogger ligger i `/tmp/s47-town-build*.log`. Eksportørens separate håndtaksprøve ligger i `/tmp/s47-town-export-check` og `.log`. Dette er midlertidige lokale bevis; flaggene nedenfor gjenskaper kjøringene.

Testene bruker `UnrealEditor <uproject> -game -RenderOffscreen -nosound`, med midlertidige lenker til hovedkopiens genererte innhold og venting på andre grafiske UnrealEditor-prøver. Runtime logget Vulkan og faktisk viewport 888 x 500. Dette er editorens spillmodus, ikke en pakket utgave, menneskelig input, lytting eller en ytelsesmåling. Kjøringene har eksisterende Python-pluginfeil i editoroppstarten, materialenes `InstancedStaticMeshes`-advarsler og varsler om håndtak som krever ny eksport. Disse er ikke kompilatorfeil eller S47-selvtestfeil; de begrenser hva bildene beviser.

TV-prøven serialiserer det faktiske `GameState.motelTv.url`, dekoder JPEG-en, fjerner dokumentet og gjeninnfører det med `Reset`/`Refile`. `TownSeen` lever videre i samme prosess. Den prøver dermed bilde-JSON og `Refile`, ikke hele StartGame/Continue med en sak lastet fra disk. Full lagring/gjenoppretting, sammenslåingen med andre hjelpere og den pakkede utgaven er UNVERIFIED.

## Testflagg

Alle brukes sammen med `-S47Jump=chapter1`; de kaller de vanlige modulene etter oppstart. Testprofil bør isoleres med `-UserDir`.

| Flagg | Tilstand |
| --- | --- |
| `-S47Town=phone\|answer\|phone2\|answer2\|lamps\|car\|cutout` | Byhendelsene, inkludert retursamtalen i kapittel fem-kontekst |
| `-S47Town=stop` | Asfalt og jord ved patruljebilens stoppested |
| `-S47Roadside=silence\|car\|roof` | Stillheten, fjernbilen og figuren på taket |
| `-S47Pasture=still\|missing` | De ni på 026 og flokken etter den niende |
| `-S47Ninth[=found]` | Kua nede ved STATION 01, eventuelt sett med lys |
| `-S47MotelTv[=frame]` | Bærebølgen; `frame` venter på bildet og prøver JSON/`Refile` etter 54 sekunder |
| `-S47TownDump=/tmp/prefix` | Skriver TV-prøvens GameState-JSON og faktiske JPEG for inspeksjon |
| `-S47Morning` | Morgenhendelsen på dineren |
| `-S47Cutout[=post]` | Pappfiguren på bakken eller stolpen |
| `-S47Caliche=attendant\|deputy\|host\|radio_woman\|souvenir_man` | En av de fem samtalene og People-oppføringen |

## Integratorens neste steg

1. Kjør ny eksport/import med `Tools/webexport/import.sh` og deretter lyd-/materialløypens ordinære kontroller. En separat `S47_HANDLES_ONLY=1`-kjøring av eksportøren i Chromium bestod med 5732 håndtak, inkludert byens nye runtime-paneler og glødepunkter. Dette prøver eksportkoden, men importerer ingen assets. Den brukte genererte Unreal-eksporten mangler `world.town.runtime.*`, fjernbilens lys, lysplassmetadata og herdets nye materialparametre. Eksisterende steders bevegelser og logikk kan prøves, men full visuell paritet for disse delene er uverifisert før regenerering. `pointScreen` eksporteres, men native billboards bruker fortsatt verdensstørrelse. Fjernbilens faste pikselstørrelse må derfor føres videre i import/runtime og kontrolleres etter regenerering.
2. Kapittel 3 og 5 finnes ikke på denne grenens utgangspunkt. `TownLayer.FreeShot()` er implementert nøyaktig og bindes til `US47Chapter3::FreeShot` når headeren er med. Etter integrasjon må det bygges og prøves med den faktiske kapittelporten. Kapittel 5 må bruke `TownLayer.Diner` ved sin `BindDiner` og de eksisterende `SetDawn`-/`SetSignLit`-kallene.
3. Den parallelle porten av lostNights må koble `Caliche.Reel.Ready/Play/Stop`. Det er ingen erstatningshistorie eller oppdiktet spole. Globale narrator-/CB-stopp kan bruke `Caliche.OnCharacterStart/OnEarlStart`; telefonstart stopper allerede Caliches stemmer. Worldlife eier `GlowPoints.update` og `windowLights.update`.
4. Kjør hele `Scripts/screenshots.sh` i et nytt pakket bygg etter sammenslåing, og prøv ekte input og lytting. Byens/veiens blanding av halvkulelys og de regenererte materialene må sammenlignes mot nettversjonen. Ingen standardstarter eller spillutgave er publisert her.

Prosjektbiblioteket ble søkt først, med et bredere nytt forsøk. Profilen `ChiR24/Unreal_mcp` ble lest: UE 5.0 til 5.8 er oppgitt, men teststatusen er UNVERIFIED og verktøyet løser et annet editorbehov. Ingen kode eller plugin derfra ble tatt inn. Repoets egne portmoduler og de fullstendige TypeScript-originalene er grunnlaget.
