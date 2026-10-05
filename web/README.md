# SIGNAL / 47 (web)

Nettversjonen av SIGNAL / 47, bygget med three.js, TypeScript og Vite. Førstepersons etterforskning ved SARO i New Mexico, 1986. Denne mappen inneholder prologen "Night Shift", kapittel 1 "The Second Exposure" (servicegården øst for kontrollrommet, motorskapet S-03, feltkameraet, fotolaben og B-12-kontrollen, fram til den lokale rapporten er arkivert), kapittel 2 "The Amended Record" (Ward ringer, korridoren sør for kontrollrommet og arkivet, de to registreringene P04 og P05, og telefonen til Nora Vega på Sierra Motor Court) kapittel 3 "The Survey Station" (lastebilen, kjøreturen sørover, målestasjonen STATION 01 med P06 til P09, Nora på felttelefonen og framkallingen av feltbildene) og kapittel 4 "Room 6" (nødutgangen og veien over riksveien til Sierra Motor Court, og samtalen med Nora Vega i rom 6 med P10 til P12).

Natten henger sammen: mellom kapitlene kommer et kort kapittelkort, ikke en sluttskjerm. Hele historien fra 23:41 til slutten klokka 05:29, også det som ikke er bygd ennå (dineren, Roswell-veien og THE EVENT), står i `HISTORIE.md`. Hvert kapittel konkretiseres i `KAPITLER.md` (dokumenter, terminalfiler, hint, gåter, falske forklaringer og callbacks). Designrådene for de neste rundene (kjerneloopen, bevisbordet, stemning, Toms idéer vurdert, rekkefølge og spilltestspørsmål) står i `SPILLDESIGN.md`.

Forslag til hva som bør gjøres videre, og hva Tom må bestemme, står i `FORSLAG.md`.

Unity-versjonen i rotmappen er arkiv. Design, historie og spillkonstanter er hentet derfra.

Spill i nettleseren: https://tombonator3000.github.io/SIGNAL-47/

## Grafikkpakken fra 4. oktober 2026

Nettversjonen bruker nå bildegenererte materialer i kontrollrommet, servicegården og fotolaben, et Melkeveis-panorama, to plakater, New Mexico-kart, SARO-logo, prosedyreark og skilt. Originalene med briefens filnavn og mål ligger i `src/assets/art/`. Det slukkede motellskiltet er levert som kildevariant; spillet viser den tente varianten. Prolog, fotobevis og spillkonstanter er beholdt.

`src/core/art.ts` laster 28 bilder før scenen bygges: de 16 fra PR #30, de seks tekstfrie flatene fra runde 3 (PR #31: B-12, R-07, gulvrammen, feltkartet, blankt skilt og papir), arkivfløyens vinylgulv og automatfront fra runde 4 (PR #34) og motellets puss, dør og to vinduer fra runde 7 (PR #39). Feltkortet og brevarket fra 1947 (også runde 7) er papirbakgrunner i dokumentvisningen og hentes av nettleseren først når Noras dokumenter åpnes. De fire flatene til STATION 01 fra runde 5 (puss, gammel betong, værslitt tre og gulvbord) lastes først når stasjonen bygges, og grusen med hjulspor fra runde 6 (PR #35) når veien bygges, med `loadArtFor`. Resten av runde 6 (dinerens skilt, meny, benk og gulv, og avisfotoet fra 1947) ligger klart og kobles inn når dineren bygges. Tekstene på instrumenter, bevis, skilt, kart og veimerking tegnes i kode oppå bildene, slik at de alltid er riktige. High og Low deler bildekildene. Himmelen har en 2K-runtimekopi for å begrense GPU-minne. PNG-originalene beholdes, mens ti lette WebP-kopier brukes i spillet. De kan bygges på nytt med `python3 tools/prepare_art.py` (Pillow), som lar uendrede kopier være i fred. Bare bilder som importeres i `art.ts`, kommer med i spillet. `concept/`, `maps/` og `production/` er arbeidsmateriale og havner aldri i bygget.

Se [grafikkontroll og testgrenser](ART_DELIVERY.md). Gulvets øvre/nedre fuge har en liten registreringsfeil ved gjentakelse; alle materialer er derfor ikke godkjent som perfekt sømløse. Nye kunstbilder er assets, mens bildene under `evidence/art-2026-10-04/` er uredigerte opptak fra spillet.

