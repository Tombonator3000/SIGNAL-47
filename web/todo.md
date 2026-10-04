# todo.md

Prioritert. Flytt ferdige punkter til log.md.

## Nå (prolog og kapittel 1 til 3 ferdige til test)

- [ ] Tom tester natten fra prologen til og med kapittel 3 på mobil og PC på https://tombonator3000.github.io/SIGNAL-47/. Noter fps-følelse, kontroller, lesbarhet, kameraet, fotopanelene, arkivbordet, telefonsamtalene, kjøringen (taster og venstre stikke på telefon), STATION 01 og om kapittelkortene gir en sammenhengende natt.
- [ ] Prøv lagringen: Save case og Load case i pausemenyen, Continue og Load case på tittelskjermen, tre saker, og at bildene i saksmappa kommer tilbake etter lasting.
- [ ] Mål ytelse på STATION 01 og under kjøringen på ekte telefon med `?debug`.
- [ ] Mål ytelse på ekte telefon med `?debug` bak adressen: ved pulten, ute på gangveien, inne i fotolaben og i arkivrommet.
- [ ] Prøv de nye innstillingene (invertert blikk, synsfelt, større tekst) på telefon.
- [ ] RX-konsollen i stående mobil: kontroller at alt får plass uten mye scrolling.

## Neste

- [ ] Kapittel 4 «Room 6»: Sierra Motor Court, rett over veien fra SARO, og Noras rom 6 (designbibelens K4). Samtalen åpnes med bevisene: originalen fra 1947, FRAME 03 og FRAME 04. Hun gir feltkortet, brevet og den signerte rettelsen, og forteller hvor C krysset riksveien. Se `HISTORIE.md`.
- [ ] Roswell-veien (nytt, etter K4): samme lastebil og kjøresystem. Motoren dør der C krysser veien, lysene blinker i grupper på 4 og 7 (`Truck.setLightLevel`, `EngineSound.stall`), et rolig lys over mesaen, et bilde som beholder lyset. Deretter dineren med servitrisen, sjåføren, avisutklippet fra 1947 og telefonautomaten til Ward.
- [ ] Grafikk fra Codex til K4 og Roswell-veien: dinerens skilt og meny, avisutklippet (oppdiktet avis), et veiskilt mot Roswell og lastebilens dørmerke. Skrives inn i `ART_BRIEF.md` når K4 starter.
- [ ] Kjøreturen tilbake fra STATION 01 er et kutt. Vurder om den skal kjøres når Roswell-veien er bygd.
- [ ] Codex: oppdater SARO-nærkartet (`src/assets/art/maps/saro-plan.svg`) med korridoren og arkivrommet slik de ligger i `src/world/Annex.ts` (sør for kontrollrommet, x fra -5,75 til 1,75 og z fra 4,65 til 10,25).
- [ ] Codex: runde 4 i ART_BRIEF.md, arkivgulvet og automatfronten. Claude kobler dem inn når de er levert.

## Småting

- [ ] GitHub varsler at configure-pages v5, setup-node v4 og upload-artifact v4 (via upload-pages-artifact v3) er laget for Node 20 og tvinges over på Node 24. Publiseringen virker i dag. Bytt til versjoner laget for Node 24 når det passer.
- [ ] `npm audit` melder 3 high i byggverktøyet (braces via vite-plugin-singlefile). Gjelder bare bygging. Varselet dekker også braces 3.0.3, som er nyeste versjon, så det finnes ingen fiks å oppgradere til ennå.

## Grafikk fra ChatGPT (se ART_BRIEF.md)

22 bilder er i spillet fra 4. oktober: 16 fra PR #30 og de seks fra runde 3 i PR #31. Se `ART_DELIVERY.md`, `CLAUDE_HANDOFF.md` og loggen.

- [ ] Gulvets heksagonfuger: fjern den lille registreringsfeilen i vanlig vertikal repeat. Speiling ble vurdert og forkastet fordi den lager smale romber.
- [ ] Mål minne og fps med den nye grafikken på ekte mobil og PC; headless draw calls er ikke en fps-måling.

Seks miljøkonsepter og fire lokasjonskart er levert 4. oktober. Se `CONCEPT_DELIVERY.md` og galleriet `src/assets/art/concept/index.html`. Kartene for STATION 01 og motellet er forslag til senere områder.

## Senere

- [ ] Evidence board med hypoteser som kan være feil.
- [ ] K5, K6 og epilogen (designbibelen og `HISTORIE.md`).
