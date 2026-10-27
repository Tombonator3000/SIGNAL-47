# Sierra Motor Court V2

Status: Blender-modellen og native Unreal-importen er kontrollert. Pakket spilltest, installasjon og fletting av denne endringen gjenstår.

Tom ba om et mer troverdig motell i New Mexico, keramiske retrodetaljer inspirert av referansebildet, mørkt trepanel, et logisk mottak og et bedre plassert basseng. Den eksisterende KELVORA-TV-en og de tre detaljerte bilene brukes videre.

## Modell og plassering

[Redigerbar Blender-modell](ArtSource/MotelV2/Sierra_Production.blend), [kildepakke](ArtSource/MotelV2/README.md) og [kreditering](ArtSource/MotelV2/CREDITS.md).

Modellen består av 24 deler med egne teksturer og tre detaljnivåer. Fasaden har rosa puss, teal-karmer og ny geometrisk keramikk i blått, terrakotta og krem. Mottaket har mørkt treverk og innredning tilpasset de eksisterende dokumentene, nøklene, telefonen og TV-en. Rom 6 får eget trepanel og teppe; eksisterende møbler og interaksjoner beholdes.

Det tidligere prøvebassenget lå over hovedveien når det ble satt inn i den faktiske spillverdenen. Bassenget er derfor flyttet bak mottaket, med en sammenhengende gangvei rundt nordenden. To terrenglag får presise åpninger på 34,44 m². Vannflaten har egen grense for gange, og gjerdet har adkomst gjennom porten. Parkeringen holder seg innenfor den faktiske veikanten.

De gamle ekstra motelldisplayene og enkle bilmodellene fjernes med avgrensede geometriutskiftninger. De tre eksisterende bilmodellene flyttes til de nye parkeringsfeltene. Bare ett nytt hovedskilt brukes.

## Spillkontrakt

`FS47MotelV2` validerer alle assets, plasseringer og tidligere geometritall før den bytter ut modellen. Ved manglende data beholdes hele det tidligere motellet. Levende dør, håndsett, nøkkel, TV-program og dokumentankre beholdes. Ingen personlige lagringer brukes i prøvene.

Den nye kildegrenen er opprettet fra oppdatert `origin/main` og kombinerer SC01 fra main med MetaHuman og atmosfære fra den installerte versjonen. Dette hindrer at den nye motelmodellen tilbakefører nyere arbeid.

## Utført kontroll

- Blender-eksport og ny åpning av den rene `.blend`-filen: PASS, 689 kontroller, 24 deler, 24 materialer og 67 pakkede teksturer.
- Native import: PASS, 1 996 kontroller.
- Uavhengig ny Unreal-prosess som leser lagrede native assets: PASS, 1 102 kontroller.
- Åpningene i begge faktiske terrengmesh: PASS, 25 prøver i og fire utenfor åpningen per lag.
- Editorbygg og 24 HorrorStory-tester: PASS.
- Pakket grafikk, gangrute, interaksjoner, hele `screenshots.sh`, installasjon og GitHub-merge: ikke utført ennå.

Lokale bevis finnes i `/home/tom/scratch-bm/Signal47-motel-v2-20261027`. Kontrollert import er ikke det samme som faktisk kjørt og vurdert spillgrafikk. Endelig spillaksept føres her etter prøvene.
