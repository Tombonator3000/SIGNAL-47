# Plaza: to utvendige plakatmotiver

**Status:** Gjeldende. **Gjelder:** CalicheAccess Cinema, bindingsversjon 2, UE 5.8.4/Linux/Vulkan. **Sist kontrollert:** 3. november 2026.

Native import og separat ferskleser PASS, 170/171 kontroller; alle fem eksisterende plakatassets byteidentiske. Fysisk kontrakt og pakket levering består; lokalt installert `501cbe81` kl. 07:45. [Levering](../../../../Docs/Evidence/UnrealDeliveries/CINEMA_ACCESS_2026-11-03.md).

Dagens kino bruker `CalicheAccessCinema-20261031`. De to additive motivene festes til aktiv Shell og følger aktiv Shell/Glass-synlighet. Retirert TB2-geometri og den gamle `Closed`-interaksjonen skal ikke aktiveres igjen. [Rapporten fra 29. oktober](../../../../Docs/Evidence/UnrealDeliveries/CINEMA_POSTERS_2026-10-29.md) dokumenterer den historiske TB2-leveransen; den er ikke dagens runtime-kontrakt.

## Kilder, flater og UV

[surfaces.json](surfaces.json) er en uendret historisk kontrakt for originalkunst, materialparametre, plane og kildehash. Den omskrives ikke til å fremstille gamle TB2-stier som dagens geometri. Aktiv geometri angis separat i versjon 2-bindingen og måles på nytt fra lagrede native CalicheAccess Shell/Glass, på alle tre LOD-er.

Motivene er `01-cinema-desert-sky.jpg` og `02-cinema-friday-matinee.jpg` fra [EnvironmentGraphics](../../../ArtSource/EnvironmentGraphics/README.md), begge 1024×1536 og 2:3. Hvert motiv er 1,073333333×1,61 m, sentrert uten strekk/beskjæring i trykkflaten på 1,13×1,61 m. Motivet ligger 1 mm foran originaltrykket, 87 mm bak eksisterende glass. Rammer, glass, lamper, atlas og originalgeometri bevares.

Native modellbasis er `UE_cm=(100*Bx,-100*By,100*Bz)`. Den eksisterende eide kopien av Engine Plane er 100×100 cm med normal +Z, `u=(x+50)/100`, `v=(y+50)/100`. Runtime legger lokal +X langs UE −Y, lokal +Y langs UE −Z og normalen langs UE +X. `MapU=(1,0,0,0)` og `MapV=(0,1,0,0)` bevares. Import og levende audit krever faktisk UV-/normal-/basislikhet; ingen Engine-asset endres.

## Eksisterende assets og proveniens

`import_assets.py` krever alle fem eksisterende assets under `/Game/Signal47/Art/CinemaPosters`:

- `T_Cinema_DesertSky` og `T_Cinema_FridayMatinee`.
- `MI_Cinema_DesertSky` og `MI_Cinema_FridayMatinee`.
- `SM_CinemaPosterPlane`.

Dette er validering og bindingsfullføring, ikke oppretting av manglende assets eller reimport. De fem filene skal være byteidentiske før/etter. Materialinstansene beholder `/Game/Web/Base/M_Web_Lit_Opaque`, egne Map-teksturer, hvit Color, ruhet 0,88, null metall/emisjon og fullflate-UV. Teksturene beholder sRGB/BC7, mip-kjede og eksisterende POT-policy.

Bevart `AssetImportData` må peke på nøyaktig ett kildefilnavn med riktig `ArtSource/EnvironmentGraphics/jpg/…`-suffiks. Den tilgjengelige original-JPG-en, katalogen, teksturens lagrede kildehash og den uendrede kontraktens hash må stemme. Hvis den historiske importstien fortsatt finnes, må også dens bytes stemme. En fjernet gammel arbeidskopi alene ugyldiggjør ikke korrekt proveniens; importfilnavn og metadata omskrives ikke.

## Import og separat ferskleser

Root kjører jobbene sekvensielt i hver sin ferske prosess gjennom prosjektets motorreservasjon og `Scripts/unreal_guard.py require-idle`. Eksemplene kjøres fra `unreal/Signal47`, med `UE_ROOT` som motorens overordnede mappe og nye rapportstier utenfor Content:

