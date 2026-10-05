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
Settings har lydnivå, musikk (eget nivå), blikkfart, invertert blikk opp og ned, synsfelt, større tekst, grafikk (Ultra, High eller Low; Ultra bare på PC) og bilde (Clean, VHS eller Worn VHS), og for slutten: blinkende bilder (Flashes eller Slow fades, også valgbart rett på tittelskjermen) og kameraet i slutten (Moving eller Still shots; Still shots er standard når nettleseren ber om redusert bevegelse). Ultra gjør de tre nærmeste lampene til ekte lys med myke skygger, legger på omgivelsesskygge (GTAO) og ekte glød før båndet, og er standard på PC. I Ultra får de 19 materialflatene også normal- og ruhetskartene fra runde 9 (Codex, PR #51), som lastes først når Ultra slås på. Telefoner har High og Low som før.

## Lagring

Tre saker, hver med tre autolagringer som går på omgang og tre manuelle plasser. Spillet autolagrer ved hvert kapittel, hvert område og hvert funn. Save case og Load case ligger i pausemenyen, og tittelskjermen har Continue (siste lagring) og Load case. Hver lagring har et lite bilde, kapittel, sted, nattens klokke, spilletid og dato. Det kan ikke lagres før 02:13, under samtaler eller under kjøring.

Alt ligger i nettleseren: lagringene og fotografiene i IndexedDB (fotografiene én gang hver, uansett hvor mange lagringer som peker på dem), innstillingene i `localStorage`. Der IndexedDB mangler, ligger alt i `localStorage`, og da får det plass til færre lagringer.

`?debug` bak adressen viser fps, 95-persentil for bildetid, draw calls og trekanter oppe til venstre. Den er laget for å måle på ekte telefoner.

`?dev` bak adressen slår på utviklermenyen. Den huskes på enheten, og `?dev=0` tar den bort igjen. DEV-knappen øverst på skjermen (eller F2) åpner den:
- **Start at:** hvert kapittel og noen punkter i prologen, «3 On the highway» og «6 At the line». En kjøring startet her lagres aldri, så den kan ikke skrive over en sak.
- **Truck:** lastebilen ut på riksveien eller den gamle veien fra der du står, uten historie, og «Back on foot» tilbake dit.
- **Clock:** +5 og +30 minutter.
- **Music:** spill hvert av musikkstykkene, eller stopp musikken.
- **Tools:** fps-visningen av og på.

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
src/core/ambience.ts       naturlydene ute: vind, sirisser og ting langt unna, aldri helt like
src/core/score.ts          musikken: utdrag fra Scott Buckley i public/music/, valgt etter sted, sjelden
src/player/                førstepersonsspiller med gangbare soner og kollisjon
src/world/                 kontrollrom, antenner, utendørs, himmel, byggeklosser (kit.ts)
src/world/ServiceYard.ts   servicegården, S-03, B-12 og fotolaben
src/world/Annex.ts         korridoren sør for kontrollrommet og arkivrommet (kapittel 2)
src/world/World.ts         områdene (SARO, veien, STATION 01), lasting ved behov og turene mellom dem, kjørt hele veien
src/world/Station01.ts     målestasjonen fra 1947 (kapittel 3), lastes først når den trengs
src/world/geo.ts           kartet: hvordan områdene ligger i forhold til hverandre, og hvor de møtes på veien
src/world/horizon.ts       den felles horisonten (mesaene) som alle områdene bilen kjører gjennom tegner
src/world/stationLayout.ts tallene i stasjonens plan som veien og kjøringen også trenger
src/drive/                 lastebilen, kjørekontrollen, motorlyden og veien sørover (lastes ved behov)
src/drive/Truck.ts         lastebilen SARO 07 utenfra og førerhuset; teksturene i truckTextures.ts
src/drive/legs.ts          turene: bakken bilen kjører på ved SARO og stasjonen, byttene mellom områdene og autopilotens ruter
src/drive/corridors.ts     veistrekningene to områder tegner likt der bilen bytter område
src/drive/roadShape.ts     veiens terreng som formler, så SARO og stasjonen kan følge det uten å laste veien
src/drive/OldRoad.ts       den gamle Roswell-veien i gry (kapittel 6), fra milestolpe 0 over riksveien fra dineren til C, eget område lagt på veiens kart ved dineren; utformingen i oldRoadLayout.ts, teksturene i oldRoadTextures.ts
src/world/Dish.ts          de 27 antennene som instanser, med egen synlighetstest
src/world/glow.ts          alle lampeglød og blinklys i ett tegnekall
src/world/props.ts         lyktestolper, rekkverk, utklippstavle, feltkamera
src/story/Prologue.ts      hele prologen som faser og tidsstyrte hendelser
src/story/Chapter1.ts      kapittel 1 som trinn, med lagring av saken
src/story/Chapter2.ts      kapittel 2: Ward, arkivet, P04 og P05, telefonen til Nora
src/story/Chapter3.ts      kapittel 3: STATION 01, P06 til P09, feltbildene, Nora på felttelefonen
src/story/Decoder.ts       signalprosessoren ved RX bank 3: panel, avspilling og displayet
src/story/Chapter4.ts      kapittel 4: rom 6, samtalen med Nora (P10 til P12), feltkortet, brevet og rettelsen
src/story/Chapter5.ts      kapittel 5: All Night, fila på SARO, lastebilen til dineren, folkene der, Ward i telefonen og kartgåta (P13)
src/ui/MapOverlay.ts       kartet i båsen: servicekartet med E09A-tracingen som dras på plass
src/story/Chapter6.ts      kapittel 6: Roswell Road og THE EVENT, kjøringen, de 47 sekundene og uttrekket
src/story/flashFrames.ts   bildene i FLASH, tegnet i kode til runde 11 fra Codex kommer
src/ui/Ending.ts           slutten: FLASH-laget, SIGNAL / 47, rulleteksten og avisbildet etter
src/world/Doors.ts         alle dører i SARO: åpne og lukke når som helst, svingeanimasjon, lagres
src/world/Crossing.ts      nødutgangen, rampa, stien og innkjørselen over riksveien til motellet (del av SARO)
src/world/Grounds.ts       bakken rundt hele SARO: gangsonene rundt huset, bakkehøyden (floorAt), serviceveien, gangstien, trappa fra østgangen, gjerdet, strømlinja, skiltene og generatoren
src/world/MotelFront.ts    Sierra Motor Court utenfra og kontoret (laget av Codex, PR #39)
src/world/Diner.ts         Mesa Diner ved riksveien (laget av Codex, PR #45), 2 km sør i veiområdet (`roadShape.DINER`), lastes med veien når det trengs
src/world/Room6.ts         rom 6 på Sierra Motor Court (laget av Codex), lastes først når spilleren går inn
src/story/state.ts         det en lagring inneholder
src/story/drawings.ts      tegningene i kapittel 2-dokumentene og saksmappens kart
src/story/Signal.ts        mottakerlogikken, portert fra Unity
src/ui/                    HUD, dokumenter, journalen med fem faner, RX-konsoll
src/ui/Panels.ts           fotolabens paneler, feltjournalen i kapittel 3 og samtalen i kapittel 4
src/ui/Board.ts            bevisbordet: kort med linjer og røde tråder mellom dem; det trådene viser, fyller spørsmålene (P04 og P05 i kapittel 2), RECORD registrerer; én kolonne på telefon
src/ui/SaveMenu.ts         menyene Load case, Save case og New case
src/ui/DevMenu.ts          utviklermenyen bak ?dev: kapitler, fri kjøring, klokka, musikken
src/assets/                lyd, fonter og grafikk (se THIRD_PARTY_NOTICES.md og ART_BRIEF.md)
tools/                     headless-tester med Playwright, lagringstest i Node
```

## Lyd og musikk

Naturlydene ute (`core/ambience.ts`) er aldri helt like. Vinden har styrke og klang som vandrer, med kast og stille perioder. Enkeltsirisser kommer og går og blir færre og saktere når natta kjølner. Sjeldne lyder kommer langt unna: ugle, hund, nattravn, prærieulver, et godstog, en lastebil på riksveien og fugler i grålysningen. Innendørs dempes alt.

Musikken (`core/score.ts`) er seks utdrag fra Scott Buckley (CC BY 4.0) i `public/music/`, og de strømmes når de spilles. Et stykke velges etter stedet, kommer innimellom og viker for scener, samtaler og radioen. Enkeltfilbygget har ikke filene ved siden av seg og spiller ingen musikk.

## Testing

`tools/walkthrough.py` spiller hele prologen gjennom de ekte interaksjonene og konsollens glidebrytere, tar skjermbilder og skriver PASS eller FAIL per steg. Den sjekker også at signalet blir klarere når mottakeren nærmer seg 1420.405, at signalprosessoren spiller av og stopper, og at døra ut til gården kan åpnes og lukkes. Løkken holdes og spillet drives med `S47.tick()`, slik at testen går i en programvare-renderer.

`tools/chapter1.py` spiller kapittel 1 fra start til sluttkort med ekte klikk i panelene: gange gjennom døra og opp gangveien, begge eksponeringene, framkalling, merking på kopiene, valg av referanse, hypotese, metode og konklusjon, og til slutt Continue etter omlasting. Den tar enten passiv eller aktiv metode, og prøver også en simulert lesefeil i IndexedDB: originalfotoet skal ligge der etter neste lagring. `tools/chapter2.py` spiller kapittel 2 fra telefonen ringer til kapittelkortet: bevisbordet med tråder trukket med musa og ved å trykke på to linjer, tomme tråder og gale tråder med svarene fra Unity, hintnivåene, plassene på P04 og P05 og RECORD, en eldre lagring som får trådene tilbake, bordet som én kolonne på telefon, lagring og Continue. `tools/chapter3.py` spiller kapittel 3: lastebilen, kjøringen hele veien med spillets egen autopilot, P06 til P09 med gale og riktige svar, begge feltbildene, Nora, Continue på stasjonen, turen tilbake og framkallingen. `tools/chapter4.py` spiller kapittel 4: nødutgangen, rampa ned og veien over riksveien, innkjørselen opp på motellplassen, kontoret med lappen fra Nora, langs plassen til døra til rom 6, samtalen med gale og riktige svar, papirene på bordet, Continue i rom 6, sluttkortet og journalen (Findings har P10 til P12), og at kapittelkortet går videre til All Night. `tools/chapter5.py` spiller kapittel 5: klokka på terminalen, fila `RUN860414_0529.DAT`, kjøreturen til dineren med spillets egen autopilot (ut av gården, 2 km sør på riksveien og inn på grusplassen), servitrisen, sjåføren, radioen, avisutklippet (E15), telefonautomaten med Ward, kartet i båsen der tracingen dras med musa, gale svar og riktig svar (P13), sjåførens bolt, lagring og Continue i dineren, lagringen i dineren før veien, at spilleren sitter i lastebilen på plassen klokka 05:20 med den gamle veien over riksveien, og Continue fra tittelskjermen som gir dineren med lastebilen klar. `tools/chapter6.py` spiller slutten: advarselen om blinkende bilder på tittelskjermen, lastebilen på grusplassen ved dineren 05:20, kjøringen med spillets egen autopilot ut av plassen, over riksveien og hele veien forbi seksmilsstolpen med klokka i takt, enden av den åpne strekningen av riksveien, klokka som står på 05:28 når spilleren stopper før linja, linja rett forbi åttemilsstolpen 05:29:00, motoren og dashbordet som blinker fire og sju, frontlysene i farger, radiobrokkene, FLASH, stillheten, uttrekket (ut gjennom frontruta, høyere, sola på mesaen 05:30, stjernene), tittelen, rulleteksten, avisbildet og tittelskjermen, og en runde til med Slow fades og Still shots. `tools/diner.py` laster dineren (med veien rundt), går fra lastebilen til krakken og telefonautomaten med den ekte bevegelseskoden, sikter på alle 13 trefflatene fra steder en spiller kan stå, tar bilder fra ståhøyde (også av avisutklippet og telefonen) og går tilbake til SARO. `tools/ultra.py` slår på Ultra og tar de samme utsnittene i Ultra og High (kontrollrommet, gården og arkivet), med tegnekall. `tools/grounds.py` går rundt huset med den ekte bevegelseskoden (østdøra, rundt lastebilen, rampa, serviceveien, gangstien, nødutgangen, vestsiden, vindussiden, trappa og gangveien) og rundt østfløyen, sjekker bakkehøyden, at kantene og tingene stopper spilleren, stedsnavn og fottrinn, og at himmelen har skarpe stjerner uten de malte. `tools/reachcheck.py` (Codex) legger et rutenett over SARO, STATION 01, rom 6 og dineren og prøver med den ekte bevegelseskoden hvor spilleren kommer, hvor gulvet hopper mer enn 0,12 m, hvilke soneskjøter som er for smale, og om hver aktiv ting kan siktes på fra et sted spilleren når; kart og rapport per område. Den er en vakt: exit 0 på dagens spill, exit 1 når noe nytt er galt (en ny lomme, et gulvhopp, en skjøt uten forbindelse eller en ting som ikke kan nås), exit 2 når noe ikke kunne prøves. Kjente lommer og ting som først blir aktive senere, står i rapporten som forventet eller UNVERIFIED. Gi `--out-dir` utenfor `production/reachcheck/`, ellers skrives rapportene i repoet over. `tools/saves.cjs` tester lagringssystemet i Node, uten nettleser. `npm run steercheck` (`tools/steercheck.ts`) kjører bilfysikken i Node og sjekker at autopiloten får det rattutslaget den ber om: ved 12 m/s på asfalt skal 2 grader bli 2 grader, og bilen skal holde den svingradien. `tools/artcheck.py` sjekker at de 28 startbildene lastes og at de 64 som hører til senere områder eller bare lastes i Ultra venter, at High og Low bruker de samme teksturene, og tar faste bilder av blant annet B-12, R-07, feltkartet og arkivet. `tools/csptest.py` laster enkeltfila under en streng innholdspolicy og sjekker at alle fonter, lyder og bilder kommer med. `tools/devmenu.py` prøver utviklermenyen: et kapittel som ikke lagres, fri kjøring på riksveien og den gamle veien og tilbake, klokka, «6 At the line» og `?dev=0`. `tools/drives.py` lar bilen kjøre seg selv fra plassen ved SARO til grusplassen ved porten på STATION 01 og tilbake, og tar bilder rett før og rett etter hvert bytte av område: byttet skal se ut som et hvilket som helst annet steg i kjøringen. Til slutt kjører den til Mesa Diner, som står i veiens eget område, så etter byttet fra SARO kommer ikke flere. `tools/ambience.py` sjekker naturlydene: sirisser ved midnatt og ingen ved daggry, vinden som vandrer, hver av de sjeldne lydene, og dempingen innendørs og i lastebilen. `tools/music.py` sjekker musikken mot Pages-bygget (`S47_URL`): stykket etter sted, strømming, at det viker for en scen, og at alle seks filene svarer. Mot enkeltfilbygget sjekker den at ingen musikk lastes. `tools/truckshots.py` tar bilder av lastebilen utenfra og fra førersetet, natt og gry (ingen sjekker).

```sh
pip install playwright && playwright install chromium
python3 tools/walkthrough.py shots 844x390 high
python3 tools/chapter1.py shots/ch1 passive
python3 tools/chapter1.py shots/ch1 active
python3 tools/chapter2.py shots/ch2
python3 tools/chapter3.py shots/ch3
python3 tools/chapter4.py shots/ch4
python3 tools/chapter5.py shots/ch5
python3 tools/chapter6.py shots/ch6
python3 tools/devmenu.py shots/dev
python3 tools/drives.py shots/drives
python3 tools/ambience.py
node tools/saves.cjs
npm run steercheck
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
