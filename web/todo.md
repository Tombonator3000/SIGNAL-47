# todo.md

Prioritert. Flytt ferdige punkter til log.md.

## Nå (hele natten, fra prologen til slutten, er ute på Pages fra 5. oktober kl. 12.08 UTC)

- [x] Ekte kjøretur SARO til STATION 01 og tilbake, uten kutt (5. oktober kveld, `drives.py`).
- [x] Ekte kjøretur fra SARO til Mesa Diner (5. oktober kveld): dineren står i veiens eget område 2 km sør, så turen har bare byttet ved SARO (`drives.py`, `chapter5.py`).
- [x] Frontlysene lyser opp dineren, og neonen gir bilen og riksveien et rødt skjær (6. oktober morgen): `World.update` kopierer lampene mellom settene (`Diner.shine`, `Diner.neon`).
- [x] Ekte kjøretur fra dineren ut den gamle veien fra milestolpe 0 til linja ved 8,15 (6. oktober natt, `chapter6.py`). Ingen kjøretur i spillet er et kutt lenger.
- [x] Kessler-ranchens innkjørsel ved mile 3,2 på den gamle veien (Codex, PR #76), koblet inn 6. oktober morgen med gap i gjerdet og gårdslampe.
- [ ] Flere rolige landemerker på den gamle veien etter samme kontrakt, hvis Tom vil ha dem (for eksempel en postkasse ved en sidevei eller en nedlagt bensinpumpe), så de 13 km har noe å se på før seksmilsstolpen.
- [ ] Tom prøver turen til dineren og hele den gamle veien: er 13 km (rundt ni minutter ved 55 mph) for langt før slutten, eller gir det ro? Radioen i kapittel 6 kommer nå etter milene.
- [ ] Tom prøver kjøringen fra SARO til stasjonen og tilbake: finner han veien ut av gården og opp på plassen igjen, og ser han noe hoppe der bilen bytter område (200 m sør for SARO og 330 m før porten)?
- [ ] Radioprogram i bilen mens man kjører (Toms plan): stemning, nyheter, info og hint. Kapittel 6 har linjer etter milene (`Chapter6.RADIO`); neste steg er samme mønster for alle turer (`World.startTrip` med et program etter meter kjørt) og et første utkast til innhold for kapittel 3 og 5, som Tom kan rette.
- [ ] Tom: svar på om lastebilen skal kunne brukes når som helst også i vanlig spill (ikke bare i utviklermenyen). Da må historien tåle at man kjører ut før kapitlet ber om det.
- [ ] Tom prøver utviklermenyen (`?dev` bak adressen, DEV-knappen øverst eller F2), den nye lastebilen utenfra og innenfra, naturlydene og musikken på ekte høyttalere. Lyd og musikk er UNVERIFIED på ekte utstyr.

- [ ] Tom går rundt SARO på Pages: ut østdøra, rundt lastebilen, ned rampa, langs serviceveien og gangstien til nødutgangen, opp vestsiden, langs vinduene og opp trappa til østgangen. Si fra om noe står i veien, ser feil ut eller mangler.
- [ ] Tom ser på stjernehimmelen på PC og mobil: er stjernene små og skarpe nok, og er Melkeveien like fin som før?
- [ ] Tom ser på Halley-plakaten og perma i Night Shift (fanfoldpapiret, arket fra 1947 på det gamle papiret) og utklippet i dineren.
- [ ] Mål oppstarten på mobil: å ta ut de malte stjernene tok rundt 115 ms i programvare-rendereren (UNVERIFIED på telefon).

- [ ] Tom prøver arkivrommet igjen: døra kan lukkes og åpnes, og telefonen kan nås rundt bordet (rettet 4. oktober kveld).
- [ ] Tom prøver det nye i Night Shift: perma over skriveren, servicekortet på racket, telexen, plakaten, terminalen på vestpulten, radioen, reléet som slår ut sju sekunder før smellet, og walkie-talkien i rom 6.

- [ ] Tom tester natten fra prologen til og med kapittel 3 på mobil og PC på https://tombonator3000.github.io/SIGNAL-47/. Noter fps-følelse, kontroller, lesbarhet, kameraet, fotopanelene, arkivbordet, telefonsamtalene, kjøringen (taster og venstre stikke på telefon), STATION 01 og om kapittelkortene gir en sammenhengende natt.
- [ ] Prøv lagringen: Save case og Load case i pausemenyen, Continue og Load case på tittelskjermen, tre saker, og at bildene i saksmappa kommer tilbake etter lasting.
- [ ] Mål ytelse på STATION 01 og under kjøringen på ekte telefon med `?debug`.
- [ ] Mål ytelse på ekte telefon med `?debug` bak adressen: ved pulten, ute på gangveien, inne i fotolaben og i arkivrommet.
- [ ] Prøv de nye innstillingene (invertert blikk, synsfelt, større tekst) på telefon.
- [ ] RX-konsollen i stående mobil: kontroller at alt får plass uten mye scrolling.

## Fra Tom 4. oktober ettermiddag (i arbeid, i denne rekkefølgen)

- [x] **Fri bevegelse:** alle dører kan åpnes og lukkes når som helst, ingen kapitler stenger spilleren inne (`world/Doors.ts`).
- [x] **Signalet som i «Contact»:** suset går over i pulserende struktur, tunge slag i 4/7-mønsteret og en metallisk hvin, tydeligere jo nærmere 1420.405 (`core/signalVoice.ts`).
- [x] **Signalprosessoren** ved RX bank 3 spiller båndet av, rått eller harmonisert, nesten som musikk (`core/decoder.ts` og `story/Decoder.ts`).
- [x] **Motellet fra Codex (PR #39) koblet inn:** plassen, gangveien, kontoret med lappen fra Nora, rom 6 ved z 55, innkjørselsrampe fra veien, rundt skilt med riktig bakside, de seks bildene i runde 7.
- [x] P05-siden på arkivbordet var tom hvis E07 ble lest i hylla før bordet ble åpnet (merkeikonene ble aldri tegnet). Rettet, med egen sjekk i `chapter2.py`.
- [x] **Rom 6:** TV-en står på med sus som lyser opp rommet i blått og flimrer (`Room6.setTv`, `AudioSys.tvHiss`).
- [x] **Walkie-talkien:** avgjort av Tom 4. oktober kveld: ingen blackout, men en guffen lyd som peker fram mot THE EVENT (se `KAPITLER.md`, Room 6). Bygges sammen med Night Shift.
- [ ] **Fritt kamera og fotoalbum (etter «They Are Here»):** ta bilder når som helst etter at kameraet er hentet. Alle bilder havner i et album med polaroidramme og en bildetekst ut fra hva som er i bildet.
- [x] **Tre lysende punkter over motellet:** droppet av Tom 4. oktober kveld.
- [x] **Journal med faner:** Tasks, Notes, Findings (med knapp som legger ut bevisbordet), Papers og Photos, etter `SPILLDESIGN.md`. Gjenstår: personer, steder og signaler (venter til kapittel 5), og notatene kortet ned til én linje i spillerens egne ord uten tolkning.
- [x] **Hånd-ikon** i stedet for siktet på dører, håndtak og ting man kan bruke.
- [ ] Voices of the Void: idéer som passer er samlet i loggen 4. oktober (signalbibliotek, vedlikeholdsturer, basen som hjem). Ikke tilfeldige overnaturlige hendelser.

## Fra Tom 4. oktober kveld (neste runder)

- [x] **Bevisbord i kapittel 2** (`ui/Board.ts`), nå v2 etter `SPILLDESIGN.md`: det trådene viser, går selv inn i spørsmålet, RECORD registrerer, én kolonne på telefon. Gjenstår: samme bord for funnene i kapittel 1 og 3 (P06, P08, P09), bevis i stedet for setninger i kapittel 4, og bordet i journalen.
- [ ] **Bevisbord i stedet for faner (Tom: «knotete og vanskelig å forstå»), resten.** Et korkbord / arbeidsbord der bevisene ligger som kort og bilder. Spilleren drar dem fritt rundt og trekker en rød tråd mellom to bevis. En tråd mellom to ting som faktisk henger sammen gir en ny lapp på bordet (for eksempel E07 + E06 gir «C er fjernet»); feil kobling får et kort svar og tråden slakker av. Funnene (P04 og videre) registreres ved å koble sammen, ikke ved å velge blant tre setninger. Samme bord brukes i alle kapitler og samler alt fra journalen. Må gå med mus og berøring.
- [x] **VHS / X-Files-etterbehandling** (`core/vhs.ts`, Settings: Picture). Camcorder-modus med REC og dato er ikke laget: designrådet er at feltkameraet er et filmkamera; venter på Tom.
- [x] **Lyd:** sirisser, ugle, hund, vindkast, fottrinn etter underlag, dører, papir og fjern rullende torden er inne. Gjenstår: prærieulv (ingen CC0-fil funnet i Toms repoer), teppe og metallrist har egne opptak, TV-sus og walkie-talkie.
- [x] **Bilen stopper (abduksjonsscenen):** avgjort av Tom 4. oktober kveld: ingen tapt tid og ingen oppvåkning, men THE EVENT som slutt (se `HISTORIE.md`). Bygges under «Neste».

## Neste

### Historien etter Toms nye slutt (4. oktober kveld)

- [x] Tom svarte ja på begge spørsmålene (kometen, reléet) 4. oktober kveld.
- [ ] `KAPITLER.md`, ett kapittel om gangen. Spesifisert og bygd: Night Shift, All Night og Roswell Road med THE EVENT. The Second Exposure er spesifisert (telexen 02:29, mørkeromsboka, historikken i S-03, merket under malingen), men ikke bygd. The Amended Record, The Survey Station og Room 6 er ikke skrevet inn ennå (kapitlene finnes i spillet fra før).
- [x] Bygg Night Shift etter `KAPITLER.md`: perma, servicekortet, telexmaskinen, terminalen (`ui/Terminal.ts`), Halley-plakaten, jakka, radioen, K3 og klokka i nedtellingen, månefasene på kalenderen. Walkie-talkien i rom 6 er også bygd.
- [x] Sluttkortet i kapittel 4 går videre til ALL NIGHT, og lastebilen hentes på SARO etter rom 6 (5. oktober morgen).
- [ ] Lokkesvaret om -39 LY i kapittel 2 (ikke lenger «not a calendar code»).
- [x] Codex, runde 10 (PR #54, `56fa505`): avisbildet fra 1947 som ett hovedbilde med tre utsnitt, Halley-plakaten uten tekst og to fanfoldark. Koblet inn: plakaten med tekst i kode, perma på fanfoldpapir, utsnitt B i dineren.
- [ ] Utsnitt A av avisbildet i saksmappa når tråd 7 bygges i The Amended Record, og hele bildet (C) etter rulleteksten.
- [x] Codex, runde 11 (bestilt 5. oktober morgen i `ART_BRIEF.md`, levert i PR #64).
- [x] PR #62 (Codex, Voices of the Void-oppsett under `Docs/Research/`): gjennomgått, 11 av 11 tester OK, begge funnene rettet i `29bed78`, flettet til main `3df5072` etter Toms ja (5. oktober morgen). Funn fra spilling kommer som oppførselsbeskrivelse, ikke som kode.
- [x] Himmelen: Halleys komet lavt i sør-sørvest til 03:05.
- [x] Kapitlet «All Night» (5. oktober morgen): fila `RUN860414_0529.DAT` på SARO, dineren (servitrisen, sjåføren, utklippet, kaffen, radioen, telefonautomaten til Ward), og kartgåta om hvor C krysser den gamle veien (P13). Test: `tools/chapter5.py`.
- [x] Daggry i himmelen etter klokka (5. oktober): fra 04:40, sola opp 05:30. SARO og dineren viser 70 prosent av det, den gamle veien alt.
- [ ] Ta bort de ubrukte `endingLines()` i kapittel 3 og 4 (kapittelkortene i `main.ts` har tatt over).
- [ ] `tools/chapter3.py`: skjermbildet på grusveien (`e04b_track`) brukte mer enn 240 s to ganger da en annen nettleser kjørte samtidig (5. oktober morgen). Alene går testen. Kjør den alene til skjermbildene får lengre tid eller tas uten å vente på en ny ramme.
- [x] Roswell Road og THE EVENT (spesifisert og bygd 5. oktober morgen, se `KAPITLER.md` og loggen): kjøringen fra dineren, milestolpene, 47 sekunder (radio, motor, dashbord 4/7, frontlys i farger, brokker, FLASH, stillhet), uttrekket, `SIGNAL / 47`, rulletekst, scenen etter rulleteksten. Advarsel og innstilling for blinking, redusert bevegelse, lagring før veien.
- [x] Runde 11 fra Codex (PR #64, `0175e32`) koblet inn 5. oktober: milestolpen, vitnestolpen, messingskiva, den gamle asfalten og fire FLASH-bilder. Reservene i kode står til bildene er lastet.
- [x] Runde 12 fra Codex (PR #68): ørkenen sett ovenfra, koblet inn 5. oktober ettermiddag.
- [x] Uttrekket: båndet ved horisonten er borte (dis i himmelens horisontfarge), og åsene på motsatt side av sola lyser når den er oppe.
- [ ] Tom: spill slutten på PC og mobil. Fps på den gamle veien og i uttrekket er UNVERIFIED; lyden av motoren som dør, radioen og stillheten er bare hørt i koden, ikke på ekte høyttalere.

### Grafikk på PC (Tom 4. oktober kveld: «bedre grafikk og lys/skygge, shaders, post processing på PC, mobil kan beholde den enkle stilen»)

Spillet kjører allerede på WebGL (WebGL2 gjennom three.js). Det som mangler på PC, er ekte skygger, omgivelsesskygge, ekte glød og lys i lufta. Forslaget er et tredje nivå, Ultra, bare på PC (ikke berøringsskjerm), mens High og Low blir som i dag:

- [x] Ultra-nivået i `core/quality.ts` og Settings (`core/ultra.ts`), standard på PC. En automatisert nettleser starter på High.
- [x] Ekte skygger fra de tre nærmeste lampene der spilleren er (gården, arkivet, motellet, stasjonen, dineren, veien) og fra lampa på vaktpulten. Ikke månelys: månen var en tynn sigd fem dager etter nymåne og gikk ned rundt midnatt 13. april 1986. De store flomlysene over arrayet blir falske. Skyggekartene tegnes 12 ganger i sekundet.
- [x] Omgivelsesskygge (GTAO) overalt i Ultra.
- [x] Ekte glød (bloom) før VHS-passet; båndets egen glød er dempet i Ultra.
- [ ] Lys i lufta: svake lyskjegler under natriumlampene og bakkedis ute.
- [x] Normal- og ruhetskart fra Codex (runde 9, PR #51 med rettet ruhet `b32263c`): koblet inn og flettet gjennom PR #52. Hele testrekka og Pages fra undermappe PASS.
- [ ] Tom ser Ultra med kartene på PC og sier om relieffet og glansen er for sterke eller for svake.
- [ ] Skapfrontene med stensiltekst og bakken på STATION 01 er sammensatte canvas uten kart i Ultra. Gi dem tvillingkart som veiene hvis det synes.
- [ ] Måle fps på Toms PC i Ultra og High, med `?debug`. Headless: rundt 580 til 780 tegnekall i Ultra mot 80 til 200 i High (skyggene og AO-passet), UNVERIFIED som fps.
- [ ] Tom ser på Ultra og sier om skyggene er sterke nok, eller om lampene og mørket skal ha mer kontrast.

### Annet

- [ ] Kjøreturen tilbake fra STATION 01 er et kutt. Vurder om den skal kjøres når Roswell-veien er bygd.

## Codex (avtalt med Tom 4. oktober 2026)

### Bestilt 6. oktober kl. 11.18: drivelook av lyset ved dineren
- [ ] Lysendringene fra i dag er bare sett i noen få stillbilder av Claude: frontlysene på dineren og neonen på bilen (PR #79), og Ultras lamper ved dineren (`f50b6c1`, kommer i neste PR). Codex utvider `tools/drivelook.py` med en tur for dette (for eksempel `--trip dinerlight`): (1) kapittel 5, turen fra SARO til dineren om natta, de siste 300 m inn på plassen med bilder hver 40. meter, og når bilen står parkert: blikk mot fasaden og mot skiltet; (2) kapittel 6, de første 300 m ut fra plassen i gryet. Low, High og Ultra, 844x390 og 1280x800. I Ultra også til fots ved plassen (`world.enter('diner')`), der Ultra gjør dinerens lamper om til ekte lys: ingen lampe skal lyse dobbelt, og ingen spotlys skal bli stående igjen der bilen var.
- Levering som PR #73 og #78: `production/drivelook_dinerlight_20261006/`, én funnliste med pose, klokke, nivå, størrelse og alvor, og eget loggavsnitt. Funn i runtime meldes til Claude. Bildene tas på main etter neste PR (Claude sier fra når den er ute); verktøyet kan lages mot `ea6bdd0` i mellomtiden.

### Levert 6. oktober kl. 09.16 (PR #78): drivelook av Kessler-strekningen i kapittel 6
- [x] K01 (Ultra, trappetrinn langs fjernt terreng) rettet i `core/vhs.ts`. Ingen har sett gården fra en lastebil i fart, på Low eller Ultra eller i telefonstørrelse. Codex utvider sin `tools/drivelook.py` med en tur for strekningen (for eksempel `--trip kessler`): kapittel 6 slik spilleren får det, autopilot i kapitlets fart fra mile 2,9 til 3,5, klokka som kapitlet setter den (rundt 05:22). Bilder framover hver 40. meter, og blikk til venstre (yaw +1,2) ved 3,15, 3,20 og 3,25. Nivåene `low`, `high` og `ultra` (`localStorage` `s47.quality`, JSON-streng), størrelsene 844x390 og 1280x800. Flimmerkontroll: stående kamera 400, 200 og 80 m før porten, to bilder med 1/30 s mellom, andel endrede piksler rundt gården og grussporet (kandidater: grusbåndene side om side, skuldrene, feristen, navnebrettet). Samme strenge konsollregel som i PR #74.
- Levering som PR #73: `production/drivelook_kessler_20261006/` med bilder, kontaktark, `review.md` og `review.json`, én funnliste med mile, koordinater, blikkretning, nivå, størrelse og alvor (P1 til P3), og et eget loggavsnitt. Funn inne i `oldRoadLandmarks.ts` retter Codex selv i en liten egen PR (modulen er Codex sin). Funn i `OldRoad.ts`, `World.ts`, lys og gjerder melder Codex til Claude, som retter dem.
- Ikke i denne runden: endringer i `OldRoad.ts`, `World.ts`, eksisterende tester eller grafikk, nye assets.

Codex spør fortløpende om behov og leverer i en egen grafikk- og støttegren med kontrollbevis. Claude integrerer og eier den samlede spilltesten.

Status 5. oktober natt: PR #34, #35, #39, #45, #51 (runde 9), #54 (runde 10), #57 og #59 (reachcheck) er levert og flettet. Dineren er koblet inn som område; kapitlet «All Night» skrives av Claude. Runde 10 er levert i PR #54 og koblet inn. Codex eier de fire nye bildene i `src/assets/art/docs/`, `production/round10_manifest.json`, `production/round10_qa/`, `ART_ROUND10_DELIVERY.md` og eventuelle nye kontrollskript under `tools/` med egne navn. Reachcheck (PR #57) og vakten (PR #59) er levert og står i testrekka. All Night er bygd (5. oktober morgen). Ingen ny bestilling til Codex før Roswell Road er spesifisert; da kommer briefen om milestolpene og oppmålingsbolten.

### Ferdig: reachcheck som vakt, oppfølging av PR #57 (5. oktober kl. 00.52)

Levert i PR #59 (`2791a7c`) og tatt inn i testrekka: exit 0 på main.

PR #57 er tatt inn. Det eneste ekte spillfunnet (sidene av motellinnkjørselen kunne gås av og på, 0,14 til 0,44 m) er rettet med kantstein og kolliderere i `Crossing.ts`; ny kjøring gir 0 gulvhopp. Det som står igjen som FAIL, er ikke feil i spillet: 

- Bestillingen min ba om sirkelklarering, men `Player.resolve` bruker en firkant rundt spilleren. Det var min feil. Bruk motorens firkant (samme regel som `Player.resolve`) når et punkt er gyldig, så forsvinner de rundt 339 hjørnepunktene.
- De tre lommene er kjente og ufarlige: bak B-12-skiltet ved enden av østgangen (x 8,4 til 10,65, z -19,2 til -17,55), et hjørne ved stasjonshytta (x -12,6 til -12,3, z 8002,65 til 8003,1) og nordhjørnet inne i dineren (lokalt x -0,4 til 1,4, z -8,4 til -7,65). Før dem som forventet med koordinater og grunn, slik at en ny lomme et annet sted gir FAIL.
- Smale skjøter der verktøyet har vist en faktisk forbindelse, er PASS med vitnet; bare skjøter uten forbindelse er FAIL.
- Hold rapportene små i repoet: sammendrag, funn og kart. De fulle rutenettene (rundt 11 MB JSON) skrives bare med et eget flagg og legges ikke i `production/`.

Målet er at verktøyet gir exit 0 på dagens main, og exit 1 bare når noe nytt er galt, så det kan stå i den samlede testrekka. Codex eier fortsatt bare `tools/reachcheck.py`, notatet og `production/reachcheck/`.

### Ferdig: `tools/reachcheck.py`, kan spilleren gå dit og nå det? (5. oktober kl. 00.10)

Levert i PR #57 (`9a6270d`), funnet ved motellinnkjørselen rettet i PR #58.

Codex bekreftet kl. 00.14 fra main `6097012`, gren `codex/reachcheck-20261005`.

Bakgrunn: de verste feilene hittil har vært steder og ting spilleren ikke kunne nå i spillet, selv om testene teleporterte dit og brukte dem (telefonen i arkivrommet, en dør som stengte halve rommet). Med bakken rundt SARO (`world/Grounds.ts`, PR #55) er det mange flere soner og kanter. Oppgaven er et kontrollverktøy som finner slike feil av seg selv, i alle områdene. Uavhengig av Toms test på PC og mobil.

**Codex eier:** `tools/reachcheck.py`, leveransenotatet `REACHCHECK_DELIVERY.md`, rapporter og kart i `production/reachcheck/` (kommer aldri med i spillet), og eget avsnitt i `log.md`. **Ingen endringer** i `src/`, eksisterende tester eller andre dokumenter. Mangler en testkrok, skriv det i notatet, så legger Claude den til.

**Slik:**
- Leser `dist-single/index.html`, eller `S47_URL` som de andre testene. Playwright med swiftshader som i `tools/ultra.py`.
- Områdene: SARO (`S47.jump('chapter1')`, alle dører åpne med `S47.doors.set(id, true, true)` for `east`, `south`, `lab`, `exit`, `records`), STATION 01, rom 6 og dineren (`await S47.world.prepare(id)` og `S47.world.enter(id)`; `S47.player.zones` og `S47.player.colliders` er det spilleren går i der).
- **Rutenett:** punkter hver 0,15 m innenfor sonene. Et punkt er gyldig når `S47.player.walkable(x, z)` er sant og spillerens sirkel (`player.radius`) ikke treffer en kollider. To naboer henger sammen bare når den ekte bevegelsen (`player.place` på det ene, `player.update` mot det andre i korte steg) kommer fram. Flomfyll fra startpunktet i hvert område (SARO: kontrollrommet ved (0, 2)).
- **Bratte kanter:** mellom to naboer skal `floorAt` (`S47.world.grounds.floorAt` i SARO, `player.floor` ellers) ikke endre seg mer enn 0,12 m. Større hopp er en kant spilleren går rett opp eller ned; rapporter hvor.
- **Skjøter:** to påslåtte soner som berører hverandre, skal enten ikke overlappe eller overlappe med mer enn 0,6 m i begge retninger der de møtes. Mindre er en usynlig vegg; rapporter paret.
- **Ting å bruke:** for hver `Interactable` i `S47.game.d.inter.items` som har en synlig label der og da: finnes det et nådd punkt der et sikte fra øyehøyde (`player.floorY + player.eye`) mot midten av trefflata faktisk gir den tingen med `inter.pick` innenfor `range` (standard 2,4 m)? Prøv noen retninger og høyder på siktet. Rapporter dem som ikke kan nås. Ting som bare er aktive i et senere kapittel, kan stå som UNVERIFIED med grunn.
- **Ut:** én JSON-rapport og ett kart sett ovenfra per område i `production/reachcheck/` (nådd grønt, gyldig men ikke nådd rødt, bratte kanter oransje, skjøter gule, ting som kan nås blå og ting som ikke kan nås svarte). PASS, FAIL eller UNVERIFIED per sjekk, og en kort liste over funn med koordinater. Kjøretid høyst rundt 10 minutter i swiftshader.
- **Kjente forventninger** (skal ikke gi FAIL): smugene mellom kontrollrommet og østgangen og mellom gangen og fotolaben er ikke gangbare med vilje; rampa ved nødutgangen er rekkverk fra siden.

**Leveranse:** egen gren `codex/reachcheck-<dato>`, én PR. Claude retter funnene i spillkoden og tar verktøyet inn i den samlede testrekka.

### Neste oppgave: dineren i «All Night» (4. oktober kl. 16.15)

Status: PR #39 (motellet og runde 7) er flettet og publisert sammen med PR #41 til #43. Denne oppgaven er uavhengig av de fire valgene Tom har fått (abduksjonen, walkie-talkien, lysene over motellet, albumet). Dineren kommer etter Roswell-veien, i kapitlet «All Night» (05:10 til 05:25, første grålysning). Se `HISTORIE.md`, avsnittet «Nytt: dineren».

**Prioritet 1, kode: `src/world/Diner.ts`.** Codex eier `src/world/Diner.ts`, `tools/dinerpreview.py` og `DINER_DELIVERY.md`.

- `export class Diner` med `new Diner(origin: THREE.Vector3)`, bygd som `Room6.ts`: et eget område langt fra de andre. Claude legger det ved `DINER_ORIGIN = (-8000, 0, 0)` i `World.ts`. Alt plasseres relativt til origo; soner, kolliderere, trefflater og ankre oppgis i verdenskoordinater som i `Room6.ts`. Nord er -Z.
- Bildene ligger klare i `art.ts` fra main etter PR #44: `DINER_ART` (`dinerFloor` 2,4 m, `dinerCounter` 0,6 m, `dinerSign` 4,0 × 2,0 m, `dinerMenu` 1,6 × 0,8 m, `clipping1947`). De lastes med `loadArtFor(DINER_ART)` før konstruksjonen (Claude gjør det i `World`, previewen selv). Bruk `artTexture()` og `artImage()`; delte bildeteksturer disponeres aldri. Bildene fra runde 8 under brukes når de finnes, med en enkel prosedyrisk reserve til de er levert.
- Plassen (lokale koordinater): riksveien går nord-sør langs x 16 til 24 (bare kulisse: asfalt, gul midtlinje, hvit kantlinje). Flat grusplass mellom veien og bygningen (x 2 til 16, z -16 til 16). Bygningen innenfor x -8 til 2 og z -9 til 9, med fasade, vinduer og dør mot øst (+X, mot veien); døra ved z -0,6 til 0,6. Skiltet på to stolper ved veien rundt (13, 0, -11), synlig fra veien og plassen. Sjåførens semitrailer står parkert i nordenden av plassen (kulisse med kollider). Lav bakke rundt med ringer som vokser utover (se `Exterior.ts`; ingen lange, smale trekanter fra sentrum) og to eller tre mesaer i silhuett mot øst og sør.
- Alt man går på, ligger på y = 0. Ingen trapper eller ramper.
- Inne: disk parallelt med fasaden (benkeplate 1,0 m høy, 0,6 m dyp, mintlaminat) med 6 til 8 krakker på kundesiden, båser langs vinduene, kaffetrakter og kanne på disken, menytavla over kjøkkenluka på bakveggen, et innrammet avisutklipp på veggen ved døra, telefonautomat i en nisje ved døra eller mot toalettene, jukeboks som kulisse, to lukkede toalettdører, rutegulv.
- Tekst i kode (engelsk): skiltet «MESA DINER» (baseline (512, 230), høyst 50 px, som i runde 6) og «OPEN ALL NIGHT» mindre under. Menytavla, fem linjer: «COFFEE .35», «TWO EGGS ANY STYLE 1.95», «HOTCAKES 1.75», «GREEN CHILE STEW 2.95», «PIE .95». Avisutklippet: en forside klippet slik at datoen er borte, avisnavnet «PECOS VALLEY SENTINEL», tittelen «LIGHT HELD OVER MESA FOR AN HOUR», undertittelen «Ranchers on the old survey road watched a steady glow; Army field office cites weather equipment», bildet `clipping1947` med teksten «Seen from the Kessler ranch, 3 a.m.», og en blyantlapp på rammen: «July '47». Avisen og navnene er oppdiktet.
- To personer i lavpoly som Nora i `Room6.ts`: servitrisen står bak disken (rundt 50 år, kittel), sjåføren sitter på en krakk (caps, jakke). Hodene er egne grupper i `objs` (`waitressHead`, `driverHead`), så Claude kan snu dem.
- Lys: eget flomlyssett `dinerFlood` (høyst 10 plasser): neon på skiltet (rødt og cyan), lysrør inne (kjølig hvitt), lampe over kjøkkenluka, lys fra vinduene ut på plassen og ett svakt grålys fra øst. En egen `HemisphereLight` i gruppen som i `Room6.ts`, ellers ingen ekte lys og ingen skygger.
- Soner (`zones`): `lot` (plassen og veikanten), `walk` (fortauet foran fasaden), `door` (døråpningen), `inside` (gulvet på kundesiden) og `phone` (nisjen). Soner som møtes, overlapper med mer enn 0,6 m.
- Kolliderere: bygningen, disken, krakkene, båsene, jukeboksen, skiltstolpene, semitraileren og SAROs lastebil der den står (`truckPark`).
- Usynlige trefflater (`proxies`): `door`, `counter`, `coffee`, `waitress`, `driver`, `menu`, `clipping`, `payphone`, `jukebox`, `window`, `booth`, `sign`, `rig`.
- Ankre (`anchors`, `{ x, z, yaw }`): `truckPark` (lastebilen fra SARO, nesen mot nord), `arrive` (ved førerdøra), `outside` (foran døra, vendt mot den), `inside` (rett innenfor, vendt inn), `stool` (krakken ved siden av sjåføren) og `phone` (foran telefonautomaten).
- `objs`: `door` (dørbladet med hengsel, så Claude kan svinge det), `coffeePot`, `cup` (koppen spilleren får), `clipping`, `payphoneHandset`, `waitressHead`, `driverHead`.
- Metoder: `setSignLit(on)`, `setDawn(k)` (0 natt til 1 grålysning: himmelskjæret i vinduene og grålyset fra øst), `update(dt, t)` (neon og lysrør som blafrer litt) og `dispose()`.
- Budsjett: høyst 120 draw calls fra det verste utsnittet (inne mot disken med plassen synlig gjennom vinduene), rundt 80 inne ellers.

**Prioritet 2, grafikk: runde 8 i `ART_BRIEF.md`** (to tekstfrie flater til dineren).

**Leveranse:** egen gren `codex/diner-<dato>`, én PR med `Diner.ts`, `dinerpreview.py`, `DINER_DELIVERY.md` og eventuelt de to bildene med manifest. Ingen endringer i `World.ts`, `art.ts`, `main.ts`, historiefilene eller testene. Claude kobler inn, skriver replikkene, kapitlet og den samlede testen. Previewen som med motellet: faste utsnitt i High og Low ved 1280x800 og 844x390, kontraktkontroller (soner, kolliderere, trefflater med `Interaction.pick` fra ankrene, gange med `Player.update` fra `arrive` via `outside` og `inside` til `stool` og `phone`), draw calls og konsollfeil. Ikke lag lyd, replikker eller spillogikk.

### Neste oppgave til kapittel 4 (avtalt 4. oktober kl. 12.45)

Kapittel 4 trenger først at spilleren kan gå fra SARO til motellet. I dag er Sierra Motor Court bare en kulisse i `src/world/Exterior.ts` (en lang boks ved x -46, z 24 til 68, en baldakin og en bil). Rom 6 (`Room6.ts`) er ferdig og blir et eget område. Det som mangler, er motellet utenfra og kontoret.

**Prioritet 1, kode: `src/world/MotelFront.ts`, Sierra Motor Court utenfra og kontoret innenfra.**

- Nye filer som Codex eier: `web/src/world/MotelFront.ts` og `web/tools/motelpreview.py` (egen forhåndsvisning utenfor spillinngangen, som `room6preview.py`). Leveransenotat i `web/MOTEL_DELIVERY.md`.
- `export class MotelFront` med `new MotelFront()`. Den bygges i SAROs scene og i SAROs koordinater (ingen egen origo), fordi spilleren går dit fra SARO. Claude fjerner den gamle boksen, baldakinen, stolpene, bilen og de to flomlysene i `Exterior.motel()` når modulen kobles inn. Motellskiltet (stolper ved (-31.5, 30) og (-29.3, 30), skiltet ved (-30.4, 5.8, 30.2)), riksveien (x -28 til -20, overflate y -0,58) og veilysene ved x -18,5 blir stående.
- Plass: bygningen innenfor x -50 til -41 og z 22 til 70, med fasaden og dørene mot øst (+X, mot veien) og en overbygd gangvei foran. Kontoret i nordenden, nærmest skiltet. Rommene nummereres 1 og utover sørover fra kontoret. Rom 6 er ett av dem. Parkeringsplassen mellom fasaden og veien (x -41 til -28). Bassenget med gjerde er bakgrunn. Grunnlag: `src/assets/art/concept/sierra_court.jpg`, `maps/motel-plan.png`, designbibelens K4 og `HISTORIE.md`.
- Spilleren har ingen høyde, så alt man går på, ligger på y = 0. Bakken utenfor ligger på y -0,62, så plassen og gangveien er en plate med fortauskant ned til bakken. Claude bygger overgangen over veien fra SARO.
- Soner (`zones`, rektangler i verdenskoordinater som i `Room6.ts`; soner som møtes, overlapper med mer enn 0,6 m): `lot` (parkeringsplassen, må dekke x -30 til -28 for z 2 til 10, der Claude sin sone over veien kommer inn), `walk` (gangveien foran dørene), `office` (inne på kontoret) og `officeDoor` (døråpningen). Kolliderere: bygningen, baldakinstolpene, bilen, skiltstolpene, isautomaten, bassenggjerdet og møblene på kontoret.
- Kontoret, rundt 4 × 5 m, kan gås inn i: disk med gjesteprotokoll, nøkkeltavle med kroker for alle rommene (nøkkel 6 mangler), tørre potteplanter, regninger på et spyd, en forseglet konvolutt på disken, en lapp på disken, og den gamle felttelefonen på veggen (eik, to bjeller og sveiv, som på STATION 01) med ledningen ut gjennom bakveggen. «This line still rings in the motel office» sier Nora i kapittel 3. Innsiden ligger i en egen gruppe `interior`, slik at Claude kan skjule den når spilleren er ute, som arkivfløyen.
- Usynlige trefflater (`proxies`): `officeDoor`, `register`, `keyBoard`, `officePhone`, `envelope`, `message`, `room6Door`, `otherDoors` (én for alle de andre dørene), `iceMachine`, `car`, `pool`.
- Egne objekter (`objs`): `room6DoorLeaf` (dørbladet hengslet, så det kan åpnes), `officePhoneHandset`, `envelope`, `message`, `key6Hook`.
- Ankre (`anchors`, `{ x, z, yaw }`): `entry` (der spilleren kommer inn fra veien), `officeInside`, `room6Outside` (foran døra, mot døra) og `fromRoom6` (rett utenfor døra, med ryggen mot den, når spilleren kommer ut av rom 6).
- Metoder: `setRoom6Light(on)` (vindu og lampe over døra), `setOfficeLight(on)`, `update(dt, t)` (neon, lamper som flimrer litt) og `dispose()`.
- Lys: SAROs lys gjelder, så ingen egne Hemisphere- eller månelys. Eget flomlyssett, `export const courtFlood = floodSet(10, 'court', 0.1)`, for alle modulens materialer: lampene over dørene, lyset fra kontoret og det røde og cyan skjæret fra skiltet. SARO-settet `site` er fullt og skal ikke brukes.
- Budsjett: høyst rundt 50 draw calls for hele modulen i ett bilde, og den skal bygges på under rundt 150 ms i headless. Den bygges ved start sammen med SARO, fordi motellet synes fra servicegården.
- Tekst i kode, på engelsk: OFFICE, VACANCY, romnumrene og det som står på lappen og konvolutten (Claude skriver selve teksten i kapittel 4; legg igjen tomme flater). Ingen ekte merkenavn.
- Bilder: bruk bilder som allerede lastes ved start (`concrete`, `asphalt`, `desert`, `cabinet`, `paper`, `sign`, `sierra`) eller lerretsteksturer i fila. Ikke bruk `stucco`, `oldConcrete`, `weatheredWood` eller `floorboards` (de lastes først med STATION 01). Nye bilder fra runde 7 leveres som filer; Claude kobler dem inn i `art.ts` og bytter dem inn.
- Test: `npx tsc --noEmit`, ingen feil i konsollen, skjermbilder i 1280×800 og 844×390 fra veien (x -27, z 6, mot sørvest), fra plassen mot kontoret, inne på kontoret, ved døra til rom 6 og en vid oversikt, med draw calls. Ekte `Interaction.pick` på trefflatene. High og Low.

**Prioritet 2, grafikk: runde 7 i `ART_BRIEF.md`** (motelldør, vinduer om natten, motellvegg, feltkortet og brevet fra 1947).

**Filer Claude eier nå, ikke endre dem:** som under, og i tillegg `web/src/world/Room6.ts` (Claude tar den over for å koble den til kapittel 4) og `web/src/world/Exterior.ts`. Claude skriver kapittel 4 (`src/story/Chapter4.ts`), overgangen fra SARO over veien, døra inn til rom 6 som eget område, samtalen, lagringen og den samlede spilltesten.

**Filer Claude eier fra før, ikke endre dem:** `web/src/main.ts`, `web/src/world/World.ts`, `web/src/world/Station01.ts`, `web/src/world/ServiceYard.ts`, `web/src/world/Annex.ts`, `web/src/world/kit.ts`, alt i `web/src/drive/`, `web/src/story/`, `web/src/ui/` og `web/src/core/`, `web/src/style.css`, `web/vite.config.ts`, de eksisterende testene i `web/tools/` og dokumentene `AGENTS.md`, `README.md`, `memory.md`, `todo.md` og `HISTORIE.md`. Nye bilder legges i `src/assets/art/<kategori>/`; Claude kobler dem inn i `art.ts`. `log.md` kan begge skrive i, med egen overskrift.

## Småting

- [ ] GitHub varsler at configure-pages v5, setup-node v4 og upload-artifact v4 (via upload-pages-artifact v3) er laget for Node 20 og tvinges over på Node 24. Publiseringen virker i dag. Bytt til versjoner laget for Node 24 når det passer.
- [ ] `npm audit` melder 3 high i byggverktøyet (braces via vite-plugin-singlefile). Gjelder bare bygging. Varselet dekker også braces 3.0.3, som er nyeste versjon, så det finnes ingen fiks å oppgradere til ennå.

## Grafikk fra ChatGPT (se ART_BRIEF.md)

28 bilder er i spillet fra 4. oktober: 16 fra PR #30, de seks fra runde 3 i PR #31 og de seks fra rundene 4 og 5 i PR #34. 24 lastes ved start; de fire til STATION 01 lastes når stasjonen bygges. Se `ART_DELIVERY.md`, `ART_ROUND45_DELIVERY.md`, `CLAUDE_HANDOFF.md` og loggen.

- [ ] Gulvets heksagonfuger: fjern den lille registreringsfeilen i vanlig vertikal repeat. Speiling ble vurdert og forkastet fordi den lager smale romber.
- [ ] Mål minne og fps med den nye grafikken på ekte mobil og PC; headless draw calls er ikke en fps-måling.

Seks miljøkonsepter og fire lokasjonskart er levert 4. oktober. Se `CONCEPT_DELIVERY.md` og galleriet `src/assets/art/concept/index.html`. Kartene for STATION 01 og motellet er forslag til senere områder.

## Senere

- [ ] Evidence board med hypoteser som kan være feil.
- [x] K5, K6 og epilogen: går ut etter Toms nye slutt 4. oktober kveld (se `HISTORIE.md`).
