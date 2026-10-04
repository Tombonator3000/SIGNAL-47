# todo.md

Prioritert. Flytt ferdige punkter til log.md.

## Nå (prolog, kapittel 1 og kapittel 2 ferdige til test)

- [ ] Tom tester prologen, kapittel 1 og kapittel 2 på mobil og PC på https://tombonator3000.github.io/SIGNAL-47/. Noter fps-følelse, kontroller, lesbarhet, kameraet, fotopanelene, arkivbordet og telefonsamtalene, og om det tar merkbar tid fra Start til prologen begynner.
- [ ] Mål ytelse på ekte telefon med `?debug` bak adressen: ved pulten, ute på gangveien, inne i fotolaben og i arkivrommet.
- [ ] Prøv de nye innstillingene (invertert blikk, synsfelt, større tekst) på telefon.
- [ ] RX-konsollen i stående mobil: kontroller at alt får plass uten mye scrolling.

## Neste

- [ ] Kapittel 3: feltreisen til STATION 01 (designbibelens K3, Unity P06 til P09): avreise og retur, feltbygningen fra 1947, den historiske oppstillingen, lampetesten, kabelen der den ble kuttet, et ekte feltfoto og lagring på tvers av områdene. Nora ber om å få se kabelen først og originalene etterpå, så kapitlet slutter med veien til Sierra Motor Court.
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

- [ ] Kjøring mellom områder (kompakte håndlagde områder).
- [ ] Sierra Motor Court og Noras rom 6 (kapittel 4 i designbibelen).
- [ ] Evidence board med hypoteser som kan være feil.
- [ ] Lagring av hele saken på tvers av kapitler, med flere lagringsplasser som i designbibelen.
