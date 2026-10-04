# todo.md

Prioritert. Flytt ferdige punkter til log.md.

## Nå (prolog og kapittel 1 ferdig til test)

- [ ] Tom tester prologen og kapittel 1 på mobil og PC på https://tombonator3000.github.io/SIGNAL-47/. Noter fps-følelse, kontroller, lesbarhet, kameraet og fotopanelene, og om det tar merkbar tid fra Start til prologen begynner.
- [ ] Mål ytelse på ekte telefon med `?debug` bak adressen: ved pulten, ute på gangveien og inne i fotolaben.
- [ ] Sammenfør dokumentasjonen med Claude-grenen `e5706fb`: den registrerer rom 6, to slutter og rollefordelingen som avgjort. Se `CLAUDE_HANDOFF.md`.
- [ ] RX-konsollen i stående mobil: kontroller at alt får plass uten mye scrolling.

## Neste (se FORSLAG.md del 3)

- [ ] Arbeidsordren om morgenserien, med Reyes og Dr. Evelyn Ward, på pulten i prologen.
- [ ] Saksmappe i notatboka med små utgaver av fotografiene og dokumentene.
- [ ] Flytt fotografiene fra `localStorage` til IndexedDB før det kommer flere eksponeringer.
- [ ] Kapittel 2 "Den strøkne protokollen": arkivrom på SARO, tre samlinger, to sammenkoblinger, telefon til Nora.

## Småting

- [ ] GitHub varsler at configure-pages v5, setup-node v4 og upload-artifact v4 (via upload-pages-artifact v3) er laget for Node 20 og tvinges over på Node 24. Publiseringen virker i dag. Bytt til versjoner laget for Node 24 når det passer.
- [ ] `npm audit` melder 3 high i byggverktøyet (braces via vite-plugin-singlefile). Gjelder bare bygging. Oppgrader når pluginen får en fiks.
- [ ] Innstillinger: invertert Y-akse, synsfelt og tekststørrelse.
- [ ] Fotsteg ute: litt annen lyd på betongen enn inne.

## Grafikk fra ChatGPT (se ART_BRIEF.md)

17 kildeassets er levert og 16 koblet inn 4. oktober. Se `ART_DELIVERY.md`, `src/assets/art/` og loggen.

- [ ] Gulvets heksagonfuger: fjern den lille registreringsfeilen i vanlig vertikal repeat. Speiling ble vurdert og forkastet fordi den lager smale romber.
- [ ] Mål minne og fps med den nye grafikken på ekte mobil og PC; headless draw calls er ikke en fps-måling.
- [ ] Koble inn de leverte runde-3-variantene av feltkart, B-12-stripe, R-07, gulvmerking, fotolabskilt og papir. Behold kodeetiketter og beviskontrakter. Se samarbeidslisten under.

Seks miljøkonsepter og fire lokasjonskart er levert 4. oktober. Se `CONCEPT_DELIVERY.md` og galleriet `src/assets/art/concept/index.html`. Kartene for STATION 01 og motellet er forslag til senere Three.js-områder.

## Senere

- [ ] Kjøring mellom områder (kompakte håndlagde områder).
- [ ] Sierra Motor Court og Noras rom 6, registrert avklart på Claude-grenen.
- [ ] Evidence board med hypoteser som kan være feil.
- [ ] Lagring av hele saken på tvers av kapitler, med flere lagringsplasser som i designbibelen.

## Samarbeid med Claude, 4. oktober

- [ ] Rett den reprodukerte CaseStore-feilen før kapittel 2 publiseres: en midlertidig lesefeil må ikke gjøre plassholderbildet til nytt originalfoto. Reproduksjon og rettelseskandidat finnes i `evidence/claude-handoff-2026-10-04/`.
- [ ] Koble inn de seks leverte runde-3-filene. Bevar PR30s forhåndslasting og den rettede `S47.hold`-accessoren ved sammenføring.
- [ ] Kjør én samlet spillkontroll etter integrasjon: kapittel 1, kapittel 2, last/lagre og faktisk foto ved feilsituasjon. Oppdater SARO-nærkartet etter den integrerte arkivgeometrien.

Runde-3-grafikken og oppdatert rom-6-konsept er levert. Se `CLAUDE_HANDOFF.md`; dette er ingen påstand om ferdig runtime-integrasjon.
