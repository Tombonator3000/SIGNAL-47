# Grafikk og gjennomgang til Claude, 4. oktober 2026

Tom ba ChatGPT kontrollere den aktive Claude-økten, samarbeide og lage manglende grafikk. Økten ble lest i nettleseren, og koordineringsmeldingen er sendt. Claude svarte at han sammenfører loaderne, retter IDB-feilen og etikettene og deretter kjører én samlet testrunde. Dette bekrefter mottak og arbeidsdeling, ikke ferdige rettelser. Claude arbeider med kapittel 2 og arkivfløyen. Denne grenen endrer ingen runtime-TypeScript eller Unity-filer.

Kontrollert Git-grunnlag: `main` på `65ad59fd43fb0dd5a9d4a1ed1a434cfa01d2dfdf` (PR30 flettet), Claude-grenen `ccr-30e38858-767d90` på `0bb91706a4df462f6c7199726d67250bb6f17090`. Funnene nedenfor gjelder disse commitene; kontroller nyere arbeid før retting.

## Ferdige filer

Alle stier under `src/assets/art/`. [Grafikkgalleri](src/assets/art/production/index.html), [konsept- og kartgalleri](src/assets/art/concept/index.html).

| Fil | Mål | Integrasjon |
| --- | --- | --- |
| `yard/vane_b12.png` | 256×1024 | Bevarer nøyaktig hovedstripe og 12 skalastreker. Echo-materialet er separat. |
| `yard/board_r07.png` | 512×512 | Tre presise striper. Legg `R-07` oppå i kode. |
| `yard/floor_paint_frame.png` | 1024×512 RGBA | Bare slitt gul maling. Legg engelsk tekst oppå. |
| `lab/map_field_yard.png` | 1024×768 | Tekstfritt feltkart. Tegn stedsnavn, nord og tittel etter bildet. |
| `lab/sign_blank.png` | 1024×384 | Kremhvit emaljeflate for kodebaserte skilttekster. |
| `lab/tex_paper_card.jpg` | 512×512 | Lavkontrast papirbakgrunn. Vanlig 2×2-repeat visuelt kontrollert. |

De tre presise bevis-/kartflatene er tegnet med redigerbar SVG. Skilt, papir og malingsramme er laget med innebygd ChatGPT-bildegenerator. Ingen bestemt modellversjon er bekreftet. Kilde, prompt, eksportmål og hash finnes i `production/PRECISE_GRAPHICS.json`, `yard/FLOOR_FRAME.json` og `lab/LAB_SURFACES.json`. Alle seks er sRGB; bare malingsrammen har alfa. Se også [filkontrollen](evidence/claude-handoff-2026-10-04/asset-verification.json).

`production/PRECISE_GRAPHICS.json` angir engelske etikettposisjoner i feltkartets eksisterende 512×384-canvas. Kartet viser kapittel 1 uten å avsløre arkivet eller senere reisemål. Plasser tittelen slik at antennepilen og nordmerket ikke dekkes.

Seks miljøkonsepter og fire lokasjonskart følger med fra forrige del av oppdraget. Det oppdaterte `concept/ch4_room6.jpg` følger Claudes avklarte rom 6: to stoler, bordlampe og papirer. Den eldre `ch3_room47.jpg` er bevart som historikk. Dette er konsepter, ikke spillbilder. SARO-planen viser hovedgrenens geometri før arkivtilbygget; det stiplede arkivinnsettet er historisk. Oppdater planen først etter at endelig arkivgeometri er integrert.

## Prioriterte funn

### P1: en lesefeil kan overskrive et originalfoto

`src/core/caseStore.ts:88–119`: `preload()` bytter en mislykket `idb:`-referanse til `missingPrint()` i cache. Neste `save()` behandler denne data-URL-en som et nytt bilde og skriver den over originalen under samme ID.

En isolert Node-test mot originalkilden bekrefter at originalen finnes etter én lesefeil, men er tapt etter neste save/reload. Den normale kontrollbanen beholder originalen. Kilden er byte-identisk i `e5706fb` og `0bb9170`. [Reproduksjon og minimalt rettelsesforslag](evidence/claude-handoff-2026-10-04/README.md) følger med. Kandidaten beholder en uløst IDB-referanse separat og besto samme to scenarioer. Den er ikke anvendt på runtime eller browser-testet.

### P2: to parallelle bildelastere må sammenføres

PR30 har allerede `core/art.ts`, forhåndslasting med TRY AGAIN, delte teksturkilder, WebP-kopier og 2K-himmel. Claudes nye `0bb9170` legger til en annen `art.ts` med asynkron utskifting og bred `import.meta.glob`. Påstanden om manglende automatisk innlasting i den eldre `e5706fb`-gjennomgangen er dermed foreldet.

Bevar den testede oppstarten fra main og legg til de seks nye flatene i samme løsning. Avgrens eventuelle filglobber slik at `concept/`, `maps/`, `production/` og QA-bilder ikke inkluderes i spillpakken. Bevar også main-versjonens veimerking, materialfarger, himmeloppløsning og High/Low-håndtering.

En ufarlig `git merge-tree`-kontroll mellom vår konseptcommit `6f2bea0` og `0bb9170` fant tekstkonflikter i `memory.md`, `core/art.ts`, `core/textures.ts`, `main.ts`, `world/ControlRoom.ts`, `world/Exterior.ts` og `world/ServiceYard.ts`. Ingen merge ble utført. Flett funksjoner bevisst; ikke erstatt hele filer med én sides versjon.

### P2: tekstfrie bilder trenger etiketter etter innlasting

I `0bb9170` bruker `barsBoard()` direkte `swapIn`, som erstatter også den kodebaserte R-07-teksten. `fieldMap()` tegner bare tittelen etter `overlayArt`, slik at alle stedsnavn forsvinner med det tekstfrie kartet. Tegn etikettene etter bakgrunnen. B-12-stripe og R-07-striper skal fortsatt være lesbare i begge fotobevis og begge grafikknivåer.

### P2: bevar den rettede testpausen

Claudes `main.ts:389–391` bruker fremdeles getter/setter inni `Object.assign`; den kopierer verdien og lager ingen accessor. `chapter2.py` bruker `S47.hold`, men den vil ikke stoppe løkken på denne grenen. Main/PR30 har allerede `Object.defineProperty`-rettelsen. Bevar den når importene for art, CaseStore og kapittel 2 flettes.

## Videre arbeidsdeling og kontroll

Claude eier runtime-integrasjonen og kapittel 2. ChatGPT leverer grafikk, kildefiler, smale kontroller og dette notatet. Ikke gjenta uendrede tunge tester på begge sider. Etter sammenføring: én samlet kjøring av typekontroll, begge bygg, prolog, kapittel 1 passiv/aktiv, kapittel 2 og CSP/bildelasting. Kontroller lagring med ekte foto, Continue og en simulert IDB-lesefeil. Ta faste bilder av B-12, R-07, feltkart og arkiv i High/Low. Ingen av disse integrasjonstestene påstås utført av denne grafikkleveransen.

Videre grafikkforbedring etter dette: et roligere arkivgulv og en enkel automatfront i `Annex.ts` kan gi arkivfløyen særpreg. Prioriter lagringssikkerhet, etiketter og samlet loader før flere dekorflater.
