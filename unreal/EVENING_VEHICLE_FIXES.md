# Åpningens biler og inngang, 23. oktober 2026

Privatbilen bruker nå sitt eget karosseri, interiør og førerdør. SARO 07 står
plant på originalplassen, og døråpningen har vern mot en påvist fastlåsing.
Arbeidet er gjort på `codex/evening-runtime` fra `ed061aca`. Pakket kilde er
`66d2b5a6`, med `unreal-port-core 04517414` innflettet.
Kortregresjon: **PASS, 6/6**. Ingen push, PR eller installasjon; Claude integrerer.

## Samlet oppfølging 24. oktober

Takeaway er lagt til på samme gren. Ny testet pakkekilde `f83f3dc9` med core
`92dc7a47` består bilkontrollen og hele inngangsmatrisen på nytt, sammen med
1298 selvtester, to mat-/lastevarianter, burgerrute og tre korte regresjoner.
Se [samlet rapport med nye bilder](EVENING_TAKEAWAY.md). Resultatene nedenfor
beholdes som historisk bevis på `66d2b5a6`; seks gamle hopp og finalen er ikke
kjørt på nytt som seks tester i matpakken. Nyere core `6614cd90` inngår ikke i
denne frosne pakken. Claude integrerer, ingen installasjon her.

## Årsak og rettelse

1. Bare SARO-riggens Body og Shell ble skjult på privatbilen. Selvstendige
   dør-/cab-noder ble tegnet videre. Hele kjøreriggens gruppe skjules nå under
   privatkjøring og på foten. Wagonen har eget komplett førermiljø, riktig
   førerøye og egen dør som åpner ved utstigning. SARO 07 får tilbake sine
   originale deler og førerøye etter ankomst.
2. Kanonisk serviceparkering beholdt pitch/roll fra kjøreriggens siste
   veiposisjon. Originalplassen `(8.9,0,8)` nuller nå helning og hjuldropp.
   Gamle åpningstilstander på akkurat denne plassen repareres også. Andre
   tilsiktede lagrede kjøreposisjoner beholdes.
3. Østdøra kunne lukkes mens spilleren sto i terskelen. Det deaktiverte den
   eneste gangsonen under spilleren, og bevegelsesløseren kom ikke videre.
   Manuell lukking avvises nå mens terskelen er opptatt. En gammel slik
   lagring gjenåpner akkurat den døra før lasterens gangbarhetskontroll,
   med samme spillerposisjon og klokke. Dette er en reprodusert mulig årsak
   til Toms symptom; hans eksakte input er ikke kjent.

Personbilen har dessuten en egen orientert collider fra samme posisjon og
retning som modellen. Den finnes etter ID hver gang, siden dørbladene endrer
colliderlista, og deaktiveres når bilen skjules. Utrygge gamle bilposisjoner
mot inngang, annekset eller serviceplassen får klar besøksplass
`(4.5,-0.62,14)`. Også saves uten `personalPark` bruker denne plassen.

Toms to feilbilder er lest. Ingen egen endring i S47Props, town.json eller
bensinstasjonsmodellen. Tidslinje, bildestempler og stemmekatalog er uendret.
Biblioteksøket ga bare den generiske profilen `spill/croftengine.md`
(UNVERIFIED, uten kompatibel løsning); ingen bibliotekskode er gjenbrukt.

## Modellimport

`fairheaven_sw84` brukes også av `streetWagon` og `motelCar3`. Importens
opt-in `keep_complete` bevarer deres komplette standardasset. Spillerbilen
bruker egne `_body` og `_door_driver`, med original pivot og materialer.

| Importert asset | Trekanter |
|---|---:|
| Komplett standardbil | 19 461 |
| Spillerbilens kropp | 18 244 |
| Spillerbilens dør | 1 217 |

Summen og standardaliasets rekkefølge er kontrollert mot faktisk importert
meshmetadata, se `wagon-model-audit.json`. Andre snittmodeller og planrader
beholder tidligere oppførsel. Genererte Free-assets er ikke i Git. På en annen
maskin må `gltf_prep.py` kjøres og modellen importeres med
`S47_FREE_ONLY=fairheaven_sw84` før pakking. Fra `unreal/Signal47` kjører
`S47_FREE_ONLY=fairheaven_sw84 Tools/freemodels/import_free.sh` både
preparering og import, under samme maskinvakt som bygg. Main-kopien har de
tre assetene nå.

## Tester på sluttpakken

| Kontroll | Resultat | Bevis i evidensmappa |
|---|---|---|
| Editorbygg etter core-fletting | PASS, 74.49 s | `build-shared-model.log` |
| Linux-bygg/cook/pakking | PASS, 158.48 s, ingen materialfallback | `package.log` |
| Kjøretøy og selvtester | PASS, 1286 sjekker, 55.16 s | `packed-vehicles.log` |
| Inngangsmatrisen | PASS, 174.39 s | `packed-entrance.log` |
| Hel burgerrute | PASS, 256.54 s | `packed-route.log` |
| Kort gammel historie/regresjon | PASS, 6/6 | `regression.json` |

Inngangsmatrisen gir 48 passeringer med vanlig walker/kollisjon i begge
retninger og tre baner. Åtte kombinasjoner dekker før/etter D01 og parkert/
flyttet wagon, med fire separate ferske disk-/verdenslastinger. I tillegg:
48 avviste terskellukkinger, åtte vanlige lukkinger/åpninger fra klar plass,
og bevisst gjenskapt gammel ugyldig terskellagring som gjenopprettes uten
endret posisjon/klokke. Alle tellinger er kontrollert i `final-audit.json`.

