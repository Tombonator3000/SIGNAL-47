# todo.md

Prioritert. Flytt ferdige punkter til log.md.

## Nå (prolog ferdig til mobiltest)

- [ ] Tom tester lenken på mobil og PC. Noter fps-følelse, kontroller og lesbarhet.
- [ ] Mål ytelse på ekte telefon. Fps-telleren bak `?debug` som var tenkt til dette, finnes ikke i koden ennå og må lages først.
- [ ] Tom: bestem om grenen `ccr-30e38858-767d90` skal bli PR mot main. web/ ligger foreløpig bare der.
- [ ] RX-konsollen i stående mobil: kontroller at alt får plass uten mye scrolling.
- [ ] Sjekkpunkt etter fullført prolog: Continue starter i dag fra 02:13. Lagre et "ferdig"-punkt som går rett til kapittel 1 når det finnes.
- [ ] Toasts kan stable seg oppå hverandre når mange kommer tett. Legg dem i kø.

## Småting funnet da web/ ble lagt inn (3. oktober)

- [ ] Sluttkortet får ikke plass i liggende mobil. Ved 844×390 kuttes toppen av tittelen (20 px) og siste kredittlinje (8 px), ved 667×375 toppen av tittelen (9 px). Knappene er synlige, og stående mobil og PC er OK. Målt headless.
- [ ] Etter Start venter spillet til all lyd er dekodet (`audio.unlock()`) før prologen begynner, og skjermen er svart så lenge. Noter under mobiltesten om det tar merkbar tid.
- [ ] Spillet tegner hele 3D-scenen hver frame bak sluttkortet, selv om kortet dekker alt med svart. Stopp tegningen mens kortet vises, for å spare batteri og varme på mobil.
- [ ] `tools/csptest.py` leser `/tmp/csp_test.html`, men ingen skript i repoet lager den fila. Lag den i skriptet eller fjern testen.
- [ ] `npm audit` melder 3 high i byggverktøyet (braces via vite-plugin-singlefile). Gjelder bare bygging. Oppgrader når pluginen får en fiks.

## Grafikk fra ChatGPT (se ART_BRIEF.md)

- [ ] Melkeveis-panorama inn som himmeltekstur, blandet med dagens stjerneshader.
- [ ] Gulv, tak, vegg og bord som ekte teksturer i stedet for kode-teksturer.
- [ ] Plakater, kart og SARO-logo.
- [ ] Neonskiltet til Sierra Motor Court.

## Kapittel 1: S-03 og servicegården

- [ ] Østdøren låses opp etter prologen. Gå ut i servicegården.
- [ ] S-03-skapet med logg: planlagt 042°, enkoder 026°, commands received 0.
- [ ] Feltkamera: søker, eksponering, film.
- [ ] Fotolab: framkalling og sammenligning med S-03-loggen.
- [ ] Bilen bak gjerdet på bildet, telefonnummeret til Sierra Motor Court.

## Senere

- [ ] Kjøring mellom områder (kompakte håndlagde områder).
- [ ] Sierra Motor Court og rom 47.
- [ ] Evidence board med hypoteser som kan være feil.
- [ ] Lagring av hele saken, ikke bare ett sjekkpunkt.
