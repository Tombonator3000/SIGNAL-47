# todo.md

Prioritert. Flytt ferdige punkter til log.md.

## Nå (prolog og kapittel 1 til 3 er ute på Pages fra 4. oktober kl. 12.39)

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
- [ ] **Rom 6:** TV-en står på med sus som lyser opp rommet i blått og flimrer.
- [ ] **Walkie-talkien (etter «Signs»):** Tomás' radio fra 1947 i Noras skoeske. Slått på gir den rare lyder (bærebølgen, 4/7, brokker). Trykker spilleren for å snakke, pulserer rommet i blendende, skiftende farger med fremmede lyder, så blir alt svart og spilleren våkner senere. Nora: han trykket også. Valgfritt, etter P12.
- [ ] **Fritt kamera og fotoalbum (etter «They Are Here»):** ta bilder når som helst etter at kameraet er hentet. Alle bilder havner i et album med polaroidramme og en bildetekst ut fra hva som er i bildet.
- [ ] **Tre lysende punkter over motellet:** står stille i en trekant over Sierra Motor Court i kapittel 4 for den som ser opp; et bilde av dem havner i albumet og journalen. Ingen forklaring i spillet (bibelens regel om at fenomenet følger referansene: tre punkter som A, B og C).
- [ ] **Journal med faner:** oppgaver, notater, dokumenter, bilder, personer, steder og signaler. Oppføringene låses opp etter hvert.
- [ ] **Hånd-ikon** i stedet for siktet på dører, håndtak og ting man kan bruke.
- [ ] Voices of the Void: idéer som passer er samlet i loggen 4. oktober (signalbibliotek, vedlikeholdsturer, basen som hjem). Ikke tilfeldige overnaturlige hendelser.

## Fra Tom 4. oktober kveld (neste runder)

- [x] **Bevisbord i kapittel 2** (`ui/Board.ts`), nå v2 etter `SPILLDESIGN.md`: det trådene viser, går selv inn i spørsmålet, RECORD registrerer, én kolonne på telefon. Gjenstår: samme bord for funnene i kapittel 1 og 3 (P06, P08, P09), bevis i stedet for setninger i kapittel 4, og bordet i journalen.
- [ ] **Bevisbord i stedet for faner (Tom: «knotete og vanskelig å forstå»), resten.** Et korkbord / arbeidsbord der bevisene ligger som kort og bilder. Spilleren drar dem fritt rundt og trekker en rød tråd mellom to bevis. En tråd mellom to ting som faktisk henger sammen gir en ny lapp på bordet (for eksempel E07 + E06 gir «C er fjernet»); feil kobling får et kort svar og tråden slakker av. Funnene (P04 og videre) registreres ved å koble sammen, ikke ved å velge blant tre setninger. Samme bord brukes i alle kapitler og samler alt fra journalen. Må gå med mus og berøring.
- [x] **VHS / X-Files-etterbehandling** (`core/vhs.ts`, Settings: Picture). Camcorder-modus med REC og dato er ikke laget: designrådet er at feltkameraet er et filmkamera; venter på Tom.
- [x] **Lyd:** sirisser, ugle, hund, vindkast, fottrinn etter underlag, dører, papir og fjern rullende torden er inne. Gjenstår: prærieulv (ingen CC0-fil funnet i Toms repoer), teppe og metallrist har egne opptak, TV-sus og walkie-talkie.
- [ ] **Bilen stopper (abduksjonsscenen):** på Roswell-veien dør motoren der C krysser veien (regel R1, ser tilfeldig ut for spilleren). Lys over bilen, lyset blinker i 4/7, hvitt, så svart. Spilleren våkner i bilen med tapt tid (klokka har hoppet), bilen står vendt en annen vei. Ingen tydelig romvesen i bildet; høyst en skikkelse i motlys, uklar. Må avgjøres med Tom før den bygges.

## Neste

- [ ] Kapittel 4 «Room 6» (neste for Claude): Sierra Motor Court, rett over veien fra SARO, og Noras rom 6 (designbibelens K4). Samtalen åpnes med bevisene: originalen fra 1947, FRAME 03 og FRAME 04. Hun gir feltkortet, brevet og den signerte rettelsen, og forteller hvor C krysset riksveien. Se `HISTORIE.md`.
- [ ] Roswell-veien (nytt, etter K4): samme lastebil og kjøresystem. Motoren dør der C krysser veien, lysene blinker i grupper på 4 og 7 (`Truck.setLightLevel`, `EngineSound.stall`), et rolig lys over mesaen, et bilde som beholder lyset. Deretter dineren med servitrisen, sjåføren, avisutklippet fra 1947 og telefonautomaten til Ward.
- [ ] Dinerens grafikk fra Codex (PR #35) kobles inn når dineren bygges: skiltet (tekst i kode på baseline (512, 230), høyst 50 px), menytavla (fem linjer), mintlaminatet (0,6 m), rutegulvet (2,4 m) og avisfotoet. PNG-ene får WebP-kopier med `tools/prepare_art.py`, og alfa og tekst kontrolleres etterpå.
- [ ] Kjøreturen tilbake fra STATION 01 er et kutt. Vurder om den skal kjøres når Roswell-veien er bygd.
- [ ] Kapittel 4, Claudes del: `src/story/Chapter4.ts`, nødutgangen i korridoren og overgangen over veien, `Room6.ts` som eget område i `World.ts`, samtalen (P10 til P12), lagringen og `tools/chapter4.py`. Codex lager motellet utenfra og kontoret (`MotelFront.ts`, se Codex-delen under). Utkast til flyten: spilleren går over veien, inn på kontoret (lappen fra Nora, nøkkel 6 mangler, felttelefonen på veggen), til døra med 6 og inn til Nora.
- [ ] Koble `Room6.ts` (levert av Codex i PR #34) inn i `World.ts` som eget område når kapittel 4 skrives. Rommet skjules med hele `group` når spilleren er et annet sted, og `motelFlood` hører bare til rommet.

## Codex (avtalt med Tom 4. oktober 2026)

Codex spør fortløpende om behov og leverer i en egen grafikk- og støttegren med kontrollbevis. Claude integrerer og eier den samlede spilltesten.

Status 4. oktober kl. 12.40: PR #34 og PR #35 er levert, flettet og publisert (main `29ceb4d`). Alt som var avtalt, er levert.

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
- [ ] K5, K6 og epilogen (designbibelen og `HISTORIE.md`).
