> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Sierra Motor Court V2

Status: Motel V2 er flettet til main via [PR #178](https://github.com/Tombonator3000/SIGNAL-47/pull/178), merge `64e2a1ae4a692212f72c36c7449c0027cb81a48d`, og samme testede R4 er installert i `/home/tom/Games/Signal47/Linux`. PR #172 er også flettet gjennom denne historikken. Standardversjonen kombinerer SC01, MetaHuman, atmosfære og det nye motellet.

Tom ba om et mer troverdig motell i New Mexico, keramiske retrodetaljer inspirert av referansebildet, mørkt trepanel, et logisk mottak og et bedre plassert basseng. Den eksisterende KELVORA-TV-en og de tre detaljerte bilene brukes videre.

## Modell og plassering

[Redigerbar Blender-modell](../../../unreal/ArtSource/MotelV2/Sierra_Production.blend), [kildepakke](../../../unreal/ArtSource/MotelV2/README.md) og [kreditering](../../../unreal/ArtSource/MotelV2/CREDITS.md).

Modellen består av 24 deler med egne teksturer og tre forfattede detaljnivåer. Native Unreal-assets har i tillegg et fjerde, svært forenklet LOD for lang avstand og resident ray tracing-geometri. De tre forfattede nivåene, skjermgrensene og kollisjonen er uendret. Fasaden har rosa puss, teal-karmer og ny geometrisk keramikk i blått, terrakotta og krem. Mottaket har mørkt treverk og innredning tilpasset de eksisterende dokumentene, nøklene, telefonen og TV-en. Rom 6 får eget trepanel og teppe; eksisterende møbler og interaksjoner beholdes.

Det tidligere prøvebassenget lå over hovedveien når det ble satt inn i den faktiske spillverdenen. Bassenget er derfor flyttet bak mottaket, med en sammenhengende gangvei rundt nordenden. To terrenglag får presise åpninger på 34,44 m². Vannflaten har egen grense for gange, og gjerdet har adkomst gjennom porten. Parkeringen holder seg innenfor den faktiske veikanten.

De gamle ekstra motelldisplayene og den enkle beige bilmodellen fjernes med avgrensede geometriutskiftninger. De tre eksisterende bilmodellene flyttes til de nye parkeringsfeltene. Bare ett nytt hovedskilt brukes.

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
- Rom 6: seks pakkede bilder består. SC01 High og Low: PASS, 120 kontroller og 17 disk-/Flow-gjenopprettinger per prøve. Den samlede selvtesten består med 1 601 kontroller. Hele `screenshots.sh`: PASS, 110 unike scenarioer, komplette logger/bilder og ingen motorfeil. Ti representative matrisebilder og elleve modell-/interiørbilder er faktisk vurdert. Installert oppstart og merge er fullført, se levering nedenfor.

De første pakkede prøvene avdekket ustabile kollisjonsindekser, vegetasjon gjennom den nye bassengplassen, feil vannmateriale, gamle skiltgløder og rester fra den enkle bilen/bassenget. R4 retter dette med stabile kollider-ID-er, presise vegetasjonsunntak, farget ensidig vann og pensjonering av eksakte gamle noder. Det opprinnelige planteløvet beholdes. Fotoalbumets eksisterende skiltmotiv følger det nye skiltets bokstavfelt.

Første RT-proxyforsøk lagret innstillinger uten å redusere faktisk geometri, fordi motoren bare bygger denne proxytypen gjennom Nanite-løypen. Dette ble avslørt i pakkede bilder og erstattet av ordinært ekstra LOD3. De 24 delene har samlet 19 905 trekanter i LOD3, mot 215 318 i tidligere siste LOD. Gamle feilbevis er beholdt.

Lokale bevis: `/home/tom/scratch-bm/Signal47-motel-v2-20261027`. Kildepakken bevarer alle 150 forfatterfiler og alle 492 beskyttede MetaHuman-, himmel- og lisensierte assets fra den installerte atmosfæreversjonen. SC01 er med fra main. Personlige Saved-filer berøres ikke av testene.

Testkvitteringen binder kandidatens komplette filinventar, kommandolinjer, SC01s målte binærhash og logg-/CSV-/bildesjekksummer. Første komplette pakkeinventar ble tatt under fullmatrisen, ikke før de første målrettede prøvene; denne begrensningen er bevart i leveransekvitteringen. Ingen endringer ble gjort i kandidatpakken etter byggingen.

## Lokal levering, 27. oktober 2026

Hele `screenshots.sh` består 110/110. Den installerte Low-prøven består 17 kontroller, inklusive tre faktiske taleklipp, pause og diskrestore. Bildet etter restore er åpnet og vurdert. Alle 50 personlige Saved-filer er uendret fra oppdragets start. Dette er automatiserte kontroller og bildevurdering, ikke menneskelig gjennomspilling eller en helspill-/FPS-garanti.

Installerte filer er kontrollert mot kandidatens komplette inventar. `installation-r4.json`, `installed-verification.json`, `saved-request-to-installed-r4.json` og `merge-r4.json` ligger i bevismappen ovenfor. Ingen ny nedlastbar spillutgave er publisert i dette oppdraget.

Kanonisk opprydding er utført: fem gamle pakkepayloads er pensjonert, inklusive den midlertidige installasjonsbackupens kjørbare payload. Kilder, Blender/assets, Saved, logger og bilder er bevart. Motel R4 og to eldre motelpakker (R3/R2) beholdes, i tillegg til særskilt beskyttede SC01-R4 og PR174-kandidaten for andre oppgaver. TB2- og nedlastingsarbeid er også beskyttet, men hadde ingen komplette pakker i denne opptellingen. De særskilte unntakene må telles på nytt når de andre oppgavene frigir dem. Rapportert 21,49 GB gjelder logisk filstørrelse, ikke garantert frigjort plass ved reflinks.