Konsepter og lokasjonskart ligger ved siden av. Se [overleveringen fra Codex](CLAUDE_HANDOFF.md), [grafikkgalleriet](src/assets/art/production/index.html) og [konsept- og kartgalleriet](src/assets/art/concept/index.html).

## Kjør

```sh
cd web
npm install
npm run dev            # utviklingsserver, åpne adressen den skriver ut
npm run build:single   # én selvstendig fil: dist-single/index.html
npm run build          # vanlig bygg i dist/
npm run typecheck
```

`dist-single/index.html` kan åpnes direkte fra disk eller publiseres som claude.ai-artefakt. GitHub Pages bruker det vanlige bygget, se Publisering.

## Publisering

`.github/workflows/pages.yml` i roten av repoet bygger mappen med `npm run build` og publiserer `dist/` på GitHub Pages hver gang main får endringer i `web/`. Den kan også startes for hånd under Actions, Publish web game to GitHub Pages, Run workflow. Pages er satt opp med GitHub Actions som kilde (Settings, Pages).

Pages legger spillet i undermappen `/SIGNAL-47/`. Det virker fordi `base: './'` i `vite.config.ts` gir relative stier. Ikke bytt til en absolutt base.

## Kontroller

PC: WASD for å gå, mus for å se, E eller klikk for å bruke, Tab for journalen (Tasks, Notes, Findings, Papers og Photos; bevisbordet kan legges ut fra Findings), Escape for pause. Med feltkameraet: C hever og senker kameraet, mellomrom, E eller klikk tar bildet, Escape senker det.
Mobil: venstre tommel går, høyre tommel ser, trykk på ting for å bruke dem. Knappene Use, Notes og pause ligger i hjørnet. Camera-knappen dukker opp når kameraet er hentet. Mens kameraet er hevet, tar et trykk på skjermen eller på Use-knappen bildet.
Kjøring: W og S gir gass og bremser (S rygger når bilen står), A og D styrer, musa ser seg rundt i førerhuset. På mobil styrer venstre tommel både gass og ratt.
Settings har lydnivå, blikkfart, invertert blikk opp og ned, synsfelt, større tekst, grafikk (Ultra, High eller Low; Ultra bare på PC) og bilde (Clean, VHS eller Worn VHS). Ultra gjør de tre nærmeste lampene til ekte lys med myke skygger, legger på omgivelsesskygge (GTAO) og ekte glød før båndet, og er standard på PC. I Ultra får de 19 materialflatene også normal- og ruhetskartene fra runde 9 (Codex, PR #51), som lastes først når Ultra slås på. Telefoner har High og Low som før.

## Lagring

Tre saker, hver med tre autolagringer som går på omgang og tre manuelle plasser. Spillet autolagrer ved hvert kapittel, hvert område og hvert funn. Save case og Load case ligger i pausemenyen, og tittelskjermen har Continue (siste lagring) og Load case. Hver lagring har et lite bilde, kapittel, sted, nattens klokke, spilletid og dato. Det kan ikke lagres før 02:13, under samtaler eller under kjøring.

Alt ligger i nettleseren: lagringene og fotografiene i IndexedDB (fotografiene én gang hver, uansett hvor mange lagringer som peker på dem), innstillingene i `localStorage`. Der IndexedDB mangler, ligger alt i `localStorage`, og da får det plass til færre lagringer.

`?debug` bak adressen viser fps, 95-persentil for bildetid, draw calls og trekanter oppe til venstre. Den er laget for å måle på ekte telefoner.

## Struktur

```
src/main.ts                oppstart, løkke, menyer, testkroker (window.S47)
src/core/                  lyd, input, interaksjon, bildeinnlasting, kodeteksturer, grafikknivå
src/core/art.ts            de 42 bildene: 28 lastet før verden bygges (TRY AGAIN hvis noe mangler), 4 med STATION 01, 1 med veien, 2 papirbakgrunner til Noras dokumenter, 7 med dineren
src/core/vhs.ts            bildet som fra et VHS-bånd fra 1986 (Settings: Picture), ett fullskjermspass
src/core/signalVoice.ts    signalet slik det høres mens man stiller inn (sus, pulser, tunge slag i 4/7, metallisk hvin)
src/core/decoder.ts        signalprosessoren: båndet spilt av rått eller harmonisert, nesten som musikk
src/core/saves.ts          lagringssystemet: tre saker, autolagring, manuelle plasser, fotografier i IndexedDB
src/core/FieldCamera.ts    søker, eksponering og filmkopi (960x600 med papirkant og håndskrevet tekst)
src/core/debug.ts          målestripa bak ?debug
src/player/                førstepersonsspiller med gangbare soner og kollisjon
src/world/                 kontrollrom, antenner, utendørs, himmel, byggeklosser (kit.ts)
src/world/ServiceYard.ts   servicegården, S-03, B-12 og fotolaben
src/world/Annex.ts         korridoren sør for kontrollrommet og arkivrommet (kapittel 2)
src/world/World.ts         områdene (SARO, veien, STATION 01), lasting ved behov og kjøringen mellom dem
src/world/Station01.ts     målestasjonen fra 1947 (kapittel 3), lastes først når den trengs
src/drive/                 lastebilen, kjørekontrollen, motorlyden og veien sørover (lastes ved behov)
src/world/Dish.ts          de 27 antennene som instanser, med egen synlighetstest
src/world/glow.ts          alle lampeglød og blinklys i ett tegnekall
src/world/props.ts         lyktestolper, rekkverk, utklippstavle, feltkamera
src/story/Prologue.ts      hele prologen som faser og tidsstyrte hendelser
src/story/Chapter1.ts      kapittel 1 som trinn, med lagring av saken
src/story/Chapter2.ts      kapittel 2: Ward, arkivet, P04 og P05, telefonen til Nora
src/story/Chapter3.ts      kapittel 3: STATION 01, P06 til P09, feltbildene, Nora på felttelefonen
src/story/Decoder.ts       signalprosessoren ved RX bank 3: panel, avspilling og displayet
src/story/Chapter4.ts      kapittel 4: rom 6, samtalen med Nora (P10 til P12), feltkortet, brevet og rettelsen
src/world/Doors.ts         alle dører i SARO: åpne og lukke når som helst, svingeanimasjon, lagres
src/world/Crossing.ts      nødutgangen, rampa, stien og innkjørselen over riksveien til motellet (del av SARO)
src/world/Grounds.ts       bakken rundt hele SARO: gangsonene rundt huset, bakkehøyden (floorAt), serviceveien, gangstien, trappa fra østgangen, gjerdet, strømlinja, skiltene og generatoren
src/world/MotelFront.ts    Sierra Motor Court utenfra og kontoret (laget av Codex, PR #39)
src/world/Diner.ts         Mesa Diner ved Roswell-veien (laget av Codex, PR #45), eget område ved (-8000, 0, 0), lastes først når det trengs
src/world/Room6.ts         rom 6 på Sierra Motor Court (laget av Codex), lastes først når spilleren går inn
src/story/state.ts         det en lagring inneholder
src/story/drawings.ts      tegningene i kapittel 2-dokumentene og saksmappens kart
src/story/Signal.ts        mottakerlogikken, portert fra Unity
src/ui/                    HUD, dokumenter, journalen med fem faner, RX-konsoll
src/ui/Panels.ts           fotolabens paneler, feltjournalen i kapittel 3 og samtalen i kapittel 4
src/ui/Board.ts            bevisbordet: kort med linjer og røde tråder mellom dem; det trådene viser, fyller spørsmålene (P04 og P05 i kapittel 2), RECORD registrerer; én kolonne på telefon
src/ui/SaveMenu.ts         menyene Load case, Save case og New case
src/assets/                lyd, fonter og grafikk (se THIRD_PARTY_NOTICES.md og ART_BRIEF.md)
tools/                     headless-tester med Playwright, lagringstest i Node
```

## Testing

`tools/walkthrough.py` spiller hele prologen gjennom de ekte interaksjonene og konsollens glidebrytere, tar skjermbilder og skriver PASS eller FAIL per steg. Den sjekker også at signalet blir klarere når mottakeren nærmer seg 1420.405, at signalprosessoren spiller av og stopper, og at døra ut til gården kan åpnes og lukkes. Løkken holdes og spillet drives med `S47.tick()`, slik at testen går i en programvare-renderer.

`tools/chapter1.py` spiller kapittel 1 fra start til sluttkort med ekte klikk i panelene: gange gjennom døra og opp gangveien, begge eksponeringene, framkalling, merking på kopiene, valg av referanse, hypotese, metode og konklusjon, og til slutt Continue etter omlasting. Den tar enten passiv eller aktiv metode, og prøver også en simulert lesefeil i IndexedDB: originalfotoet skal ligge der etter neste lagring. `tools/chapter2.py` spiller kapittel 2 fra telefonen ringer til kapittelkortet: bevisbordet med tråder trukket med musa og ved å trykke på to linjer, tomme tråder og gale tråder med svarene fra Unity, hintnivåene, plassene på P04 og P05 og RECORD, en eldre lagring som får trådene tilbake, bordet som én kolonne på telefon, lagring og Continue. `tools/chapter3.py` spiller kapittel 3: lastebilen, kjøringen, P06 til P09 med gale og riktige svar, begge feltbildene, Nora, Continue på stasjonen, turen tilbake og framkallingen. `tools/chapter4.py` spiller kapittel 4: nødutgangen, rampa ned og veien over riksveien, innkjørselen opp på motellplassen, kontoret med lappen fra Nora, langs plassen til døra til rom 6, samtalen med gale og riktige svar, papirene på bordet, Continue i rom 6, sluttkortet og journalen (Findings har P10 til P12). `tools/diner.py` laster dineren som eget område, går fra lastebilen til krakken og telefonautomaten med den ekte bevegelseskoden, sikter på alle 13 trefflatene fra steder en spiller kan stå, tar bilder fra ståhøyde (også av avisutklippet og telefonen) og går tilbake til SARO. `tools/ultra.py` slår på Ultra og tar de samme utsnittene i Ultra og High (kontrollrommet, gården og arkivet), med tegnekall. `tools/grounds.py` går rundt huset med den ekte bevegelseskoden (østdøra, rundt lastebilen, rampa, serviceveien, gangstien, nødutgangen, vestsiden, vindussiden, trappa og gangveien) og rundt østfløyen, sjekker bakkehøyden, at kantene og tingene stopper spilleren, stedsnavn og fottrinn, og at himmelen har skarpe stjerner uten de malte. `tools/reachcheck.py` (Codex) legger et rutenett over SARO, STATION 01, rom 6 og dineren og prøver med den ekte bevegelseskoden hvor spilleren kommer, hvor gulvet hopper mer enn 0,12 m, hvilke soneskjøter som er for smale, og om hver aktiv ting kan siktes på fra et sted spilleren når; kart og rapport per område. Den gir foreløpig FAIL på kjente, ufarlige lommer og på forskjellen mellom sirkel og firkant (se `todo.md`), så resultatet leses, ikke bare exit-koden. `tools/saves.cjs` tester lagringssystemet i Node, uten nettleser. `tools/artcheck.py` sjekker at de 28 startbildene lastes og at de 55 som hører til senere områder eller bare lastes i Ultra venter, at High og Low bruker de samme teksturene, og tar faste bilder av blant annet B-12, R-07, feltkartet og arkivet. `tools/csptest.py` laster enkeltfila under en streng innholdspolicy og sjekker at alle fonter, lyder og bilder kommer med.

```sh
pip install playwright && playwright install chromium
python3 tools/walkthrough.py shots 844x390 high
python3 tools/chapter1.py shots/ch1 passive
python3 tools/chapter1.py shots/ch1 active
python3 tools/chapter2.py shots/ch2
python3 tools/chapter3.py shots/ch3
python3 tools/chapter4.py shots/ch4
node tools/saves.cjs
python3 tools/artcheck.py shots/art
python3 tools/csptest.py
python3 tools/grounds.py shots/grounds
python3 tools/reachcheck.py --out-dir shots/reach
python3 tools/looks.py shots 844x390 high
```

Testene leser `dist-single/index.html` når ikke annet er oppgitt. Med `S47_URL` kan `walkthrough.py`, `chapter1.py`, `chapter2.py` og `artcheck.py` teste et annet bygg, for eksempel Pages-bygget servert fra en undermappe:

```sh
npm run build
mkdir -p /tmp/s47site && rm -rf /tmp/s47site/SIGNAL-47 && cp -r dist /tmp/s47site/SIGNAL-47
python3 -m http.server 8047 --bind 127.0.0.1 --directory /tmp/s47site &
S47_URL=http://127.0.0.1:8047/SIGNAL-47/ python3 tools/walkthrough.py shots 844x390 high
```
