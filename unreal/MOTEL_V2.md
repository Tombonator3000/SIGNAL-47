# Sierra Motor Court V2

Status: Kandidat R4 er pakket og målrettede spilltester pågår. Full testmatrise, installasjon og fletting gjenstår.

Tom ba om et mer troverdig motell i New Mexico, keramiske retrodetaljer inspirert av referansebildet, mørkt trepanel, et logisk mottak og et bedre plassert basseng. Den eksisterende KELVORA-TV-en og de tre detaljerte bilene brukes videre.

## Modell og plassering

[Redigerbar Blender-modell](ArtSource/MotelV2/Sierra_Production.blend), [kildepakke](ArtSource/MotelV2/README.md) og [kreditering](ArtSource/MotelV2/CREDITS.md).

Modellen består av 24 deler med egne teksturer og tre forfattede detaljnivåer. Native Unreal-assets har i tillegg et fjerde, svært forenklet LOD for lang avstand og resident ray tracing-geometri. De tre forfattede nivåene, skjermgrensene og kollisjonen er uendret. Fasaden har rosa puss, teal-karmer og ny geometrisk keramikk i blått, terrakotta og krem. Mottaket har mørkt treverk og innredning tilpasset de eksisterende dokumentene, nøklene, telefonen og TV-en. Rom 6 får eget trepanel og teppe; eksisterende møbler og interaksjoner beholdes.

Det tidligere prøvebassenget lå over hovedveien når det ble satt inn i den faktiske spillverdenen. Bassenget er derfor flyttet bak mottaket, med en sammenhengende gangvei rundt nordenden. To terrenglag får presise åpninger på 34,44 m². Vannflaten har egen grense for gange, og gjerdet har adkomst gjennom porten. Parkeringen holder seg innenfor den faktiske veikanten.

De gamle ekstra motelldisplayene og enkle bilmodellene fjernes med avgrensede geometriutskiftninger. De tre eksisterende bilmodellene flyttes til de nye parkeringsfeltene. Bare ett nytt hovedskilt brukes.

## Spillkontrakt

`FS47MotelV2` validerer alle assets, plasseringer og tidligere geometritall før den bytter ut modellen. Ved manglende data beholdes hele det tidligere motellet. Levende dør, håndsett, nøkkel, TV-program og dokumentankre beholdes. Ingen personlige lagringer brukes i prøvene.

Den nye kildegrenen er opprettet fra oppdatert `origin/main` og kombinerer SC01 fra main med MetaHuman og atmosfære fra den installerte versjonen. Dette hindrer at den nye motelmodellen tilbakefører nyere arbeid.

## Kontroll og rettelser

- Ren Blender-kilde: PASS, 689 kontroller, 24 deler, 24 materialer og 67 pakkede teksturer. Portable modell-, FBX- og teksturkilder er bevart.
- Native import: PASS, 1 996 kontroller. Fersk lesende kontroll av de endelige lagrede native assets: PASS, 1 369 kontroller.
- Begge terrengåpningene: 25 prøver i og fire utenfor hullet per lag består.
- Editorbygg og pakking av R4: PASS. HorrorStory har 24 beståtte tester.
- R4 har 18 ferdige motelbilder fra dag, skumring og natt. Fasade, mottak/TV, parkering og basseng er faktisk vurdert. Nattbassenget er mørkt i spillets eksisterende nattlys.
- R4s gangrute består: 26 punkter, fem vanlige range-/raycast-interaksjoner, tre kollisjonsprøver, 123,74 meter. Register, nøkkel, telefon, TV og romdør kan nås. Dette er kontroller/API, ikke menneskelig input.
- R4s avgrensede RT-minneutvalg måler 78,15 MiB alltid resident geometri under uendret varselgrense 80 MiB. Poolen er fortsatt 400 MiB, varselandelen 20 prosent og varsler er på. Ingen helspill-/FPS-påstand.
- Rom 6: seks pakkede bilder består. SC01 High og Low: PASS, 120 kontroller og 17 disk-/Flow-gjenopprettinger per prøve. Den samlede selvtesten består med 1 601 kontroller. Hele `screenshots.sh` kjører nå; installert oppstart og merge gjenstår.

De første pakkede prøvene avdekket ustabile kollisjonsindekser, vegetasjon gjennom den nye bassengplassen, feil vannmateriale, gamle skiltgløder og rester fra den enkle bilen/bassenget. R4 retter dette med stabile kollider-ID-er, presise vegetasjonsunntak, farget ensidig vann og pensjonering av eksakte gamle noder. Det opprinnelige planteløvet beholdes. Fotoalbumets eksisterende skiltmotiv følger det nye skiltets bokstavfelt.

Første RT-proxyforsøk lagret innstillinger uten å redusere faktisk geometri, fordi motoren bare bygger denne proxytypen gjennom Nanite-løypen. Dette ble avslørt i pakkede bilder og erstattet av ordinært ekstra LOD3. De 24 delene har samlet 19 905 trekanter i LOD3, mot 215 318 i tidligere siste LOD. Gamle feilbevis er beholdt.

Lokale bevis: `/home/tom/scratch-bm/Signal47-motel-v2-20261027`. Kildepakken bevarer alle 150 forfatterfiler og alle 492 beskyttede MetaHuman-, himmel- og lisensierte assets fra den installerte atmosfæreversjonen. SC01 er med fra main. Personlige Saved-filer berøres ikke av testene.
