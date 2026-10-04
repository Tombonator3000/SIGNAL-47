# todo.md

Prioritert. Flytt ferdige punkter til log.md.

## Nå (prolog og kapittel 1 til 3 er ute på Pages fra 4. oktober kl. 12.39)

- [ ] Tom tester natten fra prologen til og med kapittel 3 på mobil og PC på https://tombonator3000.github.io/SIGNAL-47/. Noter fps-følelse, kontroller, lesbarhet, kameraet, fotopanelene, arkivbordet, telefonsamtalene, kjøringen (taster og venstre stikke på telefon), STATION 01 og om kapittelkortene gir en sammenhengende natt.
- [ ] Prøv lagringen: Save case og Load case i pausemenyen, Continue og Load case på tittelskjermen, tre saker, og at bildene i saksmappa kommer tilbake etter lasting.
- [ ] Mål ytelse på STATION 01 og under kjøringen på ekte telefon med `?debug`.
- [ ] Mål ytelse på ekte telefon med `?debug` bak adressen: ved pulten, ute på gangveien, inne i fotolaben og i arkivrommet.
- [ ] Prøv de nye innstillingene (invertert blikk, synsfelt, større tekst) på telefon.
- [ ] RX-konsollen i stående mobil: kontroller at alt får plass uten mye scrolling.

## Neste

- [ ] Kapittel 4 «Room 6» (neste for Claude): Sierra Motor Court, rett over veien fra SARO, og Noras rom 6 (designbibelens K4). Samtalen åpnes med bevisene: originalen fra 1947, FRAME 03 og FRAME 04. Hun gir feltkortet, brevet og den signerte rettelsen, og forteller hvor C krysset riksveien. Se `HISTORIE.md`.
- [ ] Roswell-veien (nytt, etter K4): samme lastebil og kjøresystem. Motoren dør der C krysser veien, lysene blinker i grupper på 4 og 7 (`Truck.setLightLevel`, `EngineSound.stall`), et rolig lys over mesaen, et bilde som beholder lyset. Deretter dineren med servitrisen, sjåføren, avisutklippet fra 1947 og telefonautomaten til Ward.
- [ ] Dinerens grafikk fra Codex (PR #35) kobles inn når dineren bygges: skiltet (tekst i kode på baseline (512, 230), høyst 50 px), menytavla (fem linjer), mintlaminatet (0,6 m), rutegulvet (2,4 m) og avisfotoet. PNG-ene får WebP-kopier med `tools/prepare_art.py`, og alfa og tekst kontrolleres etterpå.
- [ ] Kjøreturen tilbake fra STATION 01 er et kutt. Vurder om den skal kjøres når Roswell-veien er bygd.
- [ ] Koble `Room6.ts` (levert av Codex i PR #34) inn i `World.ts` som eget område når kapittel 4 skrives. Rommet skjules med hele `group` når spilleren er et annet sted, og `motelFlood` hører bare til rommet.

## Codex (avtalt med Tom 4. oktober 2026)

Codex spør fortløpende om behov og leverer i en egen grafikk- og støttegren med kontrollbevis. Claude integrerer og eier den samlede spilltesten.

Status 4. oktober kl. 12.00: punkt 1, runde 4 fra punkt 2, SARO-kartet og punkt 3 (`Room6.ts` og `room6preview.py`) er levert i PR #34, og runde 6 (dineren og Roswell-veien) i PR #35. Begge er flettet inn. Flatene fra PR #34 og grusen fra PR #35 er i spillet. Codex lager ikke mer før det finnes et dokumentert behov; neste behov skrives her og i `ART_BRIEF.md` når kapittel 4 og dineren bygges.

1. **Grafikk, prioritet 1:** runde 5 i `ART_BRIEF.md`, fire sømløse flater til STATION 01.
2. **Grafikk, prioritet 2:** runde 4 (arkivgulvet og automatfronten) og runde 6 (Roswell-veien og dineren).
3. **Kode, prioritet 3, uavhengig:** rom 6 på Sierra Motor Court som eget område til kapittel 4, etter samme mønster som `src/world/Station01.ts`:
   - Nye filer som Codex eier: `web/src/world/Room6.ts` og `web/tools/room6preview.py`. En midlertidig forhåndsvisningskrok i `main.ts` i Codex sin gren er greit, merket `// PREVIEW ONLY`. Den tas ikke med.
   - `export class Room6` med `new Room6(origin)`. Origo blir (0, 0, -8000). Alt bygges lokalt i `group`, og det som oppgis i verdenskoordinater (soner, kolliderere, ankere, flomlys), tar med origo. Gulvet er y = 0.
   - Innhold: motellrom rundt 4,5 × 6 m, natt i 1986. Dør mot sør med vindu ved siden av og gardiner, dobbeltseng, lite bord med to stoler ved vinduet, bordlampe, TV fra 1970-tallet på en kommode, innrammet foto på kommoden, panelovn, lukket baderomsdør, telefon på nattbordet. Nora sitter i stolen ved vinduet: en enkel, stilisert sittende figur i lavpoly, uten ansiktsdetaljer, i en egen gruppe (`objs.nora`) så hodet kan snus. På bordet: en skoeske med papirer, et askebeger, og feltkortet, brevet og den signerte rettelsen som egne objekter (`objs.fieldCard`, `objs.letter`, `objs.correction`). Utenfor døra et par meter overbygd gangvei med romnummer 6 (tekst i kode) og skjæret fra motellskiltet.
   - Usynlige trefflater (`proxies`) med disse id-ene: `door`, `nora`, `table`, `fieldCard`, `letter`, `correction`, `photo`, `window`, `phone`, `tv`, `bed`, `lamp`, `bathroom`.
   - `zones` og `colliders` (rektangler i verdenskoordinater, se `Zone` i `Player.ts`; soner som møtes, må overlappe med mer enn 0,6 m), `anchors` med `arrive`, `talk` og `exit` som `{ x, z, yaw }`, `interior`, `setLamp(on)`, `update(dt, t)` og `dispose()`.
   - Egne lys i gruppa (HemisphereLight og et svakt månelys eller neonskjær), fordi SAROs lys skjules når spilleren er borte. Eget flomlyssett: `export const motelFlood = floodSet(6, 'motel', 0.2)`.
   - Maks rundt 120 draw calls, skal gå på mobil. Engelsk tekst i spillet, ingen ekte merkenavn.
   - Grunnlag: `src/assets/art/concept/ch4_room6.jpg`, `maps/motel-plan.png`, designbibelens K4 og `HISTORIE.md`.
   - Test: `npx tsc --noEmit`, ingen feil i konsollen, skjermbilder i 1280×800 og 844×390 fra døra, ved bordet, mot Nora og mot vinduet, med draw calls.
   - Claude skriver selve kapittel 4 (samtalen, bevisene, lagringen) og kobler rommet inn i `World.ts`.
4. **Filer Claude eier nå, ikke endre dem:** `web/src/main.ts`, `web/src/world/World.ts`, `web/src/world/Station01.ts`, `web/src/world/ServiceYard.ts`, `web/src/world/Annex.ts`, `web/src/world/kit.ts`, alt i `web/src/drive/`, `web/src/story/`, `web/src/ui/` og `web/src/core/`, `web/src/style.css`, `web/vite.config.ts`, de eksisterende testene i `web/tools/` og dokumentene `AGENTS.md`, `README.md`, `memory.md`, `todo.md` og `HISTORIE.md`. Nye bilder legges i `src/assets/art/<kategori>/`; Claude kobler dem inn i `art.ts`. `log.md` kan begge skrive i, med egen overskrift.

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
