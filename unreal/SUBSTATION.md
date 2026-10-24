# Caliche Junction Substation

Tom ba 24. oktober 2026 om flere originale 3D-modeller og en plassert strømtrafo som kan undersøkes. Kontrollen av runtime, Props-konfigurasjon og eksisterende manus fant Caliche Junction bare som planlagt innhold. Dette passet lager det fysiske stedet og en avgrenset serviceinspeksjon.

## Sted og kontrakt

Anlegget plasseres ved eksisterende servicegrusvei før STATION 01, i det allerede gangbare rsPasture-området. Root er Three `(7890, -1.10, 536.20)` med 180 graders yaw. Modellen bruker Blender meter/Z opp; importens asymmetriske prøve kontrollerer `(2,4,6)` til Unreal `(200,-400,600)` centimeter.

Transformator, bussføring, kontrollskap, statuslampe, servicehendel, gjerde, port, fundament og grus er separate originale modeller. Kontrollfronten står utenfor høyspentgjerdet. Originale Blender-kilder, teksturer og eksport beholdes under `Signal47/Tools/substation`. Stasjonen overtar ingen av STATION 01s identiteter, og SAROs interne K3 endres ikke.

## Gjeldende status

Kilde `43ab2430`, pakket kandidat S2 i `~/scratch-bm/Signal47-substation/candidate-S2`. Original Blend og eksport, 304 faktiske kildekontroller, 41 native mål-/LOD-/materialkontroller, editorbygg, Linux-pakke og 1378 selvtester PASS. Kitets tre detaljnivåer har 52 994 / 27 130 / 16 950 trekanter totalt.

Seks målrettede pakkede kontroller består: inspeksjon med fysisk gange/blikk/lagre/last, dag, skumring, natt, SARO-kontrollrom og STATION 01s normale ankomstområde. Inspeksjonen går gjennom samme Walker-bevegelse og Inter::Pick som vanlig spill før handlingene utføres: opp rampen, stopp ved gjerdet, les servicejournal, kjør lokal kontroll, sammenlign faktisk lampemateriale/lys og åpne en manuell disk-lagring gjennom Flow. Posisjon, resultat og klokkeslett gjenopprettes. Normal fotgjengermodus på denne veietappen heter `diner` i eksisterende kode. Ingen ny reise eller område er innført.

Originale Blender-kilder og 10 separate deler med tre FBX-LOD-er er bevart. Ingen full 110-runde eller ny PSO-matrise er kjørt. Dette er avgrenset funksjons-/plasseringsbevis, ingen menneskelig kontrollerprøve eller helruteytelsesmåling. `Tools/substation/acceptance.json` peker på pakkehash og råbevis. PR150 er flettet i `8bd33349`. Samme testede kandidat er installert i `~/Games/Signal47/Linux`, med 40 identiske payloadfiler og 48 uendrede Saved-filer. Sikkerhetskopi: `/home/tom/scratch-bm/Signal47-substation/installed-backup-1792857344762409257`. Standardstarteren åpner denne installasjonen.

## Visuell kontroll og grenser

Blender-forhåndsvisning og seks faktiske spillerbilder er vurdert. Gjerde, fysisk skilt, kjøleribber, tre faseisolatorer, busstativ, kontrollskap og rampe finnes i scenen med egne materialer. Det synlige håndtaket ligger nå innenfor bruksmålet. Dag og skumring er tatt ved samme spillerhøye frontposisjon; natt viser kontrollfronten, og fullført lampetilstand er fanget separat etter lagring/lasting.

Råbildene fra dette første passet hadde høy nattkontrast og utvasket nærlys på skapet. Nærlyset er siden rettet og kontrollert på kandidatP1 i SUBSTATION_SITE.md; de opprinnelige bildene beholdes som historisk bevis. Miljøet hevdes fortsatt ikke ferdig fotorealistisk eller ferdig nattpolert.

Råforsøk er beholdt: første kildeverifikasjon krevde feilaktig glatte endekapper; korreksjonen krever faktisk glatte porselenskropper og nøyaktige flate kapper. Første runtimekontroll brukte Walkable som kollisjonstest, men API-et tester gangsoner. Sluttprøven bruker faktisk bevegelse. Et senere gammelt områdeforsøk brukte ugyldig STATION 01-posisjon `(0,0)` og fikk Vulkan device-lost; årsaken til grafikkfeilen er ikke fastslått. Eksporten har stasjonsorigin z8000. Nytt forsøk ved reell ankomst `(-22,8032)` bestod. Ingen av de avviste forsøkene telles som PASS.

Prosjektbiblioteket ble søkt før implementering. Profilene `x-2108195576206488058` og `x-2111849852350894319` er generelle, uverifiserte Blender/Unreal-arbeidsflyter. Ingen geometri eller kode derfra brukes. Importoppsettet gjenbruker prosjektets egen målte FBX-/materialoppskrift fra `Tools/radioarray/import_assets.py`.