Burgerruten kjører fysisk 2651,4 m og fullfører på 223,9 kontrollsekunder:
dinerordre, kjøp/svar lagret sammen, fersk disklasting, faktisk parkering
innen første sekund av 18:00, telefon/D01 med disk-/verdenslasting og tre
papirer én gang. Den gamle rutinen slippes videre med monoton klokke.
Loggen har positive faktiske lydnivåer for 10 bilradio-, 13 posisjonelle og
13 telefonklipp. Den raske API-dialogen avbryter linjer; dette er ikke en ny
full lyttetest. Nivåene gjelder samples etter kanalfilter, før delt buss.

De fem kjøretøybildene og inngangsbilder før/etter telefon er visuelt PASS.
Rutebildene er gjennomgått; telefonbildet fanger panelet før målt talestart
og brukes ikke som tekstbevis. Duskbildet bruker et uttrykkelig testkutt til
18:35. Menneskelig tastaturgjennomspilling og lytting er **UNVERIFIED**.
Oppsett, telefonvalg, klokkesnitt og kameravinkler bruker API; selve
passeringene og kjøringen bruker de vanlige bevegelses-/kjørekontrollerne.

## Kortregresjon og leveransegrense

| Eldre hoppfixture | Resultat |
|---|---|
| Residual og notatbok | PASS |
| Kontrollrommets nattetelefon | PASS |
| SARO 07 til STATION 01, 20 s kjøring | PASS, originale cab-deler synlige igjen |
| Kapittel 3, hytta | PASS |
| Kapittel 4, Room 6 | PASS |
| Kapittel 6 | PASS, 47 s hendelse, uttrekk 05:29:47 og visuelt godkjent sluttkort |

Disse seks kjøringene brukte 392,03 s veggtid. De bruker eksisterende
hopp-fixtures og beviser ingen sammenhengende fullnatt. Sluttkortet består
prosjektets egen sparsomme tittelregel, se `ending-review.json` og
[sluttbildet](/home/tom/scratch-bm/Signal47-opening-vehicles/reg-event.png).

Kilde og pakke ble frosset ved `66d2b5a6` med core `04517414`.
Core har siden fått `3aa41334` og dokumentcommit `92dc7a47`; de inngår
ikke i denne testpakken. Claude integrerer rettelsene videre på sin core.

## Bilder

| Visning | Pakket runtimebilde |
|---|---|
| Privatbil utenfra | [Utside](/home/tom/scratch-bm/Signal47-opening-vehicles/packed-vehicles/entrance-vehicles-wagon-outside.png) |
| Privatbilens åpne dør | [Egen dør](/home/tom/scratch-bm/Signal47-opening-vehicles/packed-vehicles/entrance-vehicles-wagon-door-open.png) |
| Privatbilens førersete | [Førermiljø](/home/tom/scratch-bm/Signal47-opening-vehicles/packed-vehicles/entrance-vehicles-driver-seat.png) |
| Plan SARO 07 på originalplassen | [Servicebil](/home/tom/scratch-bm/Signal47-opening-vehicles/packed-vehicles/entrance-vehicles-service-truck.png) |
| Adskilte parkeringsplasser | [Begge biler](/home/tom/scratch-bm/Signal47-opening-vehicles/packed-vehicles/entrance-vehicles-saro-parking.png) |
| Inngangen før telefon | [Innenfra](/home/tom/scratch-bm/Signal47-opening-vehicles/packed-entrance/entrance-0-inside.png) / [utenfra](/home/tom/scratch-bm/Signal47-opening-vehicles/packed-entrance/entrance-0-outside.png) |
| Etter telefon og lasting, flyttet wagon | [Innenfra](/home/tom/scratch-bm/Signal47-opening-vehicles/packed-entrance/entrance-7-inside.png) / [utenfra](/home/tom/scratch-bm/Signal47-opening-vehicles/packed-entrance/entrance-7-outside.png) |

## Gjenta kontrollene

Bruk isolert `-UserDir`, ledig maskin og `Scripts/unreal_guard.py`.

- `-S47SelfTest`
- `-S47Evening=commute -S47EveningVehicles -S47EveningShots=<mappe>`
- `-S47Evening=parking -S47EveningEntrance -S47EveningShots=<mappe>`
- `-S47EveningRoute=burger -S47VoiceLevels -S47EveningShots=<mappe>`

Første feillogg fra lastreparasjonen er beholdt. Tidligere pakker står
adskilt i `preliminary-package/` og `pre-core-package/`. Avbrutte forsøk med
konkurrerende Unreal-prosesser er ugyldige bevis og teller ikke som PASS.
Alle godkjente sluttkjøringer har normal exit og ingen observerte andre
tunge Unreal-jobber. Ingen andres prosess er stoppet, ingen grace-unntak brukt.

Kandidat: `~/scratch-bm/Signal47-opening-vehicles/candidate/Linux/`.
Rålogger, 26 rutebilder, seks pakkefilers SHA-256 og kilde-/importhashene:
`final-audit.json`, `source-provenance.json`, `audio-regression.json` og
`runtime-results.json` i samme evidensmappe. `delivery-verification.json`
bekrefter uendrede hasher etter regresjonen og ni kjøringer uten konkurranse.
Midlertidige Content-lenker er fjernet, opprinnelig sporet Kelvora er
gjenopprettet og egne OpenOrder-endringer er ryddet. Målassetene er bevart.
Arbeidstreet leveres uten generert lyd eller modeller. Installert spill er urørt.