```sh
S47_CINEMA_MODE=import S47_CINEMA_REPORT=/absolutt/ny/import.json \
  "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PWD/Signal47.uproject" \
  -run=pythonscript -script="$PWD/Tools/cinemaposters/import_assets.py" \
  -unattended -nullrhi -nosplash -NoSound -NoLiveCoding

S47_CINEMA_MODE=verify S47_CINEMA_REPORT=/absolutt/ny/verify.json \
  S47_CINEMA_IMPORT_REPORT=/absolutt/ny/import.json \
  "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PWD/Signal47.uproject" \
  -run=pythonscript -script="$PWD/Tools/cinemaposters/import_assets.py" \
  -unattended -nullrhi -nosplash -NoSound -NoLiveCoding
```

Editorbinæren må ha `S47.Cinema.ReadSavedMaterials`. Leseren måler faktiske lagrede StaticMeshDescription-er: to trekanter per trykk-/glassflate, mål, bounds, UV, navngitt materiale og faktisk renderseksjon på alle tre LOD-er. Gruppe-ID, global materialslot og seksjonsindeks antas aldri like. Original TB2, hele CalicheAccess, delt master og Engine Plane hashbeskyttes.

Standardmodus er `verify`. Import fullfører en inaktiv kandidat. En separat fersk leser krever PASS-importrapport, ulik prosess-ID, identisk måling og alle fem filhasher før `Content/WebData/cinema-posters-binding.json` aktiveres med `version=2`, riktig `geometry_profile`, `status=PASS` og `fresh_saved_reader=true`. En allerede akseptert identisk binding verifiseres uten omskriving. Endret eller ufullstendig binding avvises; ingen sletting eller automatisk reparasjon brukes for å passere.

Begge prosessene må avslutte med kode 0, fersk JSON-status PASS og `Python script executed successfully` i loggen. Eksisterende cook-konfigurasjon tar med de fem assets og WebData; aktiv binding må finnes før staging. Native uasset-hasher er importbevis, ikke påstand om identiske omkodede cook-bytes.

## Runtime og fysisk kontrakt

`FS47CinemaPosters::Install(M->Web)` kalles umiddelbart etter `S47CalicheAccess::Init(M)` i TownLayer. Eksisterende vertsoppdatering kaller `Update()`; ingen egen timer, lagring eller interaksjon opprettes. `Readback()` og `Audit()` kontrollerer de faktiske komponentene, foreldre, materialer, transform, plane-UV og synlighet. Manglende eller endrede flater/bindinger gir ingen nye plakater. `-S47CinemaPostersOff` og `-S47PropsOff` bevarer originalene.

Døren bruker vanlig `access:Cinema`-fokus og `Player.OnUse`, med faktisk åpen/lukket leaf og begge områdekolliderne. Lukking mens spilleren overlapper dørens lukkede kollider skal avvises med «Step clear of the doorway before closing it.»; spillerpose, leaf, kollidere og eksportert dørtilstand beholdes. Vanlig lukking utenfor døråpningen og åpning fungerer fortsatt.

Den målrettede native prøven kjøres med `-S47CinemaPostersContract -S47Jump=chapter2` og fersk isolert `-UserDir`, uten konkurrerende Smoke-/pose-/klokkedriver. Den bruker ett eksplisitt utendørs startpunkt, deretter vanlig Walker, kameraray/fokus og OnUse. Den kontrollerer åpen/lukket dør, avvist lukking i åpningen, lagret ikke-standard CLOSED gjennom full disk/title/Continue, etterfølgende vanlig åpning og uendrede øvrige adgangs-/quest-/klokkefakta. En eksplisitt områdefixture kontrollerer synlighet.

Aksept krever fersk `Saved/CinemaPostersContract/<guid>/result.json` med PASS, `S47: CinemaPosters contract PASS checks=` i loggen og gjennomsette native bilder av begge motiver/glass/dør. Gjenta kontrakten i den faktiske pakken før installasjon. Dette beviser ikke menneskelig input, naturlig ankomst eller full innvendig kinogjennomgang. Før-reproen viste at OnUse kunne lukke døren rundt spilleren; permanent fastlåsing er ikke dokumentert.
